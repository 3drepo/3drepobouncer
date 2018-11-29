/**
*  Copyright (C) 2018 3D Repo Ltd
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
*  Ref BSON
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
			#define REPO_REF_LABEL_LINK "link"
			#define REPO_REF_LABEL_SIZE "size"

			class REPO_API_EXPORT RepoRef : public RepoBSON
			{
			public:

				RepoRef() : RepoBSON() {}

				RepoRef(RepoBSON bson) : RepoBSON(bson){}

				~RepoRef() {}
			};
		}// end namespace model
	} // end namespace core
} // end namespace repo
