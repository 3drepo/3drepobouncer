/**
*  Copyright (C) 2025 3D Repo Ltd
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

#include "repo_polydepth.h"
#include "geometry_tests.h"

#include "repo/manipulator/modeloptimizer/bvh/sweep_sah_builder.hpp"

using namespace geometry;

using Bvh = bvh::Bvh<double>;

#pragma optimize("", off)

Bvh buildBvh(const std::vector<repo::lib::RepoTriangle>& triangles)
{
    auto bounds = std::vector<bvh::BoundingBox<double>>();
    auto centers = std::vector<bvh::Vector3<double>>();

    bounds.reserve(triangles.size());
	centers.reserve(triangles.size());

    for (const auto& triangle : triangles)
    {
        auto aabb = bvh::BoundingBox<double>::empty();
        aabb.extend(reinterpret_cast<const bvh::Vector3<double>&>(triangle.a));
        aabb.extend(reinterpret_cast<const bvh::Vector3<double>&>(triangle.b));
        aabb.extend(reinterpret_cast<const bvh::Vector3<double>&>(triangle.c));
        centers.push_back(aabb.center());
		bounds.push_back(aabb);
    }

    auto globalBounds = bvh::compute_bounding_boxes_union(bounds.data(), bounds.size());

    Bvh bvh;
    bvh::SweepSahBuilder<Bvh> builder(bvh);
    builder.max_leaf_size = 1;
    builder.build(globalBounds, bounds.data(), centers.data(), bounds.size());

    return bvh;
}

RepoPolyDepth::RepoPolyDepth(const std::vector<repo::lib::RepoTriangle>& a, const std::vector<repo::lib::RepoTriangle>& b)
    : a(a), b(b), bvhA(buildBvh(a)), bvhB(buildBvh(b))
{
    findInitialFreeConfiguration();
}

repo::lib::RepoVector3D64 RepoPolyDepth::getPenetrationVector() const {
    return qi.translation();
}

void RepoPolyDepth::findInitialFreeConfiguration()
{
    // Finds a translation for a which is collision free. Currently this is
    // done simply by finding the minimum translation vector that can separate
    // the bounds.

    auto& left = bvhA.nodes[0].bounds;
    auto& right = bvhB.nodes[0].bounds;

    repo::lib::RepoBounds a(
        repo::lib::RepoVector3D64(left[0], left[2], left[4]),
        repo::lib::RepoVector3D64(left[1], left[3], left[5])
    );

    repo::lib::RepoBounds b(
        repo::lib::RepoVector3D64(right[0], right[2], right[4]),
        repo::lib::RepoVector3D64(right[1], right[3], right[5])
    );

    auto v = geometry::minimumSeparatingAxis(a,b);

    qt = repo::lib::RepoMatrix::translate(v);
    qi = qt;
}