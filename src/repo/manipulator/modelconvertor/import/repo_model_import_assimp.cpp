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

#include <fstream>
#include <boost/filesystem.hpp>

#include "../../../core/model/bson/repo_bson_factory.h"
#include "../../../core/model/repo_node_utils.h"

using namespace repo::manipulator::modelconvertor;

namespace model = repo::core::model;

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
		delete settings;
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

model::bson::CameraNode* AssimpModelImport::createCameraRepoNode(
	const aiCamera *assimpCamera)
{
	std::string cameraName(assimpCamera->mName.data);
	
	model::bson::CameraNode * cameraNode;

	if (assimpCamera)
	{
		cameraNode = model::bson::RepoBSONFactory::makeCameraNode(
			assimpCamera->mAspect,
			assimpCamera->mClipPlaneFar,
			assimpCamera->mClipPlaneNear,
			assimpCamera->mHorizontalFOV,
			{ assimpCamera->mLookAt.x, assimpCamera->mLookAt.y, assimpCamera->mLookAt.z },
			{ assimpCamera->mPosition.x, assimpCamera->mPosition.y, assimpCamera->mPosition.z },
			{ assimpCamera->mUp.x, assimpCamera->mUp.y, assimpCamera->mUp.z },
			cameraName
			);
	}

	return cameraNode;

}
model::bson::MaterialNode* AssimpModelImport::createMaterialRepoNode(
	aiMaterial *material,
	std::string name)
{
	model::bson::MaterialNode *materialNode;

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


		materialNode = model::bson::RepoBSONFactory::makeMaterialNode(repo_material, name, REPO_NODE_API_LEVEL_1);


		//--------------------------------------------------------------------------
		// Texture (one diffuse for the moment)
		// Textures are uniquely referenced by their name
		aiString texPath; // contains a filename of a texture
		if (AI_SUCCESS == material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath))
		{
			std::map<std::string, model::bson::RepoNode *>::const_iterator it = nameToTexture.find(texPath.data);

			if (nameToTexture.end() != it)
			{
				model::bson::RepoNode * nodeAddr = it->second;
				nodeAddr->swap(nodeAddr->cloneAndAddParent(materialNode->getSharedID()));
			}
		}
	}

	return materialNode;
}

model::bson::MeshNode* AssimpModelImport::createMeshRepoNode(
	const aiMesh *assimpMesh,
	const std::vector<model::bson::RepoNode *> &materials)
{

	model::bson::MeshNode *meshNode = 0;

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
			faces.push_back({ assimpMesh->mFaces[i].mNumIndices, assimpMesh->mFaces[i].mIndices });
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

	meshNode = model::bson::RepoBSONFactory::makeMeshNode(
		vertices, faces, normals, boundingBox, uvChannels, colors, outline);

	///*
	//*------------------------------ setParents ----------------------------------
	//*/

	if (assimpMesh->mMaterialIndex < materials.size())
	{
		model::bson::RepoNode *materialNode = materials[assimpMesh->mMaterialIndex];
		materialNode->swap(materialNode->cloneAndAddParent(meshNode->getSharedID()));
	}

	///*
	//*-----------------------------------------------------------------------------
	//*/

	return meshNode;
}

model::bson::MetadataNode* AssimpModelImport::createMetadataRepoNode(
	const aiMetadata             *assimpMeta,
	const std::string            &metadataName,
	const std::vector<repoUUID> &parents)
{
	model::bson::MetadataNode *metaNode;
	std::string val;
	if (assimpMeta)
	{
		//build the metadata as a bson
		model::bson::RepoBSONBuilder builder;

		for (uint32_t i = 0; i < assimpMeta->mNumProperties; i++)
		{
			std::string key(assimpMeta->mKeys[i].C_Str());
			aiMetadataEntry &currentValue = assimpMeta->mValues[i];

			if (key == "IfcGloballyUniqueId")
				BOOST_LOG_TRIVIAL(error) << "TODO: fix IfcGloballyUniqueId in RepoMetadata" << std::endl;


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
				builder << key << (int64_t)(*(static_cast<uint64_t *>(currentValue.mData)));
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


					builder.appendVector(key, repoVector);
				}
				break;
			case FORCE_32BIT:
				// Gracefully (but silently) handle the bogus enum used by
				// assimp to ensure the enum is 32-bits.
				break;
			}
		}

		model::bson::RepoBSON metaBSON = builder.obj();

		metaNode = model::bson::RepoBSONFactory::makeMetaDataNode(metaBSON, "", metadataName, parents);
	}//if(assimpMeta)

	return metaNode;

}

model::bson::RepoNodeSet AssimpModelImport::createTransformationNodesRecursive(
	const aiNode                                         *assimpNode,
	const std::map<std::string, model::bson::RepoNode *> &cameras,
	const std::vector<model::bson::RepoNode *>           &meshes,
	model::bson::RepoNodeSet						     &metadata,
	const std::vector<repoUUID>						 &parent
	)
{
	model::bson::RepoNodeSet transNodes;

	if (assimpNode){
		std::string transName(assimpNode->mName.data);
		//create a 4 by 4 vector
		std::vector < std::vector<float> > transMat;

		for (int i = 0; i < 4; i++){
			std::vector<float> rows;
			for (int j = 0; j < 4; j++){
				rows.push_back(assimpNode->mTransformation[i][j]);
			}
			transMat.push_back(rows);
		}


		model::bson::TransformationNode * transNode = 
			model::bson::RepoBSONFactory::makeTransformationNode(transMat, transName, parent);

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
				model::bson::RepoNode *mesh = meshes[meshIndex];
				
				if (mesh)
				{
					mesh->swap(mesh->cloneAndAddParent(sharedId));
				}
			}
		}

		//--------------------------------------------------------------------------
		// Register cameras as children of this transformation (by name) if any
		std::map<std::string, model::bson::RepoNode *>::const_iterator it =
			cameras.find(assimpNode->mName.data);
		if (cameras.end() != it)
		{
			model::bson::RepoNode * camera = it->second;
			if (camera)
			{
				camera->swap(camera->cloneAndAddParent(sharedId));

			}
		}

		//--------------------------------------------------------------------------
		// Collect metadata and add as a child
		if (assimpNode->mMetaData)
		{
			std::string metadataName = assimpNode->mName.data;
			if (metadataName == "<transformation>")
				metadataName = "<metadata>";

			model::bson::MetadataNode *metachild = createMetadataRepoNode(assimpNode->mMetaData, metadataName, myShareID);
			metadata.insert(metachild);
		}

		transNodes.insert(transNode);

	//--------------------------------------------------------------------------
	// Register child transformations as children if any
		for (unsigned int i = 0; i < assimpNode->mNumChildren; ++i)
		{
				
			model::bson::RepoNodeSet childMetadata;
			model::bson::RepoNodeSet childSet =  createTransformationNodesRecursive(assimpNode->mChildren[i], 
				cameras, meshes, childMetadata, myShareID);

			transNodes.insert(childSet.begin(), childSet.end());
			metadata.insert(childMetadata.begin(), childMetadata.end());
		}
	} //if assimpNode
	return transNodes;
}

repo::manipulator::graph::RepoScene * AssimpModelImport::generateRepoScene()
{
	repo::manipulator::graph::RepoScene *sceneGraph = 0;

	if (assimpScene)
	{
		//Turn everything into repoNodes and construct a scene
		repo::core::model::bson::RepoNodeSet cameras; //!< Cameras
		repo::core::model::bson::RepoNodeSet meshes; //!< Meshes
		repo::core::model::bson::RepoNodeSet materials; //!< Materials
		repo::core::model::bson::RepoNodeSet metadata; //!< Metadata
		repo::core::model::bson::RepoNodeSet transformations; //!< Transformations

		std::vector<model::bson::RepoNode *> originalOrderMaterial; //vector that keeps track original order for assimp indices
		std::vector<model::bson::RepoNode *> originalOrderMesh; //vector that keeps track original order for assimp indices
		std::map<std::string, model::bson::RepoNode *> camerasMap;

		BOOST_LOG_TRIVIAL(info) << "Constructing Material Nodes...";
		/*
		* ------------- Material Nodes -----------------
		*/
		// Warning: Default material might not be attached to anything,
		// hence it would not be returned by a call to getNodes().
		if (assimpScene->HasMaterials())
		{
			for (unsigned int i = 0; i < assimpScene->mNumMaterials; ++i)
			{
				aiString name;
				assimpScene->mMaterials[i]->Get(AI_MATKEY_NAME, name);

				model::bson::RepoNode* material = createMaterialRepoNode(
					assimpScene->mMaterials[i],
					name.data);

				if (!material)
					BOOST_LOG_TRIVIAL(error) << "Unable to construct material node in Assimp Model Convertor!";
				else
					materials.insert(material);
					
				originalOrderMaterial.push_back(material);
			}
		}

		/*
		* ---------------------------------------------
		*/

		BOOST_LOG_TRIVIAL(info) << "Constructing Mesh Nodes...";
		/*
		* --------------- Mesh Nodes ------------------
		*/
		if (assimpScene->HasMeshes())
		{
			for (unsigned int i = 0; i < assimpScene->mNumMeshes; ++i)
			{
				model::bson::RepoNode* mesh = createMeshRepoNode(
					assimpScene->mMeshes[i],
					originalOrderMaterial);

				if (!mesh)
					BOOST_LOG_TRIVIAL(error) << "Unable to construct mesh node in Assimp Model Convertor!";
				else
					meshes.insert(mesh);
				originalOrderMesh.push_back(mesh);
			}
		}
		/*
		* ---------------------------------------------
		*/

		BOOST_LOG_TRIVIAL(info) << "Constructing Camera Nodes...";
		/*
		* ------------- Camera Nodes ------------------
		*/

		if (assimpScene->HasCameras())
		{
			for (unsigned int i = 0; i < assimpScene->mNumCameras; ++i)
			{
				std::string cameraName(assimpScene->mCameras[i]->mName.data);
				model::bson::RepoNode* camera = createCameraRepoNode(assimpScene->mCameras[i]);
				if (!camera)
					BOOST_LOG_TRIVIAL(error) << "Unable to construct mesh node in Assimp Model Convertor!";
				else
					cameras.insert(camera);

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

		BOOST_LOG_TRIVIAL(info) << "Constructing Transformation Nodes...";
		/*
		* ----------- Transformation Nodes ------------
		*/
		// Recursively converts aiNode and all of its children to a hierarchy
		// of RepoNodeTransformations. Call with root node of aiScene.
		// RootNode will be the first entry in transformations vector.
		

		transformations = createTransformationNodesRecursive(assimpScene->mRootNode, camerasMap, originalOrderMesh, metadata);
		
		/*
		* ---------------------------------------------
		*/

		

		//Construct the scene graph
		sceneGraph = new repo::manipulator::graph::RepoScene(cameras, meshes, materials, metadata, textures, transformations);

	} //if(assimpScene)

	return sceneGraph;
}

bool AssimpModelImport::importModel(std::string filePath, std::string &errMsg)
{
	bool success = true;

	setAssimpProperties();


	//-------------------------------------------------------------------------
	// Import model

	std::string fileName = getFileName(filePath);

	assimpScene = importer.ReadFile(filePath, composeAssimpPostProcessingFlags());


	if (!assimpScene){
		errMsg = std::string(aiGetErrorString());
		success = false;
	}
	else
	{
		//-------------------------------------------------------------------------
		// Polygon count
		uint64_t polyCount = 0;
		for (unsigned int i = 0; i < assimpScene->mNumMeshes; ++i)
			polyCount += assimpScene->mMeshes[i]->mNumFaces;

		BOOST_LOG_TRIVIAL(info) << "============ IMPORTING MODEL WITh ASSIMP MODEL CONVERTOR ===============";
		BOOST_LOG_TRIVIAL(info) << "Loaded ";
		BOOST_LOG_TRIVIAL(info) << fileName << " with " << polyCount << " polygons in ";
		BOOST_LOG_TRIVIAL(info) << assimpScene->mNumMeshes << " ";
		BOOST_LOG_TRIVIAL(info) << ((assimpScene->mNumMeshes == 1) ? "mesh" : "meshes");
		BOOST_LOG_TRIVIAL(info) << std::endl;

		//-------------------------------------------------------------------------
		// Textures
		loadTextures(getDirPath(filePath));

	}

	return success;
}

void AssimpModelImport::loadTextures(std::string dirPath){

	for (uint32_t m = 0; m < assimpScene->mNumMaterials; ++m)
	{
		int texIndex = 0;
		aiReturn texFound = AI_SUCCESS;
		const aiMaterial *material = assimpScene->mMaterials[m];

		aiString path;	// filename
		while (AI_SUCCESS == texFound)
		{
			texFound = material->GetTexture(aiTextureType_DIFFUSE, texIndex, &path);
			std::string fileName(path.data);

			// Multiple materials can point to the same texture
			if (!fileName.empty() && nameToTexture.find(fileName.c_str()) == nameToTexture.end())
			{
				model::bson::RepoNode *textureNode;
				// In assimp, embedded textures name starts with asterisk and a textures array index
				// so the name can be "*0" for example
				if (assimpScene->HasTextures() && '*' == fileName.at(0))
				{
					//---------------------------------------------------------
					// Embedded texture
					int textureIndex = atoi(fileName.substr(1, fileName.size()).c_str());
					aiTexture *texture = assimpScene->mTextures[textureIndex];

					// Read the raw file to save space in the DB. QImage will happily
					// claim a file is 32 bit depth even though it is 24 for example.
					std::ifstream::pos_type size;
					char *memblock;
					std::ifstream file(dirPath + fileName, std::ios::in | std::ios::binary | std::ios::ate);
					if (!file.is_open())
					{
						BOOST_LOG_TRIVIAL(error) << "Could not open texture: " << dirPath << fileName << std::endl;
					}
					else
					{
						size = file.tellg();
						memblock = new char[size];
						file.seekg(0, std::ios::beg);
						file.read(memblock, size);
						file.close();

						textureNode = model::bson::RepoBSONFactory::makeTextureNode(
							fileName,
							memblock,
							(uint32_t)size,
							texture->mWidth,
							texture->mHeight,
							REPO_NODE_API_LEVEL_1);

						textures.insert(textureNode);
						nameToTexture[fileName] = textureNode;

						delete[] memblock;
					}
				}
			}
			texIndex++;
		}
	}


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