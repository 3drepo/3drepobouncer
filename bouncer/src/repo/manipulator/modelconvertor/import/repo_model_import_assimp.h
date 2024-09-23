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
#include "../../../core/model/bson/repo_node_camera.h"
#include "../../../core/model/bson/repo_node_material.h"
#include "../../../core/model/bson/repo_node_mesh.h"
#include "../../../core/model/bson/repo_node_metadata.h"
#include "../../../core/model/bson/repo_node_transformation.h"

#include "repo/lib/datastructure/repo_variant.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			class AssimpModelImport : public AbstractModelImport
			{
			public:

				/**
				* Create AssimpModelImport with specific settings
				* @param settings
				*/
				AssimpModelImport(const ModelImportConfig &settings);

				/**
				* Default Deconstructor
				*/
				virtual ~AssimpModelImport();

				static bool isSupportedExts(const std::string &testExt);

				static std::string getSupportedFormats();

				/**
				* Generates a repo scene graph
				* an internal representation(aiscene) needs to have
				* been created before this call
				* @return returns a populated RepoScene upon success.
				*/
				repo::core::model::RepoScene* generateRepoScene(uint8_t &errMsg);

				virtual bool requireReorientation() const;

				/**
				* Import model from a given file
				* @param path to the file
				* @param error message if failed
				* @return returns true upon success
				*/
				bool importModel(std::string filePath, uint8_t &errMsg);

				/**
				* Attempts to convert the assimp metadata entry into a RepoVariant
				* @param the assimp metadata entry
				* @param the repo variant passed as reference
				* @return the success of the operation
				*/
				static bool tryConvertMetadataEntry(aiMetadataEntry& assimpMetaEntry, repo::lib::RepoVariant& v);

			private:

				/**
				* Convert the assimp scene into Repo Scene
				* @param errMsg error message if an error occured
				* @return return a pointer to the scene
				*/
				repo::core::model::RepoScene* convertAiSceneToRepoScene();

				/**
				* Create a Camera Node given the information in ASSIMP objects
				* @param assimp camera object
				* @return returns a pointer to the created camera node
				*/
				repo::core::model::CameraNode* createCameraRepoNode(
					const aiCamera *assimpCamera,
					const std::vector<double> &offset);
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
				repo::core::model::MeshNode createMeshRepoNode(
					const aiMesh *assimpMesh,
					const std::vector<repo::core::model::RepoNode *> &materials,
					std::unordered_map < repo::core::model::RepoNode*, std::vector<repo::lib::RepoUUID>> &matMap,
					const bool hasTexture,
					const std::vector<double> &offset);

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
					const std::vector<repo::lib::RepoUUID> &parents = std::vector<repo::lib::RepoUUID>());

				/**
				* Create a Transformation Node given the information in ASSIMP objects
				* @param assimpNode assimp Transformation object
				* @param cameras a map of camera name to camera objects
				* @param meshes A vector fo mesh nodes in its original ASSIMP object order
				* @param newMeshes newly produced meshes based on original ordered meshes
				* @param metadata a RepoNode Set to store metadata nodes generated within this function
				* @param map keeps track of the mapping between assimp pointer and repoNode
				* @param parent a vector of parents to this node (optional)
				* @return returns the created Metadata Node
				*/
				repo::core::model::RepoNodeSet createTransformationNodesRecursive(
					const aiNode                                                         *assimpNode,
					const std::unordered_map<std::string, repo::core::model::RepoNode *> &cameras,
					const std::vector<repo::core::model::RepoNode >                      &meshes,
					const std::unordered_map<repo::lib::RepoUUID, repo::core::model::RepoNode *, repo::lib::RepoUUIDHasher>    &meshToMat,
					std::unordered_map<repo::core::model::RepoNode *, std::vector<repo::lib::RepoUUID>> &matParents,
					repo::core::model::RepoNodeSet                                       &newMeshes,
					repo::core::model::RepoNodeSet						                 &metadata,
					uint32_t                                                             &count,
					const std::vector<double>                                            &worldOffset,
					const std::vector<repo::lib::RepoUUID>						                     &parent = std::vector<repo::lib::RepoUUID>()
				);

				/**
				* Duplicate the given mesh and assign a new parent
				* Also create a new unique ID and shared ID for the mesh
				* @return returns a pointer to the newly constructed mesh
				*/
				repo::core::model::RepoNode* duplicateMesh(
					repo::lib::RepoUUID                    &newParent,
					repo::core::model::RepoNode &mesh,
					const std::unordered_map<repo::lib::RepoUUID, repo::core::model::RepoNode *, repo::lib::RepoUUIDHasher>    &meshToMat,
					std::unordered_map<repo::core::model::RepoNode *, std::vector<repo::lib::RepoUUID>> &matParents);

				/**
				* Get bounding box of the aimesh
				* @return returns the bounding box
				*/
				std::vector<std::vector<double>> getAiMeshBoundingBox(
					const aiMesh *mesh) const;

				/**
				* Get file extension(in caps) of the given filePath
				*/
				std::string getFileExtension(const std::string &filePath) const;

				/**
				* Get bounding box of the aiscene
				* @return returns the bounding box
				*/
				std::vector<std::vector<double>> getSceneBoundingBox() const;

				void getSceneBoundingBoxInternal(
					const aiNode                     *node,
					const aiMatrix4x4                &mat,
					std::vector<std::vector<double>> &bbox) const;

				/**
				* Normalise shininess value base on source file type
				* If normalisation factor is unknown for the file type,
				* rawValue will be returned
				*/
				float normaliseShininess(const float &rawValue) const;

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
				* Attempts to get orientation information from metadata in the fbx file,
				* and set it to the loaded assimp scene. Requires loaded assimp scene.
				* @return whether the orientation could be set from the meta data
				*/
				bool SetRootOrientationFromMetadata();

				Assimp::Importer importer;  /*! Stores ASSIMP related settings for model import */
				const aiScene *assimpScene; /*! ASSIMP scene representation of the model */
				std::string orgFile; /*! orgFileName */
				bool keepMetadata;
				bool requiresOrientation = false;
			};
		} //namespace AssimpModelImport
	} //namespace manipulator
} //namespace repo
