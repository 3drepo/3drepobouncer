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

#include <algorithm>
#include <chrono>

using namespace repo::lib;
using namespace repo::manipulator::modeloptimizer;

auto defaultGraph = repo::core::model::RepoScene::GraphType::DEFAULT;

//using Scalar = float;
//using Bvh = bvh::Bvh<Scalar>;
//using BvhVector3 = bvh::Vector3<Scalar>;


// The vertex count is used as a rough approximation of the total geometry size.
// This figure is empirically set to end up with an average bundle size of 24 Mb.
static const size_t REPO_MP_MAX_VERTEX_COUNT = 1200000;

// This limit is used to prevent metadata files becoming unwieldly, and the
// supermesh UV resolution falling below the quantisation noise floor.
static const size_t REPO_MP_MAX_MESHES_IN_SUPERMESH = 5000;

static const size_t REPO_BVH_MAX_LEAF_SIZE = 16;
static const size_t REPO_MODEL_LOW_CLUSTERING_RATIO = 0.2f;

#define CHRONO_DURATION(start) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count()

void MultipartOptimizer::processScene(
	std::string database,
	std::string collection,
	repo::lib::RepoUUID revId,
	repo::core::handler::AbstractDatabaseHandler *handler)
{
	// Initialise exporter
#ifdef REPO_ASSET_GENERATOR_SUPPORT
	auto worldOffset = getWorldOffset(database, collection, revId, handler);
	auto exporter = std::make_unique<repo::manipulator::modelconvertor::RepoBundleExport>(
		database,
		collection,
		revId,
		worldOffset
	);

	// Set file upload callback via std::function and lambdas
	auto fileManager = handler->getFileManager();
	exporter->setFileCallback([&](
		const std::string& databaseName,
		const std::string& id,
		const std::string& collectionNamePrefix,
		const std::vector<uint8_t>& bin,
		const std::unordered_map < std::string, repo::lib::RepoVariant>& metadata,
		const repo::core::handler::fileservice::FileManager::Encoding& encoding)
		{
			fileManager->uploadFileAndCommit(
				databaseName,
				id,
				collectionNamePrefix,
				bin,
				metadata,
				encoding
			);
		});
	
	// Set document upsert callback via std::function and lambdas
	exporter->setUpsertCallback([&](
		const std::string& database,
		const std::string& collection,
		const repo::core::model::RepoBSON& obj,
		const bool& overwrite)
		{
			return handler->upsertDocument(
				database,
				collection,
				obj,
				overwrite
			);
		});
#else
	repoError << "Bouncer must be built with REPO_ASSET_GENERATOR_SUPPORT ON in order to generate Repo Bundles.";
	return;
#endif // REPO_ASSETGENERATOR


	// Getting Transforms
	repoInfo << "Getting Transforms";
	auto transformMap = getAllTransforms(handler, database, collection, revId);

	// Get lookup map for material properties
	repoInfo << "Getting Materials";
	auto matPropMap = getAllMaterials(handler, database, collection, revId);

	// Get all groupings
	repoInfo << "Getting Groupings";
	auto groupings = getAllGroupings(handler, database, collection, revId);
	repoInfo << "Found " << groupings.size() << " groupings";

	for (auto grouping : groupings) {

		// Process Group (Opaque, prim 2)
		repoInfo << "Process Group - Opaque, Primitive 2";
		processUntexturedGroup(
			database,
			collection,
			revId,
			handler,
			exporter.get(),
			transformMap,
			matPropMap,
			true,
			2,
			grouping
		);

		// Process Group (Opaque, prim 3)
		repoInfo << "Process Group - Opaque, Primitive 3";
		processUntexturedGroup(
			database,
			collection,
			revId,
			handler,
			exporter.get(),
			transformMap,
			matPropMap,
			true,
			3,
			grouping
		);

		// Process Group (Transparent, prim 2)
		repoInfo << "Process Group - Transparent, Primitive 2";
		processUntexturedGroup(
			database,
			collection,
			revId,
			handler,
			exporter.get(),
			transformMap,
			matPropMap,
			false,
			2,
			grouping
		);

		// Process Group (Transparent, prim 3)
		repoInfo << "Process Group - Transparent, Primitive 3";
		processUntexturedGroup(
			database,
			collection,
			revId,
			handler,
			exporter.get(),
			transformMap,
			matPropMap,
			false,
			3,
			grouping
		);

		// Get Texture IDs
		repoInfo << "Getting all texture Ids";
		auto texIds = getAllTextureIds(handler, database, collection, revId, grouping);

		// Process each texture group
		for (auto texId : texIds) {

			// For line primitives (2)
			// One cannot map a texture to a line, however, customers can assign materials with textures to lines
			// so we need to be able to process them.
			repoInfo << "Process Group - Textured, " << texId.toString();
			processTexturedGroup(
				database,
				collection,
				revId,
				handler,
				exporter.get(),
				transformMap,
				matPropMap,
				texId,
				2,
				grouping
			);

			// For triangle primitives (3)
			processTexturedGroup(
				database,
				collection,
				revId,
				handler,
				exporter.get(),
				transformMap,
				matPropMap,
				texId,
				3,
				grouping
			);
		}

	}

	// Finalise export
	exporter->Finalise();
}

std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoMatrix, repo::lib::RepoUUIDHasher> MultipartOptimizer::getAllTransforms(
	repo::core::handler::AbstractDatabaseHandler *handler,
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
		
	std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoMatrix, repo::lib::RepoUUIDHasher> leafToTransformMap;

	if (cursor) {
		repo::core::model::RepoBSON rootNode;
		std::unordered_map<repo::lib::RepoUUID, std::vector<repo::core::model::RepoBSON>, repo::lib::RepoUUIDHasher> childNodeMap;
		for (auto bson : (*cursor)) {
			if (bson.hasField(REPO_NODE_LABEL_PARENTS)) {
				auto parentId = bson.getUUIDFieldArray(REPO_NODE_LABEL_PARENTS)[0];

				if (childNodeMap.contains(parentId)) {
					childNodeMap.at(parentId).push_back(bson);
				}
				else {
					auto children = std::vector<repo::core::model::RepoBSON>{ bson };
					childNodeMap.insert({ parentId, children });
				}
			}
			else {
				rootNode = bson;
			}
		}
		if (childNodeMap.size() == 0) {
			repoWarning << "getAllTransforms; no transformations returned by database query.";
		}
		else {
		traverseTransformTree(rootNode, childNodeMap, leafToTransformMap);
	}
	}
	else {
		repoWarning << "getAllTransforms; getting cursor was not successful; no transforms in output map";
	}

	return leafToTransformMap;
}

void MultipartOptimizer::traverseTransformTree(
	const repo::core::model::RepoBSON &root,
	const std::unordered_map<repo::lib::RepoUUID, std::vector<repo::core::model::RepoBSON>,	repo::lib::RepoUUIDHasher> &childNodeMap,
	std::unordered_map<repo::lib::RepoUUID,	repo::lib::RepoMatrix, repo::lib::RepoUUIDHasher> &leafTransforms)
{

	// Create stacks for the nodes and the matrices
	std::stack<repo::core::model::RepoBSON> bsonStack;
	std::stack<repo::lib::RepoMatrix> matStack;

	// Starting matrix
	repo::lib::RepoMatrix identity;

	// Push starting node and starting matrix on the respective stacks
	bsonStack.push(root);
	matStack.push(identity);

	// DFS traversal of the transformation tree, summing up the matrices along the way
	while (!bsonStack.empty()) {

		// Remove top node from the stack
		auto topBson = bsonStack.top();
		bsonStack.pop();
		auto topMat = matStack.top();
		matStack.pop();

		// Get node information
		auto nodeId = topBson.getUUIDField(REPO_NODE_LABEL_SHARED_ID);
		auto matrix = topBson.getMatrixField(REPO_NODE_LABEL_MATRIX);

		// Apply the node's trnsaformation
		auto newMat = matrix * topMat;
				
		if (childNodeMap.contains(nodeId)) {
			// If the node has children, push children on the stack
			auto children = childNodeMap.at(nodeId);
			for (auto child : children) {
				bsonStack.push(child);
				matStack.push(newMat);
			}
		}
		else {
			// If it has no children, it is a leaf.
			// Put result matrix in the final map
			leafTransforms.insert({ nodeId, newMat });
		}
	}
}

MultipartOptimizer::MaterialPropMap MultipartOptimizer::getAllMaterials(
	repo::core::handler::AbstractDatabaseHandler *handler,
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
		
		// Create material struct
		repo::lib::RepoUUID uniqueId = materialBson.getUUIDField(REPO_NODE_LABEL_ID);
		auto material = std::make_shared<std::pair<repo::lib::RepoUUID, repo_material_t>>();

		// Set material id
		material->first = materialBson.getUUIDField(REPO_NODE_LABEL_ID);

		// Fill material struct with values
		if (materialBson.hasField(REPO_NODE_MATERIAL_LABEL_AMBIENT))
			material->second.ambient = materialBson.getColourField(REPO_NODE_MATERIAL_LABEL_AMBIENT);
		if (materialBson.hasField(REPO_NODE_MATERIAL_LABEL_DIFFUSE))
			material->second.diffuse = materialBson.getColourField(REPO_NODE_MATERIAL_LABEL_DIFFUSE);
		if (materialBson.hasField(REPO_NODE_MATERIAL_LABEL_SPECULAR))
			material->second.specular = materialBson.getColourField(REPO_NODE_MATERIAL_LABEL_SPECULAR);
		if (materialBson.hasField(REPO_NODE_MATERIAL_LABEL_EMISSIVE))
			material->second.emissive = materialBson.getColourField(REPO_NODE_MATERIAL_LABEL_EMISSIVE);
		if (materialBson.hasField(REPO_NODE_MATERIAL_LABEL_OPACITY))
			material->second.opacity = materialBson.getDoubleField(REPO_NODE_MATERIAL_LABEL_OPACITY);
		if (materialBson.hasField(REPO_NODE_MATERIAL_LABEL_SHININESS))
			material->second.shininess = materialBson.getDoubleField(REPO_NODE_MATERIAL_LABEL_SHININESS);
		if (materialBson.hasField(REPO_NODE_MATERIAL_LABEL_SHININESS_STRENGTH))
			material->second.shininessStrength = materialBson.getDoubleField(REPO_NODE_MATERIAL_LABEL_SHININESS_STRENGTH);
		if (materialBson.hasField(REPO_NODE_MATERIAL_LABEL_LINE_WEIGHT))
			material->second.lineWeight = materialBson.getDoubleField(REPO_NODE_MATERIAL_LABEL_LINE_WEIGHT);
		if (materialBson.hasField(REPO_NODE_MATERIAL_LABEL_WIREFRAME))
			material->second.isWireframe = materialBson.getBoolField(REPO_NODE_MATERIAL_LABEL_WIREFRAME);
		if (materialBson.hasField(REPO_NODE_MATERIAL_LABEL_TWO_SIDED))
			material->second.opacity = materialBson.getBoolField(REPO_NODE_MATERIAL_LABEL_TWO_SIDED);

		// Go over the parents and add the pointer for each so that the map can be used to lookup
		// the material for a given meshNode
		auto parents = materialBson.getUUIDFieldArray(REPO_NODE_LABEL_PARENTS);
		for (auto parent : parents) {
			matMap.insert({ parent, material });
		}
	}

	return matMap;
}

std::vector<std::string> MultipartOptimizer::getAllGroupings(
	repo::core::handler::AbstractDatabaseHandler* handler,
	const std::string& database,
	const std::string& collection,
	const repo::lib::RepoUUID& revId
) {
	// Create filter
	repo::core::handler::database::query::RepoQueryBuilder filter;
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_REVISION_ID, revId));
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_LABEL_TYPE, std::string(REPO_NODE_TYPE_MESH)));
	filter.append(repo::core::handler::database::query::Exists(REPO_NODE_MESH_LABEL_GROUPING, true));

	repo::core::handler::database::query::RepoProjectionBuilder projection;
	projection.excludeField(REPO_NODE_LABEL_ID);
	projection.includeField(REPO_NODE_MESH_LABEL_GROUPING);

	std::vector<std::string> groupings;

	auto sceneCollection = collection + "." + REPO_COLLECTION_SCENE;
	auto cursor = handler->findCursorByCriteria(database, sceneCollection, filter, projection);

	if (cursor) {
		for (auto document : (*cursor)) {
			auto bson = repo::core::model::RepoBSON(document);
			groupings.push_back(bson.getStringField(REPO_NODE_MESH_LABEL_GROUPING));
		}
	}
	else {
		repoWarning << "getAllGroupings; getting cursor was not successful; no groupings from db in output vector";
	}

	// If we found no groupings, then we have just one pass without so we add "".
	if (groupings.size() == 0)
		groupings.push_back("");

	return groupings;
}

std::vector<repo::lib::RepoUUID> MultipartOptimizer::getAllTextureIds(
	repo::core::handler::AbstractDatabaseHandler *handler,
	const std::string &database,
	const std::string &collection,
	const repo::lib::RepoUUID &revId,
	const std::string& grouping) {

	// Create filter
	repo::core::handler::database::query::RepoQueryBuilder filter;
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_REVISION_ID, revId));
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_LABEL_TYPE, std::string(REPO_NODE_TYPE_TEXTURE)));
	if (!grouping.empty())
		filter.append(repo::core::handler::database::query::Eq(REPO_NODE_MESH_LABEL_GROUPING, grouping));

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

void MultipartOptimizer::processUntexturedGroup(
	const std::string &database,
	const std::string &collection,
	const repo::lib::RepoUUID &revId,
	repo::core::handler::AbstractDatabaseHandler *handler,
	repo::manipulator::modelconvertor::RepoBundleExport *exporter,
	const TransformMap& transformMap,
	const MaterialPropMap& matPropMap,
	const bool isOpaque,
	const int primitive,
	const std::string& grouping)
{
	// Create filter
	repo::core::handler::database::query::RepoQueryBuilder filter;
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_REVISION_ID, revId));
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_MESH_LABEL_PRIMITIVE, primitive));
	if (!grouping.empty())
		filter.append(repo::core::handler::database::query::Eq(REPO_NODE_MESH_LABEL_GROUPING, grouping));
	if (isOpaque)
		filter.append(repo::core::handler::database::query::Eq(REPO_FILTER_TAG_OPAQUE, true));
	else
		filter.append(repo::core::handler::database::query::Eq(REPO_FILTER_TAG_TRANSPARENT, true));	

	// Use filter to cluster and supermesh
	clusterAndSupermesh(
		database,
		collection,
		handler,
		exporter,
		transformMap,
		matPropMap,
		filter);	
}

void MultipartOptimizer::processTexturedGroup(
	const std::string &database,
	const std::string &collection,
	const repo::lib::RepoUUID &revId,
	repo::core::handler::AbstractDatabaseHandler *handler,
	repo::manipulator::modelconvertor::RepoBundleExport *exporter,
	const TransformMap& transformMap,
	const MaterialPropMap& matPropMap,
	const repo::lib::RepoUUID texId,
	const int primitive,
	const std::string& grouping)
{
	// Create filter
	repo::core::handler::database::query::RepoQueryBuilder filter;
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_REVISION_ID, revId));	
	filter.append(repo::core::handler::database::query::Eq(REPO_FILTER_TAG_TEXTURE_ID, texId));
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_MESH_LABEL_PRIMITIVE, primitive));
	if (!grouping.empty())
		filter.append(repo::core::handler::database::query::Eq(REPO_NODE_MESH_LABEL_GROUPING, grouping));
	

	// Use filter to cluster and supermesh
	clusterAndSupermesh(
		database,
		collection,
		handler,
		exporter,
		transformMap,
		matPropMap,
		filter,
		texId);
}

void MultipartOptimizer::clusterAndSupermesh(
	const std::string &database,
	const std::string &collection,
	repo::core::handler::AbstractDatabaseHandler *handler,
	repo::manipulator::modelconvertor::RepoBundleExport *exporter,
	const TransformMap& transformMap,
	const MaterialPropMap& matPropMap,
	const repo::core::handler::database::query::RepoQuery filter,
	const repo::lib::RepoUUID &texId /* = repo::lib::RepoUUID() */
) {
	// Create projection
	repo::core::handler::database::query::RepoProjectionBuilder projection;
	projection.excludeField(REPO_NODE_LABEL_ID);
	projection.includeField(REPO_NODE_LABEL_SHARED_ID);
	projection.includeField(REPO_NODE_MESH_LABEL_BOUNDING_BOX);
	projection.includeField(REPO_NODE_MESH_LABEL_VERTICES_COUNT);
	projection.includeField(REPO_NODE_LABEL_PARENTS);

	// Get cursor
	auto sceneCollection = collection + "." + REPO_COLLECTION_SCENE;
	auto cursor = handler->findCursorByCriteria(database, sceneCollection, filter, projection);


	// iterate cursor and pack outcomes in lightweight mesh node structure
	std::vector<StreamingMeshNode> nodes;
	if (cursor) {
		for (auto bson : (*cursor)) {
			nodes.push_back(StreamingMeshNode(bson));
		}
	}
	else {
		repoWarning << "clusterAndSupermesh; getting cursor was not successful; no nodes filtered for processing";
	}

	// Transform bounds before clustering
	for (auto& node : nodes) {
		auto bounds = node.getBoundingBox();

		// Transform bounds
		auto parentId = node.getParent();		

		if (transformMap.contains(parentId)) {
			auto transMat = transformMap.at(node.getParent());
			node.transformBounds(transMat);
		}
		else {
			repoWarning << "clusterAndSupermesh; current node has no transform parents; this should not happen; malformed file?";
		}		
	}

	// Cluster the mesh nodes
	repoInfo << "Clustering Nodes";
	auto clusters = clusterMeshNodes(nodes);

	// Create Supermeshes from the clusters
	repoInfo << "Creating Supermeshes from clustered Nodes";
	createSuperMeshes(database, collection, handler, exporter, transformMap, matPropMap, nodes, clusters, texId);
}

void repo::manipulator::modeloptimizer::MultipartOptimizer::createSuperMeshes(
	const std::string &database,
	const std::string &collection,
	repo::core::handler::AbstractDatabaseHandler *handler,
	repo::manipulator::modelconvertor::RepoBundleExport *exporter,
	const TransformMap& transformMap,
	const MaterialPropMap& matPropMap,
	std::vector<StreamingMeshNode>& meshNodes,
	const std::vector<std::vector<int>>& clusters,
	const repo::lib::RepoUUID &texId)
{
	// Get blobHandler
	auto sceneCollection = collection + "." + REPO_COLLECTION_SCENE;
	repo::core::handler::fileservice::BlobFilesHandler blobHandler(handler->getFileManager(), database, sceneCollection);
	
	for (auto cluster : clusters) {

		std::unordered_map<repo::lib::RepoUUID, int, repo::lib::RepoUUIDHasher> clusterMap;
		std::vector<repo::lib::RepoUUID> clusterIds;
		for (auto& index : cluster) {
			auto& node = meshNodes[index];
			auto sharedId = node.getSharedId();
			clusterMap.insert({ sharedId, index });

			clusterIds.push_back(sharedId);
		}

		// Create filter
		auto filter = repo::core::handler::database::query::Eq(REPO_NODE_LABEL_SHARED_ID, clusterIds);

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
			auto parentId = sNode.getParent();
			if (transformMap.contains(parentId)) {
				auto transform = transformMap.at(parentId);
				sNode.bakeMeshes(transform);
			}
			else {
				repoWarning << "createSuperMeshes; no transform found for this mesh node. Mesh will not be baked";
			}
			
			if (currentSupermesh.vertices.size() + sNode.getNumVertices() <= REPO_MP_MAX_VERTEX_COUNT)
			{
				// The current node can be added to the supermesh OK
				appendMesh(sNode, matPropMap, currentSupermesh, texId);
			}
			else if (sNode.getNumVertices() > REPO_MP_MAX_VERTEX_COUNT)
			{
				// The node is too big to fit into any supermesh, so it must be split
				splitMesh(sNode, exporter, matPropMap, texId);
			}
			else
			{
				// The node is small enough to fit within one supermesh, just not this one
				createSuperMesh(exporter, currentSupermesh);
				currentSupermesh = mapped_mesh_t();
				appendMesh(sNode, matPropMap, currentSupermesh, texId);
			}

			// Unload the streaming node
			sNode.unloadSupermeshingData();

		}

		// Add the last supermesh to be built
		if (currentSupermesh.vertices.size()) {
			createSuperMesh(exporter, currentSupermesh);
		}

	}
}

void MultipartOptimizer::createSuperMesh(
	repo::manipulator::modelconvertor::RepoBundleExport *exporter,
	const mapped_mesh_t& mappedMesh)
{
	// Create supermesh node
	auto supermeshNode = createSupermeshNode(mappedMesh);

	exporter->addSupermesh(supermeshNode.get());
}

std::vector<double> MultipartOptimizer::getWorldOffset(
	const std::string &database,
	const std::string &collection,
	const repo::lib::RepoUUID &revId,
	repo::core::handler::AbstractDatabaseHandler *handler)
{
	auto bson = handler->findOneByUniqueID(database, collection + "." + REPO_COLLECTION_HISTORY, revId);
	if (bson.hasField(REPO_NODE_REVISION_LABEL_WORLD_COORD_SHIFT))
	{
		return bson.getDoubleVectorField(REPO_NODE_REVISION_LABEL_WORLD_COORD_SHIFT);
	}

	return std::vector<double>({ 0, 0, 0 });
}

void MultipartOptimizer::appendMesh(	
	StreamingMeshNode &node,
	const MaterialPropMap &matPropMap,
	mapped_mesh_t &mapped,
	const repo::lib::RepoUUID &texId
)
{
	repo_mesh_mapping_t meshMap;

	// Get material information
	auto matPair = matPropMap.at(node.getSharedId());
	meshMap.material_id = matPair->first;
	meshMap.material = matPair->second;
	
	// set texture id if passed in
	if (!texId.isDefaultValue())
		meshMap.texture_id = texId;

	meshMap.mesh_id = node.getUniqueId();
	meshMap.shared_id = node.getSharedId();

	auto bbox = node.getBoundingBox();
	meshMap.min = (repo::lib::RepoVector3D)bbox.min();
	meshMap.max = (repo::lib::RepoVector3D)bbox.max();

	std::vector<repo::lib::RepoVector3D> submVertices = node.getVertices();
	std::vector<repo::lib::RepoVector3D> submNormals = node.getNormals();
	std::vector<repo_face_t> submFaces = node.getFaces();
	std::vector<std::vector<repo::lib::RepoVector2D>> submUVs = node.getUVChannelsSeparated();

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
			repoError << "Unexpected transformedMesh format mismatch occured!";
		}
	}
	else
	{
		repoError << "Failed merging meshes: Vertices or faces cannot be null!";
	}
}

// Constructs a bounding volumne hierarchy of all the Faces in the MeshNode

MultipartOptimizer::Bvh MultipartOptimizer::buildFacesBvh(
	StreamingMeshNode &node
)
{
	// Create a set of bounding boxes & centers for each Face in the oversized
	// mesh.
	// The BVH builder expects a set of bounding boxes and centers to work with.

	auto faces = node.getFaces();
	auto vertices = node.getVertices();
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

// For each node in the Bvh, return a list of unique vertex Ids that are
// referenced by the faces (primitives) in that node.

std::vector<std::set<uint32_t>> MultipartOptimizer::getUniqueVertices(
	const Bvh& bvh,
	const std::vector<repo_face_t>& primitives // The primitives in this tree are faces
)
{
	// Before starting to accumulate the indices, we need to split the tree into
	// leaf and branch nodes, so we can update the vertices for all the nodes
	// from the bottom up...
	// We can do this by peforming a breadth first traversal.

	std::vector<size_t> leaves;
	std::vector<size_t> branches;
	flattenBvh(bvh, leaves, branches);

	// Next get the unique vertex indices for each leaf node...

	std::vector<std::set<uint32_t>> uniqueVerticesByNode;
	uniqueVerticesByNode.resize(bvh.node_count);

	for (const auto nodeIndex : leaves)
	{
		auto& node = bvh.nodes[nodeIndex];
		auto& uniqueVertices = uniqueVerticesByNode[nodeIndex];
		for (int i = 0; i < node.primitive_count; i++)
		{
			auto primitiveIndex = bvh.primitive_indices[node.first_child_or_primitive + i];
			auto& face = primitives[primitiveIndex];
			std::copy(face.begin(), face.end(), std::inserter(uniqueVertices, uniqueVertices.end()));
		}
	}

	// ...and extend these into each branch node

	std::reverse(branches.begin(), branches.end());

	for (const auto nodeIndex : branches)
	{
		auto& node = bvh.nodes[nodeIndex];
		auto& uniqueVertices = uniqueVerticesByNode[nodeIndex];
		auto& left = uniqueVerticesByNode[node.first_child_or_primitive];
		auto& right = uniqueVerticesByNode[node.first_child_or_primitive + 1];

		std::copy(left.begin(), left.end(), std::inserter(uniqueVertices, uniqueVertices.end()));
		std::copy(right.begin(), right.end(), std::inserter(uniqueVertices, uniqueVertices.end()));
	}

	return uniqueVerticesByNode;
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

void MultipartOptimizer::splitMesh(
	StreamingMeshNode &node,
	repo::manipulator::modelconvertor::RepoBundleExport *exporter,
	const MaterialPropMap &matPropMap,
	const repo::lib::RepoUUID &texId
)
{
	// The purpose of this method is to split large MeshNodes into smaller ones.
	// We do this by splitting *faces* into groups, so we don't have to worry
	// about splitting faces.
	// The vertex arrays are then rebuilt based on the demands of the faces.

	auto start = std::chrono::high_resolution_clock::now();

	// Start by creating a bvh of all the faces in the oversized mesh

	auto bvh = buildFacesBvh(node);

	// The tree now contains all the faces. To know where to cut the tree, we
	// need to get the number of vertices referenced by each node.
	// We get the vertex counts by first computing all the vertices referenced
	// by the node(s), which will be used in the re-indexing.

	auto faces = node.getFaces();
	auto uniqueVerticesByNode = getUniqueVertices(bvh, faces);

	auto vertexCounts = std::vector<size_t>();
	for (const auto set : uniqueVerticesByNode)
	{
		vertexCounts.push_back(set.size());
	}

	// Next, traverse the tree again, but this time depth first, cutting the tree
	// at nodes where the vertex count drops below the target threshold.

	auto branchNodes = getSupermeshBranchNodes(bvh, vertexCounts);

	// (getSupermeshBranchNodes will also return on leaves. Perform a quick check
	// to make sure only have branches.)

	branchNodes.erase(std::remove_if(branchNodes.begin(), branchNodes.end(),
		[&](size_t node) {
			if (bvh.nodes[node].is_leaf())
			{
				// We should never make it to a leaf with more than 65k vertices,
				// because the leaves should have at most 16 faces.
				repoError << "splitMesh() encountered a leaf node with " << vertexCounts[node] << " unique vertices across " << bvh.nodes[node].primitive_count << " faces.";
				return true;
			}
			else
			{
				return false;
			}
		}),
		branchNodes.end()
	);

	// Finally, get the geometry from under these branches and return it as a
	// set of mapped_mesh_t instances

	// Get the vertex attributes for building the sub mapped meshes

	auto vertices = node.getVertices();
	auto normals = node.getNormals();
	auto uvChannels = node.getUVChannelsSeparated();

	for (const auto head : branchNodes)
	{
		// Get all the faces, from all the leaf nodes within the branch

		std::vector<size_t> primitives = getBranchPrimitives(bvh, head);

		// Now collect the faces into a new mesh.

		// Re-index each face for the new reduced vertex arrays; this can be
		// done through a reverse lookup into the set of unique vertices
		// referenced by all the faces in the new mesh (i.e. at the branch node).

		auto& meshGlobalIndicesSet = uniqueVerticesByNode[head];
		std::vector<size_t> globalVertexIndices(meshGlobalIndicesSet.begin(), meshGlobalIndicesSet.end()); // An array of indices into the gloabl vertex array, for this submesh
		std::map<size_t, size_t> globalToLocalIndex;

		// Create the inverse lookup table for the re-indexing

		for (auto i = 0; i < globalVertexIndices.size(); i++)
		{
			globalToLocalIndex[globalVertexIndices[i]] = i;
		}

		mapped_mesh_t mapped;

		for (const auto faceIndex : primitives)
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

		for (const auto globalIndex : globalVertexIndices)
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
		auto matPair = matPropMap.at(node.getSharedId());
		mapping.material_id = matPair->first;
		mapping.material = matPair->second;		

		// set texture id if passed in
		if (!texId.isDefaultValue())
			mapping.texture_id = texId;

		mapped.meshMapping.push_back(mapping);

		createSuperMesh(exporter, mapped);		
	}

	repoInfo << "Split mesh with " << node.getNumVertices() << " vertices into " << branchNodes.size() << " submeshes in " << CHRONO_DURATION(start) << " ms";
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
	const std::vector<StreamingMeshNode>& meshes
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
	const std::vector<StreamingMeshNode>& meshes
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
				repoError << "Bvh primitive index out of range. This means something has gone wrong with the BVH construction.";
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
	const std::vector<StreamingMeshNode> &meshes,
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
	const std::vector<MultipartOptimizer::StreamingMeshNode>& meshes
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