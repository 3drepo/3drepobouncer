/**
*  Copyright (C) 2019 3D Repo Ltd
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


#pragma once

#include <string>
#ifdef SYNCHRO_SUPPORT
#include <spm_reader.h>
#endif
#include <memory>

#include "repo_model_import_abstract.h"
#include "../../../core/model/collection/repo_scene.h"
#include "../../../core/model/bson/repo_node_camera.h"
#include "../../../core/model/bson/repo_node_material.h"
#include "../../../core/model/bson/repo_node_mesh.h"
#include "../../../core/model/bson/repo_node_metadata.h"
#include "../../../core/model/bson/repo_node_transformation.h"

namespace repo{
	namespace manipulator{
		namespace modelconvertor{
			class SynchroModelImport : public AbstractModelImport
			{
			public:
				/**
				* Default Constructor, generate model with default settings
				*/
				SynchroModelImport() {};
				
				/**
				* Default Deconstructor
				*/
				~SynchroModelImport() {}
#ifdef SYNCHRO_SUPPORT
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
				bool importModel(std::string filePath, uint8_t &errMsg);

			private:
				std::pair<repo::core::model::RepoNodeSet, repo::core::model::RepoNodeSet> generateMatNodes(
					std::unordered_map<std::string, repo::lib::RepoUUID> &synchroIDtoRepoID,
					std::unordered_map<repo::lib::RepoUUID, repo::core::model::RepoNode*, repo::lib::RepoUUIDHasher> &repoIDToNode);


				repo::core::model::TransformationNode* createTransNode(
					const repo::lib::RepoMatrix &matrix,
					const std::string name,
					const std::vector<repo::lib::RepoUUID> &parents = std::vector<repo::lib::RepoUUID>());

				std::unordered_map<std::string, repo::core::model::MeshNode> SynchroModelImport::createMeshTemplateNodes();

				std::shared_ptr<synchro_reader::SPMReader> reader;

				std::string orgFile;
#else
				/**
				* Generates a repo scene graph
				* an internal representation(aiscene) needs to have
				* been created before this call
				* @return returns a populated RepoScene upon success.
				*/
				repo::core::model::RepoScene* generateRepoScene() { return nullptr; }

				/**
				* Import model from a given file
				* @param path to the file
				* @param error message if failed
				* @return returns true upon success
				*/
				bool importModel(std::string filePath, uint8_t &errMsg) {
					errMsg = REPOERR_SYNCHRO_UNAVAILABLE;
					return false;
				}
#endif
			};
		} //namespace SynchroModelImport
	} //namespace manipulator
} //namespace repo
