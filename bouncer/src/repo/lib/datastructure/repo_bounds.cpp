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
#include "repo_matrix.h"
#include <cfloat>
#include <cfenv>

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

repo::lib::RepoBounds::RepoBounds(std::initializer_list<RepoVector3D64> points)
	:RepoBounds()
{
	for (auto& p : points) {
		encapsulate(p);
	}
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

bool RepoBounds::contains(const RepoVector3D64& p) const
{
	return (p.x >= bmin.x && p.x <= bmax.x &&
		p.y >= bmin.y && p.y <= bmax.y &&
		p.z >= bmin.z && p.z <= bmax.z);
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

RepoVector3D64 RepoBounds::center() const
{
	return (bmin + bmax) * 0.5;
}

REPO_API_EXPORT repo::lib::RepoBounds repo::lib::operator*(const _RepoMatrix<double>& matrix, const repo::lib::RepoBounds& bounds)
{
	/**
	* This implementation uses the center-extents approach to transform the bounds,
	* such as explained here: 
	* https://zeux.io/2010/10/17/aabb-from-obb-with-component-wise-abs/
	* This slightly more expensive than Avro's Graphics Gems approach, but it is
	* robust to non-uniform scaling, saving the expense of that check.
	*/

	auto center = (bounds.min() + bounds.max()) * 0.5;

	auto transformedCenter = matrix * center;

	repo::lib::RepoMatrix mAbs(matrix.getData());
	auto data = mAbs.getData();
	for(int i = 0; i < 3; ++i)
	{
		for(int j = 0; j < 3; ++j)
		{
			data[i * 4 + j] = fabs(data[i * 4 + j]);
		}
	}

	// The following lines configure the floating point rounding mode to always
	// round up. Accuracy loss is inevitable in fp calculations, however because
	// in this case we are working with bounding boxes, there is a correct way
	// to deal with these losses - the one that makes the bounds larger.

	auto currentRoundingMode = std::fegetround();
	std::fesetround(FE_UPWARD);

	auto extents = (bounds.max() - bounds.min()) * 0.5;
	auto transformedExtents = mAbs.transformDirection(extents);

	auto transformed = repo::lib::RepoBounds(transformedCenter - transformedExtents, transformedCenter + transformedExtents);

	std::fesetround(currentRoundingMode);

	return transformed;
}
