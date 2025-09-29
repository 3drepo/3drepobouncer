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

#pragma once

#include <repo/manipulator/modelutility/repo_clash_detection_config.h>
#include <repo/core/handler/repo_database_handler_mongo.h>

namespace testing {

	using DatabasePtr = std::shared_ptr<repo::core::handler::MongoDatabaseHandler>;

	// Helper class to build clash detection configurations using the user-fiendly
	// identifiers of Nodes in the Clash Detection database, such as their names.

	struct ClashDetectionDatabaseHelper
	{
		ClashDetectionDatabaseHelper(DatabasePtr handler)
			:handler(handler)
		{
		}

		DatabasePtr handler;

		void getChildMeshNodes(repo::lib::Container* container, 
			const repo::core::model::RepoBSON& bson, 
			std::vector<repo::lib::RepoUUID>& uuids);

		// Searches for mesh nodes only
		std::vector<repo::lib::RepoUUID> getUniqueIdsByName(
			repo::lib::Container* container,
			std::string name);

		repo::manipulator::modelutility::CompositeObject createCompositeObject(
			repo::lib::Container* container,
			std::string name
		);

		// Will replace anything in the existing sets
		void setCompositeObjectSetsByName(
			repo::manipulator::modelutility::ClashDetectionConfig& config,
			std::initializer_list<std::string> a,
			std::initializer_list<std::string> b);

		std::unique_ptr<repo::lib::Container> getContainerByName(
			std::string name);
	};


}