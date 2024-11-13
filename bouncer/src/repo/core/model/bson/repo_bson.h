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

#include <unordered_map>
#include <set>
#include "repo/repo_bouncer_global.h"
#include "repo/core/model/repo_model_global.h"
#include "repo/lib/repo_log.h"
#include "repo/lib/datastructure/repo_uuid.h"
#include "repo/lib/datastructure/repo_vector.h"
#include "repo/lib/datastructure/repo_matrix.h"
#include "repo/lib/repo_exception.h"
#include "repo/core/model/bson/repo_bson_element.h"

#include <bsoncxx/document/value.hpp>

#define REPO_BSON_MAX_BYTE_SIZE 16770000 //max size is 16MB,but leave a bit for buffer

namespace repo {
	namespace core {
		namespace handler {
			class MongoDatabaseHandler;
		}

		namespace model {
			//TODO: Eventually we should inherit from a generic BSON object.
			//work seems to have been started in here:https://github.com/jbenet/bson-cpp
			//alternatively we can use a c++ wrapper on https://github.com/mongodb/libbson
			class REPO_API_EXPORT RepoBSON : private bsoncxx::document::value
			{
				friend class RepoBSONBuilder;
				friend class repo::core::handler::MongoDatabaseHandler;

			public:

				using BinMapping = std::unordered_map<std::string, std::vector<uint8_t>>;

				/**
				* Constructor from Mongo BSON object.
				* @param mongo BSON object
				*/
				RepoBSON(const RepoBSON &obj,
					const BinMapping& binMapping = {});

				/**
				* Constructor from Mongo BSON object.
				* @param mongo BSON object
				*/
				RepoBSON(const bsoncxx::document::view &obj,
					const BinMapping& binMapping = {});

				/**
				* This constructor must exist for various container types,
				* but should be avoided in practice.
				*/
				RepoBSON();

				/**
				* Default empty deconstructor.
				*/
				virtual ~RepoBSON() {}

				/**
				* Override the equals operator to perform the swap just like mongo bson
				* but also retrieve the mapping information
				*/
				RepoBSON& operator=(RepoBSON otherCopy);

				bool hasField(const std::string& label) const;

				/**
				* returns a field from the BSON
				* @param label name of the field to retrieve
				* @return returns a RepoBSONElement
				*/
				class RepoBSONElement getField(const std::string& label) const;

				bool getBoolField(const std::string& label) const;

				std::string getStringField(const std::string& label) const;

				RepoBSON getObjectField(const std::string& label) const;

				std::vector<lib::RepoVector3D> getBounds3D(const std::string& label);

				lib::RepoVector3D getVector3DField(const std::string& label) const;

				repo::lib::RepoMatrix getMatrixField(const std::string& label) const;

				std::vector<float> getFloatVectorField(const std::string& label) const;

				std::vector<double> getDoubleVectorField(const std::string& label) const;

				std::vector<std::string> getFileList(const std::string& label) const;

				double getDoubleField(const std::string &label) const;

				long long getLongField(const std::string& label) const;

				bool isEmpty() const;

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
					auto& buf = getBinary(field);
					vec.resize(buf.size() / sizeof(T));
					memcpy(vec.data(), buf.data(), buf.size());
					return true;
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

				int getIntField(const std::string& label) const;

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
				time_t getTimeStampField(const std::string &label) const;

				std::set<std::string> getFieldNames() const;

				/**
				* Checks if a binary field exists within the RepoBSON
				* This differs from hasField() as it also checks the bigFiles mapping
				* where the field is stored outside in GridFS.
				* @param label field name in question
				* @return returns true if found
				*/
				bool hasBinField(const std::string &label) const;

				uint64_t objsize() const;

				std::string toString() const;

				bool operator==(const RepoBSON other) const;

				bool operator!=(const RepoBSON other) const;

			public:

				/*
				* ----------------- BIG FILE MANIPULATION ----------------------------------
				*/

				/**
				* Get the mapping files from the bson object
				* @return returns the map of external (gridFS) files
				*/
				const BinMapping& getFilesMapping() const
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

				std::pair<repo::core::model::RepoBSON, std::vector<uint8_t>> getBinariesAsBuffer() const;

				void replaceBinaryWithReference(const repo::core::model::RepoBSON &fileRef, const repo::core::model::RepoBSON &elemRef);

				repo::core::model::RepoBSON getBinaryReference() const;
				void initBinaryBuffer(const std::vector<uint8_t> &buffer);

				bool hasFileReference() const;

				const std::vector<uint8_t>& getBinary(const std::string& label) const;

			protected:

				/**
				* Override the swap operator to perform the swap just like mongo bson
				* but also carry over the mapping information
				*/
				void swap(RepoBSON otherCopy);

				BinMapping bigFiles;
			}; // end
		}// end namespace model
	} // end namespace core
} // end namespace repo
