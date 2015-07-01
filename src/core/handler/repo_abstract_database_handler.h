/**
 * AbstractDatabaseHandler.h
 *
 *  Created on: 26 Jun 2015
 *      Author: Carmen
 */

#ifndef SRC_CORE_HANDLER_ABSTRACTDATABASEHANDLER_H_
#define SRC_CORE_HANDLER_ABSTRACTDATABASEHANDLER_H_

#include <list>
#include <map>
#include <string>

namespace repo{
	namespace core{
		namespace handler {
			class AbstractDatabaseHandler {
			public:
				/**
				 * A Deconstructor 
				 */
				~AbstractDatabaseHandler(){};

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


			protected:
				/**
				* Default constructor
				*/
				AbstractDatabaseHandler(){};
			};

		}
	}
}
#endif /* SRC_CORE_HANDLER_ABSTRACTDATABASEHANDLER_H_ */
