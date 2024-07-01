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
#include "repo_drawing_manager.h"

#include "../../core/model/bson/repo_bson_builder.h"
#include "../../core/model/bson/repo_bson_ref.h"
#include "../../error_codes.h"

using namespace repo::manipulator::modelutility;

DrawingRevisionNode DrawingManager::fetchRevision(
	repo::core::handler::AbstractDatabaseHandler* handler,
	const std::string& teamspace,
	const repo::lib::RepoUUID& revision
)
{
	return DrawingRevisionNode(
		handler->findOneByUniqueID(teamspace, REPO_COLLECTION_DRAWINGS, revision)
	);
}

uint8_t DrawingManager::commitImage(
	repo::core::handler::AbstractDatabaseHandler* handler,
	repo::core::handler::fileservice::FileManager* fileManager,
	const std::string& teamspace,
	const DrawingRevisionNode& revision,
	DrawingImageInfo& drawing
)
{
	repo::core::model::RepoBSONBuilder metadata;
	metadata.append(REPO_NODE_LABEL_NAME, drawing.name); // This is the name of the original file (e.g. "Floor1.DWG")
	metadata.append(REPO_LABEL_MEDIA_TYPE, REPO_MEDIA_TYPE_SVG);
	metadata.append(REPO_LABEL_PROJECT, revision.getProject());
	metadata.append(REPO_LABEL_MODEL, revision.getModel());
	metadata.append(REPO_NODE_REVISION_ID, revision.getUniqueID());

	auto drawingRefNodeId = repo::lib::RepoUUID::createUUID();

	fileManager->uploadFileAndCommit(
		teamspace,
		REPO_COLLECTION_DRAWINGS,
		drawingRefNodeId.toString(),
		drawing.data,
		metadata.obj()
	);

	auto updated = revision.cloneAndAddImage(drawingRefNodeId);

	std::string error;
	handler->upsertDocument(teamspace, REPO_COLLECTION_DRAWINGS, updated, false, error);

	if (error.size())
	{
		repoError << "Error committing drawing: " << error;
		return REPOERR_UPLOAD_FAILED;
	}

	return REPOERR_OK;
}