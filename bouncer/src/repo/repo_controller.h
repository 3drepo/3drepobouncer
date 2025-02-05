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
* Controller that communicates to manipulator objects.
* Typically used by Desktop Client
* loosely following the model-view-controller design pattern
*/

#pragma once

#include <map>
#include <memory>
#include <string>

#include "core/model/bson/repo_bson_project_settings.h"
#include "core/model/bson/repo_node_transformation.h"
#include "core/model/bson/repo_node_mesh.h"
#include "core/model/bson/repo_node_reference.h"
#include "core/model/collection/repo_scene.h"
#include "lib/datastructure/repo_structs.h"
#include "lib/repo_listener_abstract.h"
#include "lib/repo_config.h"
#include "manipulator/modelconvertor/import/repo_model_import_config.h"
#include "repo_bouncer_global.h"

namespace repo {
	class REPO_API_EXPORT RepoController
	{
	public:
		class RepoToken;

		/**
		* Constructor
		* @param listeners a list of listeners subscribing to the log
		* @param numConcurrentOps maximum number of requests it can handle concurrently
		* @param numDBConn number of concurrent connections to the database
		*/
		RepoController(
			std::vector<lib::RepoAbstractListener*> listeners = std::vector<lib::RepoAbstractListener *>(),
			const uint32_t &numConcurrentOps = 1,
			const uint32_t &numDbConn = 1);

		/**
		* Destructor
		*/
		~RepoController();

		/*
		*	------------- Database Connection & Authentication --------------
		*/

		/**
		* Connect to a mongo database, authenticate by the admin database
		* @param errMsg error message if failed
		* @param config RepoConfig instance containing all connection information
		*/
		RepoToken* init(
			std::string       &errMsg,
			const lib::RepoConfig  &config
		);

		/*
		*	------------- Token operations --------------
		*/

		/**
		* Add an alias to the repo token
		* @param token tokeng
		* @param alias alias to add
		*/
		void addAlias(
			RepoToken         *token,
			const std::string &alias);

		/**
		* Destroy token from memory
		* @param token token to destroy
		*/
		void destroyToken(RepoToken* token);

		/*
		*	------------- Database info lookup --------------
		*/

		/**
		* Retrieve documents from a specified collection
		* due to limitations of the transfer protocol this might need
		* to be called multiple times, utilising the skip index to skip
		* the first n items.
		* @param token A RepoToken given at authentication
		* @param database name of database
		* @param collection name of collection
		* @param skip specify how many documents to skip
		* @param limit specifiy max. number of documents to retrieve (0 = no limit)
		* @return list of RepoBSONs representing the documents
		*/
		std::vector < repo::core::model::RepoBSON >
			getAllFromCollectionContinuous(
				const RepoToken      *token,
				const std::string    &database,
				const std::string    &collection,
				const uint64_t       &skip = 0,
				const uint32_t       &limit = 0);

		/**
		* Retrieve documents from a specified collection, returning only the specified fields
		* due to limitations of the transfer protocol this might need
		* to be called multiple times, utilising the skip index to skip
		* the first n items.
		* @param token A RepoToken given at authentication
		* @param database name of database
		* @param collection name of collection
		* @param fields fields to get back from the database
		* @param sortField field to sort upon
		* @param sortOrder 1 ascending, -1 descending
		* @param skip specify how many documents to skip (see description above)
		* @param limit specifiy max. number of documents to retrieve (0 = no limit)
		* @return list of RepoBSONs representing the documents
		*/
		std::vector < repo::core::model::RepoBSON >
			getAllFromCollectionContinuous(
				const RepoToken              *token,
				const std::string            &database,
				const std::string            &collection,
				const std::list<std::string> &fields,
				const std::string            &sortField,
				const int                    &sortOrder = -1,
				const uint64_t               &skip = 0,
				const uint32_t               &limit = 0);

		/**
		* Check if VR is enabled for this model
		* @param token repo token to the database
		* @param scene scene to query on
		* @return returns true if it is enabled, false otherwise
		*/
		bool isVREnabled(const RepoToken *token,
			const repo::core::model::RepoScene *scene);

		/*
		*	---------------- Database Retrieval -----------------------
		*/
		/**
		* Retrieve a RepoScene with a specific revision loaded.
		* @param token Authentication token
		* @param database the database the collection resides in
		* @param project name of the project
		* @param uuid if headRevision, uuid represents the branch id,
		*              otherwise the unique id of the revision branch
		* @param headRevision true if retrieving head revision
		* @return returns a pointer to a repoScene.
		*/
		repo::core::model::RepoScene* fetchScene(
			const RepoToken      *token,
			const std::string    &database,
			const std::string    &project,
			const std::string    &uuid = REPO_HISTORY_MASTER_BRANCH,
			const bool           &headRevision = true,
			const bool           &ignoreRefScene = false,
			const bool           &skeletonFetch = false,
			const std::vector<repo::core::model::ModelRevisionNode::UploadStatus> &includeStatus = {});

		/*
		*	------- Database Operations (insert/delete/update) ---------
		*/

		/**
		* Commit a scene graph
		* @param token Authentication token
		* @param scene RepoScene to commit
		* @param owner specify the owner of the scene (by default it is the user authorised to commit)
		*/
		uint8_t commitScene(
			const RepoToken                     *token,
			repo::core::model::RepoScene        *scene,
			const std::string                   &owner = "",
			const std::string                      &tag = "",
			const std::string                      &desc = "",
			const repo::lib::RepoUUID           &revId = repo::lib::RepoUUID::createUUID());

		/*
		*	------------- Logging --------------
		*/

		/**
		* Configure how verbose the log should be
		* The levels of verbosity are:
		* TRACE - log all messages
		* DEBUG - log messages of level debug or above (use for debugging)
		* INFO - log messages of level info or above (use to filter debugging messages but want informative logging)
		* WARNING - log messages of level warning or above
		* ERROR - log messages of level error or above
		* FATAL - log messages of level fatal or above
		* @param level specify logging level
		*
		*/
		void setLoggingLevel(const repo::lib::RepoLog::RepoLogLevel &level);

		/**
		* Log to a specific file
		* @param filePath path to file
		*/
		void logToFile(const std::string &filePath);

		/*
		*	------------- Import/ Export --------------
		*/

		/**
		* Create a federated scene with the given scene collections
		* @param fedMap a map of reference scene and transformation from root where the scene should lie
		* @return returns a constructed scene graph with the reference.
		*/
		repo::core::model::RepoScene* createFederatedScene(
			const std::map<repo::core::model::ReferenceNode, std::string> &fedMap);

		/**
		* Generate and commit RepoBundles for the given scene
		*/
		bool generateAndCommitRepoBundlesBuffer(
			const RepoToken* token,
			repo::core::model::RepoScene* scene);

		/**
		* Generate and commit a SRC encoding for the given scene
		* This requires the stash to have been generated already
		* @param token token for authentication
		* @param scene the scene to generate the src encoding from
		* @return returns true upon success
		*/
		bool generateAndCommitSRCBuffer(
			const RepoToken                               *token,
			repo::core::model::RepoScene            *scene);

		/**
		* Generate and commit a selection tree for the given scene
		* @param token token for authentication
		* @param scene the scene to generate from
		* @return returns true upon success
		*/
		bool generateAndCommitSelectionTree(
			const RepoToken                               *token,
			repo::core::model::RepoScene            *scene);

		/**
		* Generate a SRC encoding in the form of a buffer for the given scene
		* This requires the stash to have been generated already
		* @param scene the scene to generate the src encoding from
		* @return returns a buffer in the form of a byte vector
		*/
		repo::lib::repo_web_buffers_t generateSRCBuffer(
			repo::core::model::RepoScene *scene);

		/**
			* Get a string of supported file formats for file import
			* @return returns a string with list of supported file formats
			*/
		std::string getSupportedImportFormats();

		/**
			* Load a Repo Scene from a file
			* @param filePath path to file
			* @param config import settings(optional)
			* @return returns a pointer to Repo Scene upon success
			*/
		repo::core::model::RepoScene* loadSceneFromFile(
			const std::string &filePath,
			uint8_t           &err,
			const repo::manipulator::modelconvertor::ModelImportConfig &config = repo::manipulator::modelconvertor::ModelImportConfig());

		/**
		* Given a teamspace and drawing revision, import the original file
		* into a drawing and update the revision.
		* If an image file is provided, processing will be omitted
		*/
		void processDrawingRevision(
			const RepoController::RepoToken* token,
			const std::string& teamspace,
			const repo::lib::RepoUUID revision,
			uint8_t& err,
			const std::string &imagePath = std::string());

		/**
			* Load metadata from a file
			* @param filePath path to file
			* @return returns a set of metadata nodes read from the file
			*/
		repo::core::model::RepoNodeSet loadMetadataFromFile(
			const std::string &filePath,
			const char        &delimiter = ',');

		/*
		*	------------- Optimizations --------------
		*/

		/**
		* Get a hierachical spatial partitioning in form of a tree
		* @param scene scene to partition
		* @param maxDepth max partitioning depth
		*/
		std::shared_ptr<repo::lib::repo_partitioning_tree_t>
			getScenePartitioning(
				const repo::core::model::RepoScene *scene,
				const uint32_t                     &maxDepth = 8
			);

		void updateRevisionStatus(
			repo::core::model::RepoScene* scene,
			const repo::core::model::ModelRevisionNode::UploadStatus& status);

		/*
		*	------------- Versioning --------------
		*/
		/**
		* Get bouncer version in the form of a string
		*/
		std::string getVersion();

	private:
		class _RepoControllerImpl;
		_RepoControllerImpl *impl;
	};
}
