/**
*  Copyright (C) 2026 3D Repo Ltd
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

#pragma once

#include <repo/core/model/bson/repo_node_revision.h>
#include <repo/core/handler/repo_database_handler_mongo.h>
#include <repo/manipulator/modelconvertor/import/repo_model_import_config.h>
#include <filesystem>

namespace testing {
	repo::core::model::ModelRevisionNode makeRevisionNode(
		std::shared_ptr<repo::core::handler::MongoDatabaseHandler> handler,
		const std::string& teamspace,
		const std::string& container,
		const repo::lib::RepoUUID& revision
	);

	repo::core::model::ModelRevisionNode makeRevisionNode(
		std::shared_ptr<repo::core::handler::MongoDatabaseHandler> handler,
		const repo::manipulator::modelconvertor::ModelImportConfig& config,
		const std::filesystem::path extFilePath
	);
}