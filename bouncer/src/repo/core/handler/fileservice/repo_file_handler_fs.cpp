/**
*  Copyright (C) 2018 3D Repo Ltd
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
#include <stdio.h>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include "repo_file_handler_fs.h"

#include "../../model/repo_model_global.h"
#include "../../../lib/repo_log.h"
#include "../../../lib/repo_exception.h"
#include "../../../lib/repo_utils.h"

using namespace repo::core::handler::fileservice;

FSFileHandler::FSFileHandler(
	const std::string &dir,
	const int &nLevel) :
	AbstractFileHandler(),
	dirPath(dir),
	level(nLevel)
{
	if (!repo::lib::doesDirExist(dir)) {
		repoError << "Cannot initialise fileshare: " + dir + " does not exist/is not a directory";
		throw repo::lib::RepoException("Cannot initialise fileshare: " + dir + " is does not exist/is not a directory");
	}
}

/**
 * A Deconstructor
 */
FSFileHandler::~FSFileHandler()
{
}

bool FSFileHandler::deleteFile(
	const std::string          &database,
	const std::string          &collection,
	const std::string &keyName)
{
	bool success = false;

	auto fullPath = boost::filesystem::absolute(keyName, dirPath);
	if (repo::lib::doesFileExist(fullPath)) {
		auto fileStr = fullPath.string();
		success = std::remove(fileStr.c_str()) == 0;
	}
	return success;
}

std::vector<uint8_t> FSFileHandler::getFile(
	const std::string          &database,
	const std::string          &collection,
	const std::string &keyName)
{
	std::vector<uint8_t> results;

	auto stream = getFileStream(database, collection, keyName);

	if (stream) {
		results = std::vector<uint8_t>((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
		stream.close();
	}

	return results;
}

std::ifstream FSFileHandler::getFileStream(
	const std::string          &database,
	const std::string          &collection,
	const std::string          &keyName)
{
	auto fullPath = boost::filesystem::absolute(keyName, dirPath);
	if (repo::lib::doesFileExist(fullPath)) {
		auto fileStr = fullPath.string();
		std::ifstream stream(fileStr, std::ios::in | std::ios::binary);
		return stream;
	}
	repoError << "File " << fullPath.string() << " does not exist";

	return std::ifstream();
}

std::vector<std::string> FSFileHandler::determineHierachy(
	const std::string &name
) const {
	auto nameChunkLen = name.length() / level;
	nameChunkLen = nameChunkLen < minChunkLength ? minChunkLength : nameChunkLen;

	std::hash<std::string> stringHasher;
	std::vector<std::string> levelNames;
	for (int i = 0; i < level; ++i) {
		auto chunkStart = (i * nameChunkLen) % (name.length() - nameChunkLen);
		auto stringToHash = name.substr(i, nameChunkLen) + std::to_string((float)std::rand() / RAND_MAX);
		auto hashedValue = stringHasher(stringToHash) & 255;
		levelNames.push_back(std::to_string(hashedValue));
	}

	return levelNames;
}

std::string FSFileHandler::uploadFile(
	const std::string          &database,
	const std::string          &collection,
	const std::string          &keyName,
	const std::vector<uint8_t> &bin
)
{
	auto hierachy = level > 0 ? determineHierachy(keyName) : std::vector<std::string>();

	boost::filesystem::path path(dirPath);
	std::stringstream ss;
	for (const auto& levelName : hierachy) {
		path /= levelName;
		ss << levelName << "/";
		if (!repo::lib::doesDirExist(path)) {
			boost::filesystem::create_directories(path);
		}
	}

	path /= keyName;
	ss << keyName;
	int retries = 0;
	bool failed;
	do {
		boost::filesystem::ofstream outs(path.string(), std::ios::out | std::ios::binary);
		try {
			outs.write((char*)bin.data(), bin.size());
			outs.close();
		}
		catch (const boost::filesystem::filesystem_error& e)
		{
			repoError << "Failed to write to file on " << path.string() << " because " << e.code().message();
			//repoError << "File writing error code :  " << e.code();
		}
		
		if (failed = (!outs || !repo::lib::doesFileExist(path))) {
			repoError << "Failed to write to file " << path.string() << ((retries + 1) < 3 ? ". Retrying... " : "");
			boost::this_thread::sleep(boost::posix_time::seconds(5));
		}
	} while (failed && ++retries < 3);

	return failed ? "" : ss.str();
}