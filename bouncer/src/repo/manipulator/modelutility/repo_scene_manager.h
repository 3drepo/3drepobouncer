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
#include "../modelconvertor/export/repo_model_export_web.h"

namespace repo{
	namespace manipulator{
		namespace modelutility{
			class SceneManager
			{
			public:
				SceneManager(){}
				~SceneManager(){}

				/**
				* Commit web buffers
				* @param scene repo scene related to repo scene
				* @param geoStashExt geometry stash extension
				* @param resultBuffers buffers to write
				* @param hander mongo handler
				* @param addTimestampToSettings whether we should be adding timestamp to settings upon success
				*/
				bool commitWebBuffers(
					repo::core::model::RepoScene                 *scene,
					const std::string                            &geoStashExt,
					const repo_web_buffers_t &resultBuffers,
					repo::core::handler::AbstractDatabaseHandler *handler,
					const bool                                    addTimestampToSettings = false);

				/**
				* Retrieve a RepoScene with a specific revision loaded.
				* @param handler hander to the database
				* @param database the database the collection resides in
				* @param project name of the project
				* @param uuid if headRevision, uuid represents the branch id,
				*              otherwise the unique id of the revision branch
				* @param headRevision true if retrieving head revision
				* @param lightFetch fetches only the stash (or scene if stash failed),
				reduce computation and memory usage (ideal for visualisation only)
				* @return returns a pointer to a repoScene.
				*/
				repo::core::model::RepoScene* fetchScene(
					repo::core::handler::AbstractDatabaseHandler *handler,
					const std::string                             &database,
					const std::string                             &project,
					const repo::lib::RepoUUID                                &uuid,
					const bool                                    &headRevision = true,
					const bool                                    &lightFetch = false,
					const bool                                    &ignoreRefScenes = false);

				repo::core::model::RepoScene* fetchScene(
					repo::core::handler::AbstractDatabaseHandler *handler,
					const std::string                             &database,
					const std::string                             &project,
					const bool                             &ignoreRefScene = false)
				{
					repo::lib::RepoUUID master = repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH);
					return fetchScene(handler, database, project, master, true, false, ignoreRefScene);
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
					repo::core::model::RepoScene                 *scene,
					repo::core::handler::AbstractDatabaseHandler *handler
					);

				/**
				* Generate a stash graph for the given scene and populate it
				* into the given scene
				* If a databasehandler is given and the scene is revisioned,
				* it will commit the stash to database
				* @param scene scene to generate stash graph for
				* @param handler hander to the database
				* @return returns true upon success
				*/
				bool generateStashGraph(
					repo::core::model::RepoScene                 *scene,
					repo::core::handler::AbstractDatabaseHandler *handler = nullptr
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
					const repo::core::model::RepoScene                 *scene,
					repo::core::handler::AbstractDatabaseHandler *handler) const;

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
					repo::core::model::RepoScene                 *scene,
					const repo::manipulator::modelconvertor::WebExportType          &exType,
					repo_web_buffers_t                           &resultBuffers,
					repo::core::handler::AbstractDatabaseHandler *handler = nullptr);

				/**
				* Remove stash graph entry for the given scene
				* Will also remove it from the database should handler exist
				* @param scene scene reference to remove stash graph from
				* @param handler hander to the database
				* @return returns true upon success
				*/
				bool removeStashGraph(
					repo::core::model::RepoScene                 *scene,
					repo::core::handler::AbstractDatabaseHandler *handler = nullptr
					);

			private:
				/**
				* Generate a gltf encoding in the form of a buffer for the given scene
				* This requires the stash to have been generated already
				* @param scene the scene to generate the gltf encoding from
				* @return returns a buffer in the form of a byte vector mapped to its filename
				*/
				repo_web_buffers_t generateGLTFBuffer(
					repo::core::model::RepoScene *scene);

				/**
				* Generate a SRC encoding in the form of a buffer for the given scene
				* This requires the stash to have been generated already
				* @param scene the scene to generate the src encoding from
				* @return returns a buffer in the form of a byte vector mapped to its filename
				*/
				repo_web_buffers_t generateSRCBuffer(
					repo::core::model::RepoScene *scene);
			};
		}
	}
}
