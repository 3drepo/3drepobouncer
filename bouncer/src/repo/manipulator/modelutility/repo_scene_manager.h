/**
*  Copyright (C) 2016 3D Repo Ltd
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
#include "../../core/model/collection/repo_scene.h"
#include "../../core/handler/repo_database_handler_abstract.h"
#include "../../core/handler/fileservice/repo_file_manager.h"
#include "../modelconvertor/export/repo_model_export_abstract.h"

namespace repo {
	namespace manipulator {
		namespace modelutility {
			class SceneManager
			{
			public:
				SceneManager() {}
				~SceneManager() {}

				uint8_t commitScene(
					repo::core::model::RepoScene									*scene,
					const std::string												&owner,
					const std::string												&tag,
					const std::string												&desc,
					const repo::lib::RepoUUID										&revId,
					repo::core::handler::AbstractDatabaseHandler					*handler,
					repo::core::handler::fileservice::FileManager					*fileManager
				);

				/**
				* Retrieve a RepoScene with a specific revision loaded.
				* @param handler hander to the database
				* @param database the database the collection resides in
				* @param project name of the project
				* @param uuid if headRevision, uuid represents the branch id,
				*              otherwise the unique id of the revision branch
				* @param headRevision true if retrieving head revision
				* @return returns a pointer to a repoScene.
				*/
				repo::core::model::RepoScene* fetchScene(
					repo::core::handler::AbstractDatabaseHandler  *handler,
					const std::string                             &database,
					const std::string                             &project,
					const repo::lib::RepoUUID                     &uuid,
					const bool                                    &headRevision = true,
					const bool                                    &ignoreRefScenes = false,
					const bool                                    &skeletonFetch = false,
					const std::vector<repo::core::model::ModelRevisionNode::UploadStatus> &includeStatus = {});

				repo::core::model::RepoScene* fetchScene(
					repo::core::handler::AbstractDatabaseHandler  *handler,
					const std::string                             &database,
					const std::string                             &project,
					const bool                             &ignoreRefScene = false,
					const bool                                    &skeletonFetch = false)
				{
					repo::lib::RepoUUID master = repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH);
					return fetchScene(handler, database, project, master, true, ignoreRefScene, skeletonFetch);
				}

				/**
				* Retrieve all RepoScene representations given a partially loaded scene.
				* @param handler database handler
				* @param scene scene to fully load
				*/
				void fetchScene(
					repo::core::handler::AbstractDatabaseHandler *handler,
					repo::core::model::RepoScene              *scene);

				/**
				* Generate and commit scene's selection tree in JSON format
				* The generated data will be
				* also commited to the database/project set within the scene
				* @param scene scene to optimise
				* @param handler hander to the database
				* @return return true upon success
				*/
				bool generateAndCommitSelectionTree(
					repo::core::model::RepoScene										*scene,
					repo::core::handler::AbstractDatabaseHandler						*handler,
					repo::core::handler::fileservice::FileManager						*fileManager
				);

				/**
				* Check if the scene is VR enabled
				* This is primarily used for determining if we need to generate
				* VR version of the asset bundles.
				* @param scene scene to generate stash graph for
				* @param handler hander to the database
				* @return returns true if VR is enabled
				*/
				bool isVrEnabled(
					const repo::core::model::RepoScene					*scene,
					repo::core::handler::AbstractDatabaseHandler		*handler) const;

				/**
				* Generate a `exType` encoding for the given scene
				* if a database handler is provided, it will also commit the
				* buffers into the database
				* This requires the repo stash to have been generated already
				* @param scene the scene to generate the src encoding from
				* @param exType the type of export it is
				* @return returns repo_web_buffers upon success
				*/
				bool generateWebViewBuffers(
					repo::core::model::RepoScene									*scene,
					const repo::manipulator::modelconvertor::ExportType				&exType,
					repo::core::handler::AbstractDatabaseHandler					*handler = nullptr);

				/**
				* Remove stash graph entry for the given scene
				* Will also remove it from the database should handler exist
				* @param scene scene reference to remove stash graph from
				* @param handler hander to the database
				* @return returns true upon success
				*/
				bool removeStashGraph(
					repo::core::model::RepoScene *scene);

			private:
				/**
				* Generate a set of Repo Bundles in the form of web buffers for
				* the given scene. The stash must have beeen generated already.
				* This method requires project to have been built with
				* REPO_ASSETGENERATOR_SUPPORT ON. If not, this method will report
				* an error and return an empty buffers object.
				*/
				repo::lib::repo_web_buffers_t generateRepoBundleBuffer(
					repo::core::model::RepoScene* scene);
			};
		}
	}
}
