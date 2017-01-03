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

#include "../../../lib/repo_log.h"
#include "../../../repo_bouncer_global.h"
#include "../repo_model_global.h"
#include "../../../lib/datastructure/repo_uuid.h"
#include "repo_bson_element.h"

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
					const std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>> &binMapping =
					std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>>());

				/**
				* Constructor from Mongo BSON object builder.
				* @param mongo BSON object builder
				*/
				RepoBSON(mongo::BSONObjBuilder &builder) : mongo::BSONObj(builder.obj()) {}

				/**
				* Constructor from raw data buffer.
				* @param rawData raw data
				*/
				RepoBSON(const std::vector<char> &rawData) : mongo::BSONObj(rawData.data()) {}

				/**
				* Default empty deconstructor.
				*/
				virtual ~RepoBSON() {}

				/**
				* Construct a repobson from a json string
				*/
				static RepoBSON fromJSON(const std::string &json);

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
				* Returns the current time in the form of int64 timestamp
				*/
				static int64_t getCurrentTimestamp();

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
				* @param field field name
				* @param vec pointer to a vector to store this data
				* @return returns true upon success.
				*/
				template <class T>
				bool getBinaryFieldAsVector(
					const std::string &field,
					std::vector<T> &vec) const

				{
					bool success = false;

					if (!hasField(field) || getField(field).type() == ElementType::STRING)
					{
						//Try to get it from file mapping.
						std::vector<uint8_t> bin = getBigBinary(field);
						if (bin.size() > 0)
						{
							vec.resize(bin.size() / sizeof(T));
							memcpy(vec.data(), &bin[0], bin.size());
							success = true;
						}
						else
						{
							repoError << "Trying to retrieve binary from a field that doesn't exist(" << field << ")";
							return false;
						}
					}
					else{
						RepoBSONElement bse = getField(field);
						if (bse.type() == ElementType::BINARY && bse.binDataType() == mongo::BinDataGeneral)
						{
							bse.value();
							int length;
							const char *binData = bse.binData(length);
							if (length > 0)
							{
								vec.resize(length / sizeof(T));
								memcpy(vec.data(), binData, length);
								success = true;
							}
							else{
								repoError << "RepoBSON::getBinaryFieldAsVector : "
									<< "size of binary data (" << length << ") Unable to copy 0 bytes!";
							}
						}
						else{
							repoError << "RepoBSON::getBinaryFieldAsVector : bson element type is not BinDataGeneral!";
						}
					}

					return success;
				}

				/**
				* Overload of getField function to retreve repo::lib::RepoUUID
				* @param label name of the field
				* @return returns a repo::lib::RepoUUID from that field
				*/
				repo::lib::RepoUUID getUUIDField(const std::string &label) const;

				/**
				* Get an array of fields given an array element
				* @param label name of the array element
				* @return returns the array element in their respective type
				*/
				std::vector<repo::lib::RepoUUID> getUUIDFieldArray(const std::string &label) const;

				/**
				* Get an array of fields given an element label
				* @param label name of the array element
				* @return returns the array element in their respective type
				*/
				std::vector<float> getFloatArray(const std::string &label) const;

				/**
				* Get an array of fields given an element label
				* @param label name of the array element
				* @return returns the array element in their respective type
				*/
				std::vector<std::string> getStringArray(const std::string &label) const;

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
				* @return a List of pairs of strings
				*/
				std::list<std::pair<std::string, std::string> > getListStringPairField(
					const std::string &arrLabel,
					const std::string &fstLabel,
					const std::string &sndLabel) const;

				//! Gets double from embedded sub-object based on name fields.
				double getEmbeddedDouble(
					const std::string &embeddedObjName,
					const std::string &fieldName,
					const double &defaultValue = 0) const;

				//! Returns true if embedded object contains given fieldName.
				bool hasEmbeddedField(
					const std::string &embeddedObjName,
					const std::string &fieldName) const;

				/**
				* Checks if a binary field exists within the RepoBSON
				* This differs from hasField() as it also checks the bigFiles mapping
				* where the field is stored outside in GridFS.
				* @param label field name in question
				* @return returns true if found
				*/
				bool hasBinField(const std::string &label) const
				{
					return hasField(label) || bigFiles.find(label) != bigFiles.end();
				}

				virtual RepoBSON cloneAndAddFields(
					const RepoBSON *changes) const;

			public:

				/*
				* ----------------- BIG FILE MANIPULATION ----------------------------------
				*/

				/**
				* Clone and attempt the shrink the bson by offloading binary files to big
				* file storage
				* @return returns the shrunk BSON
				*/
				RepoBSON cloneAndShrink() const;

				std::vector<uint8_t> getBigBinary(const std::string &key) const;

				/**
				* Get the list of file names for the big files
				* needs to be stored for this bson
				* @return returns a list of {field name, file name} needed to be stored
				*/
				std::vector<std::pair<std::string, std::string>> getFileList() const;

				/**
				* Get the mapping files from the bson object
				* @return returns the map of external (gridFS) files
				*/
				std::unordered_map< std::string, std::pair<std::string, std::vector<uint8_t> > > getFilesMapping() const
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

				std::unordered_map< std::string, std::pair<std::string, std::vector<uint8_t> > > bigFiles;
			}; // end
		}// end namespace model
	} // end namespace core
} // end namespace repo
