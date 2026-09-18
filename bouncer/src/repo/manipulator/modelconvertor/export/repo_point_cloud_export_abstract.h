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

/**
* Abstract Point Cloud convertor(Export)
*/

#pragma once

#include <repo/core/model/collection/repo_scene.h>
#include <repo/core/model/bson/repo_node_point.h>

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			class AbstractPointCloudExport
			{
			public:
				AbstractPointCloudExport(
					repo::core::handler::AbstractDatabaseHandler* dbHandler,
					const std::string databaseName,
					const std::string projectName,
					const repo::lib::RepoUUID revId,
					const std::vector<double> worldOffset,
					const repo::lib::RepoBounds cloudBounds);
				
				virtual ~AbstractPointCloudExport();

				virtual void addTreeNode(
					std::vector<repo::core::model::PointData>* pointData,
					std::vector<uint8_t> treePosition
				) = 0;

				virtual void finalise() = 0;

			protected:
				repo::core::handler::AbstractDatabaseHandler* dbHandler;

				// Model info
				std::string dbName;
				std::string projectName;
				repo::lib::RepoUUID revId;
				std::vector<double> worldOffset;
				repo::lib::RepoBounds cloudBounds;
			};
		}
	} // namespace manipulator
} // namespace repo