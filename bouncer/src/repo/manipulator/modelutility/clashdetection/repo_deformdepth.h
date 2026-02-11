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

#pragma once

#include <vector>
#include "geometry_utils.h"
#include "repo/lib/datastructure/repo_triangle.h"
#include "repo/lib/datastructure/repo_bounds.h"
#include "geometry_tests_closed.h"
#include "repo/manipulator/modeloptimizer/bvh/bvh.hpp"

namespace geometry {
	
	/*
	* RepoDeformDepth is the successor to RepoPolyDepth that estimates the
	* penetration depth between polygon soups under a specific tolerance.
	* 
	* This algorithm uses local deformations, up to a limit, to attempt to find a
	* configuration where the meshes are not intersecting. If the algorithm is
	* unable to find such a configuration, the clash is considered irreconcilable.
	* 
	* Configurations are only valid where the point-wise distance between the start
	* and ending configurations is below the tolerance. The algorithm only attempts
	* to deform mesh (a), which should be the smaller of the two.
	* 
	* This approach is designed to handle cases were meshes have many contact
	* patches, but are not actually overlapping in a meaningful way, and to be
	* robust to the case where a junction is coplanar with a collider.
	*/
	struct RepoDeformDepth {
		/*
		* Optional structure to represent a RepoIndexedMesh with additional metadata.
		* RepoDeformDepth uses these internally, but they can be cached and re-used
		* to improve performance if multiple calls to RepoDeformDepth are expected for
		* the same mesh.
		*/
		struct Mesh : public geometry::RepoIndexedMesh {

			/*
			* Creates an empty RepoDeformDepthMesh. This can be populated by functions
			* that operate on RepoIndexedMeshes, but in this case initialise() must be
			* called explicitly.
			* It is suggested to use the copy-constructor where possible, which can take
			* the resources of a RepoIndexedMesh with no overhead.
			*/
			Mesh() = default;

			void initialise();

			/*
			* Converts a RepoIndexedMesh into a deformable mesh structure. Builds the Bvh
			* and other lookups automatically.
			*/
			Mesh(geometry::RepoIndexedMesh&& mesh);

			/*
			* Defines a subset of faces of the mesh with its own BVH for use with methods
			* that take a MeshView.
			*/
			struct Faces : public geometry::MeshView {
				size_t start;
				size_t length;
				bvh::Bvh<double> bvh;
				const geometry::RepoIndexedMesh& mesh;
				bool closed;
				std::vector<size_t> orderedIndices;

				Faces(const geometry::RepoIndexedMesh& mesh, 
					size_t start, 
					size_t length);

				const bvh::Bvh<double>& getBvh() const override {
					return bvh;
				}

				repo::lib::RepoTriangle getTriangle(size_t primitive) const override {
					return mesh.getTriangle(start + primitive);
				}

				std::vector<size_t> getIndices() const;

				repo::lib::RepoBounds getBounds() const;
			};

			/*
			* All the groups of faces in the mesh. As vertices are updated, all the face
			* groups update together. The mesh must have at least one group. Typically,
			* the last group will encompass the whole mesh (this is one that the getBvh()
			* method returns).
			*/
			std::vector<Faces> faceGroups;

			void addFaceRange(size_t start, size_t end);

			const bvh::Bvh<double>& getBvh() const;

			/*
			* The current configuration of the deformable mesh.
			*/
			std::vector<repo::lib::RepoVector3D64> _vertices;

			/*
			* Normals per vertex of the deformable mesh that are the best estimate of
			* its outer surface.
			*/
			std::vector<repo::lib::RepoVector3D64> pseudoNormals;

			/*
			* The current bounds of the vertices in their deformed configuration.
			*/
			repo::lib::RepoBounds bounds() const;

			/*
			* Moves the vertices back to their starting positions and refits the BVH.
			*/
			void resetConfiguration();

			/*
			* Reduces mesh A along its outer surface by the amounts specified in the per-
			* vertex displacements.
			*/
			void deflate(double amount);

			/*
			* Gets the distance of the current configuration from the starting
			* configuration. If this is greater than the tolerance, the algorithm should
			* terminate.
			*/
			double getConfigurationDistance() const;

			repo::lib::RepoTriangle getTriangle(size_t index) const override;

		protected:
			void buildBvh();
			void computePseudoNormals();

			bool deformed = false;
		};

		RepoDeformDepth(
			RepoDeformDepth::Mesh& a,
			const RepoDeformDepth::Mesh& b,
			double tolerance = 0.0);

		void iterate(int maxIterations = -1);

		double getPenetrationDepth() const;

		/*
		* Returns points that characterise the clash, if the meshes are still clashing
		* when RepoDeformDepth terminates. These are an implementation detail and are
		* not strictly given any definition.
		*/
		std::vector<repo::lib::RepoVector3D64> getContactManifold() const;

	protected:
		double tolerance;

		/*
		* An upper bound on the penetration depth found so far. If this drops below
		* tolerance, we can terminate.
		*/
		double distance;

		RepoDeformDepth::Mesh& a;
		const RepoDeformDepth::Mesh& b;

		/*
		* Intersects (a) with (b) in the current configuration. Returns true if the
		* meshes are touching (in-contact or in-collision), otherwise false.
		*/
		bool intersect(const repo::lib::RepoVector3D64& m);
		bool intersect();

		/*
		* Returns true if under the current configuration, mesh (a) is entirely
		* contained by mesh (b), or mesh (b) is entirely contained by mesh (a).
		* The configuration encompasses both the deformation of (a) and (b) and
		* an optional transform (m) applied to (a).
		*/
		bool contained(const repo::lib::RepoVector3D64& m);

		/*
		* How many steps to take along each axis when performing the local search.
		* The total number of steps completed will be 6 * numLocalSearchSteps, so
		* be careful not to make this too large.
		*/
		size_t numLocalSearchSteps = 5;

		/*
		* How much to deflate the mesh in each search step, as a fraction of the
		* tolerance.
		*/
		double deflateStepSize = 0.05;
	};
}