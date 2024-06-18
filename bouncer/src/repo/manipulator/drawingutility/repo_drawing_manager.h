/**
*  Copyright (C) 2024 3D Repo Ltd
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
#include "../../core/model/bson/repo_node_drawing_revision.h"
#include "../../manipulator/modelconvertor/import/repo_drawing_import_manager.h"
#include "../../core/handler/repo_database_handler_abstract.h"
#include "../../core/handler/fileservice/repo_file_manager.h"

namespace repo {
	namespace manipulator {
		namespace drawingutility {

			using namespace repo::core::model;

			/**
			* DrawingManager is responsible for retrieving, modifying and writing
			* drawings. It is here the collections for drawings (drawings.history)
			* is defined and the logic to acquire and update them lives. Actually
			* processing drawings and associated resources is the domain of the
			* DrawingImportManager.
			*/
			class DrawingManager
			{
			public:
				DrawingManager() {}
				~DrawingManager() {}

				DrawingRevisionNode fetchRevision(
					repo::core::handler::AbstractDatabaseHandler* handler,
					const std::string& teamspace,
					const repo::lib::RepoUUID& revision
				);

				/**
				* Writes the specificed DrawingImageInfo to the filesystem and
				* updates the DrawingRevisionNode image member to point to it.
				*/
				uint8_t commitImage(
					repo::core::handler::AbstractDatabaseHandler* handler,
					repo::core::handler::fileservice::FileManager* fileManager,
					const std::string& teamspace,
					const DrawingRevisionNode& revision,
					manipulator::drawingconverter::DrawingImageInfo& drawing
				);
			};
		}
	}
}
