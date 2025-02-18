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

#include "repo_metadata_import_csv.h"
#include <repo_log.h>
#include <fstream>

using namespace repo::manipulator::modelconvertor;

MetadataImportCSV::MetadataImportCSV()
{
}

MetadataImportCSV::~MetadataImportCSV()
{
}

std::istream& MetadataImportCSV::readLine(
	std::istream &stream,
	std::vector<std::string>& tokenizedLine)
{
	tokenizedLine.clear();
	std::string line;
	getline(stream, line);
	std::stringstream ss(line);
	std::string field;
	while (std::getline(ss, field, delimiter))
		tokenizedLine.push_back(field);

	//FIXME: remove trailing white spaces!
	return stream;
}

std::vector<repo::lib::RepoVariant> repo::manipulator::modelconvertor::MetadataImportCSV::convertToVariants(std::vector<std::string> tokens)
{
	std::vector<repo::lib::RepoVariant> metaData;

	for (auto& value : tokens) {
		repo::lib::RepoVariant v;

		// Guess-cast into the variant

		//Check if it is a number, if it is, store it as a number
		try {
			v = boost::lexical_cast<int64_t>(value);
		}
		catch (boost::bad_lexical_cast&)
		{
			//not an int, try a double

			try {
				v = boost::lexical_cast<double>(value);
			}
			catch (boost::bad_lexical_cast&)
			{
				//not an int or float, store as string
				v = value;
			}
		}

		// Store the variant
		metaData.push_back(v);
	}

	return metaData;
}

repo::core::model::RepoNodeSet MetadataImportCSV::readMetadata(
	const std::string        &path,
	std::vector<std::string> &headers,
	const char               &delimiter)
{
	repo::core::model::RepoNodeSet metadata;
	std::ifstream file(path);

	setDelimiter(delimiter);

	if (file.is_open())
	{
		std::vector<std::string> tokens;
		while (file.good() && readLine(file, tokens))
		{
			if (headers.empty())
				headers = tokens;
			else if (!tokens.empty())
			{
				repo::core::model::MetadataNode meta =
					repo::core::model::RepoBSONFactory::makeMetaDataNode(headers, convertToVariants(tokens), tokens[0]);
				metadata.insert(new repo::core::model::MetadataNode(meta));
			}
		}
		file.close();
	}
	else
	{
		repoError << "Error opening file: " << path;
	}

	return metadata;
}