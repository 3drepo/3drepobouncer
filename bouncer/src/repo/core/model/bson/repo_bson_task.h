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
#include "repo_bson.h"

namespace repo {
	namespace core {
		namespace model {
			//------------------------------------------------------------------------------
			//
			// Fields specific to ref only
			//
			//------------------------------------------------------------------------------
			#define REPO_TASK_LABEL_NAME "name"
			#define REPO_TASK_LABEL_DATA "data"
			#define REPO_TASK_LABEL_SEQ_ID "sequenceId"
			#define REPO_TASK_LABEL_START "startDate"
			#define REPO_TASK_LABEL_END "endDate"
			#define REPO_TASK_LABEL_PARENT "parent"
			#define REPO_TASK_LABEL_RESOURCES "resources"
			#define REPO_TASK_SHARED_IDS "shared_ids"
			#define REPO_TASK_META_KEY "key"
			#define REPO_TASK_META_VALUE "value"

			class REPO_API_EXPORT RepoTask
			{
			public:
				RepoTask(
					const std::string& name,
					const repo::lib::RepoUUID& uniqueId,
					const repo::lib::RepoUUID& parentId,
					const repo::lib::RepoUUID& sequenceId,
					const long long& startTime,
					const long long& endTime,
					const std::vector<repo::lib::RepoUUID>& resources,
					const std::unordered_map<std::string, std::string>& metdata
				);

				~RepoTask() {}

				operator RepoBSON() const;

			private:

				std::string name;
				repo::lib::RepoUUID uniqueId;
				repo::lib::RepoUUID parentId;
				repo::lib::RepoUUID sequenceId;
				long long startTime;
				long long endTime;
				std::vector<repo::lib::RepoUUID> resources;
				std::unordered_map<std::string, std::string> metadata;
			};
		}// end namespace model
	} // end namespace core
} // end namespace repo
