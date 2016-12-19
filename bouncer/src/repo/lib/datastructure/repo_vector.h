/**
*  Copyright (C) 2016 3D Repo Ltd
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

#include "repo_vector2d.h"
#include "repo_vector3d.h"

namespace repo{
	namespace lib{

		using RepoVector2D = _RepoVector2D < float >;
		using RepoVector3D = _RepoVector3D < float >;
		using RepoVector3D64 = _RepoVector3D < double >;

		
		template <typename T>
		bool operator==(const _RepoVector2D<T> &a, const _RepoVector2D<T> &b)
		{
			return a.x == b.x && a.y == b.y;
		}

		template <typename T>
		bool operator!=(const _RepoVector2D<T> &a, const _RepoVector2D<T> &b)
		{
			return !(a == b);
		}


		template <typename T>
		bool operator==(const _RepoVector3D<T> &a, const _RepoVector3D<T> &b)
		{
			return a.x == b.x && a.y == b.y && a.z == b.z;
		}

		template <typename T>
		bool operator!=(const _RepoVector3D<T> &a, const _RepoVector3D<T> &b)
		{
			return !(a == b);
		}
	}
	
}
