/**
*  Copyright (C) 2026 3D Repo Ltd
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

#include "repo_deformdepth.h"
#include "repo/lib/datastructure/repo_structs.h"
#include "geometry_tests.h"
#include "repo_linear_algebra.h"
#include "repo/manipulator/modeloptimizer/bvh/sweep_sah_builder.hpp"
#include "repo/manipulator/modeloptimizer/bvh/hierarchy_refitter.hpp"
#include "bvh_operators.h"

using namespace geometry;
using namespace repo::lib;

using Bvh = bvh::Bvh<double>;

namespace {
	repo::lib::RepoBounds repoBounds(const bvh::Bvh<double>::Node& a) {
		return repo::lib::RepoBounds(
			repo::lib::RepoVector3D64(a.bounds[ 0 ], a.bounds[ 2 ], a.bounds[ 4 ]),
			repo::lib::RepoVector3D64(a.bounds[ 1 ], a.bounds[ 3 ], a.bounds[ 5 ])
		);
	}

	Bvh buildBvh(const RepoIndexedMesh& mesh) {
		Bvh bvh;
		bvh::builders::build(bvh, mesh.vertices, mesh.faces);
		return bvh;
	}

	// Updates the Bvh bottom-up, refitting the bounds to the triangles under a given
	// transformation. The Bvh is updated in-place, but the original triangles are
    // not modified.

	struct BvhRefitter
	{
		BvhRefitter(Bvh& bvh,
			const std::vector<repo::lib::RepoVector3D64>& vertices,
			const std::vector<repo::lib::repo_face_t>& faces,
			const repo::lib::RepoVector3D64& m)
			: bvh(bvh), vertices(vertices), faces(faces), m(m)
		{
		}

		Bvh& bvh;
		const std::vector<repo::lib::RepoVector3D64>& vertices;
		const std::vector<repo::lib::repo_face_t>& faces;
		const repo::lib::RepoVector3D64& m;

		void refit()
		{
			if ( bvh.node_count <= 1 ) { // Special case if the BVH is just a leaf (not currently handled by BottomUpAlgorithm)
				updateLeaf(bvh.nodes[0]);
				return;
			}
			bvh::HierarchyRefitter<bvh::Bvh<double>> refitter(bvh);
			refitter.refit(*this);
		}

		void updateLeaf(Bvh::Node& leaf) const
		{
			auto b = bvh::BoundingBox<double>::empty();
			for ( size_t i = 0; i < leaf.primitive_count; i++ ) {
				auto face = faces[bvh.primitive_indices[leaf.first_child_or_primitive + i]];
				for (const auto& index : face) {
					auto v = vertices[index] + m;
					b.extend(reinterpret_cast< const bvh::Vector3<double>& >(v));
				}
			}
			leaf.bounding_box_proxy() = b;
		}

		void operator()(const Bvh::Node& node) const
		{
			updateLeaf(const_cast<Bvh::Node&>(node));
		}
	};
}

RepoDeformDepth::RepoDeformDepth(
	const RepoIndexedMesh& a,
	const RepoIndexedMesh& b,
	ContainsFunctor* containsFunctor,
	double tolerance) :
	a(a),
	b(b),
	tolerance(tolerance),
	distance(FLT_MAX),
	contains(containsFunctor),
	vertices(a.vertices),
	bvhA(buildBvh(a)),
	bvhB(buildBvh(b))
{
	distances.resize(a.vertices.size());

	// If there is no intersection, there is nothing to do, which we signal by
	// already setting the configuration distance to zero.

	if (!intersect()) {
		distance = 0.0;
		return;
	}

	// Minimum separating axis is guaranteed to provide a separating vector, so
	// provides a starting upper bounds on the penetration depth. Of course if
	// this is smaller than the tolerance, we can end immediately too.

	auto q = geometry::minimumSeparatingAxis(
		repoBounds(bvhA.nodes[0]),
		repoBounds(bvhB.nodes[0])
	);
	distance = std::min(distance, q.norm());

	if (distance < tolerance) {
		return;
	}

	// For cases where a small mesh is contained in a concavity, a local search
	// around the area can reveal a collision-free configuration.

	auto volA = bvhA.nodes[0].bounding_box_proxy().to_bounding_box().half_area();
	auto volB = bvhB.nodes[0].bounding_box_proxy().to_bounding_box().half_area();
	if (volA <= volB * localSearchRatioThreshold) {

		// This loop searches along the primary axes, not combinations thereof. This
		// is because those combinations can become very numerous, very quickly,
		// especially with multiple steps.

		auto axes = std::vector<repo::lib::RepoVector3D64>{
			repo::lib::RepoVector3D64(1,0,0),
			repo::lib::RepoVector3D64(0,1,0),
			repo::lib::RepoVector3D64(0,0,1)
		};

		auto localSearchStepSize = tolerance / static_cast<double>(numLocalSearchSteps);

		auto performLocalSearch = [&]() {
			for (double istep = 1; istep < numLocalSearchSteps; istep++) {
				for (auto& axis : axes) {
					for (int dir = -1; dir <= 1; dir += 2) {
						auto pqs = axis * dir * (istep * localSearchStepSize);
						if (!intersect(pqs)) { // Local search has found an intersection free configuration
							return pqs;
						}
					}
				}
			}
			return repo::lib::RepoVector3D64(FLT_MAX, FLT_MAX, FLT_MAX);
		};

		auto q = performLocalSearch();
		distance = std::min(distance, q.norm());

		if (distance < tolerance) {
			return;
		}
	}

	// Do early rejection tests: 
	//  * Or, a separating hyperplane (up to the tolerance) exists between the two vertex sets

	// We only need these if we get to the deflate stage.
	computePseudoNormals(); 
}

double RepoDeformDepth::getPenetrationDepth() const
{
	return distance;
}

void RepoDeformDepth::resetDisplacements()
{
	memset(distances.data(), 0, distances.size() * sizeof(double));
}

bool RepoDeformDepth::intersect()
{
	return intersect(repo::lib::RepoVector3D64(0, 0, 0));
}

std::vector<repo::lib::RepoVector3D64> RepoDeformDepth::getContactManifold() const {
	std::vector<repo::lib::RepoVector3D64> points;
	points.push_back(repoBounds(bvhA.nodes[0]).center());
	points.push_back(repoBounds(bvhB.nodes[0]).center());
	return points;
}

bool RepoDeformDepth::intersect(const repo::lib::RepoVector3D64& m)
{
	refitBvh(m);
	resetDisplacements();

	bool intersecting = false;

	bvh::traverse(bvhA, bvhB,
		[&](const Bvh::Node& a, const Bvh::Node& b)
		{
			// Note we don't need to apply m to the bounds here because they've already been refitted above
			return closestPoints(repoBounds(a), repoBounds(b))
				.magnitude() < geometry::contactThreshold(repoBounds(a), repoBounds(b));
		},
		[&](size_t _a, size_t _b)
		{
			auto& face = a.faces[_a];
			repo::lib::RepoTriangle triA(
				vertices[face[0]] + m,
				vertices[face[1]] + m,
				vertices[face[2]] + m
			);

			auto triB = b.getTriangle(_b);

			auto d = geometry::closestPoints(triA, triB);
			auto ct = geometry::contactThreshold(triA, triB);
			if (d.intersects || d.magnitude() < ct) {

				// Note that true depth will always be less than the tolerance, because an
				// intersection distance greater than the tolerance is a termination
				// condition.

				const auto& face = a.faces[_a];
				for (const auto& index : face) {
					distances[index] = std::max(tolerance * 0.05, 1e-6);
				}

				intersecting = true;
			}
		}
	);

	if (!intersecting && contains) {
		intersecting = (*contains)(vertices, verticesBounds, m);
	}

	return intersecting;
}

void RepoDeformDepth::refitBvh(const repo::lib::RepoVector3D64& m)
{
	BvhRefitter refitter(bvhA, vertices, a.faces, m);
	refitter.refit();
}

double RepoDeformDepth::getConfigurationDistance()
{
	double maxDistance = 0.0;
	for (size_t vi = 0; vi < a.vertices.size(); vi++) {
		maxDistance = std::max((vertices[vi] - a.vertices[vi]).norm(), maxDistance);
	}
	return maxDistance;
}

bool RepoDeformDepth::deflateMesh()
{
	repo::lib::RepoBounds newBounds;
	for (auto vi = 0; vi < vertices.size(); vi++) {
		auto& v = vertices[vi];
		auto& n = pseudoNormals[vi];
		double amount = distances[vi];
		if (amount > 0) {
			v = v - n * amount;
		}
		newBounds.encapsulate(v);
	}

	// Check if the mesh has started, or began by, getting larger. If this is the
	// case, its likely the mesh has broken normals and there is not much more we
	// can do here.

	if (newBounds.size().norm() > verticesBounds.size().norm()) {
		return false;
	}

	verticesBounds = newBounds;

	return true;
}

void RepoDeformDepth::iterate(int maxIterations)
{
	if (distance < tolerance) {
		return;
	}

	if (maxIterations < 0) {
		maxIterations = (1.0 / deflateStepSize);
	}

	for(int i = 0; i < maxIterations; i++) {
		if (intersect()) {
			if (!deflateMesh()) {
				break; // Likely a local minima has been reached.
			}	

			// If we've had to deform the mesh beyond the tolerance, there is no
			// point in continuing further.
			auto configDistance = getConfigurationDistance();
			if(configDistance > tolerance) {
				break;
			}
		}
		else {
			distance = getConfigurationDistance();
			break;
		}
	}

	
	return;
}

void RepoDeformDepth::computePseudoNormals()
{
	pseudoNormals.resize(a.vertices.size());

	for (const auto& f : a.faces) {
		auto v0 = vertices[f[0]];
		auto v1 = vertices[f[1]];
		auto v2 = vertices[f[2]];

		auto a0 = std::acos(((v1 - v0).normalized()).dotProduct((v2 - v0).normalized()));
		auto a1 = std::acos(((v0 - v1).normalized()).dotProduct((v2 - v1).normalized()));
		auto a2 = std::acos(((v0 - v2).normalized()).dotProduct((v1 - v2).normalized()));

		auto normal = (v1 - v0).crossProduct(v2 - v0).normalized();

		pseudoNormals[f[0]] += normal * a0;
		pseudoNormals[f[1]] += normal * a1;
		pseudoNormals[f[2]] += normal * a2;
	}

	for (auto& n : pseudoNormals) {
		n = n.normalized();
	}
}