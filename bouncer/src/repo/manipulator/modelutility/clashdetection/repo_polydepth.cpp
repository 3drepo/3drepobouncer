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
#include "repo_linear_algebra.h"

#include "repo/manipulator/modeloptimizer/bvh/sweep_sah_builder.hpp"
#include "repo/manipulator/modeloptimizer/bvh/hierarchy_refitter.hpp"
#include "bvh_operators.h"

using namespace geometry;

using Bvh = bvh::Bvh<double>;

namespace {
    repo::lib::RepoBounds repoBounds(const bvh::Bvh<double>::Node& a) {
        return repo::lib::RepoBounds(
            repo::lib::RepoVector3D64(a.bounds[0], a.bounds[2], a.bounds[4]),
            repo::lib::RepoVector3D64(a.bounds[1], a.bounds[3], a.bounds[5])
        );
    }

    Bvh buildBvh(const std::vector<repo::lib::RepoTriangle>& triangles) {
        Bvh bvh;
        bvh::builders::build(bvh, triangles);
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
        repo::lib::RepoVector3D64 m;

        void refit(const repo::lib::RepoVector3D64& m)
        {
            this->m = m;
            if (bvh.node_count <= 1) { // Special case if the BVH is just a leaf (not currently handled by BottomUpAlgorithm)
                updateLeaf(bvh.nodes[0], m);
				return;
            }
            bvh::HierarchyRefitter<bvh::Bvh<double>> refitter(bvh);
            refitter.refit(*this);
        }

        void updateLeaf(Bvh::Node& leaf, const repo::lib::RepoVector3D64& m) const
        {
            auto b = bvh::BoundingBox<double>::empty();
            for (size_t i = 0; i < leaf.primitive_count; i++) {
                auto t = primitives[bvh.primitive_indices[leaf.first_child_or_primitive + i]] + m;
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
        repo::lib::RepoVector3D64 m;

        void intersect(size_t a, size_t b) override {
            auto triA = A[a] + m;
            auto m = geometry::closestPoints(triA, B[b]).magnitude();
            if (m < d) {
                d = m;
            }
        }

        double operator()(Bvh& bvhA, const Bvh& bvhB, const repo::lib::RepoVector3D64& m) {
            this->m = m;

            BvhRefitter refitter(bvhA, A);
            refitter.refit(m);

            bvh::DistanceQuery::operator()(bvhA, bvhB);

            return d;
        }
    };
}

RepoPolyDepth::RepoPolyDepth(
    const std::vector<repo::lib::RepoTriangle>& a, 
    const std::vector<repo::lib::RepoTriangle>& b,
    const ContainsFunctor*)
    : a(a), 
    b(b),
    bvhA(buildBvh(a)),
    bvhB(buildBvh(b)),
    contains(contains)
{
    findInitialFreeConfiguration();
}

repo::lib::RepoVector3D64 RepoPolyDepth::getPenetrationVector() const {
    return qs;
}

void RepoPolyDepth::findInitialFreeConfiguration()
{
    if (a.empty() || b.empty()) {
        return;
    }

    if (intersect({}) != Collision::Collision) {
        return;
	}

    auto volA = bvhA.nodes[0].bounding_box_proxy().to_bounding_box().half_area();
    auto volB = bvhB.nodes[0].bounding_box_proxy().to_bounding_box().half_area();

    if (volA <= volB * localSearchRatioThreshold) {

        // Perform a local search for a collision-free configuration. Ideally most
        // small meshes will get a good initial guess from this. If it fails we
        // revert to a simple bounds separation.

        // This loop searches along the primary axes, not combinations thereof. This
        // is because those combinations can become very numerous, very quickly,
        // especially with multiple steps.

        auto axes = std::vector<repo::lib::RepoVector3D64>{
            repo::lib::RepoVector3D64(1,0,0),
            repo::lib::RepoVector3D64(0,1,0),
            repo::lib::RepoVector3D64(0,0,1)
		};

        for (double istep = 1; istep < numLocalSearchSteps; istep++) {
            for (auto& axis : axes) {
                for (int dir = -1; dir <= 1; dir += 2) {
					qs = axis * dir * (istep * localSearchStepSize) * volA;
                    if (intersect(qs) != Collision::Collision) { // Local search has found an intersection free configuration
                        return;
                    }
                }
            }
        }
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

    qs = geometry::minimumSeparatingAxis(a,b);
}

void RepoPolyDepth::iterate(size_t n)
{
    if (qs.norm() < FLT_EPSILON) {  // If we have begun in a collision-free state, there is nothing to do..
        return;
	}

    auto _qnorm = DBL_MAX;

    for (auto i = 0; i < n; i++) {
        // Perform out-projection to get an updated estimate of qi
        
        auto _qs = qs;
        qs = ccd(_qs, qt);

		// Perform in-projection to get a better estimate of qi

        auto q = project();

        // Check qi is collision free

		auto r = intersect(q);

        switch (r) {
        case Collision::Collision:
            // Choose a collision free-configuration to start the out-projection
            // from. Finds a configuration close to q0 but not in-contact by
            // slightly reverting configuration found by the previous ccd step.
            qs = _qs + (qs - _qs) * (1.0 - backStepSize);
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

        // In-projection, unlike out-projection, is not guaranteed to provide
        // a collision-free response, so detect if it gets stuck in local minima
        // in order to terminate early.

		auto qnorm = qs.norm();
        if (std::abs(_qnorm - qnorm) < convergenceEpsilon) {
            return;
        }
		_qnorm = qnorm;
    }
}

double RepoPolyDepth::distance(const repo::lib::RepoVector3D64& q)
{
    DistanceQuery qd(a, b);
    return qd(bvhA, bvhB, q);
}

RepoPolyDepth::Collision RepoPolyDepth::intersect(const repo::lib::RepoVector3D64& m)
{
    BvhRefitter refitter(bvhA, a);
    refitter.refit(m);

    contacts.clear();

    if (contains && contains->operator()(m)) {
        return Collision::Collision;
    }

    auto r = Collision::Free;
    bvh::traverse(bvhA, bvhB,
        [&](const Bvh::Node& a, const Bvh::Node& b) 
        {
            // Note we don't need to apply m to the bounds here because they've already been refitted above
            return closestPoints(repoBounds(a), repoBounds(b))
                .magnitude() < geometry::contactThreshold(repoBounds(a), repoBounds(b));
		},
        [&](size_t _a, size_t _b) 
        {
            auto triA = a[_a] + m;
            auto d = geometry::closestPoints(triA, b[_b]);

            if (d.intersects) {
                r = Collision::Collision;
            }
			else if (d.magnitude() < geometry::contactThreshold(triA, b[_b])) {
                if (r == Collision::Free) {
                    r = Collision::Contact; // Don't override a hard collision with an in-contact one!
                }

                // If two triangles are touching, they may form part of a closure
                // that means the two meshes are overlapping, which is considered
                // a collision, and so store these contacts to check for this
                // later.

                if (triA.normal().dotProduct(b[_b].normal()) > coplanarEpsilon) {
                    addContact(
                        triA.normal(),
                        triA.normal(), // We don't care about the point for overlaps detection - just ensure the plane constant is always positive
                        0.0
					);
                }
			}
        }
    );

    if (r == Collision::Contact) {
        // We consider an overlap to occur if a mesh cannot trivially move away.
        // A mesh can move trivially if it can translate along at least one
        // contact normal; if all normals oppose eachother then the mesh is
        // captured, being only able to move tangentially (if even then).

		for (int i = 0; i < contacts.size(); i++) {
            for (int j = i + 1; j < contacts.size(); j++) {
                if(contacts[i].normal.dotProduct(contacts[j].normal) < 0) {
					contacts[i].tau = 1.0; // Mark as opposed
					contacts[j].tau = 1.0; // Mark as opposed
                }
            }
		}
        bool free = !contacts.size();
        for (auto& c : contacts) {
            if (c.tau < 1.0) {
                free = true;
                break;
            }
        }

        if (!free) {
			r = Collision::Collision;
        }
    }

    return r;
}

repo::lib::RepoVector3D64 RepoPolyDepth::project()
{
    // As q may only translate A, once a primitive of A comes into contact with
    // a triangle of B, A may thereafter only move along that surface, but never
    // further into it.
    // As such, q can be considered a point on a plane, where the plane is the
    // surface of the Minkowski sum of B and A. These planes define the local
    // contact space.

    // Since q is the offset from the original (in-collision) state of A, finding
    // the smallest q possible will find the minimum translation to resolve the
    // collision.

    // If we say that the planes of the LCS always face outwards from the origin,
    // then finding this q' is equivalent to projecting the origin onto the planes,
    // or, finding the point closest to the origin that is on the positive side of
    // all the planes.

    // In the PolyDepth implementation, this is done by expressing the constraints
    // as a Linear Complementarity Problem (LCP).

	constexpr size_t ContactStride = sizeof(Contact) / sizeof(double); // Eigen expects the stride to be in elements (as opposed to bytes).

    repo::linearalgebra::matrix::View J(
        (double*)contacts.data(),
        contacts.size(),
        3,
        ContactStride
	);

    repo::linearalgebra::matrix::View c(
        (double*)contacts.data() + 3,
        contacts.size(),
        1,
        ContactStride
	);

    // Once we've reached the projection stage, the values of tau are no longer
    // relevant, so we can reuse that memory for lambda and avoid any allocations
    // during the solve.

    repo::linearalgebra::matrix::View x(
        (double*)contacts.data() + 4,
        contacts.size(),
        1,
        ContactStride);

    // Operands of the linear problem expressed as Ax = b (where JJ is A).

    auto Jt = J.transpose();
	auto JJ = J * Jt;

	// For the projected Gauss-Seidel solve, we use Golub & Van Loan's forward
    // substitution approach, which avoids a matrix inversion.
    // An alternative is a Jacobi iteration, which is simpler, but requires more
	// memory and is slower to converge.

   x.setZero();
   for(size_t k = 0; k < maxProjectionIterations; k++) {
        size_t n = JJ.rows();
        auto delta = 0.0;

        for (auto i = 0; i < n; i++) {
            auto sum = 0.0;

            for (auto j = 0; j < i; j++) {
                sum += JJ(i, j) * x(j);
            }
            for (auto j = i + 1; j < n; j++) {
                sum += JJ(i, j) * x(j);
            }

            auto existing = x(i);

            auto d = JJ(i, i);
			auto bs = (4.0 * c(i)) - sum;
            x(i) = bs / d;

            if (x(i) < 0) {
                x(i) = 0;
            }

            delta += std::abs(x(i) - existing);
        }

        if (std::abs(delta) < 1e-6) {
            break; // Converged
        }
    }

	auto q = Jt * x;

	return repo::lib::RepoVector3D64(q(0), q(1), q(2)) * 0.25;
}

repo::lib::RepoVector3D64 RepoPolyDepth::ccd(const repo::lib::RepoVector3D64& q0, const repo::lib::RepoVector3D64& q1)
{
	// This algorithm works by finding the time of contact along the vector
	// from q0 to q1.
 
    // Finding all features that are seperated by the same minimum distance will
    // also give us the contact manifold at the end of the motion.

    BvhRefitter refitter(bvhA, a);
    refitter.refit(q0);

    // Both traversal operations find the minimum directional distance, which is
	// how far along v the features can move before potentially colliding.

	// As we only are only interested in the contact patches for the closest
    // contact(s), we can update the traversal termination condition as we
    // find closer and closer ones.

    auto v = q1 - q0;
	auto tauMin = 1.0;

    contacts.clear();

    bvh::traverse(bvhA, bvhB,
        [&](const bvh::Bvh<double>::Node& a, const bvh::Bvh<double>::Node& b) {
            auto tau = geometry::timeOfContact(
                repoBounds(a),
                repoBounds(b),
                v
			);
            return tau <= tauMin;
        },
        [&](size_t a, size_t b) {
            auto tau = geometry::timeOfContact(
                this->a[a] + q0,
                this->b[b],
                v
			);
            if(tau <= tauMin) {
				tauMin = tau;
                auto p = q0 + v * tauMin;
                addContact(this->b[b].normal(), p, tau);
                addContact(this->a[a].normal(), p, tau);
            }
        }
    );

	filterContacts(tauMin + contactTimeEpsilon); // Keep contacts that are very close to the minimum time as well

	return q0 + v * tauMin;
}

void RepoPolyDepth::addContact(
    const repo::lib::RepoVector3D64& normal,
    const repo::lib::RepoVector3D64& point,
    double tau
)
{
    auto c = normal.dotProduct(point);
    auto n = normal;

    // Describe the plane such that point (which will be qi) always lies on the
    // positive side of it.

    if (c < 0) {
        n = -n;
        c = -c;
    }

	Contact contact = { n, c, tau };

    for(size_t i = 0; i < contacts.size(); i++) {
        auto& existing = contacts[i];

        if (tau < (existing.tau - contactTimeEpsilon)) {  // If this is a closer contact, it can be trivially replaced
            contacts[i] = contact;
            return;
        }
        else if (contact.normal.dotProduct(existing.normal) > coplanarEpsilon &&  // Duplicate contact, ignore
            (contact.constant - existing.constant) < FLT_EPSILON) {
            return;
        }
	}

    contacts.push_back(contact);
}

void RepoPolyDepth::filterContacts(double tau)
{
    contacts.erase(
        std::remove_if(
            contacts.begin(),
            contacts.end(),
            [tau](const Contact& c) {
                return c.tau > tau;
            }
        ),
        contacts.end()
	);
}
