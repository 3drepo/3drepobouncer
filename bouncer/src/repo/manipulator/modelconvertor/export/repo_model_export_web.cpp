/**
*  Copyright (C) 2016 3D Repo Ltd
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
* Allows Export functionality from 3D Repo World to SRC
*/

#include "repo_model_export_web.h"
#include "../../../lib/repo_log.h"
#include "../../../core/model/bson/repo_bson_factory.h"

#include <boost/filesystem.hpp>

using namespace repo::manipulator::modelconvertor;

WebModelExport::WebModelExport(
	const repo::core::model::RepoScene *scene
	) : AbstractModelExport(scene)
{
	//We don't cache reference scenes
	if (convertSuccess = scene && !scene->getAllReferences(repo::core::model::RepoScene::GraphType::DEFAULT).size())
	{
		if (scene->hasRoot(repo::core::model::RepoScene::GraphType::OPTIMIZED))
		{
			gType = repo::core::model::RepoScene::GraphType::OPTIMIZED;
		}
		else if (scene->hasRoot(repo::core::model::RepoScene::GraphType::DEFAULT))
		{
			gType = repo::core::model::RepoScene::GraphType::DEFAULT;
		}
		else
		{
			repoError << "Failed to export to scene : Failed to find root node within the scene!";
			convertSuccess = false;
		}
	}
}

WebModelExport::~WebModelExport()
{
}

bool WebModelExport::exportToFile(
	const std::string &filePath)
{
	if (!convertSuccess) {
		return convertSuccess;
	}

	const repo_web_buffers_t buffers = getAllFilesExportedAsBuffer();

	boost::filesystem::path boostPath(filePath);

	for (const auto &buff : buffers.geoFiles)
	{
		std::string fname = sanitizeFileName(buff.first);
		boostPath /= fname;
		FILE* fp = fopen(boostPath.string().c_str(), "wb");
		if (fp)
		{
			fwrite(buff.second.data(), sizeof(*buff.second.data()), buff.second.size(), fp);
			fclose(fp);
		}
		else
		{
			repoError << "Failed to open file for writing: " << fname;
		}
	}

	for (const auto &buff : buffers.jsonFiles)
	{
		std::string fname = sanitizeFileName(buff.first);
		boostPath /= fname;
		FILE* fp = fopen(boostPath.string().c_str(), "wb");
		if (fp)
		{
			fwrite(buff.second.data(), sizeof(*buff.second.data()), buff.second.size(), fp);
			fclose(fp);
		}
		else
		{
			repoError << "Failed to open file for writing: " << fname;
		}
	}

	return convertSuccess;
}

std::unordered_map<std::string, std::vector<uint8_t>> WebModelExport::getJSONFilesAsBuffer() const
{
	std::unordered_map < std::string, std::vector<uint8_t> > fileBuffers;

	for (const auto &treePair : jsonTrees)
	{
		std::stringstream ss;
		treePair.second.write_json(ss);
		std::string jsonStr = ss.str();

		fileBuffers[treePair.first] = std::vector<uint8_t>();
		fileBuffers[treePair.first].resize(jsonStr.size());
		memcpy(fileBuffers[treePair.first].data(), jsonStr.c_str(), jsonStr.size());
	}

	return fileBuffers;
}

std::string WebModelExport::getSupportedFormats()
{
	return ".src, .gltf";
}

std::string WebModelExport::sanitizeFileName(
	const std::string &name) const
{
	std::string res = name;
	std::replace(res.begin(), res.end(), '\\', '_');
	std::replace(res.begin(), res.end(), '/', '_');

	return res;
}