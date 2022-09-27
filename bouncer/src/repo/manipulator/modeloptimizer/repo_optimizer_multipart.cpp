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
#include "../../core/model/bson/repo_bson_factory.h"
#include "../../core/model/bson/repo_bson_builder.h"

#include "boost/chrono.hpp"

using namespace repo::manipulator::modeloptimizer;

auto defaultGraph = repo::core::model::RepoScene::GraphType::DEFAULT;

static const size_t REPO_MP_MAX_VERTEX_COUNT = 65536;
static const size_t REPO_MP_MAX_MESHES_IN_SUPERMESH = 5000;

#define CHRONO_DURATION(start) boost::chrono::duration_cast<boost::chrono::milliseconds>(boost::chrono::high_resolution_clock::now() - start).count()

MultipartOptimizer::MultipartOptimizer() :
	AbstractOptimizer()
{
}

MultipartOptimizer::~MultipartOptimizer()
{
}

bool MultipartOptimizer::apply(repo::core::model::RepoScene *scene)
{
	bool success = false;
	if (!scene)
	{
		repoError << "Failed to create Optimised scene: nullptr to scene.";
		return false;
	}

	if (!scene->hasRoot(repo::core::model::RepoScene::GraphType::DEFAULT))
	{
		repoError << "Failed to create Optimised scene: scene is empty!";
		return false;
	}

	if (scene->hasRoot(repo::core::model::RepoScene::GraphType::OPTIMIZED))
	{
		repoInfo << "The scene already has a stash graph, removing...";
		scene->clearStash();
	}

	return generateMultipartScene(scene);
}

bool MultipartOptimizer::getBakedMeshNodes(
	const repo::core::model::RepoScene* scene,
	const repo::core::model::RepoNode* node,
	repo::lib::RepoMatrix mat,
	BakedMeshNodes& nodes)
{
	bool success = false;
	if (success = scene && node)
	{
		switch (node->getTypeAsEnum())
		{
		case repo::core::model::NodeType::TRANSFORMATION:
		{
			auto trans = (repo::core::model::TransformationNode*)node;
			mat = mat * trans->getTransMatrix(false);
			auto children = scene->getChildrenAsNodes(defaultGraph, trans->getSharedID());
			for (const auto& child : children)
			{
				success &= getBakedMeshNodes(scene, child, mat, nodes);
			}
			break;
		}

		case repo::core::model::NodeType::MESH:
		{
			auto mesh = (repo::core::model::MeshNode*)node;
			repo::core::model::MeshNode transformedMesh = mesh->cloneAndApplyTransformation(mat);
			nodes.insert({ node->getUniqueID(), transformedMesh });
		}
		}
	}
	else
	{
		repoError << "Unable to get baked MeshNode: scene or node is null.";
	}

	return success;
}

void MultipartOptimizer::appendMeshes(
	const repo::core::model::RepoScene* scene,
	repo::core::model::MeshNode node,
	mapped_mesh_t& mapped
)
{
	repo_mesh_mapping_t meshMap;

	meshMap.material_id = getMaterialID(scene, &node);
	meshMap.mesh_id = node.getUniqueID();
	meshMap.shared_id = node.getSharedID();

	auto bbox = node.getBoundingBox();
	if (bbox.size() >= 2)
	{
		meshMap.min = bbox[0];
		meshMap.max = bbox[1];
	}

	std::vector<repo::lib::RepoVector3D> submVertices = node.getVertices();
	std::vector<repo::lib::RepoVector3D> submNormals = node.getNormals();
	std::vector<repo_face_t> submFaces = node.getFaces();
	std::vector<repo_color4d_t> submColors = node.getColors();
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

		if (submColors.size()) {
			mapped.colors.insert(mapped.colors.end(), submColors.begin(), submColors.end());
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
			//This shouldn't happen, if it does, then it means the mFormat isn't set correctly
			repoError << "Unexpected transformedMesh format mismatch occured!";
		}
	}
	else
	{
		repoError << "Failed merging meshes: Vertices or faces cannot be null!";
	}
}

void updateMinMax(repo::lib::RepoVector3D& min, repo::lib::RepoVector3D& max, repo::lib::RepoVector3D v)
{
	if (v.x < min.x) {
		min.x = v.x;
	}
	if (v.y < min.y) {
		min.y = v.y;
	}
	if (v.z < min.z) {
		min.z = v.z;
	}

	if (v.x > max.x) {
		max.x = v.x;
	}
	if (v.y > max.y) {
		max.y = v.y;
	}
	if (v.z > max.z) {
		max.z = v.z;
	}
}

void MultipartOptimizer::splitMeshes(
	const repo::core::model::RepoScene* scene,
	repo::core::model::MeshNode node,
	std::vector<mapped_mesh_t>& mappedMeshes
)
{
	auto start = boost::chrono::high_resolution_clock::now();

	using Scalar = float;
	using Bvh = bvh::Bvh<Scalar>;
	using Vector3 = bvh::Vector3<Scalar>;

	// Create a set of bounding boxes & centers for each Face in the oversized 
	// mesh.
	// The BVH builder expects a set of bounding boxes and centers to work with.

	auto faces = node.getFaces();
	auto vertices = node.getVertices();
	auto boundingBoxes = std::vector<bvh::BoundingBox<Scalar>>();

	for (auto& face : faces)
	{
		auto v = vertices[face[0]];
		auto b = bvh::BoundingBox<Scalar>(Vector3(v.x, v.y, v.z));

		for (int i = 1; i < face.size(); i++)
		{
			v = vertices[face[i]];
			b.extend(Vector3(v.x, v.y, v.z));
		}

		boundingBoxes.push_back(b);
	}

	auto centers = std::vector<Vector3>();
	for (auto& bounds : boundingBoxes)
	{
		centers.push_back(bounds.center());
	}

	auto globalBounds = bvh::compute_bounding_boxes_union(boundingBoxes.data(), boundingBoxes.size());

	// Create an acceleration data structure based on the face bounds

	Bvh bvh;
	bvh::SweepSahBuilder<Bvh> builder(bvh);
	builder.max_leaf_size = 16;
	builder.build(globalBounds, boundingBoxes.data(), centers.data(), boundingBoxes.size());

	// The tree now contains all the face bounds. To know where to cut the tree, 
	// we need to get the number of unique vertices in each node.

	// To do this, first get the unique counts for each leaf node by partitioning
	// the tree into leaf and branch nodes.

	std::vector<size_t> leaves;
	std::vector<size_t> flattened;

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

			flattened.push_back(index);
		}
	} while (!nodeQueue.empty());

	// Next get the unique vertex indices for each leaf node...

	std::vector<std::set<uint32_t>> uniqueVerticesByNode;
	uniqueVerticesByNode.resize(bvh.node_count);

	for (auto nodeIndex : leaves)
	{
		auto& node = bvh.nodes[nodeIndex];
		auto& uniqueVertices = uniqueVerticesByNode[nodeIndex];
		for (int i = 0; i < node.primitive_count; i++)
		{
			auto faceIndex = bvh.primitive_indices[node.first_child_or_primitive + i];
			auto& face = faces[faceIndex];
			std::copy(face.begin(), face.end(), std::inserter(uniqueVertices, uniqueVertices.end()));
		}
	}

	// ...and extend this for each branch node

	std::reverse(flattened.begin(), flattened.end());

	for (auto nodeIndex : flattened)
	{
		auto& node = bvh.nodes[nodeIndex];
		auto& uniqueVertices = uniqueVerticesByNode[nodeIndex];
		auto& left = uniqueVerticesByNode[node.first_child_or_primitive];
		auto& right = uniqueVerticesByNode[node.first_child_or_primitive + 1];

		std::copy(left.begin(), left.end(), std::inserter(uniqueVertices, uniqueVertices.end()));
		std::copy(right.begin(), right.end(), std::inserter(uniqueVertices, uniqueVertices.end()));
	}

	// Next, traverse the tree again, but this time depth first, cutting the tree 
	// at places the unique vertex count drops below the target threshold.

	std::vector<size_t> branchNodes;
	std::stack<size_t> nodeStack;
	nodeStack.push(0);
	do {
		auto index = nodeStack.top();
		nodeStack.pop();

		auto& node = bvh.nodes[index];
		auto& vertexSet = uniqueVerticesByNode[index];
		auto numVertices = vertexSet.size();

		// As we are traversing top-down, the vertex counts get smaller.
		// As soon as a node's count drops below the target, all the nodes in
		// that branch can become a mesh.

		if (node.is_leaf())
		{
			// We should never make it to a leaf with more than 65k vertices, 
			// because the leaves should have at most 16 faces.
			repoError << "splitMeshes() encountered a leaf node with " << numVertices << " unique vertices across " << node.primitive_count << " faces.";
			continue;
		}

		if (numVertices < REPO_MP_MAX_VERTEX_COUNT) // This node is the head of a branch that should become a sub mesh
		{
			branchNodes.push_back(index);
		}
		else
		{
			nodeStack.push(node.first_child_or_primitive);
			nodeStack.push(node.first_child_or_primitive + 1);
		}
	} while (!nodeStack.empty());

	// Finally, get all the leaf nodes for each branch in order to build the
	// groups of MeshNodes

	// Get the other vertex attributes for building the sub mapped meshes

	auto normals = node.getNormals();
	auto uvChannels = node.getUVChannelsSeparated();
	auto colors = node.getColors();

	for (auto head : branchNodes)
	{
		// Get all the leaf nodes within the branch

		std::vector<size_t> primitives;

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

		for (auto faceIndex : primitives)
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
		
		for (auto globalIndex : globalVertexIndices)
		{
			mapped.vertices.push_back(vertices[globalIndex]);
			if (normals.size()) {
				mapped.normals.push_back(normals[globalIndex]);
			}
			if (colors.size()) {
				mapped.colors.push_back(colors[globalIndex]);
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
		mapping.min = mapped.vertices[0];
		mapping.max = mapped.vertices[0];
		for (auto v : mapped.vertices)
		{
			updateMinMax(mapping.min, mapping.max, v);
		}
		mapping.mesh_id = node.getUniqueID();
		mapping.shared_id = node.getSharedID();
		mapping.material_id = getMaterialID(scene, &node);

		mapped.meshMapping.push_back(mapping);

		mappedMeshes.push_back(mapped);
	}
	
	repoInfo << "Split mesh with " << vertices.size() << " vertices into " << mappedMeshes.size() << " submeshes in " << boost::chrono::duration_cast<boost::chrono::milliseconds>(boost::chrono::high_resolution_clock::now() - start).count() << " ms";
}

void MultipartOptimizer::createSuperMeshes(
	const repo::core::model::RepoScene* scene,
	const std::vector<repo::core::model::MeshNode> &nodes,
	MaterialUUIDMap &materialMap,
	const bool isGrouped,
	std::vector<repo::core::model::MeshNode*> &supermeshNodes)
{
	// This will hold the final set of supermeshes

	std::vector<mapped_mesh_t> mappedMeshes;

	// Iterate over the meshes and decide what to do with each. The options are
	// to append to the existing supermesh, start a new supermesh, or split into
	// multiple supermeshes.

	mapped_mesh_t currentSupermesh;

	int vertexLimit = 65536;

	for (auto& node : nodes)
	{
		if (currentSupermesh.vertices.size() + node.getNumVertices() <= vertexLimit) 
		{
			// The current node can be added to the supermesh OK
			appendMeshes(scene, node, currentSupermesh);
		}
		else if (node.getNumVertices() > vertexLimit)
		{
			// The node is too big to fit into any supermesh, so it must be split
			splitMeshes(scene, node, mappedMeshes);
		}
		else
		{
			// The node is small enough to fit within one supermesh, just not this one
			mappedMeshes.push_back(currentSupermesh);
			currentSupermesh = mapped_mesh_t();
			appendMeshes(scene, node, currentSupermesh);
		}
	}

	// Add the last supermesh to be built

	if (currentSupermesh.vertices.size())
	{
		mappedMeshes.push_back(currentSupermesh);
	}

	// For all the mesh mappings, update the material UUIDs to re-mapped UUIDs so we can 
	// clone the material nodes in the database later

	for (auto& mapped : mappedMeshes) 
	{
		for (auto& mapping : mapped.meshMapping)
		{
			if (materialMap.find(mapping.material_id) == materialMap.end())
			{
				materialMap[mapping.material_id] = repo::lib::RepoUUID::createUUID(); // The first time we've encountered this original material
			}
			mapping.material_id = materialMap[mapping.material_id];	
		}
	}

	// Finally, construct MeshNodes for each Supermesh

	for (auto& mapped : mappedMeshes) 
	{
		supermeshNodes.push_back(createMeshNode(mapped, isGrouped));
	}
}

repo::core::model::MeshNode* MultipartOptimizer::createMeshNode(
	mapped_mesh_t& mapped,
	bool isGrouped
)
{
	repo::core::model::MeshNode* resultMesh = nullptr;

	if (!mapped.meshMapping.size())
	{
		return resultMesh;
	}

	//workout bbox and outline from meshMapping
	auto meshMapping = mapped.meshMapping;
	std::vector<repo::lib::RepoVector3D> bbox;
	bbox.push_back(meshMapping[0].min);
	bbox.push_back(meshMapping[0].max);
	for (int i = 1; i < meshMapping.size(); ++i)
	{
		if (bbox[0].x > meshMapping[i].min.x)
			bbox[0].x = meshMapping[i].min.x;
		if (bbox[0].y > meshMapping[i].min.y)
			bbox[0].y = meshMapping[i].min.y;
		if (bbox[0].z > meshMapping[i].min.z)
			bbox[0].z = meshMapping[i].min.z;

		if (bbox[1].x < meshMapping[i].max.x)
			bbox[1].x = meshMapping[i].max.x;
		if (bbox[1].y < meshMapping[i].max.y)
			bbox[1].y = meshMapping[i].max.y;
		if (bbox[1].z < meshMapping[i].max.z)
			bbox[1].z = meshMapping[i].max.z;
	}

	std::vector < std::vector<float> > outline;
	outline.push_back({ bbox[0].x, bbox[0].y });
	outline.push_back({ bbox[1].x, bbox[0].y });
	outline.push_back({ bbox[1].x, bbox[1].y });
	outline.push_back({ bbox[0].x, bbox[1].y });

	std::vector<std::vector<float>> bboxVec = { { bbox[0].x, bbox[0].y, bbox[0].z }, { bbox[1].x, bbox[1].y, bbox[1].z } };

	repo::core::model::MeshNode superMesh = repo::core::model::RepoBSONFactory::makeMeshNode(
		mapped.vertices,
		mapped.faces,
		mapped.normals, 
		bboxVec,
		mapped.uvChannels,
		mapped.colors, outline, isGrouped ? "grouped" : "");
	resultMesh = new repo::core::model::MeshNode(superMesh.cloneAndUpdateMeshMapping(meshMapping, true));

	return resultMesh;
}

bool MultipartOptimizer::generateMultipartScene(repo::core::model::RepoScene *scene)
{
	bool success = false;
	auto meshes = scene->getAllMeshes(defaultGraph);
	if (success = meshes.size())
	{
		// Bake all the meshes into model space, creating a lookup for processMeshGroup

		repoInfo << "Baking " << meshes.size() << " meshes...";

		BakedMeshNodes bakedMeshNodes;
		getBakedMeshNodes(scene, scene->getRoot(defaultGraph), repo::lib::RepoMatrix(), bakedMeshNodes);

		//Sort the meshes into 3 different groupings

		std::unordered_map<std::string, std::unordered_map<uint32_t, std::vector<std::set<repo::lib::RepoUUID>>>> transparentMeshes, normalMeshes;
		std::unordered_map < std::string, std::unordered_map < uint32_t, std::unordered_map < repo::lib::RepoUUID,
			std::vector<std::set<repo::lib::RepoUUID>>, repo::lib::RepoUUIDHasher >>>texturedMeshes;

		repoInfo << "Sorting " << meshes.size() << " meshes...";

		sortMeshes(scene, meshes, normalMeshes, transparentMeshes, texturedMeshes);		

		repo::core::model::RepoNodeSet mergedMeshes, materials, trans, textures, dummy;

		auto rootNode = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode());
		trans.insert(rootNode);
		repo::lib::RepoUUID rootID = rootNode->getSharedID();

		MaterialNodes mergedMeshesMaterials;
		MaterialUUIDMap materialIdMap;		

		for (const auto &meshGroup : normalMeshes)
		{
			for (const auto &formats : meshGroup.second)
			{
				for (const auto formatSet : formats.second)
				{
					success &= processMeshGroup(scene, bakedMeshNodes, formatSet, rootID, mergedMeshes, mergedMeshesMaterials, materialIdMap, !meshGroup.first.empty());
				}
			}
		}

		for (const auto &meshGroup : transparentMeshes)
		{
			for (const auto &groupings : meshGroup.second)
			{
				for (const auto grouping : groupings.second)
				{
					success &= processMeshGroup(scene, bakedMeshNodes, grouping, rootID, mergedMeshes, mergedMeshesMaterials, materialIdMap, !meshGroup.first.empty());
				}
			}
		}

		//textured meshes

		for (const auto &meshGroup : texturedMeshes)
		{
			for (const auto &textureMeshMap : meshGroup.second)
			{
				for (const auto &groupings : textureMeshMap.second)
				{
					for (const auto grouping : groupings.second)
					{
						success &= processMeshGroup(scene, bakedMeshNodes, grouping, rootID, mergedMeshes, mergedMeshesMaterials, materialIdMap, !meshGroup.first.empty());
					}
				}
			}
		}

		if (success)
		{
			//fill Material nodeset
			for (const auto &material : mergedMeshesMaterials)
			{
				materials.insert(material.second);
			}

			//fill Texture nodeset
			for (const auto &texture : scene->getAllTextures(defaultGraph))
			{
				//create new instance with new UUID to avoid X contamination
				repo::core::model::RepoBSONBuilder builder;
				builder.append(REPO_NODE_LABEL_ID, repo::lib::RepoUUID::createUUID());
				auto changeBSON = builder.obj();
				textures.insert(new repo::core::model::TextureNode(texture->cloneAndAddFields(&changeBSON, false)));
			}

			scene->addStashGraph(dummy, mergedMeshes, materials, textures, trans);
		}
		else
		{
			repoError << "Failed to process Mesh Groups";
		}
	}
	else
	{
		repoError << "Cannot generate a multipart scene for a scene with no meshes";
	}

	return success;
}

repo::lib::RepoUUID MultipartOptimizer::getMaterialID(
	const repo::core::model::RepoScene *scene,
	const repo::core::model::MeshNode  *mesh
)
{
	repo::lib::RepoUUID matID = repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH);
	const auto mat = scene->getChildrenNodesFiltered(defaultGraph, mesh->getSharedID(), repo::core::model::NodeType::MATERIAL);
	if (mat.size())
	{
		matID = mat[0]->getUniqueID();
	}

	return matID;
}

bool MultipartOptimizer::hasTexture(
	const repo::core::model::RepoScene *scene,
	const repo::core::model::MeshNode  *mesh,
	repo::lib::RepoUUID                           &texID)
{
	bool hasText = false;
	const auto mat = scene->getChildrenNodesFiltered(defaultGraph, mesh->getSharedID(), repo::core::model::NodeType::MATERIAL);
	if (mat.size())
	{
		const auto texture = scene->getChildrenNodesFiltered(defaultGraph, mat[0]->getSharedID(), repo::core::model::NodeType::TEXTURE);
		if (hasText = texture.size())
		{
			texID = texture[0]->getSharedID();
		}
	}

	return hasText;
}

void clusterMeshNodesBvh(
	const std::vector<repo::core::model::MeshNode>& meshes,
	std::vector<std::vector<repo::core::model::MeshNode>>& clusters) 
{
	using Scalar = float;
	using Bvh = bvh::Bvh<Scalar>;
	using Vector3 = bvh::Vector3<Scalar>;

	// The BVH builder requires a set of bounding boxes and centers to work with.
	// Each of these corresponds to a primitive. The primitive indices in the tree
	// refer to entries in this list, and so the entries in meshes, if the order
	// is the same (which it is).

	auto boundingBoxes = std::vector<bvh::BoundingBox<Scalar>>();
	for (auto& node : meshes)
	{
		auto bounds = node.getBoundingBox();
		auto min = Vector3(bounds[0].x, bounds[0].y, bounds[0].z);
		auto max = Vector3(bounds[1].x, bounds[1].y, bounds[1].z);
		boundingBoxes.push_back(bvh::BoundingBox<Scalar>(min, max));
	}

	auto centers = std::vector<Vector3>();
	for (auto& bounds : boundingBoxes)
	{
		centers.push_back(bounds.center());
	}

	auto globalBounds = bvh::compute_bounding_boxes_union(boundingBoxes.data(), boundingBoxes.size());

	// Create an acceleration data structure based on the bounds

	Bvh bvh;
	bvh::SweepSahBuilder<Bvh> builder(bvh);
	builder.build(globalBounds, boundingBoxes.data(), centers.data(), boundingBoxes.size());

	// The tree now contains all the submesh bounds grouped in space.
	// Create a list of vertex counts for each node so we can decide where to 
	// prune in order to build the clusters.

	// To do this, get the nodes in list form, 'bottom up', in order to set the
	// vertex counts of each leaf node.

	std::vector<size_t> leaves;
	std::vector<size_t> flattened;

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

			flattened.push_back(index);
		}
	} while (!nodeQueue.empty());

	// Next initialise the triangle count of the leaves

	std::vector<size_t> vertexCounts;
	vertexCounts.resize(bvh.node_count);

	for (auto nodeIndex : leaves)
	{
		size_t vertexCount = 0;
		auto& node = bvh.nodes[nodeIndex];
		for (int i = 0; i < node.primitive_count; i++)
		{
			auto meshIndex = bvh.primitive_indices[node.first_child_or_primitive + i];

			if (meshIndex < 0 || meshIndex >= meshes.size())
			{
				repoError << "Bvh primitive index out of range. This means something has gone wrong with the BVH construction.";
			}

			vertexCount += meshes[meshIndex].getNumVertices();
		}

		vertexCounts[nodeIndex] = vertexCount;
	}

	// Now, moving backwards, add up for each branch node the number of vertices in its children

	std::reverse(flattened.begin(), flattened.end());

	for (auto nodeIndex : flattened)
	{
		auto& node = bvh.nodes[nodeIndex];
		vertexCounts[nodeIndex] = vertexCounts[node.first_child_or_primitive] + vertexCounts[node.first_child_or_primitive + 1];
	}

	// Next, traverse the tree again, but this time depth first, cutting the tree 
	// at places the vertex count drops below a target threshold.

	std::vector<size_t> branchNodes;
	std::stack<size_t> nodeStack;
	nodeStack.push(0);
	do {
		auto index = nodeStack.top();
		nodeStack.pop();

		auto& node = bvh.nodes[index];
		auto numVertices = vertexCounts[index];

		// As we are traversing top-down, the vertex counts get smaller as we go.
		// As soon as a node's count drops below the target, all the nodes in 
		// that branch can become a group.
		// (And if we make it to the end, then the leaf node must exceed the target
		// count and should be split later.)

		if (numVertices < REPO_MP_MAX_VERTEX_COUNT || node.is_leaf()) // This node is the head of a branch that should become a cluster
		{
			branchNodes.push_back(index);
		}
		else
		{
			nodeStack.push(node.first_child_or_primitive);
			nodeStack.push(node.first_child_or_primitive + 1);
		}
	} while (!nodeStack.empty());

	// Finally, get all the leaf nodes for each branch in order to build the
	// groups of MeshNodes

	for (auto head : branchNodes)
	{
		std::vector<repo::core::model::MeshNode> cluster;

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
					auto mesh = meshes[primitive];
					cluster.push_back(mesh);
				}
			}
			else
			{
				nodeStack.push(node.first_child_or_primitive);
				nodeStack.push(node.first_child_or_primitive + 1);
			}
		} while (!nodeStack.empty());

		clusters.push_back(cluster);
	}
}

bool MultipartOptimizer::isTransparent(
	const repo::core::model::RepoScene *scene,
	const repo::core::model::MeshNode  *mesh)
{
	bool isTransparent = false;
	const auto mat = scene->getChildrenNodesFiltered(defaultGraph, mesh->getSharedID(), repo::core::model::NodeType::MATERIAL);
	if (mat.size())
	{
		const repo::core::model::MaterialNode* matNode = (repo::core::model::MaterialNode*)mat[0];
		const auto matStruct = matNode->getMaterialStruct();
		isTransparent = matStruct.opacity != 1;
	}

	return isTransparent;
}

void MultipartOptimizer::clusterMeshNodes(
	const std::vector<repo::core::model::MeshNode>& meshes,
	std::vector<std::vector<repo::core::model::MeshNode>>& clusters
)
{
	// Takes one set of MeshNodes and groups them into N sets of MeshNodes, 
	// where each set will form a Supermesh.

	repoInfo << "Clustering " << meshes.size() << " meshes...";
	auto start = boost::chrono::high_resolution_clock::now();

	// Start by creating a metric for each mesh which will be used to group
	// nodes by their efficiency

	struct mesh {
		repo::core::model::MeshNode node;
		float efficiency;
	};

	std::vector<mesh> metrics(meshes.size());
	for (size_t i = 0; i < meshes.size(); i++)
	{
		auto& mesh = metrics[i];
		mesh.node = meshes[i];
		auto bounds = mesh.node.getBoundingBox();
		auto numVertices = mesh.node.getNumVertices();
		auto width = bounds[1].x - bounds[0].x;
		auto height = bounds[1].y - bounds[0].y;
		auto length = bounds[1].z - bounds[0].z;
		auto metric = (float)numVertices / (width + height + length);
		mesh.efficiency = metric;
	}

	// Next order the outer nodes by their efficiency

	std::sort(metrics.begin(), metrics.end(), [](mesh a, mesh b) {
		return a.efficiency < b.efficiency;
	});

	// Bin the nodes based on vertex count. The bottom 80% will form pure-LOD
	// groups, while the top 20% will be spatialised.

	auto totalVertices = 0;
	for (auto& mesh : meshes) {
		totalVertices += mesh.getNumVertices();
	}

	auto modelLowerThreshold = (int)(totalVertices * 0.2f);
	
	// For the lower 80% of the model, supermesh purely on LOD

	auto binVertexSize = 65536;
	std::vector<repo::core::model::MeshNode> bin;
	auto binVertexCount = 0;
	auto binsVertexCount = 0;
	
	int i = 0;
	for (; i < metrics.size(); i++)
	{
		auto& item = metrics[i];

		binVertexCount += item.node.getNumVertices();
		binsVertexCount += item.node.getNumVertices();
		bin.push_back(item.node);

		if (binVertexCount > binVertexSize)
		{
			// Copy
			auto cluster = std::vector<repo::core::model::MeshNode>(bin);
			clusters.push_back(cluster);

			// And reset
			bin.clear();
			binVertexCount = 0;
		}

		if (binsVertexCount > modelLowerThreshold) {
			break;
		}
	}

	// For the ones left over...
	if (bin.size()) {
		auto cluster = std::vector<repo::core::model::MeshNode>(bin);
		clusters.push_back(cluster);
	}

	// Now the high-part
	for (; i < metrics.size(); i++)
	{
		auto& item = metrics[i];

		bin.push_back(item.node);
	}

	if (bin.size()) {
		clusterMeshNodesBvh(bin, clusters);
	}

	repoInfo << "Created " << clusters.size() << " clusters in " << CHRONO_DURATION(start) << " milliseconds.";
}

bool MultipartOptimizer::processMeshGroup(
	const repo::core::model::RepoScene* scene,
	const BakedMeshNodes& bakedMeshNodes,
	const std::set<repo::lib::RepoUUID>& groupMeshIds,
	const repo::lib::RepoUUID& rootID,
	repo::core::model::RepoNodeSet& mergedMeshes,
	MaterialNodes& mergedMeshesMaterials,
	MaterialUUIDMap& materialMap,
	const bool isGrouped
)
{
	// This method turns the submesh UUIDs into a set of supermesh MeshNodes 
	// and duplicate material nodes. 
	// They are added to the mergedMeshes and mergedMeshesMaterials arrays, to 
	// be added to the stash graph when this method has been called for all 
	// supermesh groups.

	if (!groupMeshIds.size())
	{
		return true;
	}

	// First get all the meshes to build into the supermesh set. This snippet 
	// returns the meshes baked into world space (where they should be when 
	// combined).

	std::vector<repo::core::model::MeshNode> nodes;
	for (auto id : groupMeshIds)
	{
		auto range = bakedMeshNodes.equal_range(id);
		for (auto pair = range.first; pair != range.second; pair++)
		{
			nodes.push_back(pair->second);
		}
	}

	// Next partition them into sets that should form the supermeshes

	std::vector<std::vector<repo::core::model::MeshNode>> clusters;
	clusterMeshNodes(nodes, clusters);

	// Build the actual supermesh nodes that hold the combined or split geometry from the sets

	std::vector<repo::core::model::MeshNode*> supermeshes;
	for (auto cluster : clusters)
	{
		createSuperMeshes(scene, cluster, materialMap, isGrouped, supermeshes);
	}

	// Create inverse lookup that goes from a re-mapped stash Id to the 
	// original Id so processSupermeshMaterials can find the node to
	// copy (when necessary).

	MaterialUUIDMap stashIdToOriginalId;
	for (const auto mapping : materialMap)
	{
		stashIdToOriginalId[mapping.second] = mapping.first;
	}

	for (auto supermesh : supermeshes)
	{
		// First create a new entry parented to the scene root. This will be the final 
		// object.

		auto sMeshWithParent = supermesh->cloneAndAddParent({ rootID });
		supermesh->swap(sMeshWithParent);
		mergedMeshes.insert(supermesh);

		// Next, create the re-mapped material nodes on demand, ensuring the new nodes
		// store the new mesh in their parents array.

		processSupermeshMaterials(scene, supermesh, mergedMeshesMaterials, stashIdToOriginalId);
	}

	return true;
}

void MultipartOptimizer::processSupermeshMaterials(
	const repo::core::model::RepoScene* scene,
	const repo::core::model::MeshNode* supermeshNode,
	MaterialNodes& mergedMaterialNodes,
	MaterialUUIDMap& stashIdToOriginalIdMap
)
{
	std::set<repo::lib::RepoUUID> supermeshStashMaterials;
	for (const auto& map : supermeshNode->getMeshMapping())
	{
		supermeshStashMaterials.insert(map.material_id);
	}

	// The id of this final supermesh MeshNode. This is used to set the parents 
	// array of the material nodes.

	repo::lib::RepoUUID sMeshSharedID = supermeshNode->getSharedID();

	for (const auto stashMaterialId : supermeshStashMaterials)
	{
		// Has another supermesh created a Node for this duplicated material already?
		auto existing = mergedMaterialNodes.find(stashMaterialId);
		if (existing == mergedMaterialNodes.end())
		{
			// No, so we must clone the material here.
			auto originalNode = scene->getNodeByUniqueID(defaultGraph, stashIdToOriginalIdMap[stashMaterialId]);
			if (originalNode)
			{
				repo::core::model::RepoNode clonedMat = repo::core::model::RepoNode(originalNode->removeField(REPO_NODE_LABEL_PARENTS));
				repo::core::model::RepoBSONBuilder builder;
				builder.append(REPO_NODE_LABEL_ID, stashMaterialId);
				auto changeBSON = builder.obj();
				clonedMat = clonedMat.cloneAndAddFields(&changeBSON, false);
				clonedMat = clonedMat.cloneAndAddParent({ sMeshSharedID });
				mergedMaterialNodes[stashMaterialId] = new repo::core::model::MaterialNode(clonedMat);
			}
			else
			{
				repoError << "Failed to find the original material node referenced by a supermesh.";
			}
		}
		else
		{
			// Created by another superMesh (set should have no duplicate so it shouldn't be from this mesh)
			auto updatedStashNode = existing->second->cloneAndAddParent({ sMeshSharedID });
			existing->second->swap(updatedStashNode);
		}
	}
}

void MultipartOptimizer::sortMeshes(
	const repo::core::model::RepoScene                                      *scene,
	const repo::core::model::RepoNodeSet                                    &meshes,
	std::unordered_map<std::string, std::unordered_map<uint32_t, std::vector<std::set<repo::lib::RepoUUID>>>>	&normalMeshes,
	std::unordered_map < std::string, std::unordered_map<uint32_t, std::vector<std::set<repo::lib::RepoUUID>>>>	&transparentMeshes,
	std::unordered_map < std::string, std::unordered_map < uint32_t, std::unordered_map < repo::lib::RepoUUID,
	std::vector<std::set<repo::lib::RepoUUID>>, repo::lib::RepoUUIDHasher >>> &texturedMeshes
)
{
	for (const auto &node : meshes)
	{
		auto mesh = (repo::core::model::MeshNode*) node;
		if (!mesh->getVertices().size() || !mesh->getFaces().size())
		{
			repoWarning << "mesh " << mesh->getUniqueID() << " has no vertices/faces, skipping...";
			continue;
		}
		auto meshGroup = mesh->getGrouping();

		/**
		* 1 - figure out it's mFormat (what buffers, flags and primitives does it have)
		* 2 - check if it has texture
		* 3 - if not, check if it is transparent
		*/
		uint32_t mFormat = mesh->getMFormat();

		repo::lib::RepoUUID texID;
		if (hasTexture(scene, mesh, texID))
		{
			if (texturedMeshes.find(meshGroup) == texturedMeshes.end()) {
				texturedMeshes[meshGroup] = std::unordered_map < uint32_t, std::unordered_map < repo::lib::RepoUUID,
					std::vector<std::set<repo::lib::RepoUUID>>, repo::lib::RepoUUIDHasher >>();
			}

			auto it = texturedMeshes[meshGroup].find(mFormat);
			if (it == texturedMeshes[meshGroup].end())
			{
				texturedMeshes[meshGroup][mFormat] = std::unordered_map<repo::lib::RepoUUID, std::vector<std::set<repo::lib::RepoUUID>>, repo::lib::RepoUUIDHasher>();
			}
			auto it2 = texturedMeshes[meshGroup][mFormat].find(texID);

			if (it2 == texturedMeshes[meshGroup][mFormat].end())
			{
				texturedMeshes[meshGroup][mFormat][texID] = std::vector<std::set<repo::lib::RepoUUID>>();
				texturedMeshes[meshGroup][mFormat][texID].push_back(std::set<repo::lib::RepoUUID>());
			}
			texturedMeshes[meshGroup][mFormat][texID].back().insert(mesh->getUniqueID());
		}
		else
		{
			//no texture, check if it is transparent
			const bool istransParentMesh = isTransparent(scene, mesh);
			auto &meshMap = istransParentMesh ? transparentMeshes : normalMeshes;

			if (meshMap.find(meshGroup) == meshMap.end()) {
				meshMap[meshGroup] = std::unordered_map<uint32_t, std::vector<std::set<repo::lib::RepoUUID>>>();
			}

			auto it = meshMap[meshGroup].find(mFormat);
			if (it == meshMap[meshGroup].end())
			{
				meshMap[meshGroup][mFormat] = std::vector<std::set<repo::lib::RepoUUID>>();
				meshMap[meshGroup][mFormat].push_back(std::set<repo::lib::RepoUUID>());
			}
			meshMap[meshGroup][mFormat].back().insert(mesh->getUniqueID());
		}
	}
}