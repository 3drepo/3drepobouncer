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
* Allows Export functionality into/output Repo world
*/

#include "repo_model_export_assimp.h"
#include <boost/filesystem.hpp>
#include <fstream>
#include "../../../core/model/bson/repo_node_texture.h"
#include "../../../core/model/bson/repo_node_transformation.h"
#include "../../../lib/repo_log.h"

using namespace repo::manipulator::modelconvertor;

#define REPO_DEFAULT_TEXTURE_EXT ".jpg"

AssimpModelExport::AssimpModelExport(
	const repo::core::model::RepoScene *scene) : AbstractModelExport(scene)
{
}

AssimpModelExport::~AssimpModelExport()
{
}

aiScene* AssimpModelExport::convertToAssimp(
	const repo::core::model::RepoScene            *scene,
	repo::core::model::RepoNodeSet                &textNodes,
	const repo::core::model::RepoScene::GraphType &gType)
{
	aiScene *assimpScene = nullptr;
	std::vector<aiMesh*>                      meshVec;
	std::vector<aiMaterial*>                  matVec;
	std::vector<aiCamera*>                    camVec;
	aiNode *rootNode = constructAiSceneRecursively(
		scene, scene->getRoot(gType),
		meshVec, matVec, camVec,
		textNodes);

	if (rootNode)
	{
		assimpScene = new aiScene();
		assimpScene->mFlags = 0; //getPostProcessingFlags(); // TODO FIX ME!

		assimpScene->mNumCameras = camVec.size();
		repoTrace << "CamVec: " << camVec.size();
		if (camVec.size() > 0)
		{
			assimpScene->mCameras = new aiCamera*[camVec.size()];
			std::copy(camVec.begin(), camVec.end(), assimpScene->mCameras);
		}

		assimpScene->mNumMaterials = matVec.size();
		repoTrace << "matVec: " << matVec.size();
		if (matVec.size() > 0)
		{
			assimpScene->mMaterials = new aiMaterial*[matVec.size()];
			std::copy(matVec.begin(), matVec.end(), assimpScene->mMaterials);
		}

		assimpScene->mNumMeshes = meshVec.size();
		repoTrace << "meshVec: " << meshVec.size();
		if (meshVec.size() > 0)
		{
			assimpScene->mMeshes = new aiMesh*[meshVec.size()];
			uint32_t i = 0;
			//std::copy(meshVec.begin(), meshVec.end(), assimpScene->mMeshes);
			for (auto &mesh : meshVec)
			{
				assimpScene->mMeshes[i++] = mesh;
				repoTrace << " mesh #" << i - 1 << " #vertices = " << mesh->mNumVertices;
			}
		}

		//FIXME: do i need to fill in the texture ones?

		assimpScene->mRootNode = rootNode;
	}

	return assimpScene;
}

aiNode* AssimpModelExport::constructAiSceneRecursively(
	const repo::core::model::RepoScene            *scene,
	const repo::core::model::RepoNode             *currNode,
	std::vector<aiMesh*>                          &meshVec,
	std::vector<aiMaterial*>                      &matVec,
	std::vector<aiCamera*>                        &camVec,
	repo::core::model::RepoNodeSet                &textNodes,
	const repo::core::model::RepoScene::GraphType &gType)
{
	std::unordered_map<repo::lib::RepoUUID, aiMesh*, repo::lib::RepoUUIDHasher>     meshMap;
	std::unordered_map<repo::lib::RepoUUID, aiMaterial*, repo::lib::RepoUUIDHasher> matMap;
	std::unordered_map<repo::lib::RepoUUID, aiCamera*, repo::lib::RepoUUIDHasher>   camMap;

	return constructAiSceneRecursively(scene, currNode, meshVec, matVec, camVec,
		meshMap, matMap, camMap, textNodes, gType);
}

aiNode* AssimpModelExport::constructAiSceneRecursively(
	const repo::core::model::RepoScene                        *scene,
	const repo::core::model::RepoNode                         *currNode,
	std::vector<aiMesh*>                                      &meshVec,
	std::vector<aiMaterial*>                                  &matVec,
	std::vector<aiCamera*>                                    &camVec,
	std::unordered_map<repo::lib::RepoUUID, aiMesh*, repo::lib::RepoUUIDHasher>     &meshMap,
	std::unordered_map<repo::lib::RepoUUID, aiMaterial*, repo::lib::RepoUUIDHasher> &matMap,
	std::unordered_map<repo::lib::RepoUUID, aiCamera*, repo::lib::RepoUUIDHasher>   &camMap,
	repo::core::model::RepoNodeSet                            &textNodes,
	const repo::core::model::RepoScene::GraphType             &gType)
{
	/*
	* Assumptions:
	* - A mesh will always have a transformation as parent
	* - A camera will always have a transformation as parent
	* - Material will always have a mesh as parent
	* - Texture will always be an immediate child of Material
	* - Materials/Textures/Mesh/Cameras will be in the scene graph
	*   of it's aforementioned parents, not in a referenced scene
	*/

	aiNode *node = nullptr;

	if (scene && currNode)
	{
		switch (currNode->getTypeAsEnum())
		{
			//we only need to construct an aiNode if it is a transformation, or a reference to another scene
		case repo::core::model::NodeType::TRANSFORMATION:
		{
			node = new aiNode();
			if (node)
			{
				repo::core::model::TransformationNode *currNodeTrans =
					(repo::core::model::TransformationNode*) currNode;
				node->mName = aiString(currNodeTrans->getName());
				auto transMat = currNodeTrans->getTransMatrix(false).getData();
				if (transMat.size() >= 16)
					node->mTransformation = aiMatrix4x4(transMat[0], transMat[1], transMat[2], transMat[3],
					transMat[4], transMat[5], transMat[6], transMat[7],
					transMat[8], transMat[9], transMat[10], transMat[11],
					transMat[12], transMat[13], transMat[14], transMat[15]);
				else
				{
					repoError << "AssimpModelExport: Failed to obtain Transformation matrix from Transformation Node!";
				}

				// Find Mesh/Camera childs
				std::vector<uint32_t> meshIndices;
				for (const auto & child : scene->getChildrenAsNodes(gType, currNode->getSharedID()))
				{
					repo::lib::RepoUUID childSharedID = child->getSharedID();

					//==================MESH============================
					switch (child->getTypeAsEnum())
					{
					case repo::core::model::NodeType::MESH:
					{
						auto it = meshMap.find(childSharedID);
						aiMesh *assimpMesh;

						if (it == meshMap.end())
						{
							//mesh isn't in the map yet - create a new entry
							assimpMesh = convertMesh(scene, (repo::core::model::MeshNode *)child, matVec, matMap, textNodes);
							meshMap[childSharedID] = assimpMesh;
							meshVec.push_back(assimpMesh);
						}
						else{
							assimpMesh = it->second;
						}

						//Find index within the map
						uint32_t index = std::distance(meshVec.begin(), std::find(meshVec.begin(), meshVec.end(), assimpMesh));
						meshIndices.push_back(index);
					}
					break;

					//--------------------------------------------------------------------------
					// Cameras
					// Unlike meshes, cameras are not pointed to by index from transformations.
					// Instead, corresponding camera shares the same name with transformation in Assimp.
					case repo::core::model::NodeType::CAMERA:
					{
						auto it = camMap.find(childSharedID);
						aiCamera *assimpCam;

						if (it == camMap.end())
						{
							//mesh isn't in the map yet - create a new entry
							assimpCam = convertCamera(scene, (repo::core::model::CameraNode *)child, currNode->getName());
							camMap[childSharedID] = assimpCam;
							camVec.push_back(assimpCam);
						}
						else{
							assimpCam = it->second;
						}

						//Find index within the map
						uint32_t index = std::distance(camVec.begin(), std::find(camVec.begin(), camVec.end(), assimpCam));
					}
					}
				} //end of looping for mesh/camera

				node->mMeshes = new unsigned int[meshIndices.size()];
				std::copy(meshIndices.begin(), meshIndices.end(), node->mMeshes);
				node->mNumMeshes = meshIndices.size();
			}
		}
		break;
		case repo::core::model::NodeType::REFERENCE:
		{
			//FIXME: default?
			const repo::core::model::RepoScene *refScene =
				scene->getSceneFromReference(repo::core::model::RepoScene::GraphType::DEFAULT, currNode->getSharedID());
			node = constructAiSceneRecursively(refScene, refScene->getRoot(gType),
				meshVec, matVec, camVec, meshMap, matMap, camMap, textNodes);
		}
		break;
		} //end of switch

		//FIXME: this doesn't work if there's randomly a transformation again after other nodes (like meshes, textures etc)

		if (node)
		{
			//deal with the children
			std::vector<aiNode*> children;
			for (const auto & child : scene->getChildrenAsNodes(gType, currNode->getSharedID()))
			{
				aiNode *aiChild = constructAiSceneRecursively(scene, child,
					meshVec, matVec, camVec, meshMap, matMap, camMap, textNodes);

				if (aiChild)
				{
					aiChild->mParent = node;
					children.push_back(aiChild);
				}
			}

			node->mNumChildren += children.size();
			if (children.size())
			{
				node->mChildren = new aiNode*[children.size()];
				std::copy(children.begin(), children.end(), node->mChildren);
			}
		}
	}

	return node;
}

aiCamera* AssimpModelExport::convertCamera(
	const repo::core::model::RepoScene  *scene,
	const repo::core::model::CameraNode *camNode,
	const std::string                   &name)
{
	if (!scene || !camNode) return nullptr;

	aiCamera *aiCam = new aiCamera();
	//--------------------------------------------------------------------------
	// Name
	if (!name.empty())
		aiCam->mName = aiString(name);
	else
		aiCam->mName = aiString(camNode->getName());

	//--------------------------------------------------------------------------
	// Aspect ratio
	aiCam->mAspect = camNode->getAspectRatio();

	//--------------------------------------------------------------------------
	// Far clipping plane
	aiCam->mClipPlaneFar = camNode->getFarClippingPlane();

	//--------------------------------------------------------------------------
	// Near clipping plane
	aiCam->mClipPlaneNear = camNode->getNearClippingPlane();
	//--------------------------------------------------------------------------
	// Field of view
	aiCam->mHorizontalFOV = camNode->getFieldOfView();

	//--------------------------------------------------------------------------
	// Look at vector
	repo::lib::RepoVector3D lookAt = camNode->getLookAt();
	aiCam->mLookAt = aiVector3D(lookAt.x, lookAt.y, lookAt.z);

	//--------------------------------------------------------------------------
	// Position vector
	repo::lib::RepoVector3D position = camNode->getPosition();
	aiCam->mPosition = aiVector3D(position.x, position.y, position.z);;

	//--------------------------------------------------------------------------
	// Up vector
	repo::lib::RepoVector3D up = camNode->getUp();
	aiCam->mUp = aiVector3D(up.x, up.y, up.z);

	return aiCam;
}

aiMaterial* AssimpModelExport::convertMaterial(
	const repo::core::model::RepoScene            *scene,
	const repo::core::model::MaterialNode         *matNode,
	repo::core::model::RepoNodeSet                &textNodes,
	const repo::core::model::RepoScene::GraphType &gType)
{
	if (!matNode || !scene) return nullptr;

	aiMaterial * aiMat = new aiMaterial();
	//--------------------------------------------------------------------------
	// Name
	aiMat->AddProperty(new aiString(matNode->getName()), AI_MATKEY_NAME);

	const repo_material_t matStruct = matNode->getMaterialStruct();
	//--------------------------------------------------------------------------
	// Ambient
	if (matStruct.ambient.size())
		aiMat->AddProperty(new aiColor3D(
		matStruct.ambient[0], matStruct.ambient[1], matStruct.ambient[2]), 1,
		AI_MATKEY_COLOR_AMBIENT);

	//--------------------------------------------------------------------------
	// Diffuse
	if (matStruct.diffuse.size())
		aiMat->AddProperty(new aiColor3D(
		matStruct.diffuse[0], matStruct.diffuse[1], matStruct.diffuse[2]), 1,
		AI_MATKEY_COLOR_DIFFUSE);

	//--------------------------------------------------------------------------
	// Emissive
	if (matStruct.emissive.size())
		aiMat->AddProperty(new aiColor3D(
		matStruct.emissive[0], matStruct.emissive[1], matStruct.emissive[2]), 1,
		AI_MATKEY_COLOR_EMISSIVE);

	//--------------------------------------------------------------------------
	// Specular
	if (matStruct.specular.size())
		aiMat->AddProperty(new aiColor3D(
		matStruct.specular[0], matStruct.specular[1], matStruct.specular[2]), 1,
		AI_MATKEY_COLOR_SPECULAR);

	//--------------------------------------------------------------------------
	// Wireframe

	aiMat->AddProperty(new int((int)matStruct.isWireframe),
		1, AI_MATKEY_ENABLE_WIREFRAME);

	//--------------------------------------------------------------------------
	// Two-sided
	aiMat->AddProperty(new int((int)matStruct.isTwoSided),
		1, AI_MATKEY_TWOSIDED);

	//--------------------------------------------------------------------------
	// Opacity
	if (matStruct.opacity == matStruct.opacity)
		aiMat->AddProperty(new float(matStruct.opacity), 1, AI_MATKEY_OPACITY);

	//--------------------------------------------------------------------------
	// Shininess
	if (matStruct.shininess == matStruct.shininess)
		aiMat->AddProperty(new float(matStruct.shininess), 1, AI_MATKEY_SHININESS);

	//--------------------------------------------------------------------------
	// Shininess strength (default is 1)
	if (matStruct.shininessStrength == matStruct.shininessStrength)
		aiMat->AddProperty(new float(matStruct.shininessStrength), 1,
		AI_MATKEY_SHININESS_STRENGTH);

	//--------------------------------------------------------------------------
	// Diffuse texture
	// 3D Repo supports only diffuse textures at the moment
	for (const auto &child : scene->getChildrenAsNodes(gType, matNode->getSharedID()))
	{
		if (child->getTypeAsEnum() == repo::core::model::NodeType::TEXTURE)
		{
			aiString *texName = new aiString(child->getName());
			aiMat->AddProperty(texName,
				AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));

			if (std::find(textNodes.begin(), textNodes.end(), child) == textNodes.end())
			{
				//add this texture node in the textNodes
				textNodes.insert(child);
			}
			break;
		}
	}

	return aiMat;
}

aiMesh* AssimpModelExport::convertMesh(
	const repo::core::model::RepoScene                        *scene,
	const repo::core::model::MeshNode                         *meshNode,
	std::vector<aiMaterial*>                                  &matVec,
	std::unordered_map<repo::lib::RepoUUID, aiMaterial*, repo::lib::RepoUUIDHasher> &matMap,
	repo::core::model::RepoNodeSet                            &textNodes,
	const repo::core::model::RepoScene::GraphType             &gType)
{
	if (!meshNode || !scene) return nullptr;

	aiMesh *assimpMesh = new aiMesh();

	assimpMesh->mName = aiString(meshNode->getName());

	std::vector<repo_face_t> faces = meshNode->getFaces();
	//--------------------------------------------------------------------------
	// Faces
	if (faces.size())
	{
		assimpMesh->mFaces = new aiFace[faces.size()];
		if (assimpMesh->mFaces)
		{
			uint32_t i = 0;
			for (const auto &face : faces)
			{
				assimpMesh->mFaces[i].mIndices = (unsigned int*)face.data();
				assimpMesh->mFaces[i].mNumIndices = face.size();
				i++;
			}
			assimpMesh->mNumFaces = faces.size();
			assimpMesh->mPrimitiveTypes = 4; // TODO: work out the exact primitive type of each mesh!
		}
		else
			assimpMesh->mNumFaces = 0;
	}
	else
		assimpMesh->mNumFaces = 0;

	//--------------------------------------------------------------------------
	// Vertices
	// Make a copy of vertices
	std::vector<repo::lib::RepoVector3D> vertices = meshNode->getVertices();
	assimpMesh->mVertices = new aiVector3D[vertices.size()];
	if (assimpMesh->mVertices)
	{
		uint32_t i = 0;
		for (const auto vertix : vertices)
		{
			assimpMesh->mVertices[i].x = vertix.x;
			assimpMesh->mVertices[i].y = vertix.y;
			assimpMesh->mVertices[i].z = vertix.z;
			i++;
		}
		assimpMesh->mNumVertices = vertices.size();
	}
	else
	{
		assimpMesh->mNumVertices = 0;
	}

	//--------------------------------------------------------------------------
	// Normals
	// Make a copy of normals
	std::vector<repo::lib::RepoVector3D> normals = meshNode->getNormals();
	if (normals.size())
	{
		assimpMesh->mNormals = new aiVector3D[normals.size()];
		if (assimpMesh->mNormals)
		{
			uint32_t i = 0;
			for (const auto &normal : normals)
			{
				assimpMesh->mNormals[i].x = normal.x;
				assimpMesh->mNormals[i].y = normal.y;
				assimpMesh->mNormals[i].z = normal.z;
				i++;
			}
		}
	}

	//--------------------------------------------------------------------------
	// Texture coordinates
	//
	// TODO: change to support U and UVW, not just UV as done now.
	std::vector<std::vector<repo::lib::RepoVector2D>> uvChannels = meshNode->getUVChannelsSeparated();
	if (uvChannels.size())
	{
		//figure out the number of channels, then split the serialised uvChannel vector to
		// those channels
		for (uint32_t i = 0; i < uvChannels.size() &&
			i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i)
		{
			assimpMesh->mTextureCoords[i] = new aiVector3D[meshNode->getVertices().size()];
			uint32_t ind = 0;
			for (const auto &vec : uvChannels.at(i))
			{
				assimpMesh->mTextureCoords[i][ind].x = vec.x;
				assimpMesh->mTextureCoords[i][ind].y = vec.y;
				assimpMesh->mTextureCoords[i][ind].z = 0;
				ind++;
			}
			assimpMesh->mNumUVComponents[i] = 2; // UV
		}
	}

	std::vector<repo_color4d_t> colors = meshNode->getColors();

	//--------------------------------------------------------------------------
	// Vertex colors
	if (colors.size())
	{
		assimpMesh->mColors[0] = new aiColor4D[colors.size()];
		uint32_t i = 0;
		for (const auto &color : colors)
		{
			assimpMesh->mColors[0][i].r = color.r;
			assimpMesh->mColors[0][i].g = color.g;
			assimpMesh->mColors[0][i].b = color.b;
			assimpMesh->mColors[0][i].a = color.a;
			i++;
		}
	}

	//--------------------------------------------------------------------------
	// Material index
	//
	// In assimp, mesh would be expected to have only one child.
	// If multiple children materials are found, takes the first one
	for (const auto & child : scene->getChildrenAsNodes(gType, meshNode->getSharedID()))
	{
		if (child->getTypeAsEnum() == repo::core::model::NodeType::MATERIAL)
		{
			//check if it already exist in matMap, if yes use that, if not create a new one
			auto it = matMap.find(child->getSharedID());
			aiMaterial *aiMat = nullptr;
			if (it == matMap.end())
			{
				aiMat = convertMaterial(scene, (repo::core::model::MaterialNode *)child, textNodes);
				matMap[child->getSharedID()] = aiMat;
				matVec.push_back(aiMat);
			}

			assimpMesh->mMaterialIndex = std::distance(matVec.begin(), std::find(matVec.begin(), matVec.end(), aiMat));

			break; //only support one material
		}
	}

	return assimpMesh;
}

bool AssimpModelExport::writeSceneToFile(
	const aiScene     *scene,
	const std::string &filePath)
{
	bool success = false;
	if (scene)
	{
		boost::filesystem::path boostPath(filePath);
		//--------------------------------------------------------------------------
		// NOTE: This modifies links to textures, so after export, the
		// scene for rendering won't be valid any more!
		//--------------------------------------------------------------------------
		if (scene->HasTextures())
		{ // embedded textures found, replace pointers such as "*0" with "0.jpg"
			aiReturn texFound;
			int texIndex;
			for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
			{
				aiString path;	// filename
				texIndex = 0;
				texFound = AI_SUCCESS;
				while (texFound == AI_SUCCESS)
				{
					texFound = scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, texIndex, &path);
					std::string name(path.data);
					if (!name.empty())
					{
						name = name.substr(1, name.size()) + REPO_DEFAULT_TEXTURE_EXT; // remove leading '*' char
						scene->mMaterials[i]->RemoveProperty(AI_MATKEY_TEXTURE_DIFFUSE(texIndex));
						// TODO: watch out for mem-leak
						scene->mMaterials[i]->AddProperty(new aiString(name.c_str()), AI_MATKEY_TEXTURE_DIFFUSE(texIndex));
						texIndex++;
					}
				}
			}
		}

		Assimp::Exporter exporter;
		std::string extension = boostPath.extension().string();

		std::string exportFormat = getExportFormatID(extension.substr(1, extension.size() - 1));
		repoTrace << "Exporting repo scene using ASSIMP: export format = " << exportFormat << " to " << filePath;
		if (exportFormat.empty())
		{
			repoError << "Unrecognised export format: " << boostPath.extension().string();
		}
		else
		{
			aiReturn ret = exporter.Export(scene, exportFormat, filePath, scene->mFlags);
			switch (ret)
			{
			case aiReturn_FAILURE:
				repoError << "Export failed due to unknown reason.";
				break;
			case aiReturn_OUTOFMEMORY:
				repoError << "Export failed due to running out of memory.";
				break;
			case aiReturn_SUCCESS:
				repoTrace << "Export completed successfully";
				break;
			case _AI_ENFORCE_ENUM_SIZE:
				// Silently handle assimp's bogus enum-size-enforcing value.
				break;
			}
			success = (ret == aiReturn_SUCCESS);
		}
	}
	else{
		repoError << "Trying to write a null pointer assimp scene!";
	}

	return success;
}

bool AssimpModelExport::exportToFile(
	const std::string                  &filePath)
{
	bool success = false;
	repo::core::model::RepoNodeSet textureNodes;
	aiScene *assimpScene = convertToAssimp(scene, textureNodes);

	if (success = assimpScene)
	{
		success = writeSceneToFile(assimpScene, filePath);
		writeTexturesToFiles(textureNodes, filePath);

		//FIXME: this is breaking on my assimp.dll but works on assimpd.dll...?
		//problem seems to be related to meshes/materials
		delete assimpScene;
	}
	else
	{
		repoError << "Failed to convert scene to assimp scene(nullptr) ";
		success = false;
	}

	return success;
}

std::string AssimpModelExport::getExportFormatID(
	const std::string &fileExtension)
{
	std::string ret;
	Assimp::Exporter exporter;
	for (size_t i = 0; i < exporter.GetExportFormatCount(); ++i)
	{
		const aiExportFormatDesc* desc = exporter.GetExportFormatDescription(i);
		if (fileExtension == desc->fileExtension)
		{
			ret = desc->id;
			break;
		}
	}
	return ret;
}

std::string AssimpModelExport::getSupportedFormats()
{
	std::string all = "All (";
	std::string individual = "";

	Assimp::Exporter exporter;
	for (size_t i = 0; i < exporter.GetExportFormatCount(); ++i)
	{
		const aiExportFormatDesc* desc = exporter.GetExportFormatDescription(i);
		all += "*." + std::string(desc->fileExtension) + " ";
		individual += ";;" + std::string(desc->description) + " (*." +
			std::string(desc->fileExtension) + ")";
	}
	all += ")";
	return all + individual;
}

bool AssimpModelExport::writeTexturesToFiles(
	const repo::core::model::RepoNodeSet &nodes,
	const std::string                    &filePath)
{
	bool success = true;
	boost::filesystem::path path(filePath);
	for (const auto &repoNode : nodes)
	{
		const repo::core::model::TextureNode *repoTex = (repo::core::model::TextureNode*) repoNode;
		std::vector<char> rawData = repoTex->getRawData();

		std::string ext = repoTex->getFileExtension();

		if (ext.empty()) ext = REPO_DEFAULT_TEXTURE_EXT;

		std::string fullPath = path.parent_path().string() + repoTex->getName() + ext;
		//write texture to file
		std::ofstream myfile;
		repoTrace << "Writing texture to " << fullPath;
		myfile.open(fullPath, std::ios::binary);

		if (myfile.is_open())
		{
			myfile.write(rawData.data(), sizeof(*rawData.data()) * rawData.size());

			myfile.close();
		}
		else
		{
			repoError << "Unable to open file to write texture! File path: " << fullPath;
			success = false;
		}
	}

	return success;
}
