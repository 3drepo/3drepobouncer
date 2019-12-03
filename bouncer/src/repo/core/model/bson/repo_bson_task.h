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
			#define REPO_TASK_LABEL_ACTION "action"
			#define REPO_TASK_LABEL_VALUE "name"
			#define REPO_TASK_LABEL_OBJECTS "sequence"
			#define REPO_TASK_LABEL_FOV "fov"
			#define REPO_TASK_LABEL_IS_PERSPECTIVE "isPerspective"
			#define REPO_TASK_LABEL_POSITION "position"
			#define REPO_TASK_LABEL_FORWARD "forward"
			#define REPO_TASK_LABEL_UP "up"

			class REPO_API_EXPORT RepoTask : public RepoBSON
			{
			public:				
				const static std::string RepoTask::REPO_TASK_TYPE_VISIBILITY;
				const static std::string RepoTask::REPO_TASK_TYPE_COLOR;
				const static std::string RepoTask::REPO_REF_TYPE_CAMERA;

				RepoTask() : RepoBSON() {}

				RepoTask(RepoBSON bson) : RepoBSON(bson){}

				~RepoTask() {}		

			};
		}// end namespace model
	} // end namespace core
} // end namespace repo
