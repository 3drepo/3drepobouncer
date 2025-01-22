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

#include <string>
#include <memory>
#include "repo/core/handler/repo_database_handler_abstract.h"
#include "repo/core/model/repo_model_global.h"
#include "repo/core/model/bson/repo_node.h"

#include "repo_scene.h"

namespace repo {
	namespace core {
		namespace model {
			namespace x { // Hide this RepoScene in its own namespace for now
				/*
				* RepoScene2 offers a cursor-based interaction with a Scene
				* collection.
				* It is an in-development replacement for RepoScene - eventually
				* RepoScene2 will supplant RepoScene and become RepoScene, however
				* this will take some time.
				*/
				REPO_API_EXPORT class RepoScene : public model::RepoScene
				{
				public:
					RepoScene(
						std::string database, 
						std::string project, 
						std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler
					);

					using Cursor = repo::core::handler::database::CursorPtr;

					// int - get num nodes of type
					size_t getNumNodes(repo::core::model::NodeType type);

					/*
					* Deletes nodes from the database. Nodes will not be deleted immediately -
					* make sure this is considered before performing other read operations. To
					* force RepoScene to execute all pending deletes use flush. This method does
					* not automatically remove any references to the node.
					*/
					void removeNode(const repo::core::model::RepoNode& uniqueId);

					/*
					* Push any pending remove operations to the database.
					*/
					void flushRemove();

					/*
					* Schedules the node for writing to the database. The node is not written
					* immediately. The node will be serialised immediately however - any further
					* changes will *not* be written, unless updateNode is called again.
					* Use flushUpdate() to execute all pending updates.
					*/
					void updateNode(const repo::core::model::RepoNode& node);

					Cursor getAllNodesByType(const std::string& type);

					Cursor getSiblings(const repo::core::model::RepoNode& node);

					std::string getCollectionName();

				private:
					std::string databaseName;
					std::string projectName;
					std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler;
				};
			}
		}
	}
}