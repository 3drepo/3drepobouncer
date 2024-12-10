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

#include "repo/repo_bouncer_global.h"
#include "repo/lib/datastructure/repo_uuid.h"
#include "repo/lib/datastructure/repo_vector.h"
#include <vector>

namespace repo {
	namespace core {
		namespace model {

			class RepoBSON;

			class REPO_API_EXPORT RepoCalibration
			{
			public:
				RepoCalibration(
					const repo::lib::RepoUUID& projectId,
					const repo::lib::RepoUUID& drawingId,
					const repo::lib::RepoUUID& revisionId,
					const std::vector<repo::lib::RepoVector3D>& horizontal3d,
					const std::vector<repo::lib::RepoVector2D>& horizontal2d,
					const std::string& units
				);

				operator RepoBSON() const;

				~RepoCalibration() {}

			private:
				repo::lib::RepoUUID id;
				repo::lib::RepoUUID projectId;
				repo::lib::RepoUUID drawingId;
				repo::lib::RepoUUID revisionId;
				std::vector<repo::lib::RepoVector3D> horizontal3d;
				std::vector<repo::lib::RepoVector2D> horizontal2d;
				std::string units;
			};
		}// end namespace model
	} // end namespace core
} // end namespace repo
