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

#pragma once

#include "../repo_model_global.h"
#include "../repo_node_properties.h"
#include "../repo_node_utils.h"

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

						/**
						* Overload of getField function to retreve repoUUID
						* @param name of the field
						* @return returns a repoUUID from that field
						*/
						repoUUID getUUIDField(std::string label);

						/**
						* Get an array of fields given an array element
						* @param name of the array element
						* @return returns the array element in their respective type
						*/
						std::vector<repoUUID> getUUIDFieldArray(std::string label);


					}; // end 
			}// end namespace bson
		}// end namespace model
	} // end namespace core
} // end namespace repo
