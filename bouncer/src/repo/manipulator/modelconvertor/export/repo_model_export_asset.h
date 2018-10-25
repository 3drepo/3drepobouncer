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
* Allows Export functionality from 3D Repo World to SRC
*/

#pragma once

#include <string>

#include "repo_model_export_web.h"
#include "../../../lib/repo_property_tree.h"
#include "../../../core/model/collection/repo_scene.h"
#include "../../../core/model/bson/repo_node_mesh.h"
#include "../../modelutility/repo_mesh_map_reorganiser.h"

namespace repo{
	namespace manipulator{
		namespace modelconvertor{
			class AssetModelExport : public WebModelExport
			{
			public:
				/**
				* Default Constructor, export model with default settings
				* @param scene repo scene to convert
				* @param whether the scene requires VR bundles
				*/
				AssetModelExport(const repo::core::model::RepoScene *scene,
					const bool vrEnabled = false);

				/**
				* Default Destructor
				*/
				virtual ~AssetModelExport();

				/**
				* Export all necessary files as buffers
				* @return returns a repo_src_export_t containing all files needed for this
				*          model to be rendered
				*/
				repo_web_buffers_t getAllFilesExportedAsBuffer() const;

				/**
				* Return a map of super meshes to reorganised meshes
				* @return returns a vector of reorganised meshes
				*/
				std::vector<std::shared_ptr<repo::core::model::MeshNode>>
					getReorganisedMeshes(
					std::vector<std::vector<uint16_t>> &faceBuffer,
					std::vector<std::vector<std::vector<float>>> &idMapBuffer,
					std::vector<std::vector<std::vector<repo_mesh_mapping_t>>> &meshMaps
					) const {
					faceBuffer = serialisedFaceBuf;
					idMapBuffer = idMapBuf;
					meshMaps = meshMappings;
					return reorganisedMeshes;
				}

				/**
				 * Export Unity assets list
				 * @return returns a RepoUnityAssets of the Unity assets list
				 *         for this model
				 */
				repo::core::model::RepoUnityAssets getUnityAssets() const;

			private:

				/**
				 * Insert Unity asset list file database entry into database
				 * @return returns true upon success
				 */
				bool commitUnityAssets(
					repo::core::handler::AbstractDatabaseHandler *handler,
					const repo::lib::RepoUUID &revisionID,
					std::vector<std::string> &assets,
					std::vector<double> offset,
					std::vector<std::string> &vrAssetFiles,
					std::vector<std::string> &jsonFiles,
					std::string &errMsg);

				/**
				* Generate JSON mapping for multipart meshes
				* And add it into the object
				* @param mesh mesh to generate with
				* @param scene scene for reference
				* @param splitMapping how the mapping is split after subMesh split
				*/
				bool generateJSONMapping(
					const repo::core::model::MeshNode *mesh,
					const repo::core::model::RepoScene *scene,
					const std::unordered_map<repo::lib::RepoUUID, std::vector<uint32_t>, repo::lib::RepoUUIDHasher> &splitMapping);

				/**
				* Create a tree representation for the graph
				* This creates the header of the SRC
				* @return returns true upon success
				*/
				bool generateTreeRepresentation();

				std::vector<std::shared_ptr<repo::core::model::MeshNode>> reorganisedMeshes;
				repo::core::model::RepoUnityAssets unityAssets;
				std::vector<std::vector<uint16_t>> serialisedFaceBuf;
				std::vector<std::vector<std::vector<float>>> idMapBuf;
				std::vector<std::vector<std::vector<repo_mesh_mapping_t>>> meshMappings;
				std::string assetListFile;
				const bool generateVR;
			};
		} //namespace modelconvertor
	} //namespace manipulator
} //namespace repo
