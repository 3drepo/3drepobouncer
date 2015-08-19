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
* Creates a model(scene graph). 
* Given a file, it will utilist ASSIMP library and eventually converts it into Repo world meaning
*/


#pragma once

#include <string>

#include <assimp/Importer.hpp>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "repo_model_import_abstract.h"
#include "../../graph/repo_scene.h"

namespace repo{
	namespace manipulator{
		namespace modelconvertor{
			class AssimpModelImport : public AbstractModelImport
			{
			public:
				/**
				* Default Constructor, generate model with default settings
				*/
				AssimpModelImport();

				/**
				* Create AssimpModelImport with specific settings
				* @param settings
				*/
				AssimpModelImport(const ModelImportConfig *settings);

				/**
				* Default Deconstructor
				*/
				virtual ~AssimpModelImport();

				static std::string getSupportedFormats();

				/**
				* Generates a repo scene graph
				* an internal representation(aiscene) needs to have 
				* been created before this call
				* @return returns a populated RepoScene upon success.
				*/
				repo::manipulator::graph::RepoScene* generateRepoScene();


				/**
				* Import model from a given file
				* @param path to the file
				* @param error message if failed
				* @return returns true upon success
				*/
				bool importModel(std::string filePath, std::string &errMsg);

			private:
				/**
				* Create a Camera Node given the information in ASSIMP objects
				* @param assimp camera object
				* @return returns a pointer to the created camera node
				*/
				repo::core::model::bson::CameraNode* createCameraRepoNode(
					const aiCamera *assimpCamera);
				/**
				* Create a Material Node given the information in ASSIMP objects
				* NOTE: textures must've been populated at this point to populate references
				* @param material assimp material object
				* @param name name of the material
				* @return returns the created material node
				*/
				repo::core::model::bson::MaterialNode* AssimpModelImport::createMaterialRepoNode(
					aiMaterial *material,
					std::string name);

				/**
				* Create a Mesh Node given the information in ASSIMP objects
				* NOTE: materials must've been populated at this point to populate references
				* @param assimpMesh assimp mesh object
				* @param materials RepoNodeSet of material objects (to add reference)
				* @return returns the created Mesh Node
				*/
				repo::core::model::bson::MeshNode* AssimpModelImport::createMeshRepoNode(
					const aiMesh *assimpMesh,
					const std::vector<repo::core::model::bson::RepoNode *> &materials);

				/**
				* Create a Metadata Node given the information in ASSIMP objects
				* @param assimpMeta assimp metadata object
				* @param metadataName name of metadata
				* @param parent vector of node ID of parents (optional)
				* @return returns the created Metadata Node
				*/
				repo::core::model::bson::MetadataNode* AssimpModelImport::createMetadataRepoNode(
					const aiMetadata             *assimpMeta,
					const std::string            &metadataName,
					const std::vector<repoUUID> &parents = std::vector<repoUUID>());

				/**
				* Create a Transformation Node given the information in ASSIMP objects
				* @param assimpNode assimp Transformation object
				* @param cameras a map of camera name to camera objects
				* @param meshes A vector fo mesh nodes in its original ASSIMP object order
				* @param metadata a RepoNode Set to store metadata nodes generated within this function
				* @param parent a vector of parents to this node (optional)
				* @return returns the created Metadata Node
				*/
				repo::core::model::bson::RepoNodeSet AssimpModelImport::createTransformationNodesRecursive(
					const aiNode                                                     *assimpNode,
					const std::map<std::string, repo::core::model::bson::RepoNode *> &cameras,
					const std::vector<repo::core::model::bson::RepoNode *>           &meshes,
					repo::core::model::bson::RepoNodeSet						     &metadata,
					const std::vector<repoUUID>						             &parent = std::vector<repoUUID>()
					);

				/**
				* Load Texture within the given folder
				* @param folderPath directory to folder
				*/
				void loadTextures(std::string folderPath);

				/**
				* Set ASSIMP properties on the Importer using ModelImportConfig
				*/
				void setAssimpProperties();

				/**
				* Compose Assimp post processing flags based on information from ModelImportConfig
				* @param flag to build upon
				* @return reutnrs a post processing flag.
				*/
				uint32_t AssimpModelImport::composeAssimpPostProcessingFlags(
					uint32_t flag = 0);

				Assimp::Importer importer;  /*! Stores ASSIMP related settings for model import */
				const aiScene *assimpScene; /*! ASSIMP scene representation of the model */
				repo::core::model::bson::RepoNodeSet textures;
				std::map<std::string, repo::core::model::bson::RepoNode *> nameToTexture;
			};

		} //namespace AssimpModelImport
	} //namespace manipulator
} //namespace repo

