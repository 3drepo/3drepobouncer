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
 * Abstract database handler which all database handler needs to inherit from
 */

#pragma once

#include <list>
#include <map>
#include <string>

#include "../model/bson/repo_bson.h"

namespace repo{
	namespace core{
		namespace handler {
			class AbstractDatabaseHandler {
			public:
				/**
				 * A Deconstructor 
				 */
				~AbstractDatabaseHandler(){};

				/*
				*	------------- Authentication (insert/delete/update) --------------
				*/

				/**
				* Authenticates the user to the database
				* @param username in the database
				* @param password for this username
				*/
				virtual bool authenticate(const std::string& username, const std::string& plainTextPassword)=0;

				/** Authenticates the user on a given database for a specific access.
				* @param name of database
				* @param username
				* @param password
				* @param if the password is digested
				*/
				virtual bool authenticate(
					const std::string& database, const std::string& username,
					const std::string& password, bool isPasswordDigested = false)=0;

				/*
				*	------------- Database info lookup --------------
				*/

				/**
				* Get a list of all available collections
				*/
				virtual std::list<std::string> getCollections(const std::string &database)=0;


				/**
				* Get a list of all available databases, alphabetically sorted by default.
				* @param sort the database
				* @return returns a list of database names
				*/
				virtual std::list<std::string> getDatabases(const bool &sorted = true)=0;

				/** get the associated projects for the list of database.
				* @param list of database
				* @return returns a map of database -> list of projects
				*/
				virtual std::map<std::string, std::list<std::string> > getDatabasesWithProjects(
					const std::list<std::string> &databases, const std::string &projectExt) = 0;

				/**
				* Get a list of projects associated with a given database (aka company account).
				* @param list of database
				* @param extension that indicates it is a project (.scene)
				* @return list of projects for the database
				*/
				virtual std::list<std::string> getProjects(const std::string &database, const std::string &projectExt)=0;


				/*
				*	------------- Database operations (insert/delete/update) --------------
				*/

				/**
				* Insert a single document in database.collection
				* @param database name
				* @param collection name
				* @param document to insert
				*/
				virtual void insertDocument(
					const std::string &database,
					const std::string &collection,
					const repo::core::model::bson::RepoBSON &obj)=0;

				/*
				*	------------- Query operations --------------
				*/


				/**
				* Given a list of unique IDs, find all the documents associated to them
				* @param name of database
				* @param name of collection
				* @param array of uuids in a BSON object
				* @return a vector of RepoBSON objects associated with the UUIDs given
				*/
				virtual std::vector<repo::core::model::bson::RepoBSON> findAllByUniqueIDs(
					const std::string& database,
					const std::string& collection,
					const repo::core::model::bson::RepoBSON& uuid) = 0;

				/**
				*Retrieves the first document matching given Shared ID (SID), sorting is descending
				* (newest first)
				* @param name of database
				* @param name of collectoin
				* @param share id
				* @param field to sort by
				* @return returns the first matching bson object
				*/
				virtual repo::core::model::bson::RepoBSON findOneBySharedID(
					const std::string& database,
					const std::string& collection,
					const repo_uuid& uuid,
					const std::string& sortField)=0;

				/**
				*Retrieves the document matching given Unique ID (SID), sorting is descending
				* @param name of database
				* @param name of collectoin
				* @param share id
				* @return returns the matching bson object
				*/
				virtual mongo::BSONObj findOneByUniqueID(
					const std::string& database,
					const std::string& collection,
					const repo_uuid& uuid) = 0;


			protected:
				/**
				* Default constructor
				*/
				AbstractDatabaseHandler(){};
			};

		}
	}
}
