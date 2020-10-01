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

static const size_t  REPO_MP_MAX_FACE_COUNT = 500000;
static const size_t REPO_MP_MAX_MESHES_IN_SUPERMESH = 5000;
static const size_t REPO_MP_MAX_MESHES_IN_SEPARATED_SUPERMESH = 1000;

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

bool MultipartOptimizer::collectMeshData(
	const repo::core::model::RepoScene        *scene,
	const repo::core::model::RepoNode         *node,
	const std::set<repo::lib::RepoUUID>                  &meshGroup,
	repo::lib::RepoMatrix                       &mat,
	std::vector<repo::lib::RepoVector3D>                &vertices,
	std::vector<repo::lib::RepoVector3D>                &normals,
	std::vector<repo_face_t>                  &faces,
	std::vector<std::vector<repo::lib::RepoVector2D>> &uvChannels,
	std::vector<repo_color4d_t>               &colors,
	std::vector<repo_mesh_mapping_t>          &meshMapping,
	std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoUUID, repo::lib::RepoUUIDHasher>    &matIDMap
)
{
	bool success = false;
	if (success = scene && node)
	{
		switch (node->getTypeAsEnum())
		{
		case repo::core::model::NodeType::TRANSFORMATION:
		{
			auto trans = (repo::core::model::TransformationNode *) node;
			mat = mat * trans->getTransMatrix(false);
			auto children = scene->getChildrenAsNodes(defaultGraph, trans->getSharedID());
			for (const auto &child : children)
			{
				auto childMat = mat; //We don't want actually want to update the matrix with our children's transformation
				success &= collectMeshData(scene, child, meshGroup, childMat,
					vertices, normals, faces, uvChannels, colors, meshMapping, matIDMap);
			}
			break;
		}

		case repo::core::model::NodeType::MESH:
		{
			repo::lib::RepoUUID meshUniqueID = node->getUniqueID();
			if (meshGroup.find(meshUniqueID) != meshGroup.end())
			{
				auto mesh = (repo::core::model::MeshNode *) node;
				repo::core::model::MeshNode transformedMesh = mesh->cloneAndApplyTransformation(mat);
				//this node is in the grouping, add it into the data buffers
				repo_mesh_mapping_t meshMap;
				repo::lib::RepoUUID matID = getMaterialID(scene, mesh);
				repo::lib::RepoUUID newMatID;

				if (matIDMap.find(matID) == matIDMap.end())
				{
					newMatID = repo::lib::RepoUUID::createUUID();
					matIDMap[matID] = newMatID;
				}
				else
					newMatID = matIDMap[matID];
				meshMap.material_id = newMatID;
				meshMap.mesh_id = meshUniqueID;
				auto bbox = transformedMesh.getBoundingBox();
				if (bbox.size() >= 2)
				{
					meshMap.min = bbox[0];
					meshMap.max = bbox[1];
				}

				std::vector<repo::lib::RepoVector3D> submVertices = transformedMesh.getVertices();
				std::vector<repo::lib::RepoVector3D> submNormals = transformedMesh.getNormals();
				std::vector<repo_face_t>   submFaces = transformedMesh.getFaces();
				std::vector<repo_color4d_t> submColors = transformedMesh.getColors();
				std::vector<std::vector<repo::lib::RepoVector2D>> submUVs = transformedMesh.getUVChannelsSeparated();

				if (success = submVertices.size() && submFaces.size())
				{
					meshMap.vertFrom = vertices.size();
					meshMap.vertTo = meshMap.vertFrom + submVertices.size();
					meshMap.triFrom = faces.size();
					meshMap.triTo = faces.size() + submFaces.size();

					meshMapping.push_back(meshMap);

					vertices.insert(vertices.end(), submVertices.begin(), submVertices.end());
					for (const auto face : submFaces)
					{
						repo_face_t offsetFace;
						for (const auto idx : face)
						{
							offsetFace.push_back(meshMap.vertFrom + idx);
						}
						faces.push_back(offsetFace);
					}

					if (submNormals.size())
						normals.insert(normals.end(), submNormals.begin(), submNormals.end());
					if (submColors.size())
						colors.insert(colors.end(), submColors.begin(), submColors.end());

					if (uvChannels.size() == 0 && submUVs.size() != 0)
					{
						//initialise uvChannels
						uvChannels.resize(submUVs.size());
					}

					if (success = uvChannels.size() == submUVs.size())
					{
						for (uint32_t i = 0; i < submUVs.size(); ++i)
						{
							uvChannels[i].insert(uvChannels[i].end(), submUVs[i].begin(), submUVs[i].end());
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
			break;
		}
		}
	}
	else
	{
		repoError << "Scene or node is null!";
	}

	return success;
}

repo::core::model::MeshNode* MultipartOptimizer::createSuperMesh
(
	const repo::core::model::RepoScene *scene,
	const std::set<repo::lib::RepoUUID>           &meshGroup,
	std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoUUID, repo::lib::RepoUUIDHasher>  &matIDs)
{
	std::vector<repo::lib::RepoVector3D> vertices, normals;
	std::vector<repo_face_t> faces;
	std::vector<std::vector<repo::lib::RepoVector2D>> uvChannels;
	std::vector<repo_color4d_t> colors;
	std::vector<repo_mesh_mapping_t> meshMapping;

	repo::core::model::MeshNode* resultMesh = nullptr;

	std::vector<repo::core::model::MeshNode*> resultMeshes;
	repo::lib::RepoMatrix startMat;

	bool success = collectMeshData(scene, scene->getRoot(defaultGraph), meshGroup, startMat,
		vertices, normals, faces, uvChannels, colors, meshMapping, matIDs);

	if (success && meshMapping.size())
	{
		//workout bbox and outline from meshMapping
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

		repo::core::model::MeshNode superMesh = repo::core::model::RepoBSONFactory::makeMeshNode(vertices, faces, normals, bboxVec, uvChannels, colors, outline);
		resultMesh = new repo::core::model::MeshNode(superMesh.cloneAndUpdateMeshMapping(meshMapping, true));
	}
	else
	{
		repoError << " Failed to collect mesh data (" << success << ") , meshGroup size: " << meshGroup.size() << "mesh mapping size: " << meshMapping.size();
	}

	return resultMesh;
}

bool MultipartOptimizer::generateMultipartScene(repo::core::model::RepoScene *scene)
{
	bool success = false;

	auto meshes = scene->getAllMeshes(defaultGraph);
	if (success = meshes.size())
	{
		std::unordered_map<uint32_t, std::unordered_map<repo::lib::RepoUUID, std::vector<std::set<repo::lib::RepoUUID>>, repo::lib::RepoUUIDHasher>> normalMeshes;
		std::vector<std::set<repo::lib::RepoUUID>> separateMeshes;
		//Sort the meshes into different groupings
		sortMeshes(scene, meshes, normalMeshes, separateMeshes);

		repo::core::model::RepoNodeSet mergedMeshes, materials, trans, textures, dummy;

		auto rootNode = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode());
		trans.insert(rootNode);
		repo::lib::RepoUUID rootID = rootNode->getSharedID();

		std::unordered_map<repo::lib::RepoUUID, repo::core::model::RepoNode*, repo::lib::RepoUUIDHasher> matNodes;
		std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoUUID, repo::lib::RepoUUIDHasher> matIDs;

		//textured meshes
		for (const auto &textureMeshMap : normalMeshes)
		{
			for (const auto &groupings : textureMeshMap.second)
			{
				for (const auto grouping : groupings.second)
				{
					success &= processMeshGroup(scene, grouping, rootID, mergedMeshes, matNodes, matIDs);
				}
			}
		}

		//Ungrouped meshes
		for (const auto &meshGroup : separateMeshes)
			success &= processMeshGroup(scene, meshGroup, rootID, mergedMeshes, matNodes, matIDs, true);

		if (success)
		{
			//fill Material nodeset
			for (const auto &matPair : matNodes)
			{
				materials.insert(matPair.second);
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

repo::lib::RepoUUID MultipartOptimizer::getTextureID(
	const repo::core::model::RepoScene *scene,
	const repo::core::model::MeshNode  *mesh)
{
	repo::lib::RepoUUID texID;
	const auto mat = scene->getChildrenNodesFiltered(defaultGraph, mesh->getSharedID(), repo::core::model::NodeType::MATERIAL);
	if (mat.size())
	{
		const auto texture = scene->getChildrenNodesFiltered(defaultGraph, mat[0]->getSharedID(), repo::core::model::NodeType::TEXTURE);
		if (texture.size())
		{
			texID = texture[0]->getSharedID();
		}
	}

	return texID;
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
	const repo::core::model::RepoScene                                        *scene,
	const  std::set<repo::lib::RepoUUID>                                                  &meshes,
	const repo::lib::RepoUUID                                                             &rootID,
	repo::core::model::RepoNodeSet                                             &mergedMeshes,
	std::unordered_map<repo::lib::RepoUUID, repo::core::model::RepoNode*, repo::lib::RepoUUIDHasher> &matNodes,
	std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoUUID, repo::lib::RepoUUIDHasher>                     &matIDs,
	const bool independentGroup
)
{
	if (!meshes.size()) return true;

	bool success = false;
	auto sMesh = createSuperMesh(scene, meshes, matIDs);
	if (success = sMesh)
	{
		auto sMeshWithParent = (independentGroup ? sMesh->cloneAndFlagIndependent() : *sMesh).cloneAndAddParent({ rootID });
		sMesh->swap(sMeshWithParent);
		mergedMeshes.insert(sMesh);

		repo::lib::RepoUUID sMeshSharedID = sMesh->getSharedID();
		std::set<repo::lib::RepoUUID> currentMats;
		for (const auto &map : sMesh->getMeshMapping())
		{
			currentMats.insert(map.material_id);
		}

		/**
		* FIXME: since there's multiple sMeshes, we need to make sure we only process the ones
		* that are within the current sMesh. This is really inefficient to loop through all the matIDs
		* but it will do for now. To do it properly we need a bi-directional map to find the original matID base
		* on the new one
		* This should all go away once we have proper texture support
		*/
		for (const auto matID : matIDs)
		{
			//Skip the material if it's not used in the current supermesh
			if (currentMats.find(matID.second) == currentMats.end()) continue;

			auto matIt = matNodes.find(matID.second);
			if (matIt == matNodes.end())
			{
				//This material hasn't beenn copied yet.
				//clone and wipe the parent entries, insert new parents
				auto matNode = scene->getNodeByUniqueID(defaultGraph, matID.first);
				if (matNode)
				{
					repo::core::model::RepoNode clonedMat = repo::core::model::RepoNode(matNode->removeField(REPO_NODE_LABEL_PARENTS));
					repo::core::model::RepoBSONBuilder builder;
					builder.append(REPO_NODE_LABEL_ID, matID.second);
					auto changeBSON = builder.obj();
					clonedMat = clonedMat.cloneAndAddFields(&changeBSON, false);
					clonedMat = clonedMat.cloneAndAddParent({ sMeshSharedID });
					matNodes[matID.second] = new repo::core::model::MaterialNode(clonedMat);
				}
			}
			else
			{
				//created by another superMesh (set should have no duplicate so it shouldn't be from this mesh)
				auto addedParentMat = matIt->second->cloneAndAddParent({ sMeshSharedID });
				matIt->second->swap(addedParentMat);
			}
		}
	}
	else
	{
		repoError << "Failed to create super mesh (nullptr returned)";
	}
	return success;
}

void MultipartOptimizer::sortMeshes(
	const repo::core::model::RepoScene                                      *scene,
	const repo::core::model::RepoNodeSet                                    &meshes,
	std::unordered_map < uint32_t, std::unordered_map < repo::lib::RepoUUID,
	std::vector<std::set<repo::lib::RepoUUID>>, repo::lib::RepoUUIDHasher >> &normalMeshes,
	std::vector<std::set<repo::lib::RepoUUID>> &separateMeshes
)
{
	std::unordered_map<uint32_t, std::unordered_map<repo::lib::RepoUUID, size_t, repo::lib::RepoUUIDHasher> > normalFCount;
	size_t separatesFCount = 0;

	for (const auto &node : meshes)
	{
		auto mesh = (repo::core::model::MeshNode*) node;
		if (!mesh->getVertices().size() || !mesh->getFaces().size())
		{
			repoWarning << "mesh " << mesh->getUniqueID() << " has no vertices/faces, skipping...";
			continue;
		}

		if (mesh->isIndependent()) {
			size_t faceCount = mesh->getFaces().size();
			if (!separateMeshes.size()) {
				separateMeshes.push_back(std::set<repo::lib::RepoUUID>());
			}
			if (separatesFCount + faceCount > REPO_MP_MAX_FACE_COUNT ||
				separateMeshes.back().size() > REPO_MP_MAX_MESHES_IN_SEPARATED_SUPERMESH)
			{
				//Exceed max face count or meshes, create another grouping entry for this format
				separateMeshes.push_back(std::set<repo::lib::RepoUUID>());
				separatesFCount = 0;
			}
			separateMeshes.back().insert(mesh->getUniqueID());
		}
		else {
			uint32_t mFormat = mesh->getMFormat(isTransparent(scene, mesh), scene->isHiddenByDefault(mesh->getUniqueID()));
			repo::lib::RepoUUID texID = getTextureID(scene, mesh);
			auto it = normalMeshes.find(mFormat);
			if (it == normalMeshes.end())
			{
				normalMeshes[mFormat] = std::unordered_map<repo::lib::RepoUUID, std::vector<std::set<repo::lib::RepoUUID>>, repo::lib::RepoUUIDHasher>();
				normalFCount[mFormat] = std::unordered_map<repo::lib::RepoUUID, size_t, repo::lib::RepoUUIDHasher>();
			}
			auto it2 = normalMeshes[mFormat].find(texID);

			if (it2 == normalMeshes[mFormat].end())
			{
				normalMeshes[mFormat][texID] = std::vector<std::set<repo::lib::RepoUUID>>();
				normalMeshes[mFormat][texID].push_back(std::set<repo::lib::RepoUUID>());
				normalFCount[mFormat][texID] = 0;
			}
			size_t faceCount = mesh->getFaces().size();
			if (normalFCount[mFormat][texID] + faceCount > REPO_MP_MAX_FACE_COUNT ||
				normalMeshes[mFormat][texID].back().size() > REPO_MP_MAX_MESHES_IN_SUPERMESH)
			{
				//Exceed max face count or meshes, create another grouping entry for this format
				normalMeshes[mFormat][texID].push_back(std::set<repo::lib::RepoUUID>());
				normalFCount[mFormat][texID] = 0;
			}
			normalMeshes[mFormat][texID].back().insert(mesh->getUniqueID());
			normalFCount[mFormat][texID] += mesh->getFaces().size();
		}
	}
}