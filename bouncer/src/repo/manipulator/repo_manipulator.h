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
#include <string>
#include "../core/handler/repo_database_handler_mongo.h"
#include "../core/model/collection/repo_scene.h"
#include "modelconvertor/import/repo_model_import_assimp.h"


namespace repo{
	namespace manipulator{
		class RepoManipulator
		{

		public:
			RepoManipulator();
			~RepoManipulator();


			/**
			* Connect to the given database address/port and authenticat the user
			* @param errMsg error message if the function returns false
			* @param address mongo database address
			* @param port port number
			* @param maxConnections maxmimum number of concurrent connections allowed to the database
			* @param dbName database name to authenticate against
			* @param username user name
			* @param password password of the user
			* @param pwDigested is the password provided in digested form (default: false)
			* @return returns true upon success
			*/
			bool connectAndAuthenticate(
				std::string       &errMsg,
				const std::string &address,
				const uint32_t         &port,
				const uint32_t    &maxConnections,
				const std::string &dbName,
				const std::string &username,
				const std::string &password,
				const bool        &pwDigested = false
				);

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
			* Commit a scene graph
			* @param databaseAd mongo database address:port
			* @param cred user credentials in bson form
			* @param scene scene to commit
			*/
			void commitScene(
				const std::string                             &databaseAd,
				const repo::core::model::RepoBSON 	  *cred,
				repo::core::model::RepoScene           *scene);


			/**
			* Create a bson object storing user credentials
			* @param databaseAd mongo database address:port
			* @param username user name
			* @param password password of the user
			* @param pwDigested is the password provided in digested form (default: false)
			* @return returns true upon success
			*/
			core::model::RepoBSON* createCredBSON(
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
				const std::map<repo::core::model::TransformationNode, repo::core::model::ReferenceNode> &fedMap);

			/**
			* Create a map scene with the given map
			* @param mapNode the map node to create the scene with
			* @return returns a constructed scene graph with the map.
			*/
			repo::core::model::RepoScene* createMapScene(
				const repo::core::model::MapNode &mapNode);

			/**
			* Count the number of documents within the collection
			* @param databaseAd mongo database address:port
			* @param cred user credentials in bson form
			* @param database name of database
			* @param collection name of collection
			* @return number of documents within the specified collection
			*/
			uint64_t countItemsInCollection(
				const std::string                             &databaseAd,
				const repo::core::model::RepoBSON*	  cred,
				const std::string                             &database,
				const std::string                             &collection,
				std::string                                   &errMsg
			);

			/**
			* Remove a collection from the database
			* @param databaseAd mongo database address:port
			* @param cred user credentials in bson form
			* @param database the database the collection resides in
			* @param collection name of the collection to drop
			* @param errMsg error message if failed
			* @return returns true upon success
			*/
			bool dropCollection(
				const std::string                             &databaseAd,
				const repo::core::model::RepoBSON*	  cred,
				const std::string                             &databaseName,
				const std::string                             &collectionName,
				std::string			                          &errMsg
			);

			/**
			* Remove a database from the database instance
			* @param databaseAd mongo database address:port
			* @param cred user credentials in bson form
			* @param database the database the collection resides in
			* @param errMsg error message if failed
			* @return returns true upon success
			*/
			bool dropDatabase(
				const std::string                             &databaseAd,
				const repo::core::model::RepoBSON*	  cred,
				const std::string                             &databaseName,
				std::string			                          &errMsg
			);

			/**
			* Get a list of all available databases, alphabetically sorted by default.
			* @param databaseAd mongo database address:port
			* @param cred user credentials in bson form
			* @return returns a list of database names
			*/
			std::list<std::string> fetchDatabases(
				const std::string                             &databaseAd,
				const repo::core::model::RepoBSON*	  cred
				);


			/**
			* Get a list of all available collections.
			* @param databaseAd mongo database address:port
			* @param cred user credentials in bson form
			* @param database database name
			* @return a list of collection names
			*/
			std::list<std::string> fetchCollections(
				const std::string                             &databaseAd,
				const repo::core::model::RepoBSON*	  cred,
				const std::string                             &database
				);

			/**
			* Retrieve a RepoScene with a specific revision loaded.
			* @param databaseAd mongo database address:port
			* @param cred user credentials in bson form
			* @param database the database the collection resides in
			* @param project name of the project
			* @param uuid if headRevision, uuid represents the branch id,
			*              otherwise the unique id of the revision branch
			* @param headRevision true if retrieving head revision
			* @return returns a pointer to a repoScene.
			*/
			repo::core::model::RepoScene* fetchScene(
				const std::string                             &databaseAd,
				const repo::core::model::RepoBSON*	  cred,
				const std::string                             &database,
				const std::string                             &collection,
				const repoUUID                                &uuid,
				const bool                                    &headRevision = false);

			/**
			* Retrieve documents from a specified collection
			* due to limitations of the transfer protocol this might need
			* to be called multiple times, utilising the skip index to skip
			* the first n items.
			* @param databaseAd mongo database address:port
			* @param cred user credentials in bson form
			* @param collection name of collection
			* @param skip specify how many documents to skip
			* @return list of RepoBSONs representing the documents
			*/
			std::vector<repo::core::model::RepoBSON>
				getAllFromCollectionTailable(
				const std::string                             &databaseAd,
				const repo::core::model::RepoBSON*	  cred,
				const std::string                             &database,
				const std::string                             &collection,
				const uint64_t                                &skip=0);

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
			* @return list of RepoBSONs representing the documents
			*/
			std::vector<repo::core::model::RepoBSON>
				getAllFromCollectionTailable(
				const std::string                             &databaseAd,
				const repo::core::model::RepoBSON*	  cred,
				const std::string                             &database,
				const std::string                             &collection,
				const std::list<std::string>				  &fields,
				const std::string							  &sortField = std::string(),
				const int									  &sortOrder = -1,
				const uint64_t                                &skip = 0);

			/**
			* Get the collection statistics of the given collection
			* @param databaseAd mongo database address:port
			* @param cred user credentials in bson form
			* @param database Name of database
			* @param collection Name of collection
			* @param errMsg error message when error occurs
			* @return returns a bson object with statistical info.
			*/
			repo::core::model::CollectionStats getCollectionStats(
				const std::string                             &databaseAd,
				const repo::core::model::RepoBSON*	  cred,
				const std::string                             &database,
				const std::string                             &collection,
				std::string	                                  &errMsg
			);


			/**
			* Return a list of projects with the database available to the user
			* @param databaseAd mongo database address:port
			* @param cred user credentials in bson form
			* @param databases list of databases to look up
			* @return returns a list of database names
			*/
			std::map<std::string, std::list<std::string>>
				getDatabasesWithProjects(
					const std::string                             &databaseAd,
					const repo::core::model::RepoBSON*	  cred,
					const std::list<std::string> &databases);

			/**
			* Get a list of admin roles from the database
			* @param databaseAd database address:portdatabase
			* @return returns a vector of roles
			*/
			std::list<std::string> getAdminDatabaseRoles(
				const std::string                             &databaseAd);

			/**
			* Get a list of standard roles from the database
			* @param databaseAd database address:portdatabase
			* @return returns a vector of roles
			*/
			std::list<std::string> getStandardDatabaseRoles(
				const std::string                             &databaseAd);

			/**
			* Get the name of the admin database
			* note it is done this way to support different admin database names
			* for different handlers (which may be of different database types)
			* @param databaseAd database address:portdatabase
			* @return returns the name of the admin database
			*/
			std::string getNameOfAdminDatabase(
				const std::string                             &databaseAd) const;

			/**
			* Insert a new user into the database
			* @param databaseAd database address:portdatabase
			* @param cred user credentials in bson form
			* @param user user info to insert
			*/
			void insertUser(
				const std::string                             &databaseAd,
				const repo::core::model::RepoBSON*	  cred,
				const repo::core::model::RepoUser       &user);


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
			* @param msg error message if it fails (optional)
			* @param config import config (optional)
			* @return returns a pointer to Repo Scene upon success
			*/
			repo::core::model::RepoScene*
				loadSceneFromFile(
				const std::string &filePath,
				      std::string &msg,
			    const repo::manipulator::modelconvertor::ModelImportConfig *config
					  = nullptr);

			/**
			* remove a document from the database
			* NOTE: this should never be called for a RepoNode family
			*       as you should never remove a node from a scene graph like this.
			* @param databaseAd mongo database address:port
			* @param cred user credentials in bson form
			* @param database the database the collection resides in
			* @param collection name of the collection to drop
			* @param bson document to remove
			*/
			void removeDocument(
				const std::string                             &databaseAd,
				const repo::core::model::RepoBSON*	  cred,
				const std::string                             &databaseName,
				const std::string                             &collectionName,
				const repo::core::model::RepoBSON       &bson);

			/**
			* remove a user from the database
			* @param databaseAd mongo database address:port
			* @param cred user credentials in bson form
			* @param user user info to remove
			*/
			void removeUser(
				const std::string                             &databaseAd,
				const repo::core::model::RepoBSON*	  cred,
				const repo::core::model::RepoUser       &user);

			/**
			* Save the files of the original model to a specified directory
			* @param databaseAd mongo database address:port
			* @param cred user credentials in bson form
			* @param scene Repo Scene to save
			* @param directory directory to save into
			*/
			void saveOriginalFiles(
				const std::string                    &databaseAd,
				const repo::core::model::RepoBSON	 *cred,
				const repo::core::model::RepoScene   *scene,
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

			/**
			* Update a user on the database
			* @param databaseAd mongo database address:port
			* @param cred user credentials in bson form
			* @param user user info to modify
			*/
			void updateUser(
				const std::string                             &databaseAd,
				const repo::core::model::RepoBSON*	  cred,
				const repo::core::model::RepoUser       &user);
			/**
			* upsert a document in the database
			* NOTE: this should never be called for a bson from RepoNode family
			*       as you should never update a node from a scene graph like this.
			* @param databaseAd mongo database address:port
			* @param cred user credentials in bson form
			* @param database the database the collection resides in
			* @param collection name of the collection
			* @param bson document to update/insert
			*/
			void upsertDocument(
				const std::string                             &databaseAd,
				const repo::core::model::RepoBSON*	  cred,
				const std::string                             &databaseName,
				const std::string                             &collectionName,
				const repo::core::model::RepoBSON       &bson);

		};
	}
}
