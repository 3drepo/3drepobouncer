/**
*  Copyright (C) 2015 3D Repo Ltd
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
#include "repo_spatial_partitioner_abstract.h"

namespace repo{
	namespace manipulator{
		namespace modelutility{
			class RDTreeSpatialPartitioner : public AbstractSpatialPartitioner
			{
			public:
				/**
				* RD Tree Spatial Partitioning utility class
				* to spatially divide a scene graph base on its meshes
				* This algorithm will produce an RD Tree with it
				* it's partitioning value determined by the median value of min
				* bounding boxes.
				* Note: currently only support scene with optimised graph!
				* @param scene scene to divide
				* @param depth limiting depth of the tree (0 to disable limitation)
				*/
				RDTreeSpatialPartitioner(
					const repo::core::model::RepoScene *scene,
					const uint32_t                      &maxDepth = 12)
					;

				virtual ~RDTreeSpatialPartitioner();

				virtual std::shared_ptr<repo::lib::repo_partitioning_tree_t> partitionScene();

			protected:

				/**
				* Generate a vector of mesh entries base on
				* the current scene.
				*/
				std::vector<repo::lib::repo_mesh_entry_t> createMeshEntries();

				/**
				* Create a partitioning with the given meshes
				* @param meshes meshes to divide
				* @param axis which axis to divide
				* @param depthCount current depth
				*/
				std::shared_ptr<repo::lib::repo_partitioning_tree_t> createPartition(
					const std::vector<repo::lib::repo_mesh_entry_t> &meshes,
					const repo::lib::PartitioningTreeType   &axis,
					const uint32_t               &depthCount,
					const uint32_t               &failcount,
					const std::vector<std::vector<float>>     &currentSection);

				/**
				* Sort the meshes on the given axis
				* @param meshes to sort
				* @param axis to sort on
				* @param median value to split on
				* @param lMeshes meshes belonging to the left child
				* @param rMeshes meshes belonging to the right child
				*/
				void sortMeshes(
					const std::vector<repo::lib::repo_mesh_entry_t> &meshes,
					const repo::lib::PartitioningTreeType   &axis,
					const std::vector<std::vector<float>>  &currentSection,
					float                        &median,
					std::vector<repo::lib::repo_mesh_entry_t>       &lMeshes,
					std::vector<repo::lib::repo_mesh_entry_t>       &rMeshes
					);
			};
		}
	}
}
