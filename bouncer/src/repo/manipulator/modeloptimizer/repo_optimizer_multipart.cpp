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

using namespace repo::manipulator::modeloptimizer;

auto defaultGraph = repo::core::model::RepoScene::GraphType::DEFAULT;

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
	const std::set<repoUUID>                  &meshGroup,
	std::vector<float>                        &mat,
	std::vector<repo_vector_t>                &vertices,
	std::vector<repo_vector_t>                &normals,
	std::vector<repo_face_t>                  &faces,
	std::vector<std::vector<repo_vector2d_t>> &uvChannels,
	std::vector<repo_color4d_t>               &colors,
	std::vector<repo_mesh_mapping_t>          &meshMapping
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
			mat = matMult(mat, trans->getTransMatrix());
			auto children = scene->getChildrenAsNodes(defaultGraph, trans->getSharedID());
			for (const auto &child : children)
			{
				auto childMat = mat; //We don't want actually want to update the matrix with our children's transformation
				success &= collectMeshData(scene, child, meshGroup, childMat, vertices, normals, faces, uvChannels, colors, meshMapping);
			}
			break;
		}
		
		case repo::core::model::NodeType::MESH:
		{
			repoUUID meshUniqueID = node->getUniqueID();
			if (meshGroup.find(meshUniqueID) != meshGroup.end())
			{
				auto mesh = (repo::core::model::MeshNode *) node;

				repo::core::model::MeshNode transformedMesh = mesh->cloneAndApplyTransformation(mat);
				//this node is in the grouping, add it into the data buffers
				repo_mesh_mapping_t meshMap;
				meshMap.material_id = getMaterialID(scene, &transformedMesh);
				repoTrace << "Material ID is: " << UUIDtoString(meshMap.material_id);
				meshMap.mesh_id = meshUniqueID;
				auto bbox = transformedMesh.getBoundingBox();
				if (bbox.size() >= 2)
				{
					meshMap.min = bbox[0];
					meshMap.min = bbox[1];
				}

				std::vector<repo_vector_t> submVertices = transformedMesh.getVertices();
				std::vector<repo_vector_t> submNormals  = transformedMesh.getNormals();
				std::vector<repo_face_t>   submFaces    = transformedMesh.getFaces();
				std::vector<repo_color4d_t> submColors = transformedMesh.getColors();
				std::vector<std::vector<repo_vector2d_t>> submUVs = transformedMesh.getUVChannelsSeparated();

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

repo::core::model::MeshNode* MultipartOptimizer::createSuperMesh(
	const repo::core::model::RepoScene *scene,
	const std::set<repoUUID>           &meshGroup,
	std::set<repoUUID>                 &matIDs)
{
	std::vector<repo_vector_t> vertices, normals;
	std::vector<repo_face_t> faces;
	std::vector<std::vector<repo_vector2d_t>> uvChannels;
	std::vector<repo_color4d_t> colors;
	std::vector<repo_mesh_mapping_t> meshMapping;

	repo::core::model::MeshNode* resultMesh = nullptr;

	std::vector<float> identity = { 1, 0, 0, 0,
									0, 1, 0, 0,
									0, 0, 1, 0,
									0, 0, 0, 1 };

	bool success = collectMeshData(scene, scene->getRoot(defaultGraph), meshGroup, identity,
		vertices, normals, faces, uvChannels, colors, meshMapping);

	repoTrace << "mesh mapping: " << meshMapping.size();
	
	if (success && meshMapping.size())
	{
		//workout bbox and outline from meshMapping
		std::vector<repo_vector_t> bbox;
		bbox.push_back(meshMapping[0].min);
		bbox.push_back(meshMapping[0].max);
		matIDs.insert(meshMapping[0].material_id);
		for (int i = 1; i < meshMapping.size(); ++i)
		{
			if (bbox[0].x > meshMapping[i].min.x)
				bbox[0].x = meshMapping[i].min.x;
			if (bbox[0].y > meshMapping[i].min.y)
				bbox[0].y = meshMapping[i].min.y;
			if (bbox[0].z > meshMapping[i].min.z)
				bbox[0].z = meshMapping[i].min.z;

			if (bbox[1].x > meshMapping[i].max.x)
				bbox[1].x = meshMapping[i].max.x;
			if (bbox[1].y > meshMapping[i].max.y)
				bbox[1].y = meshMapping[i].max.y;
			if (bbox[1].z > meshMapping[i].max.z)
				bbox[1].z = meshMapping[i].max.z;

			matIDs.insert(meshMapping[i].material_id);
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
		repoError << "Failed";
	}
	
	return resultMesh;

}

bool MultipartOptimizer::generateMultipartScene(repo::core::model::RepoScene *scene)
{
	bool success = false;	

	auto meshes = scene->getAllMeshes(defaultGraph);
	if (success = meshes.size())
	{
		std::unordered_map<uint32_t, std::set<repoUUID>> transparentMeshes, normalMeshes;
		std::unordered_map<uint32_t, std::unordered_map<repoUUID, std::set<repoUUID>, RepoUUIDHasher>> texturedMeshes;

		//Sort the meshes into 3 different grouping
		sortMeshes(scene, meshes, normalMeshes, transparentMeshes, texturedMeshes);

		repo::core::model::RepoNodeSet mergedMeshes, materials, trans, textures, dummy;

		auto rootNode = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode());
		trans.insert(rootNode);
		repoUUID rootID = rootNode->getSharedID();

		std::unordered_map<repoUUID, repo::core::model::RepoNode*, RepoUUIDHasher> matNodes;

		for (const auto &grouping : normalMeshes)
		{
			success &= processMeshGroup(scene, grouping.second, rootID, mergedMeshes, matNodes);
		}

		for (const auto &grouping : transparentMeshes)
		{
			success &= processMeshGroup(scene, grouping.second, rootID, mergedMeshes, matNodes);
		}

		//textured meshes
		for (const auto &textureMeshMap : texturedMeshes)
		{
			for (const auto &grouping : textureMeshMap.second)
			{
				success &= processMeshGroup(scene, grouping.second, rootID, mergedMeshes, matNodes);
			}			
		}

		if (success)
		{
			repoTrace << "matNodes: " << matNodes.size();
			//fill Material nodeset
			for (const auto &matPair : matNodes)
			{
				materials.insert(matPair.second);
			}

			//fill Texture nodeset
			for (const auto &texture : scene->getAllTextures(defaultGraph))
			{
				//create new instance to avoid X contamination
				textures.insert(new repo::core::model::TextureNode(*texture));
			}

			repoTrace << " mergedMeshes : " << mergedMeshes.size() << " materials: " << materials.size() << " textures: " << textures.size() << " trans: " << trans.size();

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

repoUUID MultipartOptimizer::getMaterialID(
	const repo::core::model::RepoScene *scene,
	const repo::core::model::MeshNode  *mesh
	)
{
	repoUUID matID;
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
	repoUUID                           &texID)
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
	const repo::core::model::RepoScene                                        *scene,
	const  std::set<repoUUID>                                                  &meshes,
	const repoUUID                                                             &rootID,
	repo::core::model::RepoNodeSet                                             &mergedMeshes,
	std::unordered_map<repoUUID, repo::core::model::RepoNode*, RepoUUIDHasher> &matNodes
	)
{
	bool success = false;
	std::set<repoUUID> matIDs;
	auto sMesh = createSuperMesh(scene, meshes, matIDs);
	repoTrace << "mat IDs: " << matIDs.size();
	if (success = sMesh)
	{
		auto sMeshWithParent = sMesh->cloneAndAddParent({ rootID });
		sMesh->swap(sMeshWithParent);
		mergedMeshes.insert(sMesh);

		repoUUID sMeshSharedID = sMesh->getSharedID();

		for (const auto matID : matIDs)
		{
			auto matIt = matNodes.find(matID);
			if (matIt == matNodes.end())
			{
				//This material hasn't beenn copied yet.
				//clone and wipe the parent entries, insert new parents
				auto matNode = scene->getNodeByUniqueID(defaultGraph, matID);
				repo::core::model::RepoNode clonedMat = matNode->removeField(REPO_NODE_LABEL_PARENTS);
				clonedMat = clonedMat.cloneAndAddParent({ sMeshSharedID });
				matNodes[matID] =  new repo::core::model::MaterialNode(clonedMat);
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
	std::unordered_map<uint32_t, std::set<repoUUID>>						&normalMeshes,
	std::unordered_map<uint32_t, std::set<repoUUID>>						&transparentMeshes,
	std::unordered_map<uint32_t, std::unordered_map<repoUUID, 
									   std::set<repoUUID>, RepoUUIDHasher> > &texturedMeshes)
{

	for (const auto &node : meshes)
	{
		auto mesh = (repo::core::model::MeshNode*) node;
		repoTrace << " faces: " << mesh->getFaces().size();
		/**
		* 1 - figure out it's mFormat (what buffers does it have)
		* 2 - check if it has texture
		* 3 - if not, check if it is transparent
		*/
		uint32_t mFormat = mesh->getMFormat();

		repoUUID texID;
		if (hasTexture(scene, mesh, texID))
		{
			auto it = texturedMeshes.find(mFormat);
			if (it == texturedMeshes.end())
			{
				texturedMeshes[mFormat] = std::unordered_map<repoUUID, std::set<repoUUID>, RepoUUIDHasher>();
			}

			auto it2 = texturedMeshes[mFormat].find(texID);
			if (it2 == texturedMeshes[mFormat].end())
			{
				texturedMeshes[mFormat][texID] = std::set<repoUUID>();
			}

			texturedMeshes[mFormat][texID].insert(mesh->getUniqueID());
		}
		else
		{
			//no mesh, check if it is transparent
			auto &meshMap = isTransparent(scene, mesh) ? transparentMeshes : normalMeshes;
			auto it = meshMap.find(mFormat);
			if (it == meshMap.end())
			{
				meshMap[mFormat] = std::set<repoUUID>();
			}


			meshMap[mFormat].insert(mesh->getUniqueID());
		}

	}
}