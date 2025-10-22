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

#include <repo/repo_bouncer_global.h>
#include <repo/lib/datastructure/repo_uuid.h>
#include <repo/lib/datastructure/repo_container.h>
#include <vector>
#include <memory>

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
				* objects in the sets. A clash is reported if the penetration depth of the
				* whole objects is greater than the tolerance.
				*/
				Hard = 2,
			};

			/*
			* A selection of MeshNodes that when transformed into project space are
			* considered as a single object for the purposes of clash detection.
			*
			* How MeshNodes are arranged into Composites will affect the estimated
			* penetration depth, if any.
			*
			* In most cases, a CompositeObject will be the TransformationNode above
			* one or more MeshNodes, but it can be far more complex than that.
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
				/*
				* Unique identifier of this Composite Object for correlation purposes.
				*/
				repo::lib::RepoUUID id;

				/*
				* Composite objects are made up of MeshNode instances. Each MeshNode
				* is transformed into project space before clash detection is performed.
				* MeshNodes can be from different Containers.
				*/
				std::vector<MeshReference> meshes;
			};

			REPO_API_EXPORT struct ClashDetectionConfig
			{
				ClashDetectionType type = ClashDetectionType::Clearance;

				double tolerance = 0.0;

				/*
				* Each clash test will compare all objects in set A against all objects in
				* set B. All Objects will be compared in Project Coordinates. It is
				* allowed for Set A and Set B to be identical - this can be used to check
				* that all objects in a single set have a given clearance to eachother. For
				* this case the Composite Ids must be the same - the engine will ignore pairs
				* with the same Composite Id.
				*/
				std::vector<CompositeObject> setA;
				std::vector<CompositeObject> setB;

				/*
				* Where to write the results of clash detection. This should be a fully
				* qualified (.json) file name.
				*/
				std::string resultsFile;

				REPO_API_EXPORT static void ParseJsonFile(const std::string& jsonFilePath, ClashDetectionConfig& config);

				/*
				* Containers which hold the meshes referenced by the composite objects. This
				* vector exists to store the container info which is referenced by the meshes.
				* This is a convenience, and implementation detail only: references may point
				* to containers outside of this. There may also be multiple entries to
				* semantically identical containers.
				*/
				std::vector<std::unique_ptr<repo::lib::Container>> containers;
			};
		}
	}
}