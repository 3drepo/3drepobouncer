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
#include "repo/repo_bouncer_global.h"
#include "repo/lib/datastructure/repo_uuid.h"

namespace repo {
	namespace lib {
		// Convenience type to pass Container addresses around where many references
		// to the same Container are expected. Uses the new terminology: teamspace is
		// database and container is project.

		REPO_API_EXPORT struct Container {
			std::string teamspace;
			std::string container;
			repo::lib::RepoUUID revision;
		};
	}
}