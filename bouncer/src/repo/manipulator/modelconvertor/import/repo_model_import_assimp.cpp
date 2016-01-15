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
* Allows Import functionality into/output Repo world using ASSIMP
*/


#include "repo_model_import_assimp.h"

#include <algorithm>
#include <fstream>
#include <regex>
#include <boost/filesystem.hpp>

#include <assimp/importerdesc.h>

#include "../../../core/model/bson/repo_bson_factory.h"
#include "../../../core/model/repo_node_utils.h"
#include "../../../lib/repo_log.h"

using namespace repo::manipulator::modelconvertor;


AssimpModelImport::AssimpModelImport()
{
	//set default ASSIMP debone threshold
	//settings->setDeboneThreshold(AI_DEBONE_THRESHOLD);
}

AssimpModelImport::AssimpModelImport(const ModelImportConfig *settings) :
AbstractModelImport(settings)
{

}

AssimpModelImport::~AssimpModelImport()
{
	if (assimpScene)
		importer.FreeScene();

	if (destroySettings)
	{
		delete settings;
		settings = nullptr;
	}
}

std::string AssimpModelImport::getSupportedFormats()
{
	Assimp::Importer importer;
	aiString ext;
	importer.GetExtensionList(ext);

	// all file extensions in convenient all package

	std::string str(ext.C_Str());

	std::replace(str.begin(), str.end(), ';', ' ');

	std::string all = "All (" + str + ")";

	// individual extensions by name
	std::string individual = "";
	for (size_t i = 0; i < importer.GetImporterCount(); ++i)
	{
		const aiImporterDesc *desc = importer.GetImporterInfo(i);
		std::string str(desc->mFileExtensions);

		//put *. in front of the 2nd+ file extensions (as some might have multiple extensions)
		std::string extension = std::regex_replace(str, std::regex(" "), " *.");

		//remove "Importer" from the name
		std::string formatName = std::regex_replace(std::string(desc->mName), std::regex(" Importer"), "");
		individual += ";;" + formatName + " (*." + extension + ")";

	}
	return all + individual;
}


uint32_t AssimpModelImport::composeAssimpPostProcessingFlags(
	uint32_t flag)
{
	if (settings->getCalculateTangentSpace())
		flag |= aiProcess_CalcTangentSpace;

	if (settings->getConvertToUVCoordinates())
		flag |= aiProcess_GenUVCoords;

	if (settings->getDegeneratesToPointsLines())
		flag |= aiProcess_FindDegenerates;

	if (settings->getDebone())
		flag |= aiProcess_Debone;

	// Debone threshold
	// Debone only if all

	if (settings->getFindInstances())
		flag |= aiProcess_FindInstances;

	if (settings->getFindInvalidData())
		flag |= aiProcess_FindInvalidData;

	// Animation accuracy

	if (settings->getFixInfacingNormals())
		flag |= aiProcess_FixInfacingNormals;

	if (settings->getFlipUVCoordinates())
		flag |= aiProcess_FlipUVs;

	if (settings->getFlipWindingOrder())
		flag |= aiProcess_FlipWindingOrder;

	if (settings->getGenerateNormals() && settings->getGenerateNormalsFlat())
		flag |= aiProcess_GenNormals;

	if (settings->getGenerateNormals() && settings->getGenerateNormalsSmooth())
		flag |= aiProcess_GenSmoothNormals;

	// Crease angle

	if (settings->getImproveCacheLocality())
		flag |= aiProcess_ImproveCacheLocality;

	// Vertex cache size

	if (settings->getJoinIdenticalVertices())
		flag |= aiProcess_JoinIdenticalVertices;

	if (settings->getLimitBoneWeights())
		flag |= aiProcess_LimitBoneWeights;

	// Max bone weights

	if (settings->getMakeLeftHanded())
		flag |= aiProcess_MakeLeftHanded;

	if (settings->getOptimizeMeshes())
		flag |= aiProcess_OptimizeMeshes;

	if (settings->getPreTransformUVCoordinates())
		flag |= aiProcess_TransformUVCoords;

	if (settings->getPreTransformVertices())
		flag |= aiProcess_PreTransformVertices;

	// Normalize

	if (settings->getRemoveComponents())
		flag |= aiProcess_RemoveComponent;

	// !individual components!

	if (settings->getRemoveRedundantMaterials())
		flag |= aiProcess_RemoveRedundantMaterials;

	// Skip materials

	if (settings->getRemoveRedundantNodes())
		flag |= aiProcess_OptimizeGraph;

	// Skip nodes

	if (settings->getSortAndRemove())
		flag |= aiProcess_SortByPType;

	// Remove types

	if (settings->getSplitByBoneCount())
		flag |= aiProcess_SplitByBoneCount;

	// Max bones

	if (settings->getSplitLargeMeshes())
		flag |= aiProcess_SplitLargeMeshes;

	// Vertex limit
	// Triangle limit

	if (settings->getTriangulate())
		flag |= aiProcess_Triangulate;

	if (settings->getValidateDataStructures())
		flag |= aiProcess_ValidateDataStructure;

	return flag;
}

repo::core::model::CameraNode* AssimpModelImport::createCameraRepoNode(
	const aiCamera *assimpCamera)
{
	std::string cameraName(assimpCamera->mName.data);

	repo::core::model::CameraNode * cameraNode;

	if (assimpCamera)
	{
		cameraNode = new repo::core::model::CameraNode( repo::core::model::RepoBSONFactory::makeCameraNode(
			assimpCamera->mAspect,
			assimpCamera->mClipPlaneFar,
			assimpCamera->mClipPlaneNear,
			assimpCamera->mHorizontalFOV,
			{ assimpCamera->mLookAt.x, assimpCamera->mLookAt.y, assimpCamera->mLookAt.z },
			{ assimpCamera->mPosition.x, assimpCamera->mPosition.y, assimpCamera->mPosition.z },
			{ assimpCamera->mUp.x, assimpCamera->mUp.y, assimpCamera->mUp.z },
			cameraName
			));
	}

	return cameraNode;

}
repo::core::model::MaterialNode* AssimpModelImport::createMaterialRepoNode(
	const aiMaterial *material,
	const std::string &name,
	const std::unordered_map<std::string, repo::core::model::RepoNode *> &nameToTexture)
{
	repo::core::model::MaterialNode *materialNode;

	if (material){
		repo_material_t repo_material;

		aiColor3D tempColor;
		float tempFloat;

		//--------------------------------------------------------------------------
		// Ambient
		if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_AMBIENT, tempColor))
		{
			repo_material.ambient.push_back(tempColor.r);
			repo_material.ambient.push_back(tempColor.g);
			repo_material.ambient.push_back(tempColor.b);

		}
		//--------------------------------------------------------------------------
		// Diffuse
		if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, tempColor))
		{
			repo_material.diffuse.push_back(tempColor.r);
			repo_material.diffuse.push_back(tempColor.g);
			repo_material.diffuse.push_back(tempColor.b);

		}


		//--------------------------------------------------------------------------
		// Specular
		if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_SPECULAR, tempColor))
		{
			repo_material.specular.push_back(tempColor.r);
			repo_material.specular.push_back(tempColor.g);
			repo_material.specular.push_back(tempColor.b);

		}

		//--------------------------------------------------------------------------
		// Emissive
		if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_EMISSIVE, tempColor))
		{
			repo_material.emissive.push_back(tempColor.r);
			repo_material.emissive.push_back(tempColor.g);
			repo_material.emissive.push_back(tempColor.b);
		}

		//--------------------------------------------------------------------------
		// Wireframe
		int wireframe;
		material->Get(AI_MATKEY_ENABLE_WIREFRAME, wireframe);
		repo_material.isWireframe = (0 != wireframe);

		//--------------------------------------------------------------------------
		// Two-sided
		int twoSided;
		material->Get(AI_MATKEY_TWOSIDED, twoSided);
		repo_material.isTwoSided = (0 != twoSided);

		//--------------------------------------------------------------------------
		// Opacity
		if (AI_SUCCESS == material->Get(AI_MATKEY_OPACITY, tempFloat))
			repo_material.opacity = tempFloat;
		else
			repo_material.opacity = std::numeric_limits<float>::quiet_NaN();
		//--------------------------------------------------------------------------
		// Shininess
		if (AI_SUCCESS == material->Get(AI_MATKEY_SHININESS, tempFloat))
			repo_material.shininess = tempFloat;
		else
			repo_material.shininess = std::numeric_limits<float>::quiet_NaN();
		//--------------------------------------------------------------------------
		// Shininess strength
		if (AI_SUCCESS == material->Get(AI_MATKEY_SHININESS_STRENGTH, tempFloat))
			repo_material.shininessStrength = tempFloat > 0 ? tempFloat : 1;
		else
			repo_material.shininessStrength = std::numeric_limits<float>::quiet_NaN();


		materialNode = new repo::core::model::MaterialNode(
			repo::core::model::RepoBSONFactory::makeMaterialNode(repo_material, name, REPO_NODE_API_LEVEL_1));


		//--------------------------------------------------------------------------
		// Texture (one diffuse for the moment)
		// Textures are uniquely referenced by their name
		aiString texPath; // contains a filename of a texture
		/*repoTrace << " #None Texture:" << material->GetTextureCount(aiTextureType_NONE);
		repoTrace << " #Diffuse Texture:" << material->GetTextureCount(aiTextureType_DIFFUSE);
		repoTrace << " #Spec Texture:" << material->GetTextureCount(aiTextureType_SPECULAR);
		repoTrace << " #Ambient Texture:" << material->GetTextureCount(aiTextureType_AMBIENT);
		repoTrace << " #Emissive Texture:" << material->GetTextureCount(aiTextureType_EMISSIVE);
		repoTrace << " #Height Texture:" << material->GetTextureCount(aiTextureType_HEIGHT);
		repoTrace << " #Normal Texture:" << material->GetTextureCount(aiTextureType_NORMALS);
		repoTrace << " #Shiniess Texture:" << material->GetTextureCount(aiTextureType_SHININESS);
		repoTrace << " #Opac Texture:" << material->GetTextureCount(aiTextureType_OPACITY);
		repoTrace << " #Disp Texture:" << material->GetTextureCount(aiTextureType_DISPLACEMENT);
		repoTrace << " #LightM Texture:" << material->GetTextureCount(aiTextureType_LIGHTMAP);
		repoTrace << " #Reflection Texture:" << material->GetTextureCount(aiTextureType_REFLECTION);
		repoTrace << " #Unknown Texture:" << material->GetTextureCount(aiTextureType_UNKNOWN);
*/


		if (AI_SUCCESS == material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath))
		{
			std::unordered_map<std::string, repo::core::model::RepoNode *>::const_iterator it = nameToTexture.find(texPath.data);

			if (nameToTexture.end() != it)
			{
				repo::core::model::RepoNode * nodeAddr = it->second;

				repo::core::model::RepoNode tmp = nodeAddr->cloneAndAddParent(materialNode->getSharedID());
				nodeAddr->swap(tmp);
			}
			else
			{
				repoError << "Could not find related texture mapping";
			}
		}
	}

	return materialNode;
}

repo::core::model::MeshNode* AssimpModelImport::createMeshRepoNode(
	const aiMesh *assimpMesh,
	const std::vector<repo::core::model::RepoNode *> &materials,
	std::unordered_map < repo::core::model::RepoNode*, std::vector<repoUUID>> &matMap)
{

	repo::core::model::MeshNode *meshNode = 0;

	//Avoid using assimp objects everywhere -> converting assimp objects into repo structs
	std::vector<repo_vector_t> vertices;
	std::vector<repo_face_t> faces;
	std::vector<repo_vector_t> normals;
	std::vector<std::vector<repo_vector2d_t>> uvChannels;
	std::vector<repo_color4d_t> colors;
	std::vector<std::vector<float>>   outline;

	repo_vector_t minVertex = { assimpMesh->mVertices[0].x, assimpMesh->mVertices[0].y, assimpMesh->mVertices[0].z };
	repo_vector_t maxVertex = { assimpMesh->mVertices[0].x, assimpMesh->mVertices[0].y, assimpMesh->mVertices[0].z };

	/*
	 *--------------------- Vertices (always present) -----------------------------
	*/
	for (uint32_t i = 0; i < assimpMesh->mNumVertices; i++)
	{
		vertices.push_back({ assimpMesh->mVertices[i].x, assimpMesh->mVertices[i].y, assimpMesh->mVertices[i].z });

		minVertex.x = minVertex.x < assimpMesh->mVertices[i].x ? minVertex.x : assimpMesh->mVertices[i].x;
		minVertex.y = minVertex.y < assimpMesh->mVertices[i].y ? minVertex.y : assimpMesh->mVertices[i].y;
		minVertex.z = minVertex.z < assimpMesh->mVertices[i].z ? minVertex.z : assimpMesh->mVertices[i].z;

		maxVertex.x = maxVertex.x > assimpMesh->mVertices[i].x ? maxVertex.x : assimpMesh->mVertices[i].x;
		maxVertex.y = maxVertex.y > assimpMesh->mVertices[i].y ? maxVertex.y : assimpMesh->mVertices[i].y;
		maxVertex.z = maxVertex.z > assimpMesh->mVertices[i].z ? maxVertex.z : assimpMesh->mVertices[i].z;

	}

	/*
	*-----------------------------------------------------------------------------
	*/

	/*
	*------------------------------- Faces ----------------------------------------
	*/
	if (assimpMesh->HasFaces())
	{
		for (uint32_t i = 0; i < assimpMesh->mNumFaces; i++)
		{

			faces.push_back({ std::vector<uint32_t>(assimpMesh->mFaces[i].mIndices,
				assimpMesh->mFaces[i].mIndices + assimpMesh->mFaces[i].mNumIndices) });
		}
	}
	/*
	*-----------------------------------------------------------------------------
	*/

	/*
	*------------------------------- Normals --------------------------------------
	*/
	if (assimpMesh->HasNormals())
	{
		for (uint32_t i = 0; i < assimpMesh->mNumVertices; i++)
		{
			normals.push_back({ assimpMesh->mNormals[i].x, assimpMesh->mNormals[i].y, assimpMesh->mNormals[i].z });
		}
	}
	/*
	*-----------------------------------------------------------------------------
	*/

	/*
	*--------------------------------- Bones ---------------------------------------
	*/
	/*
	TODO:
	if (mesh->HasBones())
	{
	}


	if (mesh->HasPositions())
	{

	}

	if (mesh->HasTangentsAndBitangents())
	{

	}*/
	/*
	*-----------------------------------------------------------------------------
	*/

	/*
	*------------------------------ UV Channels ----------------------------------
	*/
	// Copies only the very first UV channel over
	// TODO: make sure enough memory can be allocated
	// TODO: add support for all UV channels.
	if (assimpMesh->HasTextureCoords(0))
	{
		std::vector<repo_vector2d_t> channelVector;
		for (uint32_t i = 0; i < assimpMesh->mNumVertices; i++)
		{
			channelVector.push_back({ assimpMesh->mTextureCoords[0][i].x, assimpMesh->mTextureCoords[0][i].y });
		}
		uvChannels.push_back(channelVector);

	}

	// Consider only first color set
	if (assimpMesh->HasVertexColors(0))
	{
		for (uint32_t i = 0; i < assimpMesh->mNumVertices; i++)
		{
			colors.push_back({
				assimpMesh->mColors[0][i].r,
				assimpMesh->mColors[0][i].g,
				assimpMesh->mColors[0][i].b,
				assimpMesh->mColors[0][i].a });
		}
	}
	/*
	*-----------------------------------------------------------------------------
	*/

	/*
	*----------------------------- SHA-256 hash ----------------------------------
	*/
	//FIXME:

	//vertexHash = hash(*vertices, boundingBox);

	/*
	*-----------------------------------------------------------------------------
	*/

	std::vector<std::vector<float>>  boundingBox;
	std::vector<float> minBound, maxBound;
	minBound.push_back(minVertex.x);
	minBound.push_back(minVertex.y);
	minBound.push_back(minVertex.z);

	maxBound.push_back(maxVertex.x);
	maxBound.push_back(maxVertex.y);
	maxBound.push_back(maxVertex.z);


	boundingBox.push_back(minBound);
	boundingBox.push_back(maxBound);

	outline.push_back({ minVertex.x, minVertex.y });
	outline.push_back({ maxVertex.x, minVertex.y });
	outline.push_back({ maxVertex.x, maxVertex.y });
	outline.push_back({ minVertex.x, maxVertex.y });

	meshNode = new repo::core::model::MeshNode(repo::core::model::RepoBSONFactory::makeMeshNode(
		vertices, faces, normals, boundingBox, uvChannels, colors, outline));

	///*
	//*------------------------------ setParents ----------------------------------
	//*/

	if (assimpMesh->mMaterialIndex < materials.size())
	{
		repo::core::model::RepoNode *materialNode = materials[assimpMesh->mMaterialIndex];
		matMap[materialNode].push_back(meshNode->getSharedID());
	}

	///*
	//*-----------------------------------------------------------------------------
	//*/

	return meshNode;
}

repo::core::model::MetadataNode* AssimpModelImport::createMetadataRepoNode(
	const aiMetadata             *assimpMeta,
	const std::string            &metadataName,
	const std::vector<repoUUID> &parents)
{
	repo::core::model::MetadataNode *metaNode;
	std::string val;
	if (assimpMeta)
	{
		//build the metadata as a bson
		repo::core::model::RepoBSONBuilder builder;

		for (uint32_t i = 0; i < assimpMeta->mNumProperties; i++)
		{
			std::string key(assimpMeta->mKeys[i].C_Str());
			aiMetadataEntry &currentValue = assimpMeta->mValues[i];

			if (key == "IfcGloballyUniqueId")
				repoError << "TODO: fix IfcGloballyUniqueId in RepoMetadata" << std::endl;


			switch (currentValue.mType)
			{
			case AI_BOOL:
				builder << key << *(static_cast<bool *>(currentValue.mData));
				break;

			case AI_INT:
				builder << key << *(static_cast<int *>(currentValue.mData));
				break;

			case AI_UINT64:
				//mongo doesn't support 64bit unsigned. storing it as a signed number
				builder << key << (long long)(*(static_cast<uint64_t *>(currentValue.mData)));
				break;

			case AI_FLOAT:
				builder << key << *(static_cast<float *>(currentValue.mData));
				break;

			case AI_AISTRING:
				val = (static_cast<aiString *>(currentValue.mData))->C_Str();

				if (val.compare(key))
					builder << key << val;

				break;
			case AI_AIVECTOR3D:
				{
					aiVector3D *vector = (static_cast<aiVector3D *>(currentValue.mData));

					repo_vector_t repoVector = { vector->x, vector->y, vector->z };


					builder.append(key, repoVector);
				}
				break;
			case FORCE_32BIT:
				// Gracefully (but silently) handle the bogus enum used by
				// assimp to ensure the enum is 32-bits.
				break;
			}
		}

		repo::core::model::RepoBSON metaBSON = builder.obj();

		metaNode = new repo::core::model::MetadataNode(
			repo::core::model::RepoBSONFactory::makeMetaDataNode(metaBSON, "", metadataName, parents));
	}//if(assimpMeta)

	return metaNode;

}

repo::core::model::RepoNodeSet AssimpModelImport::createTransformationNodesRecursive(
const aiNode                                                     *assimpNode,
const std::unordered_map<std::string, repo::core::model::RepoNode *> &cameras,
const std::vector<repo::core::model::RepoNode *>           &meshes,
repo::core::model::RepoNodeSet						     &metadata,
assimp_map													&map,
uint32_t                                               &count,
const std::vector<repoUUID>						             &parent

	)
{
	repo::core::model::RepoNodeSet transNodes;

	if (assimpNode){
		std::string transName(assimpNode->mName.data);
		if(count % 1000 == 0)
			repoInfo << "Constructing transName: " << transName << " (" << count << " of ???)";

		//create a 4 by 4 vector
		std::vector < std::vector<float> > transMat;

		for (int i = 0; i < 4; i++){
			std::vector<float> rows;
			for (int j = 0; j < 4; j++){
				rows.push_back(assimpNode->mTransformation[i][j]);
			}
			transMat.push_back(rows);
		}


		repo::core::model::TransformationNode * transNode =
			new repo::core::model::TransformationNode(
			repo::core::model::RepoBSONFactory::makeTransformationNode(transMat, transName, parent));

		map.insert(assimp_map::value_type(reinterpret_cast<uintptr_t>(assimpNode), transNode));


		repoUUID sharedId = transNode->getSharedID();
		std::vector<repoUUID> myShareID;
		myShareID.push_back(sharedId);

		//--------------------------------------------------------------------------
		// Register meshes as children of this transformation if any
		for (unsigned int i = 0; i < assimpNode->mNumMeshes; ++i)
		{
			unsigned int meshIndex = assimpNode->mMeshes[i];
			if (meshIndex < meshes.size())
			{
				repo::core::model::RepoNode *mesh = meshes[meshIndex];

				if (mesh)
				{
					repo::core::model::RepoNode tmp = mesh->cloneAndAddParent(sharedId);
					mesh->swap(tmp);
				}
			}
		}

		//--------------------------------------------------------------------------
		// Register cameras as children of this transformation (by name) if any
		std::unordered_map<std::string, repo::core::model::RepoNode *>::const_iterator it =
			cameras.find(assimpNode->mName.data);
		if (cameras.end() != it)
		{
			repo::core::model::RepoNode * camera = it->second;
			if (camera)
			{
				repo::core::model::RepoNode tmp = camera->cloneAndAddParent(sharedId);
				camera->swap(tmp);
			}
		}

		//--------------------------------------------------------------------------
		// Collect metadata and add as a child
		if (assimpNode->mMetaData)
		{
			std::string metadataName = assimpNode->mName.data;
			if (metadataName == "<transformation>")
				metadataName = "<metadata>";

			repo::core::model::MetadataNode *metachild = createMetadataRepoNode(assimpNode->mMetaData, metadataName, myShareID);
			metadata.insert(metachild);
		}

		transNodes.insert(transNode);

	//--------------------------------------------------------------------------
	// Register child transformations as children if any
		for (unsigned int i = 0; i < assimpNode->mNumChildren; ++i)
		{

			repo::core::model::RepoNodeSet childMetadata;
			repo::core::model::RepoNodeSet childSet =  createTransformationNodesRecursive(assimpNode->mChildren[i],
				cameras, meshes, childMetadata, map, ++count, myShareID);

			transNodes.insert(childSet.begin(), childSet.end());
			metadata.insert(childMetadata.begin(), childMetadata.end());
		}
	} //if assimpNode

	return transNodes;
}

repo::core::model::RepoScene* AssimpModelImport::convertAiSceneToRepoScene(
	assimp_map                    &map,
	repo::core::model::RepoScene  *scene)
{
	repo::core::model::RepoScene *scenePtr = scene;

	if (assimpScene)
	{
		//Turn everything into repoNodes and construct a scene
		repo::core::model::RepoNodeSet cameras; //!< Cameras
		repo::core::model::RepoNodeSet meshes; //!< Meshes
		repo::core::model::RepoNodeSet materials; //!< Materials
		repo::core::model::RepoNodeSet metadata; //!< Metadata
		repo::core::model::RepoNodeSet transformations; //!< Transformations
		repo::core::model::RepoNodeSet textures;


		std::vector<repo::core::model::RepoNode *> originalOrderMaterial; //vector that keeps track original order for assimp indices
		std::unordered_map<repo::core::model::RepoNode *, std::vector<repoUUID>> matParents;//Tracks material parents
		std::vector<repo::core::model::RepoNode *> originalOrderMesh; //vector that keeps track original order for assimp indices
		std::unordered_map<std::string, repo::core::model::RepoNode *> camerasMap;
		std::unordered_map<std::string, repo::core::model::RepoNode *> nameToTexture;

		//-------------------------------------------------------------------------
		// Textures
		repoInfo << "Constructing Texture Nodes...";
		for (uint32_t m = 0; m < assimpScene->mNumMaterials; ++m)
		{
			int texIndex = 0;
			aiReturn texFound = AI_SUCCESS;
			const aiMaterial *material = assimpScene->mMaterials[m];


			uint32_t nTex = material->GetTextureCount(aiTextureType_DIFFUSE);
			for (uint32_t iTex = 0; iTex < nTex; ++iTex)
			{
				aiString path;	// filename
				if (AI_SUCCESS == material->GetTexture(aiTextureType_DIFFUSE, iTex, &path))
				{
					std::string texName(path.data);
					if (!texName.empty())
					{
						repo::core::model::RepoNode *textureNode = nullptr;
						if (assimpScene->HasTextures() && '*' == texName.at(0))
						{
							repoTrace << "Embedded texture name: " << texName;
							//---------------------------------------------------------
							// Embedded texture
							int textureIndex = atoi(texName.substr(1, texName.size()).c_str());
							aiTexture *texture = assimpScene->mTextures[textureIndex];



							//FIXME: Untested!
							textureNode = new repo::core::model::TextureNode(repo::core::model::RepoBSONFactory::makeTextureNode(
								texName,
								(char*)texture->pcData,
								sizeof(texture->pcData) * texture->mWidth * texture->mHeight ? texture->mHeight : 1,
								texture->mWidth,
								texture->mHeight,
								REPO_NODE_API_LEVEL_1));

						}
						else
						{
							repoTrace << "External texture name: " << texName;
							//External texture
							std::ifstream::pos_type size;
							std::string dirPath = getDirPath(orgFile);
							char *memblock;
							boost::filesystem::path filePath = boost::filesystem::path(dirPath) / boost::filesystem::path(texName);
							std::ifstream file(filePath.string(), std::ios::in | std::ios::binary | std::ios::ate);
							if (!file.is_open())
							{
								repoError << "Could not open texture: " << dirPath << texName << std::endl;
							}
							else
							{
								std::ifstream::pos_type size = file.tellg();
								char *memblock = new char[size];
								file.seekg(0, std::ios::beg);
								file.read(memblock, size);
								file.close();

								textureNode = new repo::core::model::TextureNode(repo::core::model::RepoBSONFactory::makeTextureNode(
									texName,
									memblock,
									size,
									size,
									0,
									REPO_NODE_API_LEVEL_1));


								delete[] memblock;
							}


						}

						if (textureNode)
						{
							textures.insert(textureNode);
							nameToTexture[texName] = textureNode;
							repoTrace << "Added texture :" << texName;
						}
					}
					else
					{
						repoWarning << "Texture name is empty!";

					}

				}
				else
				{
					repoWarning << "Unable to get texture from material.";
				}
			}

		}


		repoInfo << "Constructing Material Nodes...";
		/*
		* ------------- Material Nodes -----------------
		*/
		// Warning: Default material might not be attached to anything,
		// hence it would not be returned by a call to getNodes().
		if (assimpScene->HasMaterials())
		{
			for (uint32_t i = 0; i < assimpScene->mNumMaterials; ++i)
			{
				if (i % 100 == 0 || i == assimpScene->mNumMaterials - 1)
				{
					repoInfo << "Constructing " << i << " of " << assimpScene->mNumMaterials;
				}
				aiString name;
				assimpScene->mMaterials[i]->Get(AI_MATKEY_NAME, name);

				repo::core::model::RepoNode* material = createMaterialRepoNode(
					assimpScene->mMaterials[i],
					name.data, nameToTexture);

				if (!material)
					repoError << "Unable to construct material node in Assimp Model Convertor!";
				else
				{
					map.insert(assimp_map::value_type(reinterpret_cast<uintptr_t>(assimpScene->mMaterials[i]), material));
					materials.insert(material);
				}


				originalOrderMaterial.push_back(material);
				std::pair<repo::core::model::RepoNode *, std::vector<repoUUID>> a;
				a.first = material;
				a.second = std::vector<repoUUID>();
				matParents.insert(a);
			}
		}

		/*
		* ---------------------------------------------
		*/

		repoInfo << "Constructing Mesh Nodes...";
		/*
		* --------------- Mesh Nodes ------------------
		*/
		if (assimpScene->HasMeshes())
		{
			for (unsigned int i = 0; i < assimpScene->mNumMeshes; ++i)
			{
				if (i % 500 == 0 || i == assimpScene->mNumMeshes-1)
				{
					repoInfo << "Constructing " << i << " of " << assimpScene->mNumMeshes;
				}
				repo::core::model::RepoNode* mesh = createMeshRepoNode(
					assimpScene->mMeshes[i],
					originalOrderMaterial,
					matParents);

				if (!mesh)
					repoError << "Unable to construct mesh node in Assimp Model Convertor!";
				else
				{
					map.insert(assimp_map::value_type(reinterpret_cast<uintptr_t>(assimpScene->mMeshes[i]), mesh));
					meshes.insert(mesh);
				}
				originalOrderMesh.push_back(mesh);
			}

			//Update material parents
			for (const auto &matPair : matParents)
			{
				if (matPair.second.size() > 0)
				{
					repo::core::model::RepoNode updatedMat = matPair.first->cloneAndAddParent(matPair.second);
					matPair.first->swap(updatedMat);
				}
			}
		}

		/*
		* ---------------------------------------------
		*/

		repoInfo << "Constructing Camera Nodes...";
		/*
		* ------------- Camera Nodes ------------------
		*/

		if (assimpScene->HasCameras())
		{
			for (unsigned int i = 0; i < assimpScene->mNumCameras; ++i)
			{

				if (i % 100 == 0 || i == assimpScene->mNumCameras - 1)
				{
					repoInfo << "Constructing " << i << " of " << assimpScene->mNumCameras;
				}
				std::string cameraName(assimpScene->mCameras[i]->mName.data);
				repo::core::model::RepoNode* camera = createCameraRepoNode(assimpScene->mCameras[i]);
				if (!camera)
					repoError << "Unable to construct mesh node in Assimp Model Convertor!";
				else
				{
					map.insert(assimp_map::value_type(reinterpret_cast<uintptr_t>(assimpScene->mCameras[i]), camera));
					cameras.insert(camera);
				}

				camerasMap.insert(std::make_pair(cameraName, camera));
			}
		}
		/*
		* ---------------------------------------------
		*/
		//--------------------------------------------------------------------------
		// TODO: Animations
		//if (assimpScene->HasAnimations())
		//{

		//}

		//--------------------------------------------------------------------------
		// TODO: Lights
		//if (assimpScene->HasLights())
		//{

		//}

		// TODO: Bones

		repoInfo << "Constructing Transformation Nodes...";
		/*
		* ----------- Transformation Nodes ------------
		*/
		// Recursively converts aiNode and all of its children to a hierarchy
		// of RepoNodeTransformations. Call with root node of aiScene.
		// RootNode will be the first entry in transformations vector.

		uint32_t count = 0;
		transformations = createTransformationNodesRecursive(assimpScene->mRootNode, camerasMap, originalOrderMesh, metadata, map, count);

		repoInfo << "Node Construction completed. (# of transformation nodes created: ) " << transformations.size();

		/*
		* ---------------------------------------------
		*/

		//Construct the scene graph
		if (scenePtr)
		{
			scenePtr->addStashGraph(cameras, meshes, materials, textures, transformations);
		}
		else
		{
			std::vector<std::string> fileVect;
			if (!orgFile.empty())
				fileVect.push_back(orgFile);
			scenePtr = new repo::core::model::RepoScene(fileVect, cameras, meshes, materials, metadata, textures, transformations);
		}
	}
	else
	{
		repoError << "Failed to load scene from file (aiScene is null)";
	}//if(assimpScene)


	return scenePtr;
}

repo::core::model::RepoScene * AssimpModelImport::generateRepoScene()
{
	repo::core::model::RepoScene *scene;
	assimp_map orgMap, optMap;

	//This will generate the non optimised scene
	repoTrace << "Converting AiScene to repoScene";
	scene = convertAiSceneToRepoScene(orgMap);

	if (scene)
	{
		// Assign the unoptimized node map, and start optimization
		importer.ApplyPostProcessing(composeAssimpPostProcessingFlags());

		//This will generate the optimised scene graph and put it in the RepoScene referenced
		repoTrace << "Converting AiScene to Optimised RepoScene";
		convertAiSceneToRepoScene(optMap, scene);

		repo::core::model::RepoNode *stashRoot =  scene->getRoot(repo::core::model::RepoScene::GraphType::OPTIMIZED);

		if (!populateOptimMaps(stashRoot, scene, orgMap, optMap))
		{
			//populateOptimMaps is false so stash is invalid. clear it out.
			repoError << "Stash invalid ... clearing.";
			scene->clearStash();
		}

	}
	else
	{
		repoError << "Failed to construct default graph scene.";
	}


	return scene;
}

bool AssimpModelImport::importModel(std::string filePath, std::string &errMsg)
{
	bool success = true;
	orgFile = filePath;
	setAssimpProperties();


	//-------------------------------------------------------------------------
	// Import model

	std::string fileName = getFileName(filePath);

	repoInfo << "IMPORT [" << fileName << "]";

	//check if a file exist first
	std::ifstream fs(filePath);
	if (!fs.good())
	{
		errMsg += "File doesn't exist ("+ filePath +")";

		return false;
	}

	importer.SetPropertyInteger(AI_CONFIG_GLOB_MEASURE_TIME, 1);
	assimpScene = importer.ReadFile(filePath, 0);


	if (!assimpScene){
		errMsg = " Failed to convert file to aiScene : "  + std::string(aiGetErrorString());
		success = false;
	}
	else
	{
		//-------------------------------------------------------------------------
		// Polygon count
		uint64_t polyCount = 0;
		for (unsigned int i = 0; i < assimpScene->mNumMeshes; ++i)
			polyCount += assimpScene->mMeshes[i]->mNumFaces;

		repoInfo << "=== IMPORTING MODEL WITH ASSIMP MODEL CONVERTOR ===";
		repoInfo << "Loaded " << fileName << " with " << polyCount << " polygons in " << assimpScene->mNumMeshes << " " << ((assimpScene->mNumMeshes == 1) ? "mesh" : "meshes");


	}

	return success;
}


bool AssimpModelImport::populateOptimMaps(
	repo::core::model::RepoNode		   *current,
	repo::core::model::RepoScene       *scene,
	const assimp_map                   &orgMap,
	const assimp_map                   &optMap)
{
	std::unordered_map<repoUUID, std::vector<repo_mesh_mapping_t>, RepoUUIDHasher> meshMapping;
	// If this is a mesh then we the optimization map is transferred for
	// it's parent.
	if (current->getTypeAsEnum() ==  repo::core::model::NodeType::TRANSFORMATION)
	{
		// First find the assimp node that relates to the abstract node
		assimp_map::right_const_iterator ait = optMap.right.find(current);

		if (ait == optMap.right.end()) // Orphan part of the graph
		{
			repoError << "Unable to find a transformation node within the assimp mappings. Optimised graph is invalid.";
			return false;
		}

		aiNode *node = reinterpret_cast<aiNode *>(ait->second); // All nodes in the optimized graph should exist

		if (node && node->mOptimMap)
		{
			aiOptimMap *ai_map = node->mOptimMap;

			std::vector<repoUUID> mergeMap;
			if (ai_map->getMergeMap().size() > 0)
				mergeMap.reserve(ai_map->getMergeMap().size());

			for (uintptr_t mergedNode : ai_map->getMergeMap())
			{
				// Find the corresponding abstract node for an assimp
				assimp_map::left_const_iterator nit = orgMap.left.find(mergedNode);

				if (nit != orgMap.left.end())
				{
					repo::core::model::RepoNode *node = nit->second;
					repoUUID mergedUUID = node->getUniqueID();

					mergeMap.push_back(mergedUUID);
				}
				else
				{
					repoDebug << "Could not find the RepoNode corresponding to the assimp node!";
				}
			}

			if (mergeMap.size() != ai_map->getMergeMap().size())
			{
				repoError << "Error mapping stash graph: mergeMap.size() != ai_map->getMergeMap().size()";
				return false;
			}

			if (mergeMap.size() > 0)
			{
				repo::core::model::RepoNode newCurrent = current->cloneAndAddMergedNodes(mergeMap);
				//swap contents of newCurrent onto current (it was cloned since RepoBSONs are immutable)
				current->swap(newCurrent);
			}


			// Now populate the vertex maps
			for (const auto &map : ai_map->getMeshMaps())
			{
				// First find the mesh that all the meshes were merged into
				assimp_map::left_const_iterator mergedMeshIT = optMap.left.find(map.first);

				if (mergedMeshIT != optMap.left.end())
				{
					boost::uuids::uuid parentUUID = mergedMeshIT->second->getUniqueID();

					for (const aiMap& mMap : map.second)
					{
						// Search for the Unique ID for this child

						assimp_map::left_const_iterator childIT = orgMap.left.find(mMap.childMesh);

						if (childIT != orgMap.left.end())
						{
							repoUUID childUUID = childIT->second->getUniqueID();

							// Now find the UUID of the material
							assimp_map::left_const_iterator materialIT = optMap.left.find(mMap.material);

							if (materialIT != orgMap.left.end())
							{
								repoUUID materialUUID = materialIT->second->getUniqueID();

								repo_vector_t min = { mMap.min.x, mMap.min.y, mMap.min.z };
								repo_vector_t max = { mMap.max.x, mMap.max.y, mMap.max.z };


								//check if there's already a mapping for this node
								if (meshMapping.find(parentUUID) == meshMapping.end())
								{
									meshMapping[parentUUID] = std::vector<repo_mesh_mapping_t>();
								}

								//create a repo_mesh_mapping struct and push it to the vector
								meshMapping[parentUUID].push_back({ min, max, childUUID, materialUUID, mMap.startVertexIDX,
									mMap.endVertexIDX, mMap.startTriangleIDX, mMap.endTriangleIDX });

							}
							else {
								repoError << "populateOptimMaps: Unable to find material node from orgMap!";
								return false;
							}
						}
						else {
							repoError << "populateOptimMaps: Unable to identify child node from aiMap!";
							return false;
						}
					}
				}
				else {
					repoError << "populateOptimMaps: Unable to find mesh from orgMap!";
					return false;
				}
			}
		}
		else {
			if (!node)
			{
				//it's only an error if node is null, optimMap may or may not exist
				repoError << "populateOptimMaps: node is null pointer!";
				return false;
			}
		}
	}

	const std::vector<repo::core::model::RepoNode *> &children = scene->getChildrenAsNodes(
		repo::core::model::RepoScene::GraphType::OPTIMIZED,
		current->getSharedID());

	//--------------------------------------------------------------------------
	// Register child transformations as children if any
	for (repo::core::model::RepoNode *child : children)
	{
		if (!populateOptimMaps(child, scene, orgMap, optMap)) return false;

		// If the child is a mesh then we may need to
		// transfer the optimization map to it.
		if (child->getTypeAsEnum() == repo::core::model::NodeType::MESH)
		{
			if (optMap.right.find(child) == optMap.right.end())
			{
				//This should never ever happen, unless the graph was tampered with between import and
				//this call without updating the mappings.
				repoError << "populateOptimMaps: Cannot find Mesh node in optimized graph in optMap!";
				return false;
			}
			else {
				repo::core::model::MeshNode *meshChild = (repo::core::model::MeshNode *) child;

				auto meshMappingIt = meshMapping.find(child->getUniqueID());
                                if (meshMappingIt != meshMapping.end())
                                {
                                    repo::core::model::MeshNode updatedChild = meshChild->cloneAndUpdateMeshMapping(meshMappingIt->second);
                                    meshChild->swap(updatedChild);


                                    // We also need to transfer the materials so that they are children of the mesh

                                    for (const auto &rMap : meshChild->getMeshMapping())
                                    {
                                            repo::core::model::RepoNode *matNode =
                                                    scene->getNodeByUniqueID(repo::core::model::RepoScene::GraphType::OPTIMIZED, rMap.material_id);
                                            if (matNode)
                                            {
                                                    scene->addInheritance(repo::core::model::RepoScene::GraphType::OPTIMIZED,
                                                            child->getUniqueID(), rMap.material_id);
                                            }
                                    }
                                }
			}
		}
	}

	return true;
}

void AssimpModelImport::setAssimpProperties(){

	if (settings->getCalculateTangentSpace())
		importer.SetPropertyFloat(AI_CONFIG_PP_CT_MAX_SMOOTHING_ANGLE, settings->getCalculateTangentSpaceMaxSmoothingAngle());

	if (settings->getDebone())
	{
		importer.SetPropertyFloat(AI_CONFIG_PP_DB_THRESHOLD, settings->getDeboneThreshold());
		importer.SetPropertyBool(AI_CONFIG_PP_DB_ALL_OR_NONE, settings->getDeboneOnlyIfAll());
	}

	if (settings->getFindInvalidData())
		importer.SetPropertyFloat(AI_CONFIG_PP_FID_ANIM_ACCURACY, settings->getFindInvalidDataAnimationAccuracy());

	if (settings->getGenerateNormals() && settings->getGenerateNormalsSmooth())
		importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, settings->getGenerateNormalsSmoothCreaseAngle());

	if (settings->getImproveCacheLocality())
		importer.SetPropertyInteger(AI_CONFIG_PP_ICL_PTCACHE_SIZE, settings->getImproveCacheLocalityVertexCacheSize());

	if (settings->getLimitBoneWeights())
		importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, settings->getLimitBoneWeightsMaxWeight());

	if (settings->getPreTransformVertices())
		importer.SetPropertyBool(AI_CONFIG_PP_PTV_NORMALIZE, settings->getPreTransformVerticesNormalize());

	if (settings->getRemoveComponents()){
		int32_t removeComponents = 0;
		removeComponents |= settings->getRemoveComponentsAnimations()         ? aiComponent_ANIMATIONS              : 0;
		removeComponents |= settings->getRemoveComponentsBiTangents()         ? aiComponent_TANGENTS_AND_BITANGENTS : 0;
		removeComponents |= settings->getRemoveComponentsBoneWeights()        ? aiComponent_BONEWEIGHTS             : 0;
		removeComponents |= settings->getRemoveComponentsCameras()            ? aiComponent_CAMERAS                 : 0;
		removeComponents |= settings->getRemoveComponentsColors()             ? aiComponent_COLORS                  : 0;
		removeComponents |= settings->getRemoveComponentsLights()             ? aiComponent_LIGHTS                  : 0;
		removeComponents |= settings->getRemoveComponentsMaterials()          ? aiComponent_MATERIALS               : 0;
		removeComponents |= settings->getRemoveComponentsMeshes()             ? aiComponent_MESHES                  : 0;
		removeComponents |= settings->getRemoveComponentsNormals()            ? aiComponent_NORMALS                 : 0;
		removeComponents |= settings->getRemoveComponentsTextureCoordinates() ? aiComponent_TEXCOORDS               : 0;
		removeComponents |= settings->getRemoveComponentsTextures()           ? aiComponent_TEXTURES                : 0;

		importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, removeComponents);
	}

	if (settings->getRemoveRedundantMaterials())
		importer.SetPropertyString(AI_CONFIG_PP_RRM_EXCLUDE_LIST, settings->getRemoveRedundantMaterialsSkip());

	if (settings->getRemoveRedundantNodes())
		importer.SetPropertyString(AI_CONFIG_PP_OG_EXCLUDE_LIST, settings->getRemoveRedundantNodesSkip());

	if (settings->getSortAndRemove())
	{
		int32_t removePrimitives = 0;
		removePrimitives |= settings->getSortAndRemovePoints()    ? aiPrimitiveType_POINT    : 0;
		removePrimitives |= settings->getSortAndRemoveLines ()    ? aiPrimitiveType_LINE     : 0;
		removePrimitives |= settings->getSortAndRemoveTriangles() ? aiPrimitiveType_TRIANGLE : 0;
		removePrimitives |= settings->getSortAndRemovePolygons()  ? aiPrimitiveType_POLYGON  : 0;
		importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, removePrimitives);

	}
	if (settings->getSplitLargeMeshes())
	{
		importer.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, settings->getSplitLargeMeshesTriangleLimit());
		importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, settings->getSplitLargeMeshesVertexLimit());

	}
	if (settings->getSplitByBoneCount())
		importer.SetPropertyInteger(AI_CONFIG_PP_SBBC_MAX_BONES, settings->getSplitByBoneCountMaxBones());
}
