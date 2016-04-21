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
#include "../../../lib/repo_log.h"
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
		repoTrace << "Reading... " << path;
		std::vector<std::string> tokens;
		while (file.good() && readLine(file, tokens))
		{
			if (headers.empty())
				headers = tokens;
			else if (!tokens.empty())
			{
				repo::core::model::MetadataNode meta =
					repo::core::model::RepoBSONFactory::makeMetaDataNode(headers, tokens, tokens[0]);
				metadata.insert(new repo::core::model::MetadataNode(meta));

				repoDebug << " metadata: " << meta.toString();
			}
		}
		file.close();

		repoTrace << "Created " << metadata.size() << " metadata nodes.";
	}
	else
	{
		repoError << "Error opening file: " << path;
	}

	return metadata;
}