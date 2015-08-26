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
* Import data with CSV
*/

#pragma once

#include "../../../core/model/bson/repo_bson_factory.h"

namespace repo{
	namespace manipulator{
		namespace modelconvertor{
			class REPO_API_EXPORT MetadataImportCSV
			{
			public:
				/**
				* Default constructor
				*/
				MetadataImportCSV();

				/**
				* destructor
				*/
				~MetadataImportCSV();

				/**
				* read a csv file and returns a set of metadata
				* @param filePath path to the file
				* @param headers vector of headers names (if empty, take the first line as headers)
				* @param delimiter symbol that separates the fields
				*/
				repo::core::model::RepoNodeSet readMetadata(
					const std::string        &filePath,
					std::vector<std::string> &headers = std::vector<std::string>(),
					const char               &delimiter = ',');

				//! Sets the delimiter
				void setDelimiter(char delimiter) { this->delimiter = delimiter; }

			private:
				/**
				* Reads a single line. Same as operator>> but with class members access.
				* @param stream stream to read from
				* @param tokenizedLine the returning results of the line read, tokenized by the delimiter
				* @return returns the stream after the operation
				*/
				std::istream& readLine(
					std::istream             &stream,
					std::vector<std::string> &tokenizedLine);
				char delimiter;
			};
		} //namespace modelconvertor
	} //namespace manipulator
} //namespace repo
