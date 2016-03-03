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

#include <boost/bimap.hpp>

#include "repo_model_import_abstract.h"
#include "../../../core/model/collection/repo_scene.h"

namespace repo{
	namespace manipulator{
		namespace modelconvertor{
			using assimp_map = boost::bimap<uintptr_t, repo::core::model::RepoNode*>;
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
				repo::core::model::RepoScene* generateRepoScene();


				/**
				* Import model from a given file
				* @param path to the file
				* @param error message if failed
				* @return returns true upon success
				*/
				bool importModel(std::string filePath, std::string &errMsg);

			private:

				/**
				* Convert the assimp scene into Repo Scene
				* If scene is null, construct a new repo scene
				* if scene already exists, this must be a stash representation
				* thus add it into the existing scene
				* @param scene Repo Scene (if exists)
				* @return return a pointer to the scene (same pointer if scene != nullptr)
				*/
				repo::core::model::RepoScene* convertAiSceneToRepoScene(
					assimp_map                    &map,
					repo::core::model::RepoScene  *scene = nullptr);

				/**
				* Create a Camera Node given the information in ASSIMP objects
				* @param assimp camera object
				* @return returns a pointer to the created camera node
				*/
				repo::core::model::CameraNode* createCameraRepoNode(
					const aiCamera *assimpCamera);
				/**
				* Create a Material Node given the information in ASSIMP objects
				* NOTE: textures must've been populated at this point to populate references
				* @param material assimp material object
				* @param name name of the material
				* @param nameToTexture a mapping of texture name to texture node
				* @return returns the created material node
				*/
				repo::core::model::MaterialNode* createMaterialRepoNode(
					const aiMaterial *material,
					const std::string &name,
					const std::unordered_map<std::string, repo::core::model::RepoNode *> &nameToTexture);

				/**
				* Create a Mesh Node given the information in ASSIMP objects
				* NOTE: materials must've been populated at this point to populate references
				* @param assimpMesh assimp mesh object
				* @param materials RepoNodeSet of material objects (to add reference)
				* @return returns the created Mesh Node
				*/
				repo::core::model::MeshNode* createMeshRepoNode(
					const aiMesh *assimpMesh,
					const std::vector<repo::core::model::RepoNode *> &materials,
					std::unordered_map < repo::core::model::RepoNode*, std::vector<repoUUID>> &matMap,
					const bool hasTexture);

				/**
				* Create a Metadata Node given the information in ASSIMP objects
				* @param assimpMeta assimp metadata object
				* @param metadataName name of metadata
				* @param parent vector of node ID of parents (optional)
				* @return returns the created Metadata Node
				*/
				repo::core::model::MetadataNode* createMetadataRepoNode(
					const aiMetadata             *assimpMeta,
					const std::string            &metadataName,
					const std::vector<repoUUID> &parents = std::vector<repoUUID>());

				/**
				* Create a Transformation Node given the information in ASSIMP objects
				* @param assimpNode assimp Transformation object
				* @param cameras a map of camera name to camera objects
				* @param meshes A vector fo mesh nodes in its original ASSIMP object order
				* @param metadata a RepoNode Set to store metadata nodes generated within this function
				* @param map keeps track of the mapping between assimp pointer and repoNode
				* @param parent a vector of parents to this node (optional)
				* @return returns the created Metadata Node
				*/
				repo::core::model::RepoNodeSet createTransformationNodesRecursive(
					const aiNode                                                     *assimpNode,
					const std::unordered_map<std::string, repo::core::model::RepoNode *> &cameras,
					const std::vector<repo::core::model::RepoNode *>           &meshes,
					repo::core::model::RepoNodeSet						     &metadata,
					assimp_map													&map,
					uint32_t &count ,
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
				uint32_t composeAssimpPostProcessingFlags(
					uint32_t flag = 0);

				/**
				* Populate the optimization linkage between the org. scene graph
				* and the optimised scene graph
				* note: this is a recursive function
				* @param node  node we are currently trasversing
				* @param scene repo scene in process
				* @param orgMap the org mapping
				* @param optMap optimised mapping
				* @return returns whether it has successfully mapped everything.
 				*/

				bool populateOptimMaps(
					repo::core::model::RepoNode  *node,
					repo::core::model::RepoScene *scene,
					const assimp_map             &orgMap,
					const assimp_map             &optMap);

				Assimp::Importer importer;  /*! Stores ASSIMP related settings for model import */
				const aiScene *assimpScene; /*! ASSIMP scene representation of the model */
				std::string orgFile; /*! orgFileName */
			};

		} //namespace AssimpModelImport
	} //namespace manipulator
} //namespace repo

