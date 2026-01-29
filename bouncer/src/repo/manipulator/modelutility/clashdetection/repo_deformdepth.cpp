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

#include <set>

using namespace geometry;
using namespace repo::lib;

using Bvh = bvh::Bvh<double>;

namespace {
	repo::lib::RepoBounds repoBounds(const bvh::Bvh<double>::Node& a) {
		return repo::lib::RepoBounds(
			repo::lib::RepoVector3D64(a.bounds[0], a.bounds[2], a.bounds[4]),
			repo::lib::RepoVector3D64(a.bounds[1], a.bounds[3], a.bounds[5])
		);
	}

	// Updates the Bvh bottom-up, refitting the bounds to the triangles under a given
	// transformation. The Bvh is updated in-place, but the original triangles are
    // not modified.

	struct BvhRefitter
	{
		BvhRefitter(Bvh& bvh,
			const std::vector<repo::lib::RepoVector3D64>& vertices,
			const std::vector<repo::lib::repo_face_t>& faces)
			: bvh(bvh), vertices(vertices), faces(faces)
		{
		}

		Bvh& bvh;
		const std::vector<repo::lib::RepoVector3D64>& vertices;
		const std::vector<repo::lib::repo_face_t>& faces;

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
					b.extend(reinterpret_cast< const bvh::Vector3<double>&>(vertices[index]));
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
	RepoDeformDepth::Mesh& a,
	const RepoDeformDepth::Mesh& b,
	ContainsFunctor* containsFunctor,
	double tolerance) :
	a(a),
	b(b),
	tolerance(tolerance),
	distance(FLT_MAX),
	contains(containsFunctor)
{
	a.resetConfiguration();

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
		a.bounds(),
		b.bounds()
	);
	distance = std::min(distance, q.norm());

	if (distance < tolerance) {
		return;
	}

	// Perform a local search around the origin of (a). This is primarily to resolve
	// collisions where (a) is a small object in a concavity, but it can also help
	// in situations with very loose bounds.

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

	q = performLocalSearch();
	distance = std::min(distance, q.norm());

	if (distance < tolerance) {
		return;
	}

	// Do early rejection tests: 
	//  * Or, a separating hyperplane (up to the tolerance) exists between the two vertex sets
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
	points.push_back(a.bounds().center());
	points.push_back(b.bounds().center());
	return points;
}

bool RepoDeformDepth::intersect(const repo::lib::RepoVector3D64& m)
{
	resetDisplacements();

	bool intersecting = false;

	bvh::traverse(a.bvh, b.bvh,
		[&](const Bvh::Node& a, const Bvh::Node& b)
		{
			return closestPoints(repoBounds(a) + m, repoBounds(b))
				.magnitude() < geometry::contactThreshold(repoBounds(a) + m, repoBounds(b));
		},
		[&](size_t _a, size_t _b)
		{
			auto triA = a.getTriangle(_a) + m;
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
		intersecting = (*contains)(a._vertices, a.bounds(), m);
	}

	return intersecting;
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
			a.deflate(distances);

			// If we've had to deform the mesh beyond the tolerance, there is no
			// point in continuing further.
			auto configDistance = a.getConfigurationDistance();
			if(configDistance > tolerance) {
				break;
			}
		}
		else {
			distance = a.getConfigurationDistance();
			break;
		}
	}
}

RepoDeformDepth::Mesh::Mesh(geometry::RepoIndexedMesh&& mesh)
{
	vertices = std::move(mesh.vertices);
	faces = std::move(mesh.faces);
	_vertices = vertices;
	buildBvh();
	computePseudoNormals();
}

void RepoDeformDepth::Mesh::deflate(const std::vector<double>& displacements)
{
	for (auto vi = 0; vi < _vertices.size(); vi++) {
		auto& v = _vertices[vi];
		auto& n = pseudoNormals[vi];
		double amount = displacements[vi];
		if (amount > 0) {
			v = v - n * amount;
		}
	}
	BvhRefitter refitter(this->bvh, _vertices, faces);
	refitter.refit();
	deformed = true;
}

void RepoDeformDepth::Mesh::resetConfiguration()
{
	if(!deformed) {
		return;
	}
	_vertices = vertices;
	BvhRefitter refitter(this->bvh, _vertices, faces);
	refitter.refit();
	deformed = false;
}

repo::lib::RepoTriangle RepoDeformDepth::Mesh::getTriangle(size_t index) const
{
	auto& face = faces[index];
	return repo::lib::RepoTriangle(
		_vertices[face[0]],
		_vertices[face[1]],
		_vertices[face[2]]
	);
}

repo::lib::RepoBounds RepoDeformDepth::Mesh::bounds() const
{
	return repoBounds(bvh.nodes[0]);
}

double RepoDeformDepth::Mesh::getConfigurationDistance() const
{
	double maxDistance = 0.0;
	for (size_t vi = 0; vi < vertices.size(); vi++) {
		maxDistance = std::max((vertices[vi] - _vertices[vi]).norm(), maxDistance);
	}
	return maxDistance;
}

void RepoDeformDepth::Mesh::initialise()
{
	_vertices = vertices;
	buildBvh();
	computePseudoNormals();
	deformed = false;
}

void RepoDeformDepth::Mesh::buildBvh()
{
	bvh::builders::build(bvh, vertices, faces);
}

void RepoDeformDepth::Mesh::computePseudoNormals() {
	pseudoNormals.resize(vertices.size());

	// The following algorithm computes boundary-adapted pseudo-normals for each
	// vertex. These are angle-weighted normals from the faces, averaged with
	// the 3d normals any boundary edges. 3d normals are orthogonal to both
	// the face and the edge, that is, pointing 'outwards' from the boundary
	// approximating the surface that might close the opening.

	std::vector<repo::lib::RepoVector3D64> edgeNormals;
	edgeNormals.resize(vertices.size());

	std::set<std::pair<size_t, size_t>> edges;

	for (const auto& f : faces) {
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

		edges.insert({f[0], f[1]});
		edges.insert({f[1], f[2]});
		edges.insert({f[2], f[0]});
	}

	for (auto& n : pseudoNormals) {
		n = n.normalized();
	}

	for (const auto& f : faces) {

		auto v01 = vertices[f[1]] - vertices[f[0]];
		auto v12 = vertices[f[2]] - vertices[f[1]];
		auto v20 = vertices[f[0]] - vertices[f[2]];
		auto v02 = vertices[f[2]] - vertices[f[0]];

		auto n = v01.crossProduct(v02).normalized();

		auto n01 = v01.crossProduct(n).normalized();
		auto n12 = v12.crossProduct(n).normalized();
		auto n20 = v20.crossProduct(n).normalized();

		// If edges are shared/non-boundary, then they don't have normals of their own

		if (!edges.contains({f[1], f[0]})) {
			edgeNormals[f[0]] += n01;
			edgeNormals[f[1]] += n01;
		}

		if (!edges.contains({f[2], f[1]})) {
			edgeNormals[f[1]] += n12;
			edgeNormals[f[2]] += n12;
		}

		if (!edges.contains({f[0], f[2]})) {
			edgeNormals[f[2]] += n20;
			edgeNormals[f[0]] += n20;
		}
	}

	for (size_t vi = 0; vi < pseudoNormals.size(); vi++) {
		pseudoNormals[vi] += edgeNormals[vi];
		pseudoNormals[vi].normalize();
	}
}