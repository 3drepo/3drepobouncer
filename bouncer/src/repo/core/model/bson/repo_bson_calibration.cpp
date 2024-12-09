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

#include "repo_bson_calibration.h"
#include "repo_bson_builder.h"

using namespace repo::core::model;

RepoCalibration::RepoCalibration(
	const repo::lib::RepoUUID& projectId,
	const repo::lib::RepoUUID& drawingId,
	const repo::lib::RepoUUID& revisionId,
	const std::vector<repo::lib::RepoVector3D>& horizontal3d,
	const std::vector<repo::lib::RepoVector2D>& horizontal2d,
	const std::string& units
)
	:projectId(projectId),
	drawingId(drawingId),
	revisionId(revisionId),
	horizontal3d(horizontal3d),
	horizontal2d(horizontal2d),
	units(units),
	id(repo::lib::RepoUUID::createUUID())
{
	if (horizontal2d.size() != 2 || horizontal3d.size() != 2)
	{
		throw repo::lib::RepoException("Incomplete calibration vectors supplied to makeRepoCalibration");
	}
}

RepoCalibration::operator RepoBSON() const
{
	RepoBSONBuilder builder;
	builder.append(REPO_LABEL_ID, id);
	builder.append(REPO_LABEL_PROJECT, projectId);
	builder.append(REPO_LABEL_DRAWING, drawingId.toString()); // By convention drawing ids must be strings
	builder.append(REPO_LABEL_REVISION, revisionId);
	builder.appendTimeStamp(REPO_LABEL_CREATEDAT);

	RepoBSONBuilder horizontalBuilder;
	horizontalBuilder.appendArray< std::vector<float> >(REPO_LABEL_MODEL, {
		horizontal3d[0].toStdVector(),
		horizontal3d[1].toStdVector()
		});
	horizontalBuilder.appendArray< std::vector<float> >(REPO_LABEL_DRAWING, {
		horizontal2d[0].toStdVector(),
		horizontal2d[1].toStdVector()
		});
	builder.append(REPO_LABEL_HORIZONTAL, horizontalBuilder.obj());

	builder.append(REPO_LABEL_UNITS, units);

	return builder.obj();
}