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

#include "repo_spatial_partitioner_abstract.h"

using namespace repo::manipulator::modelutility;

AbstractSpatialPartitioner::AbstractSpatialPartitioner(
	const repo::core::model::RepoScene *scene,
	const uint32_t                      &maxDepth)
	: scene(scene),
	maxDepth(maxDepth),
	gType(repo::core::model::RepoScene::GraphType::OPTIMIZED)

{
}

AbstractSpatialPartitioner::~AbstractSpatialPartitioner()
{
}

repo::lib::PropertyTree AbstractSpatialPartitioner::generatePropertyTreeForPartitioning()
{	
	return generatePropertyTreeForPartitioningInternal(partitionScene());	
}

repo::lib::PropertyTree AbstractSpatialPartitioner::generatePropertyTreeForPartitioningInternal(
	const std::shared_ptr<PartitioningTree> &spTree) const
{
	repo::lib::PropertyTree tree;

	if (spTree)
	{
		if (PartitioningTreeType::LEAF_NODE == spTree->type)
		{
			repo::lib::PropertyTree meshesTree;
			
			for(const auto &mesh: spTree->meshes)
			{
				std::string idString = UUIDtoString(mesh.id);
				repo::lib::PropertyTree childTree;
				childTree.addToTree("min", mesh.min);
				childTree.addToTree("max", mesh.max);
				meshesTree.mergeSubTree(idString, childTree);
			}
			
			tree.mergeSubTree("meshes", meshesTree);
		}
		else
		{
			std::string axisStr = PartitioningTreeType::PARTITION_X == spTree->type ? "X" 
					: (PartitioningTreeType::PARTITION_Y == spTree->type ? "Y" : "Z");
			tree.addToTree("axis", axisStr);
			tree.addToTree("value", spTree->pValue);
			tree.mergeSubTree("left", generatePropertyTreeForPartitioningInternal(spTree->left));
			tree.mergeSubTree("right", generatePropertyTreeForPartitioningInternal(spTree->right));
		}
			
	}
	return tree;
	
}