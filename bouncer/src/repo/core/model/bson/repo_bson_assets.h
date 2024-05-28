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
*  Assets BSON
*/

#pragma once

#include "repo_bson.h"
#include "repo/core/model/repo_model_global.h"
#include "repo/lib/datastructure/repo_vector.h"

namespace repo {
	namespace core {
		namespace model {
			//------------------------------------------------------------------------------
			//
			// Fields that define information about the assets that make up a container
			//
			//------------------------------------------------------------------------------
			class REPO_API_EXPORT RepoSupermeshMetadata
			{
			public:
				size_t numVertices;
				size_t numFaces;
				size_t numUVChannels;
				size_t primitive;
				repo::lib::RepoVector3D min;
				repo::lib::RepoVector3D max;
			};

			class REPO_API_EXPORT RepoAssets : public RepoBSON
			{
			public:

				RepoAssets() : RepoBSON() {}

				RepoAssets(RepoBSON bson) : RepoBSON(bson) {}

				~RepoAssets() {}
			};
		}// end namespace model
	} // end namespace core
} // end namespace repo
