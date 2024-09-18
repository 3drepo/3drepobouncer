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
* Repo Manipulator which handles all the manipulation
*/

#pragma once
#include <map>
#include <string>

#include "../core/model/bson/repo_node_reference.h"
#include "../core/model/bson/repo_node_transformation.h"
#include "../core/model/bson/repo_node_mesh.h"
#include "../core/model/collection/repo_scene.h"
#include "../lib/repo_config.h"
#include "modelconvertor/export/repo_model_export_web.h"
#include "modelconvertor/import/repo_model_import_config.h"
#include "modelutility/spatialpartitioning/repo_spatial_partitioner_abstract.h"

namespace repo {
	namespace manipulator {
		class RepoManipulator
		{
		public:
			RepoManipulator();
			~RepoManipulator();

			/**
			* Commit a scene graph
			* @param databaseAd mongo database address:port
			* @param cred user credentials in bson form
			* @param scene scene to commit
			* @param owner specify the owner of the scene (by default it is the user authorised to commit)
			*/
			uint8_t commitScene(
				const std::string                     &databaseAd,
				const repo::core::model::RepoBSON     *cred,
				const std::string                     &bucketName,
				const std::string                     &bucketRegion,
				repo::core::model::RepoScene          *scene,
				const std::string                     &owner = "",
				const std::string                     &tag = "",
				const std::string                     &desc = "",
				const repo::lib::RepoUUID             &revId = repo::lib::RepoUUID::createUUID());

			/**
			* Create a bson object storing user credentials
			* @param databaseAd mongo database address:port
			* @param username user name
			* @param password password of the user
			* @param pwDigested is the password provided in digested form (default: false)
			* @return returns true upon success
			*/
			repo::core::model::RepoBSON* createCredBSON(
				const std::string &databaseAd,
				const std::string &username,
				const std::string &password,
				const bool        &pwDigested);

			/**
			* Create a federated scene with the given scene collections
			* @param fedMap a map of reference scene and transformation from root where the scene should lie
			* @return returns a constructed scene graph with the reference.
			*/
			repo::core::model::RepoScene* createFederatedScene(
				const std::map<repo::core::model::ReferenceNode, std::string> &fedMap);

			/**
			* Disconnects from the given database host
			* @param databaseAd database address:port
			*/
			void disconnectFromDatabase(const std::string &databaseAd);

			/**
			* Retrieve a RepoScene with a specific revision loaded.
			* @param databaseAd mongo database address:port
			* @param cred user credentials in bson form
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
				const std::string                             &databaseAd,
				const repo::core::model::RepoBSON             *cred,
				const std::string                             &database,
				const std::string                             &collection,
				const repo::lib::RepoUUID                                &uuid,
				const bool                                    &headRevision = false,
				const bool                                    &ignoreRefScene = false,
				const bool                                    &skeletonFetch = false,
				const std::vector<repo::core::model::RevisionNode::UploadStatus> &includeStatus = {});

			/**
			* Retrieve all RepoScene representations given a partially loaded scene.
			* @param databaseAd mongo database address:port
			* @param cred user credentials in bson form
			* @param scene scene to fully load
			*/

			void fetchScene(
				const std::string                         &databaseAd,
				const repo::core::model::RepoBSON         *cred,
				repo::core::model::RepoScene              *scene,
				const bool                                &ignoreRefScene = false,
				const bool                                &skeletonFetch = false);

			/**
			* Generate and commit scene's selection tree in JSON format
			* The generated data will be
			* also commited to the database/project set within the scene
			* @param databaseAd mongo database address:port
			* @param cred user credentials in bson form
			* @param scene scene to optimise
			* @param return true upon success
			*/
			bool generateAndCommitSelectionTree(
				const std::string                     &databaseAd,
				const repo::core::model::RepoBSON     *cred,
				const std::string                     &bucketName,
				const std::string                     &bucketRegion,
				repo::core::model::RepoScene          *scene
			);

			/**
			* Generate and commit a `exType` encoding for the given scene
			* This requires the stash to have been generated already
			* @param databaseAd database address:portdatabase
			* @param cred user credentials in bson form
			* @param scene the scene to generate the src encoding from
			* @param buffers buffers that are geneated and commited
			* @param exType the type of export it is
			* @return returns true upon success
			*/
			bool generateAndCommitWebViewBuffer(
				const std::string                     &databaseAd,
				const repo::core::model::RepoBSON     *cred,
				const std::string                     &bucketName,
				const std::string                     &bucketRegion,
				repo::core::model::RepoScene          *scene,
				repo_web_buffers_t                    &buffers,
				const modelconvertor::WebExportType   &exType);

			/**
			* Generate and commit RepoBundles for the given scene
			* @param databaseAd database address:portdatabase
			* @param cred user credentials in bson form
			* @param scene the scene to generate the repobundles encoding from
			* @return returns true upon success
			*/

			bool generateAndCommitRepoBundlesBuffer(
				const std::string& databaseAd,
				const repo::core::model::RepoBSON* cred,
				const std::string& bucketName,
				const std::string& bucketRegion,
				repo::core::model::RepoScene* scene);

			/**
			* Generate and commit a SRC encoding for the given scene
			* This requires the stash to have been generated already
			* @param databaseAd database address:portdatabase
			* @param cred user credentials in bson form
			* @param scene the scene to generate the src encoding from
			* @return returns true upon success
			*/
			bool generateAndCommitSRCBuffer(
				const std::string                     &databaseAd,
				const repo::core::model::RepoBSON     *cred,
				const std::string                     &bucketName,
				const std::string                     &bucketRegion,
				repo::core::model::RepoScene          *scene);

			/**
			* Generate a SRC encoding in the form of a buffer for the given scene
			* This requires the stash to have been generated already
			* @param scene the scene to generate the src encoding fro
			* @return returns a buffer in the form of a byte vector mapped to its filename
			*/
			repo_web_buffers_t generateSRCBuffer(
				repo::core::model::RepoScene *scene);

			/**
			* Generate a stash graph for the given scene and populate it
			* into the given scene
			* @param scene scene to generate stash graph for
			* @return returns true upon success
			*/
			bool generateStashGraph(
				repo::core::model::RepoScene              *scene
			);

			/**
			* Retrieve documents from a specified collection
			* due to limitations of the transfer protocol this might need
			* to be called multiple times, utilising the skip index to skip
			* the first n items.
			* @param databaseAd mongo database address:port
			* @param cred user credentials in bson form
			* @param collection name of collection
			* @param skip specify how many documents to skip
			* @param limit limits the max amount of documents to retrieve (0 = no limit)
			* @return list of RepoBSONs representing the documents
			*/
			std::vector<repo::core::model::RepoBSON>
				getAllFromCollectionTailable(
					const std::string                             &databaseAd,
					const repo::core::model::RepoBSON             *cred,
					const std::string                             &database,
					const std::string                             &collection,
					const uint64_t                                &skip = 0,
					const uint32_t                                &limit = 0);

			/**
			* Retrieve documents from a specified collection
			* due to limitations of the transfer protocol this might need
			* to be called multiple times, utilising the skip index to skip
			* the first n items.
			* @param databaseAd mongo database address:port
			* @param cred user credentials in bson form
			* @param collection name of collection
			* @param fields fields to get back from the database
			* @param sortField field to sort upon
			* @param sortOrder 1 ascending, -1 descending
			* @param skip specify how many documents to skip
			* @param limit limits the max amount of documents to retrieve (0 = no limit)
			* @return list of RepoBSONs representing the documents
			*/
			std::vector<repo::core::model::RepoBSON>
				getAllFromCollectionTailable(
					const std::string                             &databaseAd,
					const repo::core::model::RepoBSON             *cred,
					const std::string                             &database,
					const std::string                             &collection,
					const std::list<std::string>				  &fields,
					const std::string							  &sortField = std::string(),
					const int									  &sortOrder = -1,
					const uint64_t                                &skip = 0,
					const uint32_t                                &limit = 0);

			/**
			* Get a hierachical spatial partitioning in form of a tree
			* @param scene scene to partition
			* @param maxDepth max partitioning depth
			*/
			std::shared_ptr<repo_partitioning_tree_t> getScenePartitioning(
				const repo::core::model::RepoScene *scene,
				const uint32_t                     &maxDepth = 8
			);

			/**
			* Connect to all services required as per config provided
			* @param errMsg error message if failed
			* @param config RepoConfig instance containing all connection information
			* @param nDbConnections number of parallel database connections
			*/
			bool init(
				std::string       &errMsg,
				const lib::RepoConfig  &config,
				const int            &nDbConnections
			);

			/**
			* Check if a given scene is VR Enabled
			* @param databaseAd database address:portdatabase
			* @param cred user credentials in bson form
			* @param scene the given scene
			* @return returns true if the scene is VR enabled
			*/
			bool isVREnabled(
				const std::string                       &databaseAd,
				const repo::core::model::RepoBSON       *cred,
				const repo::core::model::RepoScene      *scene) const;

			/**
			* Load metadata from a file
			* @param filePath path to file
			* @param delimiter
			* @return returns a pointer to Repo Scene upon success
			*/
			repo::core::model::RepoNodeSet
				loadMetadataFromFile(
					const std::string &filePath,
					const char        &delimiter = ',');

			/**
			* Load a Repo Scene from a file
			* @param filePath path to file
			* @param msg error message if it fails
			* @param apply transformation reduction optimizer (default = true)
			* @param rotateModel rotate model by 270degrees on x (default: false)
			* @param config import config (optional)
			* @return returns a pointer to Repo Scene upon success
			*/
			repo::core::model::RepoScene*
				loadSceneFromFile(
					const std::string                                          &filePath,
					uint8_t													   &error,
					const repo::manipulator::modelconvertor::ModelImportConfig &config);

			/**
			* Initialise the image of a drawing revision from its rfile
			*/
			void processDrawingRevision(
				const std::string databaseAd,
				const std::string& teamspace,
				const lib::RepoUUID revision,
				uint8_t& error,
				const std::string &imagePath);

			/**
			* Reduce redundant transformations from the scene
			* to optimise the graph
			* @param scene RepoScene to optimize
			* @param gType graph type to diff (default: unoptimised)
			*/
			void reduceTransformations(
				repo::core::model::RepoScene                  *scene,
				const repo::core::model::RepoScene::GraphType &gType
				= repo::core::model::RepoScene::GraphType::DEFAULT);

			/**
			* Save the files of the original model to a specified directory
			* @param databaseAd mongo database address:port
			* @param cred user credentials in bson form
			* @param scene Repo Scene to save
			* @param directory directory to save into
			*/
			bool saveOriginalFiles(
				const std::string                    &databaseAd,
				const repo::core::model::RepoBSON	 *cred,
				const repo::core::model::RepoScene   *scene,
				const std::string                    &directory);

			/**
			* Save the files of the original model to a specified directory
			* @param databaseAd mongo database address:port
			* @param cred user credentials in bson form
			* @param database name of database
			* @param project name of project
			* @param directory directory to save into
			*/
			bool saveOriginalFiles(
				const std::string                    &databaseAd,
				const repo::core::model::RepoBSON	 *cred,
				const std::string                    &database,
				const std::string                    &project,
				const std::string                    &directory);

			/**
			* Save a Repo Scene to file
			* @param filePath path to file
			* @param scene scene to export
			* @return returns true upon success
			*/
			bool saveSceneToFile(
				const std::string &filePath,
				const repo::core::model::RepoScene* scene);

		private:

			/**
			* Connect to the given database address/port and authenticat the user using Admin database
			* @param errMsg error message if the function returns false
			* @param address mongo database address
			* @param port port number
			* @param maxConnections maxmimum number of concurrent connections allowed to the database
			* @param username user name
			* @param password password of the user
			* @param pwDigested is the password provided in digested form (default: false)
			* @return returns true upon success
			*/
			bool connectAndAuthenticateWithAdmin(
				std::string       &errMsg,
				const std::string &address,
				const uint32_t    &port,
				const uint32_t    &maxConnections,
				const std::string &username,
				const std::string &password,
				const bool        &pwDigested = false
			);
			/**
			* Connect to the given database address/port and authenticat the user using Admin database
			* @param errMsg error message if the function returns false
			* @param connString connection string to connect to mongo
			* @param maxConnections maxmimum number of concurrent connections allowed to the database
			* @param username user name
			* @param password password of the user
			* @param pwDigested is the password provided in digested form (default: false)
			* @return returns true upon success
			*/
			bool connectAndAuthenticateWithAdmin(
				std::string       &errMsg,
				const std::string &connString,
				const uint32_t    &maxConnections,
				const std::string &username,
				const std::string &password,
				const bool        &pwDigested = false
			);
		};
	}
}
