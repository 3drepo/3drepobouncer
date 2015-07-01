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
* repo_bson.h
*
*  Created on: 29 Jun 2015
*      Author: Carmen
*
*  Base Bson object. Generic bson that is inherited by others.
*/

#ifndef REPO_BSON_H
#define REPO_BSON_H

#include "../repo_model_global.h"

////------------------------------------------------------------------------------
//#if defined(_WIN32) || defined(_WIN64)
//#include <WinSock2.h>
//#include <Windows.h>
//#endif
// FIXME: this should be purely a bson representation. 
// Not including dbclient.h to make sure we have no extra access to mongo functionality

#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#include <Windows.h>

#define strcasecmp _stricmp
#endif
#include <mongo/bson/bson.h> 
////------------------------------------------------------------------------------
////#include "../conversion/repo_transcoder_bson.h"
////#include "../conversion/repo_transcoder_string.h"
//////------------------------------------------------------------------------------
//


namespace repo {
	namespace core {
		namespace model {
			namespace bson {

					enum RepoBSONCommands { CREATE, DROP, UPDATE };

					//TODO: Eventually we should inherit from a generic BSON object. 
					//work seems to have been started in here:https://github.com/jbenet/bson-cpp
					//alternatively we can use a c++ wrapper on https://github.com/mongodb/libbson
					class RepoBSON : public mongo::BSONObj
					{

					public:

						/**
						 * Default empty constructor.
						 */
						RepoBSON() : mongo::BSONObj() {}

						/** 
						 * Constructor from Mongo BSON object.
						 * @param mongo BSON object
						 */
						RepoBSON(const mongo::BSONObj &obj) : mongo::BSONObj(obj) {}

						/**
						* Constructor from Mongo BSON object builder.
						* @param mongo BSON object builder
						*/
						RepoBSON(mongo::BSONObjBuilder &builder) : mongo::BSONObj(builder.obj()) {}

						/**
						* Default empty deconstructor.
						*/
						~RepoBSON() {}

//						//--------------------------------------------------------------------------
//
//						/*!
//						* Adds all fields from the given object to this bson if empty. Returns
//						* true if successful, and false otherwise (eg when non-empty or not owned).
//						*/
//						int addFields(mongo::BSONObj &obj);
//
//						//! Returns a new full (and owned) copy of the object.
//						inline RepoBSON copy() const { return RepoBSON(mongo::BSONObj::copy()); }
//
//						/*!
//						* Returns a drop db.runCommand bson object by _id field. If _id is not set,
//						* returns empty object.
//						*
//						* See http://docs.mongodb.org/manual/reference/command/delete/#dbcmd.delete
//						*/
//						virtual RepoBSON drop(const std::string &collection) const;
//
//						//--------------------------------------------------------------------------
//						//
//						// Getters
//						//
//						//--------------------------------------------------------------------------
//
//						//! Given an array, returns a list of pairs stored as given labels if any.
//						static std::list<std::pair<std::string, std::string> > getArrayStringPairs(
//							const mongo::BSONElement &arrayElement,
//							const std::string &fstLabel,
//							const std::string &sndLabel);
//
//						//! Returns an embedded element by label, EOO if not present.
//						mongo::BSONElement getEmbeddedElement(
//							const std::string &fstLevelLabel,
//							const std::string &sndLevelLabel) const;
//
//						//! Returns true if it is valid and not empty, false otherwise.
//						bool isOk() const { return isValid() && !isEmpty(); }
//
//						static mongo::BSONArray toArray(const std::list<std::pair<std::string, std::string> > &list,
//							const std::string &fstLabel,
//							const std::string &sndLabel);
//
					}; // end 
			}// end namespace bson
		}// end namespace model
	} // end namespace core
} // end namespace repo

#endif // REPO_BSON_H
