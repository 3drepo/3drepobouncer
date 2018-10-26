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
*  Unity assets BSON
*/

#pragma once
#include "repo_bson.h"

namespace repo {
	namespace core {
		namespace model {
			//------------------------------------------------------------------------------
			//
			// Fields specific to user only
			//
			//------------------------------------------------------------------------------
			#define REPO_UNITY_ASSETS_LABEL_ASSETS "assets"
			#define REPO_UNITY_ASSETS_LABEL_OFFSET "offset"
			#define REPO_UNITY_ASSETS_LABEL_VRASSETS "vrAssets"
			#define REPO_UNITY_ASSETS_LABEL_JSONFILES "jsonFiles"
			class REPO_API_EXPORT RepoUnityAssets : public RepoBSON
			{
			public:

				RepoUnityAssets() : RepoBSON() {}

				RepoUnityAssets(RepoBSON bson) : RepoBSON(bson){}

				~RepoUnityAssets() {}
			};
		}// end namespace model
	} // end namespace core
} // end namespace repo
