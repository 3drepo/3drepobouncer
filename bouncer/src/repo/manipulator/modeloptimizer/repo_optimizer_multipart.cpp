/**
*  Copyright (C) 2015 3D Repo Ltd
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU Affero General Public License as
*  published by the Free Software Foundation, either version 3 of the
*  License, or (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU Affero General Public License for more details.
*
*  You should have received a copy of the GNU Affero General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
* Abstract optimizer class
*/

// Include the BVH library first because the mongo drivers (undef_macros.h) #define conflicting names

#include "bvh/bvh.hpp"
#include "bvh/sweep_sah_builder.hpp"

#include "repo_optimizer_multipart.h"
#include "repo/core/model/bson/repo_bson_factory.h"
#include "repo/core/model/repo_model_global.h"
#include "repo/lib/repo_hash_combine.h"

#include <algorithm>
#include <chrono>

using namespace repo::lib;
using namespace repo::manipulator::modeloptimizer;
using namespace repo::core::handler::database;

auto defaultGraph = repo::core::model::RepoScene::GraphType::DEFAULT;

// This limit is used to prevent metadata files becoming unwieldly, and the
// supermesh UV resolution falling below the quantisation noise floor.
static const size_t REPO_MP_MAX_MESHES_IN_SUPERMESH = 5000;

static const size_t REPO_BVH_MAX_LEAF_SIZE = 16;
static const size_t REPO_MODEL_LOW_CLUSTERING_RATIO = 0.2f;

#define CHRONO_DURATION(start) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count()

MultipartOptimizer::MultipartOptimizer(
	repo::core::handler::AbstractDatabaseHandler* handler,
	repo::manipulator::modelconvertor::AbstractModelExport* exporter
):
	handler(handler),
	exporter(exporter),
	splitByFloor(false)
{
}

void MultipartOptimizer::processScene(
	std::string database,
	std::string collection,
	repo::lib::RepoUUID revId)
{
	// Getting Transforms
	repoInfo << "Getting Transforms";
	auto transformMap = getAllTransforms(database, collection, revId);

	// Get lookup map for material properties
	repoInfo << "Getting Materials";
	auto matPropMap = getAllMaterials(database, collection, revId);

	// Create jobs
	repoInfo << "Creating Processing Jobs";
	std::vector<ProcessingJob> jobs;

	jobs.push_back(createUntexturedJob(revId, 2, false, false));
	jobs.push_back(createUntexturedJob(revId, 2, false, true));
	jobs.push_back(createUntexturedJob(revId, 2, true, false));
	jobs.push_back(createUntexturedJob(revId, 2, true, true));
	
	jobs.push_back(createUntexturedJob(revId, 3, false, false));
	jobs.push_back(createUntexturedJob(revId, 3, false, true));
	jobs.push_back(createUntexturedJob(revId, 3, true, false));
	jobs.push_back(createUntexturedJob(revId, 3, true, true));
	
	// Get Texture IDs
	repoInfo << "Getting all texture Ids";
	auto texIds = getAllTextureIds(database, collection, revId);

	// Create jobs for each texture group
	for (auto texId : texIds) {
		// One cannot map a texture to a line, however, customers can assign materials with textures to lines
		// so we need to be able to process them.
		jobs.push_back(createTexturedJob(revId, 2, false, texId));
		jobs.push_back(createTexturedJob(revId, 2, true, texId));

		jobs.push_back(createTexturedJob(revId, 3, false, texId));
		jobs.push_back(createTexturedJob(revId, 3, true, texId));
	}
	
	// Process jobs
	repoInfo << "Processing Jobs";

	for (auto job : jobs) {
		clusterAndSupermesh(
			database,
			collection,
			transformMap,
			matPropMap,
			job);
	}

	// Finalise export
	exporter->finalise();
}

MultipartOptimizer::TransformMap MultipartOptimizer::getAllTransforms(
	const std::string &database,
	const std::string &collection,
	const repo::lib::RepoUUID &revId
)
{
	repo::core::handler::database::query::RepoQueryBuilder filter;
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_REVISION_ID, revId));
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_LABEL_TYPE, std::string(REPO_NODE_TYPE_TRANSFORMATION)));

	repo::core::handler::database::query::RepoProjectionBuilder projection;
	projection.excludeField(REPO_NODE_LABEL_ID);
	projection.includeField(REPO_NODE_LABEL_SHARED_ID);
	projection.includeField(REPO_NODE_LABEL_MATRIX);
	projection.includeField(REPO_NODE_LABEL_PARENTS);

	auto sceneCollection = collection + "." + REPO_COLLECTION_SCENE;
	auto cursor = handler->findCursorByCriteria(database, sceneCollection, filter, projection);

	TransformMap transformMap;

	repo::core::model::RepoBSON rootNode;
	std::unordered_map<repo::lib::RepoUUID, std::vector<repo::core::model::RepoBSON>, repo::lib::RepoUUIDHasher> childNodeMap;
	for (auto bson : (*cursor)) {
		if (bson.hasField(REPO_NODE_LABEL_PARENTS)) {
			auto parentId = bson.getUUIDFieldArray(REPO_NODE_LABEL_PARENTS)[0];

			if (childNodeMap.contains(parentId)) {
				childNodeMap.at(parentId).push_back(bson);
			}
			else {
				auto children = std::vector<repo::core::model::RepoBSON>{bson};
				childNodeMap.insert({parentId, children});
			}
		}
		else {
			rootNode = bson;
		}

		// Pre-create all the transform info nodes
		transformMap.insert({bson.getUUIDField(REPO_NODE_LABEL_SHARED_ID), {}});
	}

	// Find all branch groupings using magic metadata fields.
	if (splitByFloor) {
		applyBranchGroups(transformMap, database, collection, revId);
	}

	if (rootNode.isEmpty()) {
		throw repo::lib::RepoException("getAllTransforms; no root node found.");
	}

	traverseTransformTree(rootNode, childNodeMap, transformMap);

	return transformMap;
}

void MultipartOptimizer::applyBranchGroups(
	TransformMap& transforms, 
	const std::string& database,
	const std::string& collection, 
	const repo::lib::RepoUUID &revId)
{
	// In the future we may extend this method to support user-defined queries,
	// but for now we split based on the presence of the magic metadata field.

	query::RepoQueryBuilder filter;
	filter.append(query::Eq(REPO_NODE_REVISION_ID, revId));
	filter.append(query::Eq(REPO_NODE_LABEL_TYPE, std::string(REPO_NODE_TYPE_METADATA)));
	filter.append(query::ArrayContains(REPO_NODE_LABEL_METADATA, query::Eq(REPO_NODE_LABEL_META_KEY, std::string(REPO_METADATA_GROUPING_FLOOR))));

	repo::core::handler::database::query::RepoProjectionBuilder projection;
	projection.excludeField(REPO_NODE_LABEL_ID);
	projection.includeField(REPO_NODE_LABEL_SHARED_ID);
	projection.includeField(REPO_NODE_LABEL_PARENTS);
	projection.includeField(REPO_NODE_LABEL_METADATA);

	auto sceneCollection = collection + "." + REPO_COLLECTION_SCENE;
	auto cursor = handler->findCursorByCriteria(database, sceneCollection, filter, projection);

	std::unordered_map<std::string, BranchGroup*> branchGroupsByTag;

	for (auto bson : (*cursor)) {
		auto metadataNode = repo::core::model::MetadataNode(bson);
		auto& metadata = metadataNode.getAllMetadata();
		auto& variant = metadata.at(REPO_METADATA_GROUPING_FLOOR);
		auto value = boost::get<std::string>(variant);

		auto& groupPtr = branchGroupsByTag[value];
		if (!groupPtr) {
			groupPtr = new BranchGroup();
			groupPtr->name = value;
		}

		for (auto& parent : metadataNode.getParentIDs()) {
			auto transformIt = transforms.find(parent);
			if (transformIt != transforms.end()) {
				transformIt->second.branch = groupPtr;
			}
		}
	}
}

void MultipartOptimizer::traverseTransformTree(
	const repo::core::model::RepoBSON &root,
	const std::unordered_map<repo::lib::RepoUUID, std::vector<repo::core::model::RepoBSON>,	repo::lib::RepoUUIDHasher> &childNodeMap,
	TransformMap &transforms)
{
	/*
	* This method will bake the transform tree, updating each entry in transforms
	* with its final state after performing a depth-first evaluation.
	*/

	// Create stacks for the nodes and the matrices
	std::stack<std::pair<repo::core::model::RepoBSON, TransformInfo>> stack;

	// Push starting node onto the stack. The default initialiser will create an
	// identity matrix and a null branch group.
	stack.push({root, {}});

	// DFS traversal of the transformation tree, summing up the matrices along the way
	while (!stack.empty()) {

		// Remove top node from the stack
		auto top = stack.top();
		stack.pop();

		auto bson = top.first;
		auto parentInfo = top.second;

		// Get node information
		auto nodeId = bson.getUUIDField(REPO_NODE_LABEL_SHARED_ID);
		auto matrix = bson.getMatrixField(REPO_NODE_LABEL_MATRIX);
		
		auto& info = transforms.at(nodeId); // Transform to update

		// Update the node's transformation
		info.matrix = parentInfo.matrix * matrix;

		// Inherit the branch group
		if (!info.branch) {
			info.branch = parentInfo.branch;
		}

		// if this node has other transforms as children, push them on the stack
		if (childNodeMap.contains(nodeId)) {			
			auto children = childNodeMap.at(nodeId);
			for (auto child : children) {
				stack.push({ child, info });
			}
		}
	}
}

MultipartOptimizer::MaterialPropMap MultipartOptimizer::getAllMaterials(
	const std::string &database,
	const std::string &collection,
	const repo::lib::RepoUUID &revId)
{
	repo::core::handler::database::query::RepoQueryBuilder filter;
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_REVISION_ID, revId));
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_LABEL_TYPE, std::string(REPO_NODE_TYPE_MATERIAL)));

	auto sceneCollection = collection + "." + REPO_COLLECTION_SCENE;
	auto materialBsons = handler->findAllByCriteria(database, sceneCollection, filter);

	MaterialPropMap matMap;
	for (auto &materialBson : materialBsons) {

		// Create material node
		auto matNode = std::make_shared<repo::core::model::MaterialNode>(materialBson);			
				
		// Go over the parents and add the pointer for each so that the map can be used to lookup
		// the material for a given meshNode
		auto parents = materialBson.getUUIDFieldArray(REPO_NODE_LABEL_PARENTS);
		for (auto parent : parents) {
			matMap.insert({ parent, matNode });
		}
	}

	return matMap;
}

std::vector<repo::lib::RepoUUID> MultipartOptimizer::getAllTextureIds(
	const std::string &database,
	const std::string &collection,
	const repo::lib::RepoUUID &revId) {

	// Create filter
	repo::core::handler::database::query::RepoQueryBuilder filter;
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_REVISION_ID, revId));
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_LABEL_TYPE, std::string(REPO_NODE_TYPE_TEXTURE)));

	repo::core::handler::database::query::RepoProjectionBuilder projection;
	projection.includeField(REPO_NODE_LABEL_ID);

	std::vector<repo::lib::RepoUUID> texIds;

	auto sceneCollection = collection + "." + REPO_COLLECTION_SCENE;
	auto cursor = handler->findCursorByCriteria(database, sceneCollection, filter, projection);

	if (cursor) {
		for (auto document : (*cursor)) {
			auto bson = repo::core::model::RepoBSON(document);
			texIds.push_back(bson.getUUIDField(REPO_NODE_LABEL_ID));
		}
	}
	else {
		repoWarning << "getAllTextureIds; getting cursor was not successful; no texture Ids in output vector";
	}

	return texIds;
}

MultipartOptimizer::ProcessingJob repo::manipulator::modeloptimizer::MultipartOptimizer::createUntexturedJob(
	const repo::lib::RepoUUID &revId,
	const int primitive,
	const bool isOpaque,
	const bool hasNormals)
{
	// Create filter
	repo::core::handler::database::query::RepoQueryBuilder filter;
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_LABEL_TYPE, REPO_NODE_TYPE_MESH));
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_REVISION_ID, revId));
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_MESH_LABEL_PRIMITIVE, primitive));
	if (isOpaque)
		filter.append(repo::core::handler::database::query::Eq(REPO_FILTER_TAG_OPAQUE, true));
	else
		filter.append(repo::core::handler::database::query::Eq(REPO_FILTER_TAG_TRANSPARENT, true));
	filter.append(repo::core::handler::database::query::Exists(REPO_FILTER_TAG_NORMALS, hasNormals));

	std::string description = "Untextured Job - Primitive: " + std::to_string(primitive) + "; " + (isOpaque ? "Opaque" : "Transparent") + "; " + (hasNormals ? "Has Normals" : "No Normals");

	// Create job
	return ProcessingJob({ description, filter, {} });
}

MultipartOptimizer::ProcessingJob repo::manipulator::modeloptimizer::MultipartOptimizer::createTexturedJob(
	const repo::lib::RepoUUID &revId,
	const int primitive,
	const bool hasNormals,
	const repo::lib::RepoUUID &texId)
{
	// Create filter
	repo::core::handler::database::query::RepoQueryBuilder filter;
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_LABEL_TYPE, REPO_NODE_TYPE_MESH));
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_REVISION_ID, revId));
	filter.append(repo::core::handler::database::query::Eq(REPO_FILTER_TAG_TEXTURE_ID, texId));
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_MESH_LABEL_PRIMITIVE, primitive));
	filter.append(repo::core::handler::database::query::Exists(REPO_FILTER_TAG_NORMALS, hasNormals));

	std::string description = "Textured Job - Texture: " + texId.toString() + "; Primitive: " + std::to_string(primitive) + "; " + (hasNormals ? "Has Normals" : "No Normals");

	// Create job
	return ProcessingJob({ description, filter, texId });
}

void MultipartOptimizer::clusterAndSupermesh(
	const std::string &database,
	const std::string &collection,
	const TransformMap& transformMap,
	const MaterialPropMap& matPropMap,
	const MultipartOptimizer::ProcessingJob &job
) {
	repoInfo << "Processing Job: " << job.description;

	// Create projection
	repo::core::handler::database::query::RepoProjectionBuilder projection;
	projection.excludeField(REPO_NODE_LABEL_ID);
	projection.includeField(REPO_NODE_LABEL_SHARED_ID);
	projection.includeField(REPO_NODE_MESH_LABEL_BOUNDING_BOX);
	projection.includeField(REPO_NODE_MESH_LABEL_VERTICES_COUNT);
	projection.includeField(REPO_NODE_LABEL_PARENTS);
	projection.includeField(REPO_NODE_MESH_LABEL_GROUPING);

	// Get cursor
	auto sceneCollection = collection + "." + REPO_COLLECTION_SCENE;
	auto cursor = handler->findCursorByCriteria(database, sceneCollection, job.filter, projection);

	// Currently we support two levels of grouping when supermeshing, the
	// MeshNode branch group, and the group tag of the Mesh itself (the
	// resource groupings from importers such as Synchro).

	struct group {
		std::string mesh;
		BranchGroup* branch;
	};

	struct group_hash {
		std::size_t operator()(const group& k) const {
			size_t h = 0;
			hash_combine(h, k.mesh);
			hash_combine(h, k.branch);
			return h;
		}
	};

	struct group_equal {
		bool operator()(const group& a, const group& b) const {
			return a.mesh == b.mesh && a.branch == b.branch;
		}
	};

	std::unordered_map<
		group,
		std::vector<repo::core::model::StreamingMeshNode>,
		group_hash,
		group_equal
	> groups;

	// iterate cursor and pack outcomes in lightweight mesh node structure
	for (auto bson : (*cursor)) {
		repo::core::model::StreamingMeshNode node(bson);
		auto transform = transformMap.at(node.getParent());

		// Transform bounds
		node.transformBounds(transform.matrix);

		// Get grouping
		auto& group = groups[{ node.getGrouping(), transform.branch }];
		group.emplace_back(std::move(node));
	}

	if (groups.size() == 0) {
		repoInfo << "No groups to process for this job. Returning.";
		return;
	}

	// Process the mesh groups
	for (auto& group : groups) {
		auto& branchGroup = group.first.branch;
		auto& nodes = group.second;

		// Cluster the mesh nodes
		repoInfo << "Clustering Nodes";
		auto clusters = clusterMeshNodes(nodes);

		// Create Supermeshes from the clusters
		repoInfo << "Creating Supermeshes from clustered Nodes";
		auto texId = job.isTexturedJob() ? job.texId : repo::lib::RepoUUID();
		auto tag = branchGroup ? branchGroup->name : std::string(); // Resource groups never contribute to the name, only branch groups
		createSuperMeshes(database, collection, transformMap, matPropMap, nodes, clusters, texId, tag);
	}
}

void repo::manipulator::modeloptimizer::MultipartOptimizer::createSuperMeshes(
	const std::string &database,
	const std::string &collection,
	const TransformMap& transformMap,
	const MaterialPropMap& matPropMap,
	std::vector<repo::core::model::StreamingMeshNode>& meshNodes,
	const std::vector<std::vector<int>>& clusters,
	const repo::lib::RepoUUID &texId,
	const std::string& namedGrouping)
{
	// Get blobHandler
	auto sceneCollection = collection + "." + REPO_COLLECTION_SCENE;
	repo::core::handler::fileservice::BlobFilesHandler blobHandler(handler->getFileManager(), database, sceneCollection);

	for (auto cluster : clusters) {

		std::unordered_map<repo::lib::RepoUUID, int, repo::lib::RepoUUIDHasher> clusterMap;
		std::vector<repo::lib::RepoUUID> sharedIdsInCluster;
		for (auto& index : cluster) {
			auto& node = meshNodes[index];
			auto sharedId = node.getSharedId();
			clusterMap.insert({ sharedId, index });

			sharedIdsInCluster.push_back(sharedId);
		}

		// Create filter
		auto filter = repo::core::handler::database::query::Eq(REPO_NODE_LABEL_SHARED_ID, sharedIdsInCluster);

		// Create projection
		repo::core::handler::database::query::RepoProjectionBuilder projection;
		projection.excludeField(REPO_NODE_LABEL_ID);
		projection.includeField(REPO_NODE_LABEL_SHARED_ID);
		projection.includeField(REPO_NODE_MESH_LABEL_VERTICES_COUNT);
		projection.includeField(REPO_NODE_MESH_LABEL_FACES_COUNT);
		projection.includeField(REPO_NODE_MESH_LABEL_UV_CHANNELS_COUNT);
		projection.includeField(REPO_NODE_MESH_LABEL_PRIMITIVE);
		projection.includeField(REPO_LABEL_BINARY_REFERENCE);

		auto binNodes = handler->findAllByCriteria(database, sceneCollection, filter, projection);

		// Iterate over the meshes and decide what to do with each. The options are
		// to append to the existing supermesh, start a new supermesh, or split into
		// multiple supermeshes.

		mapped_mesh_t currentSupermesh;

		for (auto& nodeBson : binNodes) {

			// Find streamed node
			auto sharedId = nodeBson.getUUIDField(REPO_NODE_LABEL_SHARED_ID);
			auto nodeIndex = clusterMap.at(sharedId);
			auto& sNode = meshNodes[nodeIndex];

			// Load geometry for this node.
			// Placed In its own scope so that buffer can be discarded as soon as it is processed
			{
				auto binRef = nodeBson.getBinaryReference();
				auto dataRef = repo::core::handler::fileservice::DataRef::deserialise(binRef);
				auto buffer = blobHandler.readToBuffer(dataRef);

				// If there is no texture present, we ignore UV values.
				// This allows us to group more meshes together.
				bool ignoreUVs = texId.isDefaultValue();

				sNode.loadSupermeshingData(nodeBson, buffer, ignoreUVs);
			}

			// Bake the streaming mesh node by applying the transformation to the vertices
			// Note that the bounds have already been transformed by calling transformBounds earlier
			auto transform = transformMap.at(sNode.getParent());
			sNode.bakeLoadedMeshes(transform.matrix);

			if (currentSupermesh.vertices.size() + sNode.getNumLoadedVertices() <= REPO_MP_MAX_VERTEX_COUNT)
			{
				// The current node can be added to the supermesh OK				
				appendMesh(sNode, matPropMap, currentSupermesh, texId);
			}
			else if (sNode.getNumLoadedVertices() > REPO_MP_MAX_VERTEX_COUNT)
			{
				// The node is too big to fit into any supermesh, so it must be split
				splitMesh(sNode, matPropMap, texId, namedGrouping);
			}
			else
			{
				// The node is small enough to fit within one supermesh, just not this one
				createSuperMesh(currentSupermesh, namedGrouping);
				currentSupermesh = mapped_mesh_t();
				appendMesh(sNode, matPropMap, currentSupermesh, texId);
			}

			// Unload the streaming node
			sNode.unloadSupermeshingData();
		}

		// Add the last supermesh to be built
		if (currentSupermesh.vertices.size()) {
			createSuperMesh(currentSupermesh, namedGrouping);
		}
	}
}

void MultipartOptimizer::createSuperMesh(
	const mapped_mesh_t& mappedMesh, const std::string& tag)
{
	// Create supermesh node
	auto supermeshNode = createSupermeshNode(mappedMesh);
	supermeshNode->setGrouping(tag);

	exporter->addSupermesh(supermeshNode.get());
}

void MultipartOptimizer::appendMesh(
	repo::core::model::StreamingMeshNode &node,
	const MaterialPropMap &matPropMap,
	mapped_mesh_t &mapped,
	const repo::lib::RepoUUID &texId
)
{
	repo_mesh_mapping_t meshMap;

	// Get material information
	auto matNode = matPropMap.at(node.getSharedId());
	meshMap.material_id = matNode->getUniqueID();
	meshMap.material = matNode->getMaterialStruct();

	// set texture id if passed in
	if (!texId.isDefaultValue())
		meshMap.texture_id = texId;

	meshMap.mesh_id = node.getUniqueId();
	meshMap.shared_id = node.getSharedId();

	auto bbox = node.getBoundingBox();
	meshMap.min = (repo::lib::RepoVector3D)bbox.min();
	meshMap.max = (repo::lib::RepoVector3D)bbox.max();

	const auto& submVertices = node.getLoadedVertices();
	const auto& submNormals = node.getLoadedNormals();
	const auto& submFaces = node.getLoadedFaces();
	const auto& submUVs = node.getLoadedUVChannelsSeparated();

	if (submVertices.size() && submFaces.size())
	{
		meshMap.vertFrom = mapped.vertices.size();
		meshMap.vertTo = meshMap.vertFrom + submVertices.size();
		meshMap.triFrom = mapped.faces.size();
		meshMap.triTo = mapped.faces.size() + submFaces.size();

		mapped.meshMapping.push_back(meshMap);

		mapped.vertices.insert(mapped.vertices.end(), submVertices.begin(), submVertices.end());
		for (const auto face : submFaces)
		{
			repo_face_t offsetFace;
			for (const auto idx : face)
			{
				offsetFace.push_back(meshMap.vertFrom + idx);
			}
			mapped.faces.push_back(offsetFace);
		}

		if (submNormals.size()) {
			mapped.normals.insert(mapped.normals.end(), submNormals.begin(), submNormals.end());
		}

		if (mapped.uvChannels.size() == 0 && submUVs.size() != 0)
		{
			//initialise uvChannels
			mapped.uvChannels.resize(submUVs.size());
		}

		if (mapped.uvChannels.size() == submUVs.size())
		{
			for (uint32_t i = 0; i < submUVs.size(); ++i)
			{
				mapped.uvChannels[i].insert(mapped.uvChannels[i].end(), submUVs[i].begin(), submUVs[i].end());
			}
		}
		else
		{
			//This shouldn't happen, if it does, then it means that mesh nodes with and without uvs have been grouped together
			throw repo::lib::RepoException("Inconsistent UV channel count when appending mesh to supermesh. This likely means that meshes with and without UVs have been grouped together, which is not supported.");
		}
	}
	else
	{
		throw repo::lib::RepoException("Failed merging meshes: Vertices or faces cannot be null!");
	}
}

// Constructs a bounding volumne hierarchy of all the Faces in the MeshNode

MultipartOptimizer::Bvh MultipartOptimizer::buildFacesBvh(
	repo::core::model::StreamingMeshNode &node
)
{
	// Create a set of bounding boxes & centers for each Face in the oversized
	// mesh.
	// The BVH builder expects a set of bounding boxes and centers to work with.

	const auto& faces = node.getLoadedFaces();
	const auto& vertices = node.getLoadedVertices();
	auto boundingBoxes = std::vector<bvh::BoundingBox<Scalar>>();
	auto centers = std::vector<BvhVector3>();

	for (const auto& face : faces)
	{
		auto v = vertices[face[0]];
		auto b = bvh::BoundingBox<Scalar>(BvhVector3(v.x, v.y, v.z));

		for (int i = 1; i < face.size(); i++)
		{
			v = vertices[face[i]];
			b.extend(BvhVector3(v.x, v.y, v.z));
		}

		boundingBoxes.push_back(b);
		centers.push_back(b.center());
	}

	auto globalBounds = bvh::compute_bounding_boxes_union(boundingBoxes.data(), boundingBoxes.size());

	// Create an acceleration data structure based on the face bounds

	Bvh bvh;
	bvh::SweepSahBuilder<Bvh> builder(bvh);
	builder.max_leaf_size = REPO_BVH_MAX_LEAF_SIZE;
	builder.build(globalBounds, boundingBoxes.data(), centers.data(), boundingBoxes.size());

	return bvh;
}

// Create a breadth first list of all the leaf, and branch, nodes in a Bvh.
// Nodes will only appear in one of the two lists.

void MultipartOptimizer::flattenBvh(
	const Bvh& bvh,
	std::vector<size_t> &leaves,
	std::vector<size_t> &branches
)
{
	std::queue<size_t> nodeQueue; // Using a queue instead of a stack means the child nodes are handled later, resulting in a breadth first traversal
	nodeQueue.push(0);
	do {
		auto index = nodeQueue.front();
		nodeQueue.pop();

		auto& node = bvh.nodes[index];

		if (node.is_leaf())
		{
			leaves.push_back(index);
		}
		else
		{
			nodeQueue.push(node.first_child_or_primitive);
			nodeQueue.push(node.first_child_or_primitive + 1);

			branches.push_back(index);
		}
	} while (!nodeQueue.empty());
}

// Get a list of all the primitives under a branch of a Bvh.

std::vector<size_t> MultipartOptimizer::getBranchPrimitives(
	const Bvh& bvh,
	size_t head
)
{
	std::vector<size_t> primitives;

	std::stack<size_t> nodeStack;
	nodeStack.push(head);
	do
	{
		auto node = bvh.nodes[nodeStack.top()];
		nodeStack.pop();

		if (node.is_leaf())
		{
			for (int i = 0; i < node.primitive_count; i++)
			{
				auto primitive = bvh.primitive_indices[node.first_child_or_primitive + i];
				primitives.push_back(primitive);
			}
		}
		else
		{
			nodeStack.push(node.first_child_or_primitive);
			nodeStack.push(node.first_child_or_primitive + 1);
		}
	} while (!nodeStack.empty());

	return primitives;
}

// Gets the branch nodes that contain fewer than REPO_MP_MAX_VERTEX_COUNT beneath
// them in total.

std::vector<size_t> MultipartOptimizer::getSupermeshBranchNodes(
	const Bvh &bvh,
	const std::vector<size_t> &vertexCounts)
{
	std::vector<size_t> branchNodes;
	std::stack<size_t> nodeStack;
	nodeStack.push(0);
	do {
		auto index = nodeStack.top();
		nodeStack.pop();

		auto& node = bvh.nodes[index];
		auto numVertices = vertexCounts[index];

		// As we are traversing top-down, the vertex counts get smaller.
		// As soon as a node's count drops below the target, all the nodes in
		// that branch can become a mesh.

		if (numVertices < REPO_MP_MAX_VERTEX_COUNT || node.is_leaf()) // This node is the head of a branch that should become a group
		{
			branchNodes.push_back(index);
		}
		else
		{
			nodeStack.push(node.first_child_or_primitive);
			nodeStack.push(node.first_child_or_primitive + 1);
		}
	} while (!nodeStack.empty());

	return branchNodes;
}

void MultipartOptimizer::createSupermeshFromBranch(
	repo::core::model::StreamingMeshNode& node,
	const MaterialPropMap& matPropMap,
	const repo::lib::RepoUUID& texId,
	std::set<uint32_t>* globalVertexIndices,
	std::vector<uint32_t>* primitives,
	const std::string& namedGrouping
)
{
	const auto& faces = node.getLoadedFaces();
	const auto& vertices = node.getLoadedVertices();
	const auto& normals = node.getLoadedNormals();
	const auto& uvChannels = node.getLoadedUVChannelsSeparated();


	// Now collect the faces into a new mesh.

	// Re-index each face for the new reduced vertex arrays; this can be
	// done through a reverse lookup into the set of unique vertices
	// referenced by all the faces in the new mesh (i.e. at the branch node).
	std::map<size_t, size_t> globalToLocalIndex;

	// Create the inverse lookup table for the re-indexing
	size_t pos = 0;
	for (const auto index : *globalVertexIndices)
	{
		globalToLocalIndex[index] = pos;
		pos++;
	}

	mapped_mesh_t mapped;

	for (const auto faceIndex : *primitives)
	{
		mapped.faces.push_back(faces[faceIndex]);
	}

	for (auto& face : mapped.faces)
	{
		for (auto i = 0; i < face.size(); i++)
		{
			face[i] = globalToLocalIndex[face[i]]; // Re-index the face
		}
	}

	// Using the same sets, create the local vertex arrays

	mapped.uvChannels.resize(uvChannels.size());

	for (const auto globalIndex : *globalVertexIndices)
	{
		mapped.vertices.push_back(vertices[globalIndex]);
		if (normals.size()) {
			mapped.normals.push_back(normals[globalIndex]);
		}
		for (auto i = 0; i < uvChannels.size(); i++)
		{
			if (uvChannels[i].size()) {
				mapped.uvChannels[i].push_back(uvChannels[i][globalIndex]);
			}
		}
	}

	// Finally, add the mapping for this mesh...

	repo_mesh_mapping_t mapping;

	mapping.vertFrom = 0;
	mapping.vertTo = mapped.vertices.size();
	mapping.triFrom = 0;
	mapping.triTo = mapped.faces.size();
	repo::lib::RepoBounds bounds;
	for (const auto v : mapped.vertices)
	{
		bounds.encapsulate(v);
	}
	mapping.min = (repo::lib::RepoVector3D)bounds.min();
	mapping.max = (repo::lib::RepoVector3D)bounds.max();
	mapping.mesh_id = node.getUniqueId();
	mapping.shared_id = node.getSharedId();

	// Get material information
	auto matNode = matPropMap.at(node.getSharedId());
	mapping.material_id = matNode->getUniqueID();
	mapping.material = matNode->getMaterialStruct();

	// set texture id if passed in
	if (!texId.isDefaultValue())
		mapping.texture_id = texId;

	mapped.meshMapping.push_back(mapping);

	createSuperMesh(mapped, namedGrouping);
}

void MultipartOptimizer::splitMesh(
	repo::core::model::StreamingMeshNode& node,
	const MaterialPropMap& matPropMap,
	const repo::lib::RepoUUID& texId,
	const std::string& namedGrouping
)
{
	// Note: Explanation of the advancing front approach in header.

	auto start = std::chrono::high_resolution_clock::now();

	auto bvh = buildFacesBvh(node);
	const auto& faces = node.getLoadedFaces();

	// Flatten the tree to process leaves and branch nodes separately
	std::vector<size_t> leaves;
	std::vector<size_t> branches;
	flattenBvh(bvh, leaves, branches);

	// Create structure to store the sets for the unique vertices.
	// The majority of this vector will always be null pointers.
	// Only fields corresponding to nodes currently used by the advancing
	// front will have a valid pointer at any given time.
	std::vector<std::unique_ptr<std::set<uint32_t>>> verts;
	verts.resize(bvh.node_count);

	// Create structure to store the vectors for unique faces
	// Only fields corresponding to nodes currently used by the advancing
	// front will have a valid pointer at any given time.
	std::vector<std::unique_ptr<std::vector<uint32_t>>> primitives;
	primitives.resize(bvh.node_count);

	// First, do the leaves.
	for (const auto nodeIndex : leaves)
	{
		auto& node = bvh.nodes[nodeIndex];
		auto uniqueVertices = std::make_unique<std::set<uint32_t>>();
		auto uniquePrimitives = std::make_unique<std::vector<uint32_t>>();
		for (int i = 0; i < node.primitive_count; i++)
		{
			// Gather primitive indices
			auto primitiveIndex = bvh.primitive_indices[node.first_child_or_primitive + i];
			uniquePrimitives->push_back(primitiveIndex);

			// Gather unique vertex indices
			auto& face = faces[primitiveIndex];
			std::copy(face.begin(), face.end(), std::inserter(*uniqueVertices, uniqueVertices->end()));
		}

		// Move pointers into the front
		verts[nodeIndex] = std::move(uniqueVertices);
		primitives[nodeIndex] = std::move(uniquePrimitives);
	}


	// Now, we do the branch nodes

	// Reverse order of branch nodes so we process bottom to top
	std::reverse(branches.begin(), branches.end());

	int meshesCreated = 0;
	for (const auto nodeIndex : branches)
	{
		auto& bvhNode = bvh.nodes[nodeIndex];

		// Get vertex indices from the children
		auto& leftVerts = verts[bvhNode.first_child_or_primitive];
		auto& rightVerts = verts[bvhNode.first_child_or_primitive + 1];

		// Get primitive indices from the children
		auto& leftPrimitives = primitives[bvhNode.first_child_or_primitive];
		auto& rightPrimitives = primitives[bvhNode.first_child_or_primitive + 1];

		// If we find the pointers to be nullptrs, the branch has been removed already.
		// We can skip that node.
		if (leftVerts == nullptr && rightVerts == nullptr)
			continue;
		else if (rightVerts == nullptr)
		{
			// If only the left is valid, we check just that against the threshold
			if (leftVerts->size() <= REPO_MP_MAX_VERTEX_COUNT)
			{
				// If it is under the threshold, we can just move the pointers up
				verts[nodeIndex] = std::move(leftVerts);
				primitives[nodeIndex] = std::move(leftPrimitives);
			}
			else
			{
				// If it does exceed the threshold, we cut this branch off
				createSupermeshFromBranch(
					node,
					matPropMap,
					texId,
					leftVerts.get(),
					leftPrimitives.get(),
					namedGrouping);
				meshesCreated++;

				// Release vertex and primitive indices from memory
				leftVerts.reset();
				leftPrimitives.reset();
			}
		}
		else if (leftVerts == nullptr)
		{
			// If only the right is valid, we check just that against the threshold
			if (rightVerts->size() <= REPO_MP_MAX_VERTEX_COUNT)
			{
				// If it is under the threshold, we can just move the pointers up
				verts[nodeIndex] = std::move(rightVerts);
				primitives[nodeIndex] = std::move(rightPrimitives);
			}
			else
			{
				// If it does exceed the threshold, we cut this branch off
				createSupermeshFromBranch(
					node,
					matPropMap,
					texId,
					rightVerts.get(),
					rightPrimitives.get(),
					namedGrouping);
				meshesCreated++;

				// Release vertex and primitive indices from memory
				rightVerts.reset();
				rightPrimitives.reset();
			}
		}
		else
		{
			// If both are valid, we will need to combine the vertex indices

			// Combine both sets
			auto uniqueVertices = std::make_unique<std::set<uint32_t>>();
			std::copy(leftVerts->begin(), leftVerts->end(), std::inserter(*uniqueVertices, uniqueVertices->end()));
			std::copy(rightVerts->begin(), rightVerts->end(), std::inserter(*uniqueVertices, uniqueVertices->end()));

			// Check whether the new size exceeds the threshold
			if (uniqueVertices->size() <= REPO_MP_MAX_VERTEX_COUNT)
			{
				// If it does not, insert the new set and reset the sets of the two children to free memory
				verts[nodeIndex] = std::move(uniqueVertices);
				leftVerts.reset();
				rightVerts.reset();

				// Then combine the primitives, by appending the right to the left
				leftPrimitives->insert(leftPrimitives->end(), rightPrimitives->begin(), rightPrimitives->end());

				// Now move the pointer of the left (now containing both) up to the parent
				primitives[nodeIndex] = std::move(leftPrimitives);

				// Lastly, reset the pointer for the right, releasing the memory for those primitives
				rightPrimitives.reset();
			}
			else {
				// If it does exceed the threshold, then both children will be branches that are cut off.

				// Release the memory of the combined set as early as possible
				uniqueVertices.reset();

				// First, do the left
				{
					createSupermeshFromBranch(
						node,
						matPropMap,
						texId,
						leftVerts.get(),
						leftPrimitives.get(),
						namedGrouping);
					meshesCreated++;
				}

				// Then, the right
				{
					createSupermeshFromBranch(
						node,
						matPropMap,
						texId,
						rightVerts.get(),
						rightPrimitives.get(),
						namedGrouping);
					meshesCreated++;
				}

				// Release vertex and primitive indices from memory
				leftVerts.reset();
				rightVerts.reset();
				leftPrimitives.reset();
				rightPrimitives.reset();
			}
		}
	}

	// Check for leftovers
	// Since we are accumulating from the bottom up now as we are cutting branches away,
	// it is possible to leave one branch with vertices that did not make it over the
	// threshold. Gather them into their own supermesh.
	auto rootIndex = branches.back();
	auto& leftoverVerts = verts[rootIndex];
	auto& leftoverPrimitives = primitives[rootIndex];
	if (leftoverVerts != nullptr)
	{
		createSupermeshFromBranch(
			node,
			matPropMap,
			texId,
			leftoverVerts.get(),
			leftoverPrimitives.get(),
			namedGrouping);
		meshesCreated++;
	}

	repoInfo << "Split mesh with " << node.getNumLoadedVertices() << " vertices into " << meshesCreated << " submeshes in " << CHRONO_DURATION(start) << " ms";
}

std::unique_ptr<repo::core::model::SupermeshNode> MultipartOptimizer::createSupermeshNode(
	const mapped_mesh_t &mapped
)
{
	if (!mapped.meshMapping.size())
	{
		return nullptr;
	}

	//workout bbox from meshMapping
	auto meshMapping = mapped.meshMapping;


	repo::lib::RepoBounds bbox(meshMapping[0].min, meshMapping[0].max);
	for (int i = 1; i < meshMapping.size(); ++i)
	{
		bbox.encapsulate(meshMapping[i].min);
		bbox.encapsulate(meshMapping[i].max);
	}

	return repo::core::model::RepoBSONFactory::makeSupermeshNode(
		mapped.vertices,
		mapped.faces,
		mapped.normals,
		bbox,
		mapped.uvChannels,
		"",
		meshMapping);
}

// Builds a Bvh of the bounds of the MeshNodes. Each MeshNode in the array is
// the primitive.

MultipartOptimizer::Bvh MultipartOptimizer::buildBoundsBvh(
	const std::vector<int>& binIndexes,
	const std::vector<repo::core::model::StreamingMeshNode>& meshes
)
{
	// The BVH builder requires a set of bounding boxes and centers to work with.
	// Each of these corresponds to a primitive. The primitive indices in the tree
	// refer to entries in this list, and so the entries in meshes, if the order
	// is the same (which it is).

	auto boundingBoxes = std::vector<bvh::BoundingBox<Scalar>>();
	for (const int index : binIndexes)
	{
		auto& node = meshes[index];
		auto bounds = node.getBoundingBox();
		auto min = BvhVector3(bounds.min().x, bounds.min().y, bounds.min().z);
		auto max = BvhVector3(bounds.max().x, bounds.max().y, bounds.max().z);
		boundingBoxes.push_back(bvh::BoundingBox<Scalar>(min, max));
	}

	auto centers = std::vector<BvhVector3>();
	for (const auto& bounds : boundingBoxes)
	{
		centers.push_back(bounds.center());
	}

	auto globalBounds = bvh::compute_bounding_boxes_union(boundingBoxes.data(), boundingBoxes.size());

	// Create an acceleration data structure based on the bounds

	Bvh bvh;
	bvh::SweepSahBuilder<Bvh> builder(bvh);
	builder.build(globalBounds, boundingBoxes.data(), centers.data(), boundingBoxes.size());

	return bvh;
}

// Gets the vertex counts of each node in the Bvh, when the Bvh primitives are
// MeshNodes.

std::vector<size_t> MultipartOptimizer::getVertexCounts(
	const Bvh& bvh,
	const std::vector<int>& binIndexes,
	const std::vector<repo::core::model::StreamingMeshNode>& meshes
)
{	// To do this, get the nodes in list form, 'bottom up', in order to set the
	// vertex counts of each leaf node.

	std::vector<size_t> leaves;
	std::vector<size_t> flattened;
	flattenBvh(bvh, leaves, flattened);

	// Next initialise the vertex count of the leaves

	std::vector<size_t> vertexCounts;
	vertexCounts.resize(bvh.node_count);

	for (const auto nodeIndex : leaves)
	{
		size_t vertexCount = 0;
		auto& node = bvh.nodes[nodeIndex];
		for (int i = 0; i < node.primitive_count; i++)
		{
			auto primitiveIndex = bvh.primitive_indices[node.first_child_or_primitive + i];

			if (primitiveIndex < 0 || primitiveIndex >= binIndexes.size())
			{
				throw repo::lib::RepoException("Primitive index out of bounds when calculating vertex counts for BVH nodes. This likely means there is a mismatch between the primitives in the BVH and the input meshes.");
			}

			auto& node = meshes[binIndexes[primitiveIndex]];
			vertexCount += node.getNumVertices();
		}

		vertexCounts[nodeIndex] = vertexCount;
	}

	// Now, moving backwards, add up for each branch node the number of vertices in its children

	std::reverse(flattened.begin(), flattened.end());

	for (const auto nodeIndex : flattened)
	{
		auto& node = bvh.nodes[nodeIndex];
		vertexCounts[nodeIndex] = vertexCounts[node.first_child_or_primitive] + vertexCounts[node.first_child_or_primitive + 1];
	}

	return vertexCounts;
}

// Groups MeshNodes into clusters based on their location (given by the bounds)
// and vertex count.

void MultipartOptimizer::clusterMeshNodesBvh(
	const std::vector<repo::core::model::StreamingMeshNode> &meshes,
	const std::vector<int> &binIndexes,
	std::vector<std::vector<int>> &clusters)
{
	auto bvh = buildBoundsBvh(binIndexes, meshes);

	// The tree contains all the submesh bounds grouped in space.
	// Create a list of vertex counts for each node so we can decide where to
	// prune in order to build the clusters.

	auto vertexCounts = getVertexCounts(bvh, binIndexes, meshes);

	// Next, traverse the tree again, but this time depth first, cutting the tree
	// at places the vertex count drops below a target threshold.

	auto branchNodes = getSupermeshBranchNodes(bvh, vertexCounts);

	// Finally, get all the leaf nodes for each branch in order to build the
	// groups of MeshNodes

	for (const auto head : branchNodes)
	{
		std::vector<int> cluster;
		for (const auto primitive : getBranchPrimitives(bvh, head))
		{
			cluster.push_back(binIndexes[primitive]);
		}
		clusters.push_back(cluster);
	}
}

void MultipartOptimizer::splitBigClusters(
	std::vector<std::vector<int>>& clusters)
{
	auto clustersToSplit = std::vector<std::vector<int>>();
	clusters.erase(std::remove_if(clusters.begin(), clusters.end(),
		[&](std::vector<int> cluster)
		{
			if (cluster.size() > REPO_MP_MAX_MESHES_IN_SUPERMESH)
			{
				clustersToSplit.push_back(cluster);
				return true;
			}
			else
			{
				return false;
			}
		}),
		clusters.end()
	);

	std::vector<int> cluster;
	for (auto const clusterToSplit : clustersToSplit)
	{
		int i = 0;
		while (i < clusterToSplit.size())
		{
			cluster.push_back(clusterToSplit[i++]);
			if (cluster.size() >= REPO_MP_MAX_MESHES_IN_SUPERMESH)
			{
				clusters.push_back(std::vector<int>(cluster));
				cluster.clear();
			}
		}

		if (cluster.size())
		{
			clusters.push_back(std::vector<int>(cluster));
		}
	}
}

std::vector<std::vector<int>> MultipartOptimizer::clusterMeshNodes(
	const std::vector<repo::core::model::StreamingMeshNode>& meshes
)
{
	// Takes one set of MeshNodes and groups them into N sets of MeshNodes,
	// where each set will form a Supermesh.

	repoInfo << "Clustering " << meshes.size() << " meshes...";
	auto start = std::chrono::high_resolution_clock::now();

	// Start by creating a metric for each mesh which will be used to group
	// nodes by their efficiency

	struct meshMetric {
		int nodeIndex;
		float efficiency;
	};

	std::vector<meshMetric> metrics(meshes.size());
	for (size_t i = 0; i < meshes.size(); i++)
	{
		auto& mesh = metrics[i];
		mesh.nodeIndex = i;
		auto& node = meshes[i];
		auto bounds = node.getBoundingBox();
		auto numVertices = node.getNumVertices();
		auto width = bounds.size().x;
		auto height = bounds.size().y;
		auto length = bounds.size().z;
		auto metric = (float)numVertices / (width + height + length);
		mesh.efficiency = metric;
	}

	std::sort(metrics.begin(), metrics.end(), [](meshMetric a, meshMetric b) {
		return a.efficiency < b.efficiency;
		});

	// Bin the nodes based on vertex count. The bottom 20% will form pure-LOD
	// groups, while the top 80% will be spatialised.

	auto totalVertices = 0;
	for (const auto& mesh : meshes)
	{
		totalVertices += mesh.getNumVertices();
	}

	auto modelLowerThreshold = std::max<int>(REPO_MP_MAX_VERTEX_COUNT, (int)(totalVertices * REPO_MODEL_LOW_CLUSTERING_RATIO));

	std::vector<int> bin;
	auto binVertexCount = 0;
	auto binsVertexCount = 0;

	std::vector<std::vector<int>> clusters;
	auto metricsIterator = metrics.begin();

	while (metricsIterator != metrics.end() && binsVertexCount <= modelLowerThreshold)
	{
		auto& item = *metricsIterator++;
		auto& node = meshes[item.nodeIndex];

		binVertexCount += node.getNumVertices();
		binsVertexCount += node.getNumVertices();
		bin.push_back(item.nodeIndex);

		if (binVertexCount >= REPO_MP_MAX_VERTEX_COUNT) // If we've filled up one supermesh
		{
			// Copy
			auto cluster = std::vector<int>(bin);
			clusters.push_back(cluster);

			// And reset
			bin.clear();
			binVertexCount = 0;
		}
	}

	if (bin.size()) // Either we have some % of the model remaining, or the entire model fits below 65K
	{
		auto cluster = std::vector<int>(bin);
		clusters.push_back(cluster);

		bin.clear();
	}

	// Now the high-part
	while (metricsIterator != metrics.end())
	{
		auto& item = *metricsIterator++;

		bin.push_back(item.nodeIndex);
	}

	if (bin.size())
	{
		clusterMeshNodesBvh(meshes, bin, clusters);
	}

	// If clusters contain too many meshes, we can exceed the maximum BSON size.
	// Do a quick check and split any clusters that are too large.

	splitBigClusters(clusters);

	repoInfo << "Created " << clusters.size() << " clusters in " << CHRONO_DURATION(start) << " milliseconds.";

	return clusters;
}