/**
*  Copyright (C) 2019 3D Repo Ltd
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
#include "repo_blob_files_handler.h"
#include "repo/lib/datastructure/repo_uuid.h"

using namespace repo::core::handler::fileservice;

BlobFilesHandler::~BlobFilesHandler() {
	commitActiveFile();
	for (auto &entry : readStreams) {
		entry.second.close();
	}
}
void BlobFilesHandler::commitActiveFile() {
	if (activeFile) {
		manager->uploadFileAndCommit(database, collection, activeFile->name, activeFile->buffer, metadata);
	}

	activeFile.reset();
}

void BlobFilesHandler::newActiveFile() {
	if (activeFile) {
		commitActiveFile();
	}

	activeFile = std::make_shared<fileEntry>();
	activeFile->name = repo::lib::RepoUUID::createUUID().toString();
	activeFile->buffer.reserve(MAX_FILE_SIZE_BYTES);
}

DataRef BlobFilesHandler::insertBinary(const std::vector<uint8_t> &data) {
	if (!activeFile || activeFile->buffer.size() + data.size() > MAX_FILE_SIZE_BYTES) {
		// either there is no activeFile or adding this data will exceed the max size to the blob file
		// create a new one.
		newActiveFile();
	}

	auto startPos = activeFile->buffer.size();
	auto dataSize = data.size();
	activeFile->buffer.resize(startPos + dataSize);

	memcpy(&(activeFile->buffer.data())[startPos], data.data(), dataSize);

	return DataRef(activeFile->name, startPos, dataSize);
}

std::istream BlobFilesHandler::fetchStream(const std::string &name) {
	manager->getFile(database, collection, name);
}

std::vector<uint8_t> BlobFilesHandler::readToBuffer(const DataRef &ref) {
	std::vector<uint8_t> res;
	if (readStreams.find(ref.fileName) == readStreams.end()) {
		readStreams[ref.fileName] = manager->getFileStream(database, collection, ref.fileName);
	}

	res.resize(ref.size);
	readStreams[ref.fileName].seekg(ref.startPos, std::ios_base::beg);
	readStreams[ref.fileName].read((char*)res.data(), ref.size);

	return res;
}

std::shared_ptr<FileManager>  BlobFilesHandler::getFileManager()
{
	return manager;
}
