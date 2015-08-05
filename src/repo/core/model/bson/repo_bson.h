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

#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#include <Windows.h>

#define strcasecmp _stricmp
#endif
#include <mongo/bson/bson.h> 

#include <boost/log/trivial.hpp>

#include "repo_bson_element.h"

#include "../repo_model_global.h"
#include "../repo_node_properties.h"
#include "../repo_node_utils.h"

#include "../../../repo_bouncer_global.h"

namespace repo {
	namespace core {
		namespace model {
			namespace bson {

					enum class RepoBSONCommands { CREATE, DROP, UPDATE };

					//TODO: Eventually we should inherit from a generic BSON object. 
					//work seems to have been started in here:https://github.com/jbenet/bson-cpp
					//alternatively we can use a c++ wrapper on https://github.com/mongodb/libbson
					class REPO_API_EXPORT RepoBSON : public mongo::BSONObj
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
						virtual ~RepoBSON() {}

						/**
						* returns a field from the BSON
						* @param label name of the field to retrieve
						* @return returns a RepoBSONElement
						*/
						RepoBSONElement getField(std::string label) const
						{
							return RepoBSONElement(mongo::BSONObj::getField(label));
						}

						/**
						* get a binary field in the form of vector of T
						* @param bse bson element
						* @param vectorSize size of the vector
						* @param vec pointer to a vector to store this data
						* @return returns true upon success.
						*/
						template <class T>
						bool getBinaryFieldAsVector(
							const RepoBSONElement &bse,
							const uint32_t vectorSize,
							std::vector<T> * vec) const
						{
							bool success = false;
							uint64_t vectorSizeInBytes = vectorSize * sizeof(T);
							if (vec && bse.binDataType() == mongo::BinDataGeneral)
							{
								
								bse.value();
								int length;
								const char *binData = bse.binData(length);

								vec->resize(length/sizeof(T));

								if (length > vectorSizeInBytes)
								{
									BOOST_LOG_TRIVIAL(warning) << "RepoBSON::getBinaryFieldAsVector : "
										<< "size of binary data (" << length << ") is bigger than expected vector size(" 
										<< vectorSizeInBytes << ")";
								}
								if (success = (length >= vectorSizeInBytes))
								{ 
									//can copy as long as length is bigger or equal to vectorSize
									memcpy(&(vec->at(0)), binData, length);
									
								}
								else{
									BOOST_LOG_TRIVIAL(error) << "RepoBSON::getBinaryFieldAsVector : "
										<< "size of binary data (" << length << ") is smaller than expected vector size("
										<< vectorSizeInBytes << ")";

									//copy the length amount off anyway
									memcpy(&(vec->at(0)), binData, length);
								}
							}
							else{
								BOOST_LOG_TRIVIAL(error) << "RepoBSON::getBinaryFieldAsVector :" <<
									(!vec ? " nullptr to vector " : "bson element type is not BinDataGeneral!");
							}

							return success;
						}

						/**
						* get a binary field in the form of vector of T
						* @param bse bson element
						* @param vec pointer to a vector to store this data
						* @return returns true upon success.
						*/
						template <class T>
						bool getBinaryFieldAsVector(
							const RepoBSONElement &bse,
							std::vector<T> * vec) const
						{
							bool success = false;
							if (vec && bse.binDataType() == mongo::BinDataGeneral)
							{
								
								bse.value();
								int length;
								const char *binData = bse.binData(length);

								

								if (length > 0)
								{
									vec->resize(length / sizeof(T));
									memcpy(&(vec->at(0)), binData, length);

								}
								else{
									BOOST_LOG_TRIVIAL(error) << "RepoBSON::getBinaryFieldAsVector : "
										<< "size of binary data (" << length << ") Unable to copy 0 bytes!";
								}
							}
							else{
								BOOST_LOG_TRIVIAL(error) << "RepoBSON::getBinaryFieldAsVector :" <<
									(!vec ? " nullptr to vector " : "bson element type is not BinDataGeneral!");
							}

							return success;
						}


					

						/**
						* Overload of getField function to retreve repoUUID
						* @param name of the field
						* @return returns a repoUUID from that field
						*/
						repoUUID getUUIDField(std::string label) const;

						/**
						* Get an array of fields given an array element
						* @param name of the array element
						* @return returns the array element in their respective type
						*/
						std::vector<repoUUID> getUUIDFieldArray(std::string label) const;


						/**
						* Get an array of fields given an array element
						* @param name of the array element
						* @return returns the array element in their respective type
						*/
						std::vector<float> getFloatArray(std::string label) const;


					}; // end 
			}// end namespace bson
		}// end namespace model
	} // end namespace core
} // end namespace repo
