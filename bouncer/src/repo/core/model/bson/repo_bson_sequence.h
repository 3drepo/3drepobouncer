/**
*  Copyright (C) 2019 3D Repo Ltd
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
#include <unordered_map>
#include "repo_bson.h"

namespace repo {
	namespace core {
		namespace model {
			//------------------------------------------------------------------------------
			//
			// Fields specific to ref only
			//
			//------------------------------------------------------------------------------
#define REPO_SEQUENCE_LABEL_REV_ID "rev_id"
#define REPO_SEQUENCE_LABEL_NAME "name"
#define REPO_SEQUENCE_LABEL_FRAMES "frames"
#define REPO_SEQUENCE_LABEL_DATE "dateTime"
#define REPO_SEQUENCE_LABEL_START_DATE "startDate"
#define REPO_SEQUENCE_LABEL_END_DATE "endDate"
#define REPO_SEQUENCE_LABEL_STATE "state"

			class REPO_API_EXPORT RepoSequence : public RepoBSON
			{
			public:

				struct FrameData {
					uint64_t timestamp;
					std::string ref;
				};

				RepoSequence() : RepoBSON() {}

				RepoSequence(RepoBSON bson) : RepoBSON(bson) {}

				RepoSequence cloneAndAddRevision(const repo::lib::RepoUUID &rid) const;

				~RepoSequence() {}
			};
		}// end namespace model
	} // end namespace core
} // end namespace repo
