/*
 * AbstractDatabaseHandler.h
 *
 *  Created on: 26 Jun 2015
 *      Author: Carmen
 */

#ifndef SRC_CORE_HANDLER_ABSTRACTDATABASEHANDLER_H_
#define SRC_CORE_HANDLER_ABSTRACTDATABASEHANDLER_H_

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
