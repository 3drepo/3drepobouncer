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
* Assimp Model convertor(Export)
*/

#pragma once

#include <string>

#include <assimp/scene.h>
#include <assimp/Exporter.hpp>

#include "repo_model_export_abstract.h"
#include "../../../core/model/collection/repo_scene.h"

namespace repo{
	namespace manipulator{
		namespace modelconvertor{//x3d shader
			class AssimpModelExport : public AbstractModelExport
			{
			public:
				/**
				* Default Constructor, export model with default settings
				*/
				AssimpModelExport(
					const repo::core::model::RepoScene *scene);

				/**
				* Default Deconstructor
				*/
				virtual ~AssimpModelExport();

				/**
				* Export a repo scene graph to file
				* @param scene repo scene representation
				* @param filePath path to destination file
				* @return returns true upon success
				*/
				bool exportToFile(
					const std::string                  &filePath);

				/**
				* Get supported file formats for this exporter
				*/
				static std::string getSupportedFormats();

			private:

				/**
				* Construct an assimp node from 3DRepo scene
				* this is a recursive function call and will
				* construct aiNodes from its children
				* NB: The reason why we have meshVec/meshMap etc is because map is a sorted container
				*     and thus order will change as we add items in it. We need a vector container
				*     to keep track of the original order, which is the order in which will be going into the
				*     aiscene. A possible alternative to this is using boost's multiIndex container
				* @param scene the scene the node came from
				* @param currNode the node to convert
				* @param meshVec a list of aiMesh pointers in the order that is going into aiScene
				* @param matVec  a list of aiMaterial pointers in the order that is going into aiScene
				* @param camVec  a list of aiCamera pointers in the order that is going into aiScene
				* @param textNodes keeps track of the texture nodes that are referenced in this scene
				* @param gType which graph to convert (default: unoptimised)
				*/

				aiNode* constructAiSceneRecursively(
					const repo::core::model::RepoScene            *scene,
					const repo::core::model::RepoNode             *currNode,
					std::vector<aiMesh*>                          &meshVec,
					std::vector<aiMaterial*>                      &matVec,
					std::vector<aiCamera*>                        &camVec,
					repo::core::model::RepoNodeSet                &textNodes,
					const repo::core::model::RepoScene::GraphType &gType
					= repo::core::model::RepoScene::GraphType::DEFAULT);

				/**
				* Construct an assimp node from 3DRepo scene
				* this is a recursive function call and will
				* construct aiNodes from its children
				* NB: The reason why we have meshVec/meshMap etc is because map is a sorted container
				*     and thus order will change as we add items in it. We need a vector container
				*     to keep track of the original order, which is the order in which will be going into the
				*     aiscene. A possible alternative to this is using boost's multiIndex container
				* @param scene the scene the node came from
				* @param currNode the node to convert
				* @param meshVec a list of aiMesh pointers in the order that is going into aiScene
				* @param matVec  a list of aiMaterial pointers in the order that is going into aiScene
				* @param camVec  a list of aiCamera pointers in the order that is going into aiScene
				* @param meshMap a map of the uuid of the mesh that has been converted to aiMesh so far
				* @param materialMap a map of the uuid of the material that has been converted to aiMaterial so far
				* @param camMap a map of the uuid of the camera that has been converted to aiCamera so far
				* @param textNodes keeps track of the texture nodes that are referenced in this scene
				* @param gType which graph to convert (default: unoptimised)
				*/
				aiNode* constructAiSceneRecursively(
					const repo::core::model::RepoScene                        *scene,
					const repo::core::model::RepoNode                         *currNode,
					std::vector<aiMesh*>                                      &meshVec,
					std::vector<aiMaterial*>                                  &matVec,
					std::vector<aiCamera*>                                    &camVec,
					std::unordered_map<repoUUID, aiMesh*, RepoUUIDHasher>     &meshMap,
					std::unordered_map<repoUUID, aiMaterial*, RepoUUIDHasher> &materialMap,
					std::unordered_map<repoUUID, aiCamera*, RepoUUIDHasher>   &camMap,
					repo::core::model::RepoNodeSet                            &textNodes,
					const repo::core::model::RepoScene::GraphType             &gType
					= repo::core::model::RepoScene::GraphType::DEFAULT);

				/**
				* Convert a camera node to aiCamera
				* @param scene the scene the node came from
				* @param camNode the camera node itself
				* @param name override in assimp node to this
				*             (used to ensure it has the same name as the transformation it's attached to)
				* @return returns a aiCamera node
				*/
				aiCamera* convertCamera(
					const repo::core::model::RepoScene  *scene,
					const repo::core::model::CameraNode *camNode,
					const std::string                   &name = std::string());

				/**
				* Convert a material node to aiMaterial
				* @param scene the scene the node came from
				* @param matNode the material node itself
				* @return returns a aiMaterial node
				* @param gType which graph to convert (default: unoptimised)
				*/
				aiMaterial* convertMaterial(
					const repo::core::model::RepoScene            *scene,
					const repo::core::model::MaterialNode         *matNode,
					repo::core::model::RepoNodeSet                &textNodes,
					const repo::core::model::RepoScene::GraphType &gType
					= repo::core::model::RepoScene::GraphType::DEFAULT);

				/**
				* Convert a mesh node to aiMesh
				* @param scene the scene the node came from
				* @param meshnode the mesh node itself
				* @param matVec  a list of aiMaterial pointers in the order that is going into aiScene
				* @param materialMap a map of the uuid of the material that has been converted to aiMaterial so far
				* @param textNodes a vector of texture nodes that are referenced (will be updated if applicable)
				* @param gType which graph to convert (default: unoptimised)
				* @return returns a aiMesh node
				*/

				aiMesh* convertMesh(
					const repo::core::model::RepoScene                        *scene,
					const repo::core::model::MeshNode                         *meshNode,
					std::vector<aiMaterial*>                                  &matVec,
					std::unordered_map<repoUUID, aiMaterial*, RepoUUIDHasher> &matMap,
					repo::core::model::RepoNodeSet                            &textNodes,
					const repo::core::model::RepoScene::GraphType             &gType
					= repo::core::model::RepoScene::GraphType::DEFAULT);

				/**
				* Convert repo scene to assimp interpretation
				* @param scene repo scene to convert
				* @param textNodes Keeps track on the textures that are used in this scene.
				* @param gType which graph to convert (default: unoptimised)
				* @return returns a pointer to aiScene
				*/
				aiScene* convertToAssimp(
					const repo::core::model::RepoScene            *scene,
					repo::core::model::RepoNodeSet                &textNodes,
					const repo::core::model::RepoScene::GraphType &gType
					= repo::core::model::RepoScene::GraphType::DEFAULT);

				/**
				* Returns the ID of the exported file type
				* @param fileExtension string of the extension
				* @return returns the format ID for this file type
				*/
				std::string getExportFormatID(
					const std::string &fileExtension);

				/**
				* Write textures to file. Textures are written in the format
				* they were originally written from.  (default: jpg)
				* @param nodes list of texture nodes to write
				* @param filePath path at which this needs to be stored in
				* @return returns true upon success
				*/
				bool writeTexturesToFiles(
					const repo::core::model::RepoNodeSet &nodes,
					const std::string                    &filePath);

				/**
				* Write an aiScene to file
				* @param scene aiScene representation
				* @param filePath path to destination file (including file extension)
				* @return returns true upon success
				*/
				bool writeSceneToFile(
					const aiScene     *scene,
					const std::string &filePath);
			};
		} //namespace modelconvertor
	} //namespace manipulator
} //namespace repo
