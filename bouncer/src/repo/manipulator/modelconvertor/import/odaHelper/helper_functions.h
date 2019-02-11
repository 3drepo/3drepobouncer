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

#include "../../../../lib/datastructure/repo_structs.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
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
			}
		}
	}
}