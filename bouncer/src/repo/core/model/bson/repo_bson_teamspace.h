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

#pragma once

#include "repo/repo_bouncer_global.h"
#include <string>
#include <set>

namespace repo {
	namespace core {
		namespace model {
			
			class RepoBSON;

			#define REPO_USER_LABEL_VR_ENABLED					"vrEnabled"
			#define REPO_USER_LABEL_SRC_ENABLED					"srcEnabled"

			class REPO_API_EXPORT RepoTeamspace
			{
			public:

				// RepoTeamspace provides access to the teamspace settings
				// document. This is a read-only object.
	
				RepoTeamspace(RepoBSON bson);

				~RepoTeamspace() {}

				// Currently the only supported member is the addons

				bool isAddOnEnabled(const std::string& addOnName);

			private:
				std::set<std::string> addOns;
			};
		}// end namespace model
	} // end namespace core
} // end namespace repo
