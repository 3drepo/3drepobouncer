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
#include "repo/manipulator/modeloptimizer/bvh/hierarchy_refitter.hpp"
#include "bvh_operators.h"

using namespace geometry;

using Bvh = bvh::Bvh<double>;

namespace {

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

    // Updates the Bvh bottom-up, refitting the bounds to the triangles under a given
    // transformation. The Bvh is updated in-place, but the original triangles are
    // not modified.

    struct BvhRefitter
    {
        BvhRefitter(Bvh& bvh, const std::vector<repo::lib::RepoTriangle>& primitives)
            : bvh(bvh), primitives(primitives)
        {
        }

        Bvh& bvh;
        const std::vector<repo::lib::RepoTriangle>& primitives;
        repo::lib::RepoMatrix m;

        void refit(const repo::lib::RepoMatrix& m)
        {
            this->m = m;
            bvh::HierarchyRefitter<bvh::Bvh<double>> refitter(bvh);
            refitter.refit(*this);
        }

        void updateLeaf(Bvh::Node& leaf, const repo::lib::RepoMatrix& m) const
        {
            auto b = bvh::BoundingBox<double>::empty();
            for (size_t i = 0; i < leaf.primitive_count; i++) {
                auto t = m * primitives[bvh.primitive_indices[leaf.first_child_or_primitive + i]];
                b.extend(reinterpret_cast<const bvh::Vector3<double>&>(t.a));
                b.extend(reinterpret_cast<const bvh::Vector3<double>&>(t.b));
                b.extend(reinterpret_cast<const bvh::Vector3<double>&>(t.c));
            }
            leaf.bounding_box_proxy() = b;
        }

        void operator()(const Bvh::Node& node) const
        {
            updateLeaf(const_cast<Bvh::Node&>(node), m);
        }
    };

    struct DistanceQuery : public bvh::DistanceQuery
    {
        DistanceQuery(const std::vector<repo::lib::RepoTriangle>& A, const std::vector<repo::lib::RepoTriangle>& B)
            : A(A), B(B)
        {
            this->d = std::numeric_limits<double>::max();
        }

        const std::vector<repo::lib::RepoTriangle>& A;
        const std::vector<repo::lib::RepoTriangle>& B;
        repo::lib::RepoMatrix m;

        void intersect(size_t a, size_t b) override {
            auto triA = m * A[a];
            auto m = geometry::closestPointTriangleTriangle(triA, B[b]).magnitude();
            if (m < d) {
                d = m;
            }
        }

        double operator()(Bvh& bvhA, const Bvh& bvhB, const repo::lib::RepoMatrix& m) {
            this->m = m;

            BvhRefitter refitter(bvhA, A);
            refitter.refit(m);

            bvh::DistanceQuery::operator()(bvhA, bvhB);

            return d;
        }
    };
}

RepoPolyDepth::RepoPolyDepth(const std::vector<repo::lib::RepoTriangle>& a, const std::vector<repo::lib::RepoTriangle>& b)
    : a(a), b(b), bvhA(buildBvh(a)), bvhB(buildBvh(b))
{
    findInitialFreeConfiguration();
}

repo::lib::RepoVector3D64 RepoPolyDepth::getPenetrationVector() const {
    return qs.translation();
}

void RepoPolyDepth::findInitialFreeConfiguration()
{
    if (intersect({}) != Collision::Collision) {
        return;
	}

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

    qs = repo::lib::RepoMatrix::translate(v);
}

void RepoPolyDepth::iterate(size_t n)
{
    if (qs.isIdentity()) {  // If we have begun in a collision-free state, there is nothing to do..
        return;
	}

    for (auto i = 0; i < n; i++) {
        // Perform out-projection to get an updated estimate of qi

        auto q = ccd(qs, qt);

        // Check qi is collision free

		auto r = intersect(q);

        switch (r) {
        case Collision::Collision:
            // qs remains unchanged..
            qt = q;
            break;
        case Collision::Contact:
            qs = q;
            return;
        case Collision::Free:
            qs = q;
            qt = {};
            break;
        }
    }
}

double RepoPolyDepth::distance(const repo::lib::RepoMatrix& q)
{
    DistanceQuery qd(a, b);
    return qd(bvhA, bvhB, q);
}

RepoPolyDepth::Collision RepoPolyDepth::intersect(const repo::lib::RepoMatrix& m)
{
    BvhRefitter refitter(bvhA, a);
    refitter.refit(m);

    auto r = Collision::Free;
    contacts.clear();
    bvh::intersect(bvhA, bvhB, [&](size_t _a, size_t _b) 
        {
            auto triA = m * a[_a];
            auto d = geometry::intersects(triA, b[_b]);

            if (d > geometry::coplanarityThreshold(triA, b[_b])) {
                r = Collision::Collision;
            }
            else if (d > 0) {
                if (r == Collision::Free) {
                    r = Collision::Contact;
                }
            }

            if (r != Collision::Free) {
                // If we detect a contact, append the contact information as a
                // plane, where the point on the plane is the offset (translation
                // of q).

                auto j = b[_b].normal();
                auto q = m.translation();
                auto c = -j.dotProduct(q);

                contacts.push_back({ j, c });
            }
        }
    );

    return r;
}

repo::lib::RepoMatrix RepoPolyDepth::ccd(const repo::lib::RepoMatrix& q0, const repo::lib::RepoMatrix& q1)
{
    // The CCD algorithm used is based on Conservative Advancement, as this is
    // amenable to polygon soups.

    // This algorithm works by advancing between q0 and q1 in bounded steps that
	// are guaranteed to be collision free. This is done by computing the shortest
    // vector between any two features, and advancing along it by a fraction. The
    // fraction is the upper motion bound, which is the distance along the vector
    // covered by any feature in the moving set during the transformation.

    // See:
    // Interactive Continuous Collision Detection for Non-Convex Polyhedra. Xinyu
    // Zhang, Minkyoung Lee, Young J. Kim. The Visual Computer: International
    // Journal of Computer Graphics, Volume 22, Issue 9. 
    // https://graphics.ewha.ac.kr/FAST/FAST.pdf

    // As we do not consider rotations, the upper motion bound can be set to 1, if
    // this changes, this is where the computation would be implemented.

	auto translation = q1.translation() - q0.translation();
	auto d = this->distance(q0);

    // Step q0 towards q1 by the maximum safe translation, which is d
    
    auto n = translation;
    n.normalize();
    auto dt = n * std::min(translation.norm(), d);
	return repo::lib::RepoMatrix::translate(q0.translation() + dt);
}