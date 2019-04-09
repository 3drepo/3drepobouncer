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
#include "core/model/bson/repo_bson_role.h"
#include "core/model/bson/repo_bson_role_settings.h"
#include "core/model/bson/repo_bson_user.h"
#include "core/model/bson/repo_node_transformation.h"
#include "core/model/bson/repo_node_mesh.h"
#include "core/model/bson/repo_node_reference.h"
#include "core/model/collection/repo_scene.h"
#include "lib/datastructure/repo_structs.h"
#include "lib/repo_listener_abstract.h"
#include "lib/repo_config.h"
#include "manipulator/modelconvertor/import/repo_model_import_config.h"
#include "repo_bouncer_global.h"

namespace repo{
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

		/**
			* Disconnect the controller from a database connection
			* and destroys the token
			* FIXME: CURRENTLY NOT THREAD SAFE! POTENTIALLY DANGEROUS
			* @param token token to the database
			*/
		void disconnectFromDatabase(const RepoToken* token);

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
			* Count the number of documents within the collection
			* @param token A RepoToken given at authentication
			* @param database name of database
			* @param collection name of collection
			* @return number of documents within the specified collection
			*/
		uint64_t countItemsInCollection(
			const RepoToken            *token,
			const std::string    &database,
			const std::string    &collection);

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
		* Retrieve roles from a specified database
		* due to limitations of the transfer protocol this might need
		* to be called multiple times, utilising the skip index to skip
		* the first n items.
		* @param token A RepoToken given at authentication
		* @param database name of database
		* @param skip specify how many documents to skip (see description above)
		* @param limit specifiy max. number of documents to retrieve (0 = no limit)
		* @return list of RepoRole representing the roles
		*/
		std::vector < repo::core::model::RepoRole >
			getRolesFromDatabase(
			const RepoToken              *token,
			const std::string            &database,
			const uint64_t               &skip = 0,
			const uint32_t               &limit = 0);

		/**
		* Get all role settings within a database
		* @param token A RepoToken given at authentication
		* @param database name of database
		* @param skip specify how many documents to skip (see description above)
		* @param limit specifiy max. number of documents to retrieve (0 = no limit)
		*/
		std::vector < repo::core::model::RepoRoleSettings >
			getRoleSettingsFromDatabase(
			const RepoToken              *token,
			const std::string            &database,
			const uint64_t               &skip = 0,
			const uint32_t               &limit = 0);

		repo::core::model::RepoRoleSettings getRoleSettings(
			const RepoToken *token,
			const repo::core::model::RepoRole &role);

		/**
		* Get a role settings within a database
		* @param token A RepoToken given at authentication
		* @param database name of database
		* @param uniqueRoleName name of the role to look for
		*/
		repo::core::model::RepoRoleSettings getRoleSettings(
			const RepoToken *token,
			const std::string &database,
			const std::string &uniqueRoleName
			);

		/**
			* Return a list of collections within the database
			* @param token A RepoToken given at authentication
			* @param databaseName database to get collections from
			* @return returns a list of collection names
			*/
		std::list<std::string> getCollections(
			const RepoToken             *token,
			const std::string     &databaseName
			);

		/**
			* Return a CollectionStats BSON containing statistics about
			* this collection
			* @param token A RepoToken given at authentication
			* @param database name of database
			* @param collection name of collection
			* @return returns a BSON object containing this information
			*/
		repo::core::model::CollectionStats getCollectionStats(
			const RepoToken            *token,
			const std::string    &database,
			const std::string    &collection);

		/**
			* Return a list of database available to the user
			* @param token A RepoToken given at authentication
			* @return returns a list of database names
			*/
		std::list<std::string> getDatabases(
			const RepoToken *token);
		//FIXME: vectors are much better than list for traversal efficiency...
		// (but they also require large continuos memory allocation should they be massive)

		/**
				* Return a DatabaseStats BSON containing statistics about
				* this database
				* @param token A RepoToken given at authentication
				* @param database name of database
				* @return returns a BSON object containing this information
				*/
		repo::core::model::DatabaseStats getDatabaseStats(
			const RepoToken *token,
			const std::string &database);

		/**
			* Return a list of projects with the database available to the user
			* @param token A RepoToken given at authentication
			* @param databases list of databases to look up
			* @return returns a list of database names
			*/
		std::map<std::string, std::list<std::string>>
			getDatabasesWithProjects(
			const RepoToken *token,
			const std::list<std::string> &databases);

		/**
			* Get a list of Admin roles from the database
			* @param token repo token to the database
			* @return returns a vector of roles
			*/
		std::list<std::string> getAdminDatabaseRoles(const RepoToken *token);

		/**
			* Get the name of the admin database
			* @param token repo token to the database
			* @return returns the name of the admin database
			*/
		std::string getNameOfAdminDatabase(const RepoToken *token);

		/**
			* Get a list of standard roles from the database
			* @param token repo token to the database
			* @return returns a vector of roles
			*/
		std::list<std::string> getStandardDatabaseRoles(const RepoToken *token);

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
		* Clean up any incomplete commits within the project
		* @param token repo token to the database
		* @param dbName name of the database
		* @param projectName name of the project
		*/
		bool cleanUp(
			const RepoToken                        *token,
			const std::string                      &dbName,
			const std::string                      &projectName
			);

		/**
			* Retrieve a RepoScene with a specific revision loaded.
			* @param token Authentication token
			* @param database the database the collection resides in
			* @param project name of the project
			* @param uuid if headRevision, uuid represents the branch id,
			*              otherwise the unique id of the revision branch
			* @param headRevision true if retrieving head revision
			* @param lightFetch fetches only the stash (or scene if stash failed),
			*                   reduce computation and memory usage (ideal for visualisation)
			* @return returns a pointer to a repoScene.
			*/
		repo::core::model::RepoScene* fetchScene(
			const RepoToken      *token,
			const std::string    &database,
			const std::string    &project,
			const std::string    &uuid = REPO_HISTORY_MASTER_BRANCH,
			const bool           &headRevision = true,
			const bool           &lightFetch = false,
			const bool           &ignoreRefScene = false,
			const bool           &skeletonFetch = false);

		/**
			* Save the files of the original model to a specified directory
			* @param token Authentication token
			* @param scene Repo Scene to save
			* @param directory directory to save into
			*/
		bool saveOriginalFiles(
			const RepoToken                    *token,
			const repo::core::model::RepoScene *scene,
			const std::string                   &directory);

		/**
		* Save the original file of the head of the project into a specified directory
		* @param token Authentication token
		* @param database name of database
		* @param project  name of project
		* @param directory directory to save into
		*/
		bool saveOriginalFiles(
			const RepoToken                    *token,
			const std::string                   &database,
			const std::string                   &project,
			const std::string                   &directory);

		/*
			*	------- Database Operations (insert/delete/update) ---------
			*/

		/**
			* Commit a scene graph
			* @param token Authentication token
			* @param scene RepoScene to commit
			* @param owner specify the owner of the scene (by default it is the user authorised to commit)
			*/
		bool commitScene(
			const RepoToken                     *token,
			repo::core::model::RepoScene        *scene,
			const std::string                   &owner = "",
			const std::string                      &tag = "",
			const std::string                      &desc = "");

		/**
		* Insert a binary file into the database (GridFS)
		* @param token Authentication token
		* @param database name of the database
		* @param collection name of the collection (it'll be saved into *.files, *.chunks)
		* @param  rawData data in the form of byte vector
		* @param mimeType the MIME type of the data (optional)
		*/
		void insertBinaryFileToDatabase(
			const RepoToken            *token,
			const std::string          &database,
			const std::string          &collection,
			const std::string          &name,
			const std::vector<uint8_t> &rawData,
			const std::string          &mimeType = "");
		/**
			* Insert a new role into the database
			* @param token Authentication token
			* @param role role info to insert
			*/
		void insertRole(
			const RepoToken                     *token,
			const repo::core::model::RepoRole   &role);

		/**
			* Insert a new user into the database
			* @param token Authentication token
			* @param user user info to insert
			*/
		void insertUser(
			const RepoToken                    *token,
			const repo::core::model::RepoUser  &user);

		/**
			* Remove a collection from the database
			* @param token Authentication token
			* @param database the database the collection resides in
			* @param collection name of the collection to drop
			* @param errMsg error message if failed
			* @return returns true upon success
			*/
		bool removeCollection(
			const RepoToken             *token,
			const std::string     &databaseName,
			const std::string     &collectionName,
			std::string			  &errMsg
			);

		/**
			* Remove a database
			* @param token Authentication token
			* @param database the database the collection resides in
			* @param errMsg error message if failed
			* @return returns true upon success
			*/
		bool removeDatabase(
			const RepoToken             *token,
			const std::string           &databaseName,
			std::string			        &errMsg
			);

		/**
			* remove a document from the database
			* NOTE: this should never be called for a bson from RepoNode family
			*       as you should never remove a node from a scene graph like this.
			* @param token Authentication token
			* @param database the database the collection resides in
			* @param collection name of the collection to drop
			* @param bson document to remove
			*/
		void removeDocument(
			const RepoToken                          *token,
			const std::string                        &databaseName,
			const std::string                        &collectionName,
			const repo::core::model::RepoBSON  &bson);

		/**
		* Remove a project from the database
		* This removes:
		*   1. all collections associated with the project,
		*   2. the project entry within project settings
		*   3. all privileges assigned to any roles, related to this project
		* @param token Authentication token
		* @param database name of the datbase
		* @param name of the project
		* @param errMsg error message if the operation fails
		* @return returns true upon success
		*/
		bool removeProject(
			const RepoToken                          *token,
			const std::string                        &databaseName,
			const std::string                        &projectName,
			std::string								 &errMsg);

		void removeProjectSettings(
			const RepoToken *token,
			const std::string &database,
			const repo::core::model::RepoProjectSettings &projectSettings)
		{
			removeDocument(token, database, REPO_COLLECTION_SETTINGS_PROJECTS, projectSettings);
		}

		void removeRoleSettings(
			const RepoToken *token,
			const std::string &database,
			const repo::core::model::RepoRoleSettings &roleSettings)
		{
			removeDocument(token, database, REPO_COLLECTION_SETTINGS_ROLES, roleSettings);
		}

		void removeRoleSettings(
			const RepoToken *token,
			const repo::core::model::RepoRole &role,
			const repo::core::model::RepoRoleSettings &settings)
		{
			removeRoleSettings(token, role.getDatabase(), settings);
		}

		/**
			* remove a user from the database
			* @param token Authentication token
			* @param role role to remove
			*/
		void removeRole(
			const RepoToken                          *token,
			const repo::core::model::RepoRole  &role);

		/**
			* remove a user from the database
			* @param token Authentication token
			* @param user user info to remove
			*/
		void removeUser(
			const RepoToken                          *token,
			const repo::core::model::RepoUser  &user);

		/**
			* Update a role on the database
			* @param token Authentication token
			* @param role role info to modify
			*/
		void updateRole(
			const RepoToken                          *token,
			const repo::core::model::RepoRole		 &role);

		/**
			* Update a user on the database
			* @param token Authentication token
			* @param user user info to modify
			*/
		void updateUser(
			const RepoToken                          *token,
			const repo::core::model::RepoUser  &user);

		/**
			* upsert a document in the database
			* NOTE: this should never be called for a bson from  RepoNode family
			*       as you should never update a node from a scene graph like this.
			* @param token Authentication token
			* @param database the database the collection resides in
			* @param collection name of the collection
			* @param bson document to update/insert
			*/
		void upsertDocument(
			const RepoToken                          *token,
			const std::string                        &databaseName,
			const std::string                        &collectionName,
			const repo::core::model::RepoBSON  &bson);

		void upsertRoleSettings(
			const RepoToken *token,
			const std::string &database,
			const repo::core::model::RepoRoleSettings &roleSettings)
		{
			upsertDocument(token, database, REPO_COLLECTION_SETTINGS_ROLES, roleSettings);
		}

		void upsertRoleSettings(
			const RepoToken *token,
			const repo::core::model::RepoRole &role,
			const repo::core::model::RepoRoleSettings &settings)
		{
			upsertRoleSettings(token, role.getDatabase(), settings);
		}

		void upsertProjectSettings(
			const RepoToken *token,
			const std::string &database,
			const repo::core::model::RepoProjectSettings &projectSettings)
		{
			upsertDocument(token, database, REPO_COLLECTION_SETTINGS, projectSettings);
		}

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
		* Commit asset bundle buffers
		* @param database database name
		* @param project project name
		* @param buffers web buffers
		*/
		bool commitAssetBundleBuffers(
			const RepoController::RepoToken *token,
			repo::core::model::RepoScene    *scene,
			const repo_web_buffers_t &buffers);

		/**
			* Create a federated scene with the given scene collections
			* @param fedMap a map of reference scene and transformation from root where the scene should lie
			* @return returns a constructed scene graph with the reference.
			*/
		repo::core::model::RepoScene* createFederatedScene(
			const std::map<repo::core::model::TransformationNode, repo::core::model::ReferenceNode> &fedMap);

		/**
		* Generate and commit a GLTF encoding for the given scene
		* This requires the stash to have been generated already
		* @param token token for authentication
		* @param scene the scene to generate the gltf encoding from
		* @return returns true upon success
		*/
		bool generateAndCommitGLTFBuffer(
			const RepoToken                               *token,
			repo::core::model::RepoScene            *scene);

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
		* Generate a GLTF encoding in the form of a buffer for the given scene
		* This requires the stash to have been generated already
		* @param scene the scene to generate the gltf encoding from
		* @return returns a buffer in the form of a byte vector
		*/
		repo_web_buffers_t generateGLTFBuffer(
			repo::core::model::RepoScene *scene);

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
		repo_web_buffers_t generateSRCBuffer(
			repo::core::model::RepoScene *scene);

		/**
			* Get a string of supported file formats for file export
			* @return returns a string with list of supported file formats
			*/
		std::string getSupportedExportFormats();

		/**
			* Get a string of supported file formats for file import
			* @return returns a string with list of supported file formats
			*/
		std::string getSupportedImportFormats();

		/**
		* Initialise Assetbuffer by generating all the required work to allow
		* the user to generate an asset bundle
		* @param scene the scene to generate the src encoding from
		* @param json  (output) generated json files generated by this initilaisation
		* @return returns mesh nodes reorganised for bundling
		*/
		std::vector<std::shared_ptr<repo::core::model::MeshNode>> initialiseAssetBuffer(
			const RepoController::RepoToken                    *token,
			repo::core::model::RepoScene *scene,
			std::unordered_map<std::string, std::vector<uint8_t>> &jsonFiles,
			repo::core::model::RepoUnityAssets &unityAssets,
			std::vector<std::vector<uint16_t>> &serialisedFaceBuf,
			std::vector<std::vector<std::vector<float>>> &idMapBuf,
			std::vector<std::vector<std::vector<repo_mesh_mapping_t>>> &meshMappings);

		/**
			* Load a Repo Scene from a file
			* @param filePath path to file
			* @param apply transformation reduction (default: true)
			* @param rotateModel rotate model by 270degrees on x (default: false)
			* @param config import settings(optional)
			* @return returns a pointer to Repo Scene upon success
			*/
		repo::core::model::RepoScene* loadSceneFromFile(
			const std::string &filePath,
			uint8_t           &err,
			const bool &applyReduction = true,
			const bool &rotateModel = false,
			const repo::manipulator::modelconvertor::ModelImportConfig *config
			= nullptr);

		/**
			* Load metadata from a file
			* @param filePath path to file
			* @return returns a set of metadata nodes read from the file
			*/
		repo::core::model::RepoNodeSet loadMetadataFromFile(
			const std::string &filePath,
			const char        &delimiter = ',');

		/**
			* Save a Repo Scene to file
			* @param filePath path to file
			* @param scene scene to export
			* @return returns true upon success
			*/
		bool saveSceneToFile(
			const std::string &filePath,
			const repo::core::model::RepoScene* scene);

		/*
			*	------------- Optimizations --------------
			*/

		/**
		* Generate and commit stash graph (multipart viewing graph)
		* The generated graph will be added into the scene provided
		* also commited to the database/project set within the scene
		* @param token database token
		* @param scene scene to optimise
		* @param return true upon success
		*/
		bool generateAndCommitStashGraph(
			const RepoToken              *token,
			repo::core::model::RepoScene* scene
			);

		/**
		* Get a hierachical spatial partitioning in form of a tree
		* @param scene scene to partition
		* @param maxDepth max partitioning depth
		*/
		std::shared_ptr<repo_partitioning_tree_t>
			getScenePartitioning(
			const repo::core::model::RepoScene *scene,
			const uint32_t                     &maxDepth = 8
			);

		/**
			* Reduce redundant transformations from the scene
			* to optimise the graph
			* @param token to load full scene from database if required
			(if not required, a nullptr can be passed in)
			* @param scene RepoScene to optimize
			*/
		void reduceTransformations(
			const RepoToken              *token,
			repo::core::model::RepoScene *scene);

		/*
		*	------------- 3D Diff --------------
		*/

		/**
		* Compare 2 scenes via IDs.
		* @param token to load full scene from database if required
		*		(if not required, a nullptr can be passed in)
		* @param base base scene to compare against
		* @param compare scene to compare base scene against
		* @param baseResults Diff results in the perspective of base
		* @param compResults Diff results in the perspective of compare
		* @param repo::DiffMode mode to use on comparison
		*/
		void compareScenes(
			const RepoToken                     *token,
			repo::core::model::RepoScene        *base,
			repo::core::model::RepoScene        *compare,
			repo_diff_result_t &baseResults,
			repo_diff_result_t &compResults,
			const repo::DiffMode       &diffMode
			);

		/**
			* Compare 2 scenes via IDs.
			* @param token to load full scene from database if required
			*		(if not required, a nullptr can be passed in)
			* @param base base scene to compare against
			* @param compare scene to compare base scene against
			* @param baseResults Diff results in the perspective of base
			* @param compResults Diff results in the perspective of compare
			*/
		void compareScenesByIDs(
			const RepoToken                     *token,
			repo::core::model::RepoScene        *base,
			repo::core::model::RepoScene        *compare,
			repo_diff_result_t                          &baseResults,
			repo_diff_result_t                          &compResults
			)
		{
			compareScenes(token, base, compare, baseResults, compResults, repo::DiffMode::DIFF_BY_ID);
		}

		/**
		* Compare 2 scenes via Names.
		* @param token to load full scene from database if required
		*		(if not required, a nullptr can be passed in)
		* @param base base scene to compare against
		* @param compare scene to compare base scene against
		* @param baseResults Diff results in the perspective of base
		* @param compResults Diff results in the perspective of compare
		*/
		void compareScenesByNames(
			const RepoToken                     *token,
			repo::core::model::RepoScene        *base,
			repo::core::model::RepoScene        *compare,
			repo_diff_result_t &baseResults,
			repo_diff_result_t &compResults
			)
		{
			compareScenes(token, base, compare, baseResults, compResults, repo::DiffMode::DIFF_BY_NAME);
		}

		/*
		*	------------- Statistics --------------
		*/
		/**
		* Generate database statistics and print the result in the given filepath
		* @params outputFilePath
		*/
		void getDatabaseStatistics(
			const RepoToken   *token,
			const std::string &outputFilePath);

		/**
		* Get a list of users and print the result in the given filepath
		* @params outputFilePath
		*/
		void getUserList(
			const RepoToken   *token,
			const std::string &outputFilePath);

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
