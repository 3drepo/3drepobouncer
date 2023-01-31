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

#include <algorithm>
#include <chrono>

using namespace repo::manipulator::modeloptimizer;

auto defaultGraph = repo::core::model::RepoScene::GraphType::DEFAULT;

using Scalar = float;
using Bvh = bvh::Bvh<Scalar>;
using BvhVector3 = bvh::Vector3<Scalar>;

static const size_t REPO_MP_MAX_VERTEX_COUNT = 65536;
static const size_t REPO_MP_MAX_MESHES_IN_SUPERMESH = 5000;
static const size_t REPO_BVH_MAX_LEAF_SIZE = 16;
static const size_t REPO_MODEL_LOW_CLUSTERING_RATIO = 0.2f;

#define CHRONO_DURATION(start) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count()

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
	MeshMap& nodes)
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

void MultipartOptimizer::appendMesh(
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

void updateMin(repo::lib::RepoVector3D& min, const repo::lib::RepoVector3D v)
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
}

void updateMax(repo::lib::RepoVector3D& max, const repo::lib::RepoVector3D v)
{
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

void updateMinMax(repo::lib::RepoVector3D& min, repo::lib::RepoVector3D& max, const repo::lib::RepoVector3D v)
{
	updateMin(min, v);
	updateMax(max, v);
}

// Constructs a bounding volumne hierarchy of all the Faces in the MeshNode

Bvh buildFacesBvh(
	const repo::core::model::MeshNode node
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

void flattenBvh(
	const Bvh& bvh,
	std::vector<size_t>& leaves,
	std::vector<size_t>& branches
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

std::vector<size_t> getBranchPrimitives(
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

std::vector<std::set<uint32_t>> getUniqueVertices(
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

std::vector<size_t> getSupermeshBranchNodes(
	const Bvh& bvh, 
	std::vector<size_t> vertexCounts)
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
	const repo::core::model::RepoScene* scene,
	repo::core::model::MeshNode node,
	std::vector<mapped_mesh_t>& mappedMeshes
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
	auto colors = node.getColors();

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
		for (const auto v : mapped.vertices)
		{
			updateMinMax(mapping.min, mapping.max, v);
		}
		mapping.mesh_id = node.getUniqueID();
		mapping.shared_id = node.getSharedID();
		mapping.material_id = getMaterialID(scene, &node);

		mapped.meshMapping.push_back(mapping);

		mappedMeshes.push_back(mapped);
	}

	repoInfo << "Split mesh with " << node.getNumVertices() << " vertices into " << mappedMeshes.size() << " submeshes in " << CHRONO_DURATION(start) << " ms";
}

void MultipartOptimizer::createSuperMeshes(
	const repo::core::model::RepoScene* scene,
	const std::vector<repo::core::model::MeshNode> &nodes,
	UUIDMap &materialMap,
	const bool isGrouped,
	std::vector<repo::core::model::MeshNode*> &supermeshNodes)
{
	// This will hold the final set of supermeshes

	std::vector<mapped_mesh_t> mappedMeshes;

	// Iterate over the meshes and decide what to do with each. The options are
	// to append to the existing supermesh, start a new supermesh, or split into
	// multiple supermeshes.

	mapped_mesh_t currentSupermesh;

	for (const auto& node : nodes)
	{
		if (currentSupermesh.vertices.size() + node.getNumVertices() <= REPO_MP_MAX_VERTEX_COUNT)
		{
			// The current node can be added to the supermesh OK
			appendMesh(scene, node, currentSupermesh);
		}
		else if (node.getNumVertices() > REPO_MP_MAX_VERTEX_COUNT)
		{
			// The node is too big to fit into any supermesh, so it must be split
			splitMesh(scene, node, mappedMeshes);
		}
		else
		{
			// The node is small enough to fit within one supermesh, just not this one
			mappedMeshes.push_back(currentSupermesh);
			currentSupermesh = mapped_mesh_t();
			appendMesh(scene, node, currentSupermesh);
		}
	}

	// Add the last supermesh to be built

	if (currentSupermesh.vertices.size())
	{
		mappedMeshes.push_back(currentSupermesh);
	}

	for (auto& mapped : mappedMeshes) 
	{
		// For all the mesh mappings, update the material UUIDs to re-mapped UUIDs so we can 
		// clone the material nodes in the database later

		for (auto& mapping : mapped.meshMapping)
		{
			if (materialMap.find(mapping.material_id) == materialMap.end()) // materialMap maps from original (default graph) material node uuids to stash graph uuids
			{
				materialMap[mapping.material_id] = repo::lib::RepoUUID::createUUID(); // The first time we've encountered this original material
			}
			mapping.material_id = materialMap[mapping.material_id];	
		}

		// Finally, construct MeshNodes for each Supermesh

		supermeshNodes.push_back(createMeshNode(mapped, isGrouped));
	}
}

repo::core::model::MeshNode* MultipartOptimizer::createMeshNode(
	const mapped_mesh_t& mapped,
	bool isGrouped
)
{
	repo::core::model::MeshNode* resultMesh = nullptr;

	if (!mapped.meshMapping.size())
	{
		return resultMesh;
	}

	//workout bbox from meshMapping
	auto meshMapping = mapped.meshMapping;
	std::vector<repo::lib::RepoVector3D> bbox;
	bbox.push_back(meshMapping[0].min);
	bbox.push_back(meshMapping[0].max);
	for (int i = 1; i < meshMapping.size(); ++i)
	{
		updateMin(bbox[0], meshMapping[i].min);
		updateMax(bbox[1], meshMapping[i].max);
	}

	std::vector<std::vector<float>> bboxVec = { { bbox[0].x, bbox[0].y, bbox[0].z }, { bbox[1].x, bbox[1].y, bbox[1].z } };

	repo::core::model::MeshNode superMesh = repo::core::model::RepoBSONFactory::makeMeshNode(
		mapped.vertices,
		mapped.faces,
		mapped.normals, 
		bboxVec,
		mapped.uvChannels,
		mapped.colors, 
		isGrouped ? "grouped" : "");
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

		MeshMap bakedMeshNodes;
		getBakedMeshNodes(scene, scene->getRoot(defaultGraph), repo::lib::RepoMatrix(), bakedMeshNodes);

		//Sort the meshes into 3 different groupings

		std::unordered_map<std::string, std::unordered_map<uint32_t, std::vector<std::set<repo::lib::RepoUUID>>>> transparentMeshes, normalMeshes;
		std::unordered_map < std::string, std::unordered_map < uint32_t, std::unordered_map < repo::lib::RepoUUID,
			std::vector<std::set<repo::lib::RepoUUID>>, repo::lib::RepoUUIDHasher >>>texturedMeshes;

		repoInfo << "Sorting " << meshes.size() << " meshes...";

		sortMeshes(scene, meshes, normalMeshes, transparentMeshes, texturedMeshes);		

		repo::core::model::RepoNodeSet mergedMeshes, trans, textures, dummy;

		auto rootNode = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode());
		trans.insert(rootNode);
		repo::lib::RepoUUID rootID = rootNode->getSharedID();

		UUIDMap defaultIdToStashIdMap;		

		for (const auto &meshGroup : normalMeshes)
		{
			for (const auto &formats : meshGroup.second)
			{
				for (const auto formatSet : formats.second)
				{
					success &= processMeshGroup(scene, bakedMeshNodes, formatSet, rootID, mergedMeshes, defaultIdToStashIdMap, !meshGroup.first.empty());
				}
			}
		}

		for (const auto &meshGroup : transparentMeshes)
		{
			for (const auto &groupings : meshGroup.second)
			{
				for (const auto grouping : groupings.second)
				{
					success &= processMeshGroup(scene, bakedMeshNodes, grouping, rootID, mergedMeshes, defaultIdToStashIdMap, !meshGroup.first.empty());
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
						success &= processMeshGroup(scene, bakedMeshNodes, grouping, rootID, mergedMeshes, defaultIdToStashIdMap, !meshGroup.first.empty());
					}
				}
			}
		}

		if (success)
		{
			// fill material nodeset

			// When the supermeshes are created in createSuperMeshes, the new 
			// meshes will have their material Ids set to new UUIDs. The next 
			// step is to create the actual MaterialNodes in the stash graph 
			// for the new materials. defaultIdToStashIdMap contains the list
			// of materials to be copied (as UUIDs to the original materials)
			// and the new UUID that they should be copied as in the stash.

			auto materials = processSupermeshMaterials(scene, mergedMeshes, defaultIdToStashIdMap);

			// fill texture nodeset

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
	repo::lib::RepoUUID	&texID
)
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

// Builds a Bvh of the bounds of the MeshNodes. Each MeshNode in the array is 
// the primitive.

Bvh buildBoundsBvh(
	const std::vector<repo::core::model::MeshNode>& meshes
)
{
	// The BVH builder requires a set of bounding boxes and centers to work with.
	// Each of these corresponds to a primitive. The primitive indices in the tree
	// refer to entries in this list, and so the entries in meshes, if the order
	// is the same (which it is).

	auto boundingBoxes = std::vector<bvh::BoundingBox<Scalar>>();
	for (const auto& node : meshes)
	{
		auto bounds = node.getBoundingBox();
		auto min = BvhVector3(bounds[0].x, bounds[0].y, bounds[0].z);
		auto max = BvhVector3(bounds[1].x, bounds[1].y, bounds[1].z);
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

std::vector<size_t> getVertexCounts(
	const Bvh& bvh,
	const std::vector<repo::core::model::MeshNode>& primitives
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

			if (primitiveIndex < 0 || primitiveIndex >= primitives.size())
			{
				repoError << "Bvh primitive index out of range. This means something has gone wrong with the BVH construction.";
			}

			vertexCount += primitives[primitiveIndex].getNumVertices();
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

void clusterMeshNodesBvh(
	const std::vector<repo::core::model::MeshNode>& meshes,
	std::vector<std::vector<repo::core::model::MeshNode>>& clusters) 
{
	auto bvh = buildBoundsBvh(meshes);

	// The tree contains all the submesh bounds grouped in space.
	// Create a list of vertex counts for each node so we can decide where to 
	// prune in order to build the clusters.

	auto vertexCounts = getVertexCounts(bvh, meshes);

	// Next, traverse the tree again, but this time depth first, cutting the tree 
	// at places the vertex count drops below a target threshold.

	auto branchNodes = getSupermeshBranchNodes(bvh, vertexCounts);

	// Finally, get all the leaf nodes for each branch in order to build the
	// groups of MeshNodes

	for (const auto head : branchNodes)
	{
		std::vector<repo::core::model::MeshNode> cluster;
		for (const auto primitive : getBranchPrimitives(bvh, head))
		{
			cluster.push_back(meshes[primitive]);
		}
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

void splitBigClusters(std::vector<std::vector<repo::core::model::MeshNode>>& clusters)
{
	auto clustersToSplit = std::vector<std::vector<repo::core::model::MeshNode>>();
	clusters.erase(std::remove_if(clusters.begin(), clusters.end(),
		[&](std::vector<repo::core::model::MeshNode> cluster)
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

	std::vector<repo::core::model::MeshNode> cluster;
	for (auto const clusterToSplit : clustersToSplit)
	{
		int i = 0;
		while (i < clusterToSplit.size())
		{
			cluster.push_back(clusterToSplit[i++]);
			if (cluster.size() >= REPO_MP_MAX_MESHES_IN_SUPERMESH)
			{
				clusters.push_back(std::vector<repo::core::model::MeshNode>(cluster));
				cluster.clear();
			}		
		}

		if (cluster.size())
		{
			clusters.push_back(std::vector<repo::core::model::MeshNode>(cluster));
		}
	}
}

std::vector<std::vector<repo::core::model::MeshNode>> MultipartOptimizer::clusterMeshNodes(
	const std::vector<repo::core::model::MeshNode>& meshes
)
{
	// Takes one set of MeshNodes and groups them into N sets of MeshNodes, 
	// where each set will form a Supermesh.

	repoInfo << "Clustering " << meshes.size() << " meshes...";
	auto start = std::chrono::high_resolution_clock::now();

	// Start by creating a metric for each mesh which will be used to group
	// nodes by their efficiency

	struct meshMetric {
		repo::core::model::MeshNode node;
		float efficiency;
	};

	std::vector<meshMetric> metrics(meshes.size());
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

	std::vector<repo::core::model::MeshNode> bin;
	auto binVertexCount = 0;
	auto binsVertexCount = 0;

	std::vector<std::vector<repo::core::model::MeshNode>> clusters;
	auto metricsIterator = metrics.begin();

	while (metricsIterator != metrics.end() && binsVertexCount <= modelLowerThreshold)
	{
		auto& item = *metricsIterator++;

		binVertexCount += item.node.getNumVertices();
		binsVertexCount += item.node.getNumVertices();
		bin.push_back(item.node);

		if (binVertexCount >= REPO_MP_MAX_VERTEX_COUNT) // If we've filled up one supermesh
		{
			// Copy
			auto cluster = std::vector<repo::core::model::MeshNode>(bin);
			clusters.push_back(cluster);

			// And reset
			bin.clear();
			binVertexCount = 0;
		}
	}

	if (bin.size()) // Either we have some % of the model remaining, or the entire model fits below 65K
	{
		auto cluster = std::vector<repo::core::model::MeshNode>(bin);
		clusters.push_back(cluster);

		bin.clear();
	}

	// Now the high-part
	while (metricsIterator != metrics.end())
	{
		auto& item = *metricsIterator++;

		bin.push_back(item.node);
	}

	if (bin.size())
	{
		clusterMeshNodesBvh(bin, clusters);
	}

	// If clusters contain too many meshes, we can exceed the maximum BSON size.
	// Do a quick check and split any clusters that are too large.

	splitBigClusters(clusters);

	repoInfo << "Created " << clusters.size() << " clusters in " << CHRONO_DURATION(start) << " milliseconds.";

	return clusters;
}

bool MultipartOptimizer::processMeshGroup(
	const repo::core::model::RepoScene* scene,
	const MeshMap& bakedMeshNodes,
	const std::set<repo::lib::RepoUUID>& groupMeshIds,
	const repo::lib::RepoUUID& rootID,
	repo::core::model::RepoNodeSet& mergedMeshes,
	UUIDMap& materialMap,
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
	for (const auto id : groupMeshIds)
	{
		auto range = bakedMeshNodes.equal_range(id);
		for (auto pair = range.first; pair != range.second; pair++)
		{
			nodes.push_back(pair->second);
		}
	}

	// Next partition them into sets that should form the supermeshes

	auto clusters = clusterMeshNodes(nodes);

	// Build the actual supermesh nodes that hold the combined or split geometry from the sets

	std::vector<repo::core::model::MeshNode*> supermeshes;
	for (const auto cluster : clusters)
	{
		createSuperMeshes(scene, cluster, materialMap, isGrouped, supermeshes);
	}

	for (const auto supermesh : supermeshes)
	{
		// First create a new entry parented to the scene root. This will be the final 
		// object.

		auto sMeshWithParent = supermesh->cloneAndAddParent({ rootID });
		supermesh->swap(sMeshWithParent);
		mergedMeshes.insert(supermesh);
	}

	return true;
}

repo::core::model::RepoNodeSet MultipartOptimizer::processSupermeshMaterials(
	const repo::core::model::RepoScene* scene,
	const repo::core::model::RepoNodeSet& supermeshes,
	const UUIDMap& originalIdToStashIdMap
)
{
	// originalIdToStashIdMap contains all the referenced materials. Start by
	// getting a list of which supermeshes are referenced by the materials.
	// Here, we get a list of references for the *new ids*.

	auto stashMaterialParents = std::unordered_map< // Use a map of vectors instead of a multimap so we can pass the vector directly to cloneAndAddParent
		repo::lib::RepoUUID, std::vector<repo::lib::RepoUUID>, 
		repo::lib::RepoUUIDHasher>();

	for(const auto supermesh : supermeshes)
	{
		for (const auto mapping : dynamic_cast<repo::core::model::MeshNode*>(supermesh)->getMeshMapping())
		{
			stashMaterialParents[mapping.material_id].push_back(supermesh->getSharedID());
		}
	}

	// Now create nodes for each new material

	repo::core::model::RepoNodeSet materials;

	for (const auto cloned : originalIdToStashIdMap)
	{
		auto originalId = cloned.first;
		auto stashId = cloned.second;

		auto originalNode = scene->getNodeByUniqueID(defaultGraph, originalId);
		if (originalNode)
		{
			repo::core::model::RepoNode clonedMat = repo::core::model::RepoNode(originalNode->removeField(REPO_NODE_LABEL_PARENTS));
			repo::core::model::RepoBSONBuilder builder;
			builder.append(REPO_NODE_LABEL_ID, stashId);
			auto changeBSON = builder.obj();
			clonedMat = clonedMat.cloneAndAddFields(&changeBSON, false);
			clonedMat = clonedMat.cloneAndAddParent(stashMaterialParents[stashId]);
			materials.insert(new repo::core::model::MaterialNode(clonedMat));
		}
		else
		{
			repoError << "Failed to find the original material node referenced by a supermesh.";
		}
	}

	return materials;
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