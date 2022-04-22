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

#include "repo_optimizer_multipart.h"
#include "../../core/model/bson/repo_bson_factory.h"
#include "../../core/model/bson/repo_bson_builder.h"

using namespace repo::manipulator::modeloptimizer;

auto defaultGraph = repo::core::model::RepoScene::GraphType::DEFAULT;

static const size_t  REPO_MP_MAX_VERTEX_COUNT = 65536;
static const size_t REPO_MP_MAX_MESHES_IN_SUPERMESH = 5000;

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
	repo::lib::RepoUUID id,
	std::vector<repo::core::model::MeshNode>& nodes)
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
				success &= getBakedMeshNodes(scene, child, mat, id, nodes);
			}
			break;
		}

		case repo::core::model::NodeType::MESH:
		{
			if (node->getUniqueID() == id)
			{
				auto mesh = (repo::core::model::MeshNode*)node;
				repo::core::model::MeshNode transformedMesh = mesh->cloneAndApplyTransformation(mat);
				nodes.push_back(transformedMesh);
			}
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

void MultipartOptimizer::splitMeshes(
	const repo::core::model::RepoScene* scene,
	repo::core::model::MeshNode node,
	std::vector<mapped_mesh_t>& mappedMeshes
)
{
	repoError << "MultipartOptimizer::splitMeshes not yet implemented.";
}

std::vector<repo::core::model::MeshNode*> MultipartOptimizer::createSuperMeshes(
	const repo::core::model::RepoScene* scene,
	const std::set<repo::lib::RepoUUID>& meshGroup,
	MaterialUUIDMap &materialMap,
	const bool isGrouped)
{
	// This will hold the final set of supermeshes

	std::vector<mapped_mesh_t> mappedMeshes;

	// First get all the meshes to build into the supermesh set. This snippet 
	// returns the meshes baked into world space (where they should be when 
	// combined).

	std::vector<repo::core::model::MeshNode> nodes;
	for (auto id : meshGroup) 
	{
		getBakedMeshNodes(scene, scene->getRoot(defaultGraph), repo::lib::RepoMatrix(), id, nodes);
	}

	// Iterate over the meshes and decide what to do with each. The options are
	// to append to the existing supermesh, start a new supermesh, or split into
	// multiple supermeshes.

	mapped_mesh_t currentSupermesh;

	int vertexLimit = 2147483647; //65536

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

	std::vector<repo::core::model::MeshNode*> resultMeshes;

	for (auto& mapped : mappedMeshes) 
	{
		resultMeshes.push_back(createMeshNode(mapped, isGrouped));
	}

	return resultMeshes;
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
		std::unordered_map<std::string, std::unordered_map<uint32_t, std::vector<std::set<repo::lib::RepoUUID>>>> transparentMeshes, normalMeshes;
		std::unordered_map < std::string, std::unordered_map < uint32_t, std::unordered_map < repo::lib::RepoUUID,
			std::vector<std::set<repo::lib::RepoUUID>>, repo::lib::RepoUUIDHasher >>>texturedMeshes;

		//Sort the meshes into 3 different grouping
		
		// ISSUE #422 (Don't sort)
		sortMeshes(scene, meshes, normalMeshes, transparentMeshes, texturedMeshes);
		

		repo::core::model::RepoNodeSet mergedMeshes, materials, trans, textures, dummy;

		auto rootNode = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode());
		trans.insert(rootNode);
		repo::lib::RepoUUID rootID = rootNode->getSharedID();

		MaterialNodes mergedMeshesMaterials;
		MaterialUUIDMap materialIdMap;


		// ISSUE #422 -- This tweak simply makes the stash graph store one supermesh for each regular mesh
		/*
		for (auto mesh : meshes)
		{
			std::set<repo::lib::RepoUUID> s { ((repo::core::model::MeshNode*)mesh)->getUniqueID() };
			success &= processMeshGroup(scene, s, rootID, mergedMeshes, mergedMeshesMaterials, materialIdMap, true);
		}
		*/
		

		for (const auto &meshGroup : normalMeshes)
		{
			for (const auto &groupings : meshGroup.second)
			{
				for (const auto grouping : groupings.second)
				{
					success &= processMeshGroup(scene, grouping, rootID, mergedMeshes, mergedMeshesMaterials, materialIdMap, !meshGroup.first.empty());
				}
			}
		}

		for (const auto &meshGroup : transparentMeshes)
		{
			for (const auto &groupings : meshGroup.second)
			{
				for (const auto grouping : groupings.second)
				{
					success &= processMeshGroup(scene, grouping, rootID, mergedMeshes, mergedMeshesMaterials, materialIdMap, !meshGroup.first.empty());
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
						success &= processMeshGroup(scene, grouping, rootID, mergedMeshes, mergedMeshesMaterials, materialIdMap, !meshGroup.first.empty());
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

bool MultipartOptimizer::processMeshGroup(
	const repo::core::model::RepoScene *scene,
	const std::set<repo::lib::RepoUUID> &meshes,
	const repo::lib::RepoUUID &rootID,
	repo::core::model::RepoNodeSet &mergedMeshes,
	MaterialNodes &matNodes,
	MaterialUUIDMap &materialMap,
	const bool isGrouped
)
{
	// This method turns the submesh UUIDs into a set of supermesh MeshNodes and duplicate material nodes.
	// They are added to the mergedMeshes and mergedMeshesMaterials arrays and will be added to the stash graph
	// when this method has been called for all supermesh groups.

	if (!meshes.size()) 
	{
		return true;
	}

	auto sMeshes = createSuperMeshes(scene, meshes, materialMap, isGrouped);
	for (auto sMesh : sMeshes)
	{
		// First create a new entry parented to the scene root. This will be the final 
		// object.

		auto sMeshWithParent = sMesh->cloneAndAddParent({ rootID });
		sMesh->swap(sMeshWithParent);
		mergedMeshes.insert(sMesh);

		// Next, create the re-mapped material nodes on demand, ensuring the new nodes
		// store the new mesh in their parents array.

		std::set<repo::lib::RepoUUID> supermeshStashMaterials;
		for (const auto &map : sMesh->getMeshMapping())
		{
			supermeshStashMaterials.insert(map.material_id);
		}

		MaterialUUIDMap stashIdToOriginalId; // From remapped Id to original Id
		for (const auto mapping : materialMap)
		{
			stashIdToOriginalId[mapping.second] = mapping.first;
		}

		// The id of this final supermesh MeshNode. This is used to set the parents 
		// array of the material nodes.

		repo::lib::RepoUUID sMeshSharedID = sMesh->getSharedID();

		for (const auto stashMaterialId : supermeshStashMaterials)
		{
			// Has another supermesh created a Node for this duplicated material already?
			auto& existing = matNodes.find(stashMaterialId);
			if (existing == matNodes.end())
			{
				// No, so we must clone the material here.
				auto originalNode = scene->getNodeByUniqueID(defaultGraph, stashIdToOriginalId[stashMaterialId]);
				if (originalNode)
				{
					repo::core::model::RepoNode clonedMat = repo::core::model::RepoNode(originalNode->removeField(REPO_NODE_LABEL_PARENTS));
					repo::core::model::RepoBSONBuilder builder;
					builder.append(REPO_NODE_LABEL_ID, stashMaterialId);
					auto changeBSON = builder.obj();
					clonedMat = clonedMat.cloneAndAddFields(&changeBSON, false);
					clonedMat = clonedMat.cloneAndAddParent({ sMeshSharedID });
					matNodes[stashMaterialId] = new repo::core::model::MaterialNode(clonedMat);
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

	return true;
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
	std::unordered_map < std::string, std::unordered_map<uint32_t, size_t>> normalFCount, transparentFCount;
	std::unordered_map < std::string, std::unordered_map<uint32_t, std::unordered_map<repo::lib::RepoUUID, size_t, repo::lib::RepoUUIDHasher>> > texturedFCount;

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
				texturedFCount[meshGroup] = std::unordered_map<uint32_t, std::unordered_map<repo::lib::RepoUUID, size_t, repo::lib::RepoUUIDHasher>>();
			}

			auto it = texturedMeshes[meshGroup].find(mFormat);
			if (it == texturedMeshes[meshGroup].end())
			{
				texturedMeshes[meshGroup][mFormat] = std::unordered_map<repo::lib::RepoUUID, std::vector<std::set<repo::lib::RepoUUID>>, repo::lib::RepoUUIDHasher>();
				texturedFCount[meshGroup][mFormat] = std::unordered_map<repo::lib::RepoUUID, size_t, repo::lib::RepoUUIDHasher>();
			}
			auto it2 = texturedMeshes[meshGroup][mFormat].find(texID);

			if (it2 == texturedMeshes[meshGroup][mFormat].end())
			{
				texturedMeshes[meshGroup][mFormat][texID] = std::vector<std::set<repo::lib::RepoUUID>>();
				texturedMeshes[meshGroup][mFormat][texID].push_back(std::set<repo::lib::RepoUUID>());
				texturedFCount[meshGroup][mFormat][texID] = 0;
			}
			size_t faceCount = mesh->getFaces().size();
			if (texturedFCount[meshGroup][mFormat][texID] + faceCount > REPO_MP_MAX_VERTEX_COUNT ||
				texturedMeshes[meshGroup][mFormat][texID].back().size() > REPO_MP_MAX_MESHES_IN_SUPERMESH)
			{
				//Exceed max face count or meshes, create another grouping entry for this format
				texturedMeshes[meshGroup][mFormat][texID].push_back(std::set<repo::lib::RepoUUID>());
				texturedFCount[meshGroup][mFormat][texID] = 0;
			}
			texturedMeshes[meshGroup][mFormat][texID].back().insert(mesh->getUniqueID());
			texturedFCount[meshGroup][mFormat][texID] += mesh->getFaces().size();
		}
		else
		{
			//no texture, check if it is transparent
			const bool istransParentMesh = isTransparent(scene, mesh);
			auto &meshMap = istransParentMesh ? transparentMeshes : normalMeshes;
			auto &meshVertexCount = istransParentMesh ? transparentFCount : normalFCount;

			if (meshMap.find(meshGroup) == meshMap.end()) {
				meshMap[meshGroup] = std::unordered_map<uint32_t, std::vector<std::set<repo::lib::RepoUUID>>>();
				meshVertexCount[meshGroup] = std::unordered_map<uint32_t, size_t >();
			}

			auto it = meshMap[meshGroup].find(mFormat);
			if (it == meshMap[meshGroup].end())
			{
				meshMap[meshGroup][mFormat] = std::vector<std::set<repo::lib::RepoUUID>>();
				meshMap[meshGroup][mFormat].push_back(std::set<repo::lib::RepoUUID>());
				meshVertexCount[meshGroup][mFormat] = 0;
			}
			size_t vertexCount = mesh->getVertices().size();
			if (meshVertexCount[meshGroup][mFormat] && meshVertexCount[meshGroup][mFormat] + vertexCount > REPO_MP_MAX_VERTEX_COUNT ||
				meshMap[meshGroup][mFormat].back().size() > REPO_MP_MAX_MESHES_IN_SUPERMESH)
			{
				//Exceed max face count or meshes, create another grouping entry for this format
				meshMap[meshGroup][mFormat].push_back(std::set<repo::lib::RepoUUID>());
				meshVertexCount[meshGroup][mFormat] = 0;
			}
			meshMap[meshGroup][mFormat].back().insert(mesh->getUniqueID());
			meshVertexCount[meshGroup][mFormat] += mesh->getFaces().size();
		}
	}
}