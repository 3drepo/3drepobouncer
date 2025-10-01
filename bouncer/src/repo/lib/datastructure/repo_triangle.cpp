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

#include "repo_triangle.h"
#include "repo_matrix.h"

using namespace repo::lib;

template<typename T, typename N>
inline _RepoTriangle<T> repo::lib::operator*(const _RepoMatrix<N>& m, const _RepoTriangle<T>& t)
{
	return _RepoTriangle<T>(
		m * t.a,
		m * t.b,
		m * t.c
	);
}

template _RepoTriangle<double> repo::lib::operator*(const RepoMatrix& m, const _RepoTriangle<double>& t);
