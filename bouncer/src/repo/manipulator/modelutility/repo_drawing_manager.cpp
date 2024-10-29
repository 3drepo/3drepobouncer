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

#include "repo/core/model/bson/repo_bson_ref.h"
#include "repo/core/model/bson/repo_bson_factory.h"
#include "repo/core/model/bson/repo_bson.h"
#include "repo/error_codes.h"

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
	DrawingRevisionNode& revision,
	const DrawingImageInfo& drawing
)
{
	auto drawingRefNodeId = repo::lib::RepoUUID::createUUID();
	auto name = drawing.name.substr(0, drawing.name.size() - 3) + "svg"; // The name should be the drawing's original name with an updated extension

	repo::core::handler::AbstractDatabaseHandler::Metadata metadata;
	auto revId = revision.getUniqueID();
	metadata[REPO_NODE_LABEL_NAME] = name;
	metadata[REPO_LABEL_MEDIA_TYPE] = std::string(REPO_MEDIA_TYPE_SVG);
	metadata[REPO_LABEL_PROJECT] = revision.getProject();
	metadata[REPO_LABEL_MODEL] = revision.getModel();
	metadata[REPO_NODE_REVISION_ID] = revId;

	fileManager->uploadFileAndCommit(
		teamspace,
		REPO_COLLECTION_DRAWINGS,
		drawingRefNodeId,
		drawing.data,
		metadata,
		repo::core::handler::fileservice::FileManager::Encoding::Gzip
	);

	revision.addImage(drawingRefNodeId);

	std::string error;
	handler->upsertDocument(teamspace, REPO_COLLECTION_DRAWINGS, revision, false, error);

	if (error.size())
	{
		repoError << "Error committing drawing: " << error;
		return REPOERR_UPLOAD_FAILED;
	}

	// Retreive and process calibration - drawing processors do not have to return a calibration,
	// in which case the vectors will be empty.

	auto calibration = drawing.calibration;
	if (calibration.valid()) {

		auto calibrationBSON = repo::core::model::RepoBSONFactory::makeRepoCalibration(
			revision.getProject(),
			revision.getModel(),
			revId,
			calibration.horizontalCalibration3d,
			calibration.horizontalCalibration2d,
			calibration.units
		);

		handler->insertDocument(teamspace, REPO_COLLECTION_CALIBRATIONS, calibrationBSON, error);

		if (error.size())
		{
			repoError << "Error committing calibration: " << error;
			return REPOERR_UPLOAD_FAILED;
		}
	}

	return REPOERR_OK;
}