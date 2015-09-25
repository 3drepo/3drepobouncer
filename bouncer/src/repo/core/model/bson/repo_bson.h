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
#include <unordered_map>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>

#include "repo_bson_element.h"

#include "../repo_model_global.h"
#include "../repo_node_properties.h"
#include "../repo_node_utils.h"
#include "../../../lib/repo_log.h"
#include "../../../repo_bouncer_global.h"

#define REPO_BSON_MAX_BYTE_SIZE 16770000 //max size is 16MB,but leave a bit for buffer

namespace repo {
	namespace core {
		namespace model {

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
						RepoBSON(const mongo::BSONObj &obj,
							const std::unordered_map<std::string, std::vector<uint8_t>> &binMapping =
							std::unordered_map<std::string, std::vector<uint8_t>>());

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
						* Override the equals operator to perform the swap just like mongo bson
						* but also retrieve the mapping information
						*/
						RepoBSON& operator=(RepoBSON otherCopy) {
							swap(otherCopy);
							return *this;
						}
						/**
						* Override the swap operator to perform the swap just like mongo bson
						* but also carry over the mapping information
						*/
						void swap(RepoBSON otherCopy)
						{
							mongo::BSONObj::swap(otherCopy);
							bigFiles = otherCopy.bigFiles;
						}


						/**
						* returns a field from the BSON
						* @param label name of the field to retrieve
						* @return returns a RepoBSONElement
						*/
						RepoBSONElement getField(const std::string &label) const
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
							if (bse.isNull()) return false;


							if (vec && bse.type() == ElementType::STRING)
							{
								//this is a reference, try to get it from map
								repoTrace << "getting Binary from reference...";
								std::vector<uint8_t> bin = getBigBinary(bse.str());
								repoTrace << " Get binary from Grid FS";
								if (bin.size() > 0)
								{
									repoTrace << " size of bin : " << bin.size();
									vec->resize(bin.size() / sizeof(T));
									memcpy(&vec->at(0), &bin[0], bin.size());
									success = true;
								}
								else
								{
									repoWarning << "Binary obtained from gridFS is of size = 0";
								}

							}
							else if(vec && bse.binDataType() == mongo::BinDataGeneral)
							{
								uint64_t vectorSizeInBytes = vectorSize * sizeof(T);
								

								bse.value();
								int length;
								const char *binData = bse.binData(length);

								vec->resize(length / sizeof(T));

								if (length > vectorSizeInBytes)
								{
									repoWarning << "RepoBSON::getBinaryFieldAsVector : "
										<< "size of binary data (" << length << ") is bigger than expected vector size("
										<< vectorSizeInBytes << ")";
								}

								if (success = (length >= vectorSizeInBytes))
								{
									//can copy as long as length is bigger or equal to vectorSize
									memcpy(&(vec->at(0)), binData, length);

								}
								else{
									repoError << "RepoBSON::getBinaryFieldAsVector : "
										<< "size of binary data (" << length << ") is smaller than expected vector size("
										<< vectorSizeInBytes << ")";

									//copy the length amount off anyway
									memcpy(&(vec->at(0)), binData, length);
								}
								

							}
							else
							{
								repoError << "RepoBSON::getBinaryFieldAsVector :" <<
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
							repoTrace << bse;

							if (bse.eoo())
							{
								repoError << "Trying to get binary object from an empty field!";
								return false;
							}

	
							if (vec && bse.type() == ElementType::STRING)
							{
								repoTrace << "getting Binary from reference...";
								//this is a reference, try to get it from map
								std::vector<uint8_t> bin = getBigBinary(bse.str());
								repoTrace << " Get binary from Grid FS";
								if (bin.size() > 0)
								{
									repoTrace << "Size of bin = " << bin.size();
									vec->resize(bin.size() / sizeof(T));
									memcpy(&vec->at(0), &bin[0], bin.size());
									success = true;
								}
								else
								{
									repoWarning << "Binary obtained from gridFS is of size = 0";
								}

							}
							else if (vec && bse.type() == ElementType::BINARY && bse.binDataType() == mongo::BinDataGeneral)
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
									repoError << "RepoBSON::getBinaryFieldAsVector : "
										<< "size of binary data (" << length << ") Unable to copy 0 bytes!";
								}
							}
							else{
								repoTrace << "passed1 jhere";
								repoError << "RepoBSON::getBinaryFieldAsVector :" <<
									(!vec ? " nullptr to vector " : "bson element type is not BinDataGeneral!");
							
							}

						
							return success;
						}
					

						/**
						* Overload of getField function to retreve repoUUID
						* @param label name of the field
						* @return returns a repoUUID from that field
						*/
						repoUUID getUUIDField(const std::string &label) const;

						/**
						* Get an array of fields given an array element
						* @param label name of the array element
						* @return returns the array element in their respective type
						*/
						std::vector<repoUUID> getUUIDFieldArray(const std::string &label) const;


						/**
						* Get an array of fields given an array element
						* @param label name of the array element
						* @return returns the array element in their respective type
						*/
						std::vector<float> getFloatArray(const std::string &label) const;

						/**
						* Get a field as timestamp
						* @param label name of the element
						* @return returns timestamp as int64, return -1 if not found
						*/
						int64_t getTimeStampField(const std::string &label) const;

						/**
						* Get a field as a List of string pairs
						* This is for scenarios were there is an element that is an array of objects
						* and you want to get 2 fields within all the objects.
						* @param arrLabel the element where this data is stored
						* @param fstLabel label for #1 in pair
						* @param sndLabel label for #2 in pair
						* @returns a List of pairs of strings
						*/
						std::list<std::pair<std::string, std::string> > getListStringPairField(
							const std::string &arrLabel,
							const std::string &fstLabel,
							const std::string &sndLabel) const;


						/*
						* ----------------- BIG FILE MANIPULATION --------------------
						*/


						/**
						* Clone and attempt the shrink the bson by offloading binary files to big file storage
						* @return returns the shrunk BSON
						*/
						RepoBSON cloneAndShrink() const;

						std::vector<uint8_t> getBigBinary(const std::string &key) const;

						/**
						* Get the list of file names for the big files 
						* needs to be stored for this bson
						* @return returns a list of file names needed to be stored
						*/
						std::vector<std::string> getFileList() const;

						/**
						* Get the mapping files from the bson object
						* @return returns the map of external (gridFS) files
						*/
						std::unordered_map< std::string, std::vector<uint8_t>> getFilesMapping() const
						{
							return bigFiles;
						}

						/**
						* Check if this bson object has oversized files
						* @return returns true if there are oversized files
						*/
						bool hasOversizeFiles() const
						{
							return bigFiles.size() > 0;
						}

						
						protected:
							std::unordered_map< std::string, std::vector<uint8_t> > bigFiles;
						

					}; // end 
		}// end namespace model
	} // end namespace core
} // end namespace repo
