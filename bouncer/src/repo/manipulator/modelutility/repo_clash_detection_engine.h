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

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <repo/repo_bouncer_global.h>
#include <repo/lib/datastructure/repo_container.h>
#include <repo/lib/datastructure/repo_uuid.h>
#include <repo/lib/datastructure/repo_vector.h>
#include <repo/core/handler/repo_database_handler_abstract.h>

namespace repo {
	namespace manipulator {
		namespace modelutility {

			enum class ClashDetectionType
			{
				/*
				* Finds the minimum distance between any two primitives of the composite
				* objects in the sets. A clash is reported if the distance is less than
				* the tolerance.
				*/
				Clearance = 1,

				/*
				* Finds an intersection between any two primitives of the composite objects
				* objects in the sets. A clash is reported if the penetration depth is
				* greater than the tolerance.
				*/
				Hard = 2,
			};

			/*
			* A selection of MeshNodes that when transformed into project space are
			* considered as a single object for the purposes of clash detection.
			* 
			* How MeshNodes are arranged into Composites will affect the estimated
			* penetration depth.
			* 
			* In most cases, a CompositeObject will be the TransformationNode above
			* one or more MeshNodes, but in theory it could be any set of MeshNodes.
			*/

			struct MeshReference
			{
				repo::lib::Container* container;
				repo::lib::RepoUUID uniqueId;

				MeshReference(repo::lib::Container* container, const repo::lib::RepoUUID& uniqueId)
					:container(container),
					uniqueId(uniqueId)
				{
				}
			};

			struct CompositeObject
			{
				repo::lib::RepoUUID id;

				/*
				* Composite objects are made up of MeshNode instances. Each MeshNode
				* is transformed into project space before clash detection is performed.
				* MeshNodes must come from the same teamspace, but can be from different
				* Containers.
				*/
				std::vector<MeshReference> meshes;
			};

			struct ClashDetectionConfig
			{
				ClashDetectionType type = ClashDetectionType::Clearance;

				double tolerance = 0.0;

				/*
				* Each clash test will compare all objects in set A against all objects in
				* set B. Objects in set A will not be compared against each other. Objects
				* are compared in Project Space.
				*/
				std::vector<CompositeObject> setA;
				std::vector<CompositeObject> setB;

				/* 
				* Containers which hold the meshes referenced by the composite objects. This
				* vector exists to store the container info which is referenced by the meshes.
				*/
				std::vector<std::unique_ptr<repo::lib::Container>> containers;
			};

			struct ClashDetectionResult
			{
				// The IDs of the two Composite Objects involved in the clash.
				// Composite Objects are the lowest level of granularity reported - if
				// it is desirable to know the details of the clashes between two
				// MeshNodes, they should be in separate Composite Objects.

				repo::lib::RepoUUID idA;
				repo::lib::RepoUUID idB;

				// The positions in project coordinates that represent the clash in a
				// high level way. The exact meaning of these depends on the clash test.
				// For example, in a clearance clash, these will be the single two
				// closest points.

				std::vector<repo::lib::RepoVector3D64> positions;

				// Unique identifier of this clash. If the same objects clash in the
				// exact same configuration, the fingerprint will be the same.
				// The fingerprint does not include the composite ids, only the geometry.
				// In this way the Id pair & fingerprint can be used to determine if a
				// clash has changed since the last run.

				size_t fingerprint;
			};

			struct ClashDetectionReport
			{
				std::vector<ClashDetectionResult> clashes;
			};


			REPO_API_EXPORT class ClashDetectionEngine
			{
			public:
				ClashDetectionEngine(std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler);
				~ClashDetectionEngine() = default;

				ClashDetectionReport runClashDetection(const ClashDetectionConfig& config);

			protected:
				std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler;



			};
		} // namespace modelutility
	} // namespace manipulator
}