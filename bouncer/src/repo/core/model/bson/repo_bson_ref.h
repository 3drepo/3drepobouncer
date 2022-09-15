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
#define REPO_REF_LABEL_TYPE "type"

			class REPO_API_EXPORT RepoRef : public RepoBSON
			{
			public:
				const static std::string REPO_REF_TYPE_FS;
				const static std::string REPO_REF_TYPE_GRIDFS;
				const static std::string REPO_REF_TYPE_UNKNOWN;

				enum class RefType {
					GRIDFS, FS, UNKNOWN
				};

				RepoRef() : RepoBSON() {}

				RepoRef(RepoBSON bson) : RepoBSON(bson) {}

				~RepoRef() {}

				static std::string convertTypeAsString(const RefType &type);

				std::string getFileName() const;

				std::string getRefLink() const;

				RefType getType() const;
			};
		}// end namespace model
	} // end namespace core
} // end namespace repo
