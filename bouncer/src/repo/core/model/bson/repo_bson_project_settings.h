/**
*  Copyright (C) 2015 3D Repo Ltd
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
*  Project setting BSON
*/

#pragma once

#include "repo/repo_bouncer_global.h"
#include <string>
#include <ctime>

namespace repo {
	namespace core {
		namespace model {

			class RepoBSON;

			#define REPO_PROJECT_SETTINGS_LABEL_STATUS "status"
			#define REPO_PROJECT_SETTINGS_LABEL_TIMESTAMP "timestamp"

			class REPO_API_EXPORT RepoProjectSettings
			{
			public:

				// RepoProjectSettings can only be initialised from an existing
				// document, and should only be used with upsert

				RepoProjectSettings(RepoBSON bson);

				~RepoProjectSettings() {}

				operator RepoBSON() const;

				void setErrorStatus();

				void clearErrorStatus();

				const std::string& getProjectId() const
				{
					return id;
				}

				const std::string& getStatus() const
				{
					return status;
				}

			private:
				std::string id;
				std::string status;
			};
		}// end namespace model
	} // end namespace core
} // end namespace repo
