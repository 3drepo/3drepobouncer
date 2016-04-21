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

#include "../../../lib/repo_property_tree.h"
#include "../../../lib/datastructure/repo_structs.h"
#include "../../../core/model/collection/repo_scene.h"

namespace repo{
	namespace manipulator{
		namespace modelutility{

			class AbstractSpatialPartitioner
			{
			public:
				/**
				* Abstract Spatial Partitioning utility class
				* to spatially divide a scene graph base on its meshes
				* Note: currently only support scene with optimised graph!
				* @param scene scene to divide
				* @param depth limiting depth of the tree
				*/
				AbstractSpatialPartitioner(
					const repo::core::model::RepoScene *scene,
					const uint32_t                      &maxDepth);
				virtual ~AbstractSpatialPartitioner();

				/**
				* Partition the scene
				* @return returns the spatial partitioning information as a tree
				*/
				virtual std::shared_ptr<PartitioningTree>
										partitionScene() = 0;

				/**
				* Generate a property tree representing the PartitioningTree from partitionScene()
				* @return returns the spatial partitioning information as a property tree
				*/
				virtual repo::lib::PropertyTree 
							generatePropertyTreeForPartitioning();

			protected:
				const repo::core::model::RepoScene            *scene;
				const uint32_t                                maxDepth;
				const repo::core::model::RepoScene::GraphType gType;

				repo::lib::PropertyTree generatePropertyTreeForPartitioningInternal(
					const std::shared_ptr<PartitioningTree> &spTree) const;
			};

		}
	}
}

