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

#pragma once

#include <string>
#include <SharedPtr.h>
#include <OdString.h>
#include <OdaCommon.h>
#include <BimCommon.h>
#include <DbBaseDatabase.h>
#include <Database/BmDatabase.h>
#include <Database/Entities/BmDBView.h>
#include <Database/Entities/BmViewport.h>
#include <Database/Entities/BmDBDrawing.h>
#include <Database/Enums/BmViewTypeEnum.h>

#include "repo/lib/datastructure/repo_structs.h"
#include "repo/lib/datastructure/repo_bounds.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				extern const double DOUBLE_TOLERANCE;

				std::string convertToStdString(const OdString &value);

				template <class T>
				struct _RepoVector3DSortComparator
				{
					bool operator()(const T& vec1, const T& vec2) const
					{
						if (vec1.x > vec2.x)
							return true;
						if (vec1.x < vec2.x)
							return false;
						if (vec1.y > vec2.y)
							return true;
						if (vec1.y < vec2.y)
							return false;
						if (vec1.z > vec2.z)
							return true;

						return false;
					}
				};

				using RepoVector3DSortComparator = _RepoVector3DSortComparator < repo::lib::RepoVector3D >;
				using RepoVector3D64SortComparator = _RepoVector3DSortComparator < repo::lib::RepoVector3D64 >;

				//.. NOTE: this function iterates over database views
				//.. we already have a couple of functions that uses this iteration
				//.. so in order to reduce code duplication we are using iteration with an action callback
				void forEachBmDBView(OdBmDatabasePtr database, std::function<void(OdBmDBViewPtr viewPtr)> func);

				int compare(double d1, double d2);

				repo::lib::RepoVector3D64 calcNormal(repo::lib::RepoVector3D64 p1, repo::lib::RepoVector3D64 p2, repo::lib::RepoVector3D64 p3);

				repo::lib::RepoVector3D64 toRepoVector(const OdGePoint3d& p);
				repo::lib::RepoVector3D64 toRepoVector(const OdGeVector3d& p);
				repo::lib::RepoBounds toRepoBounds(const OdGeExtents3d& b);
			}
		}
	}
}