/**
*  Copyright (C) 2025 3D Repo Ltd
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

#include <repo/lib/datastructure/repo_uuid.h>
#include <repo/core/model/bson/repo_node_model_revision.h>
#include <filesystem>

#include "repo_test_revision_utils.h"
#include "repo_test_utils.h"

using namespace repo::core::model;

ModelRevisionNode testing::makeRevisionNode(
	std::shared_ptr<repo::core::handler::MongoDatabaseHandler> handler,
	const std::string& teamspace,
	const std::string& container,
	const repo::lib::RepoUUID& revision
)
{
	ModelRevisionNode revisionNode;
	revisionNode.setUniqueID(revision);
	revisionNode.setAuthor(testing::getRandomString());
	revisionNode.setTimestamp();
	revisionNode.setFiles({testing::getRandomString()});
	revisionNode.setTag(testing::getRandomString());
	revisionNode.setSharedID(repo::lib::RepoUUID::defaultValue);

	handler->insertDocument(
		teamspace,
		container + "." + REPO_COLLECTION_HISTORY,
		revisionNode
	);

	return revisionNode;
}

repo::core::model::ModelRevisionNode testing::makeRevisionNode(
	std::shared_ptr<repo::core::handler::MongoDatabaseHandler> handler,
	const repo::manipulator::modelconvertor::ModelImportConfig& config,
	const std::filesystem::path extFilePath
)
{
	auto refId = repo::lib::RepoUUID::createUUID().toString();
	auto filename = extFilePath.filename();
	auto extfs = std::filesystem::path(getDataPath(""));
	auto size = 0;
	if (std::filesystem::exists(extFilePath)) {
		size = std::filesystem::file_size(extFilePath);
	}

	ModelRevisionNode revisionNode;
	revisionNode.setUniqueID(config.revisionId);
	revisionNode.setAuthor(testing::getRandomString());
	revisionNode.setTimestamp();
	revisionNode.setFiles({refId});
	revisionNode.setTag(testing::getRandomString());
	revisionNode.setSharedID(repo::lib::RepoUUID::defaultValue);
	revisionNode.updateStatus(ModelRevisionNode::UploadStatus::GEN_DEFAULT);

	handler->insertDocument(
		config.databaseName,
		config.projectName + "." + REPO_COLLECTION_HISTORY,
		revisionNode
	);

	// RepoRef does not allow the construction or serialisation of EXTFS type
	// refs by design - these represent read-only files outside of bouncer's
	// control, that are expected to already exist in the database. For testing,
	// we build the equivalent bson by hand using RepoBSONBuilder so we can
	// exercise code paths that read pre-existing EXTFS refs.

	// On Windows, converting to std::string will try to conver the encoding
	// to ANSI, which will corrupt unicode names. Since BSON is inherently
	// UTF8, make sure we convert to UTF8 first, and then construct a std::string
	// from the underlying bytes. This way the BSON object will hold the correct
	// string.

	auto u8link = extFilePath.u8string();
	auto u8filename = filename.u8string();

	RepoBSONBuilder builder;
	builder.append(REPO_LABEL_ID, refId);
	builder.append(REPO_REF_LABEL_TYPE, RepoRef::REPO_REF_TYPE_EXTFS);
	builder.append(REPO_REF_LABEL_LINK, std::string(u8link.begin(), u8link.end()));
	builder.append(REPO_REF_LABEL_NAME, std::string(u8filename.begin(), u8filename.end()));
	builder.append(REPO_REF_LABEL_SIZE, (int64_t)size);

	handler->insertDocument(
		config.databaseName,
		config.projectName + "." + REPO_COLLECTION_HISTORY + "." + REPO_COLLECTION_EXT_REF,
		builder.obj()
	);

	return revisionNode;
}
