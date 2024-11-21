/**
*  Copyright (C) 2024 3D Repo Ltd
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

#include "repo_bounds.h"

using namespace repo::lib;

RepoBounds::RepoBounds()
	:bmin(RepoVector3D64(DBL_MAX, DBL_MAX, DBL_MAX)), bmax(RepoVector3D64(-DBL_MAX, -DBL_MAX, -DBL_MAX))
{
}

RepoBounds::RepoBounds(const RepoVector3D64& min, const RepoVector3D64& max)
	:RepoBounds()
{
	encapsulate(min); // Make the constructor robust against the min and max (or their elements) being swapped
	encapsulate(max);
}

RepoBounds::RepoBounds(const RepoVector3D& min, const RepoVector3D& max)
	:RepoBounds(RepoVector3D64(min.x, min.y, min.z), RepoVector3D64(max.x, max.y, max.z))
{
}

void RepoBounds::encapsulate(const RepoVector3D64& p)
{
	bmin = repo::lib::RepoVector3D64::min(bmin, p);
	bmax = repo::lib::RepoVector3D64::max(bmax, p);
}

void RepoBounds::encapsulate(const RepoBounds& v)
{
	bmin = repo::lib::RepoVector3D64::min(bmin, v.bmin);
	bmax = repo::lib::RepoVector3D64::max(bmax, v.bmax);
}

bool RepoBounds::operator==(const RepoBounds& other) const
{
	return this->bmin == other.bmin && this->bmax == other.bmax;
}

const RepoVector3D64& RepoBounds::min() const
{
	return bmin;
}

const RepoVector3D64& RepoBounds::max() const
{
	return bmax;
}

RepoVector3D64 RepoBounds::size() const
{
	return bmax - bmin;
}