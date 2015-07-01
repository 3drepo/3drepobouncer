/*
*  Copyright(C) 2015 3D Repo Ltd
*
*  This program is free software : you can redistribute it and / or modify
*  it under the terms of the GNU Affero General Public License as
*  published by the Free Software Foundation, either version 3 of the
*  License, or(at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
*  GNU Affero General Public License for more details.
*
*  You should have received a copy of the GNU Affero General Public License
*  along with this program.If not, see <http://www.gnu.org/licenses/>.
*/

/**
* BSON object builder. Currently it does nothing more than inherit mongo. 
* It is a layer of abstraction between 3D Repo and mongo.
* In the future this may be replaced by utilising a BSON library
*/

#pragma once


#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#include <Windows.h>

#define strcasecmp _stricmp
#endif
#include <boost/lexical_cast.hpp>
#include <string>
#include <mongo/bson/bson.h>
#include "../repo_node_utils.h"
#include "repo_bson.h"

namespace repo {
	namespace core {
		namespace model {
			namespace bson {



				class RepoBSONBuilder: public mongo::BSONObjBuilder
				{
					public:
						RepoBSONBuilder();
						~RepoBSONBuilder();

						//! Appends a vector as an array to BSON builder.
						template <class T>
						RepoBSON createArrayBSON(
							const std::vector<T> &vec)
						{
							RepoBSONBuilder array;
							for (unsigned int i = 0; i < vec.size(); ++i)
								array.append(boost::lexical_cast<std::string>(i), vec[i]);
							return array.obj();
						}

						/**
						* @brief Append a UUID into the builder
						* This is a is a wrapper around appendBinData from mongo as
						* there is no support to append boost::uuid directly
						* @param label of the field
						* @param UUID
						*/
						void RepoBSONBuilder::append(
							const std::string &label,
							const repo_uuid &uuid);

						/**
						* builds the BSON object as RepoBSON given the information within the builder
						* @return returns a RepoBSON object with the fields given.
						*/
						RepoBSON obj();
				};
			}// end namespace bson
		}// end namespace model
	} // end namespace core
} // end namespace repo
