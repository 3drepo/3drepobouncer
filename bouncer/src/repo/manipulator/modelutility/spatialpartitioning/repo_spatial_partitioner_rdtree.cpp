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

#include "repo_spatial_partitioner_rdtree.h"

using namespace repo::manipulator::modelutility;

RDTreeSpatialPartitioner::RDTreeSpatialPartitioner(
	const repo::core::model::RepoScene *scene,
	const uint32_t                      &depth)
	: AbstractSpatialPartitioner(scene, depth)
{
}


RDTreeSpatialPartitioner::~RDTreeSpatialPartitioner()
{
}

std::vector<MeshEntry> RDTreeSpatialPartitioner::createMeshEntries()
{
	std::vector<MeshEntry> entries;

	/*
		We only cater for scene graph with pretransformed vertices so 
		non multiparted graph will bound to fail (unless it has pretransformed 
		vertices, which is not expected)
	*/
	assert(gType == repo::core::model::RepoScene::GraphType::OPTIMIZED);

	auto meshes = scene->getAllMeshes(gType);
	repoTrace << "meshes.size() : " << meshes.size();
	for (const auto &node : meshes)
	{
		const auto mesh = dynamic_cast<repo::core::model::MeshNode*>(node);
		if (mesh)
		{
			auto meshMaps = mesh->getMeshMapping();
			repoTrace << "meshMaps.size() : " << meshMaps.size();
			if (meshMaps.size())
			{
				for (const auto map : meshMaps)
				{
					entries.resize(entries.size() + 1);
					entries.back().id = map.mesh_id;
					entries.back().min[0] = map.min.x;
					entries.back().min[1] = map.min.y;
					entries.back().min[2] = map.min.z;
					entries.back().max[0] = map.max.x;
					entries.back().max[1] = map.max.y;
					entries.back().max[2] = map.max.z;
				}
				
			}
			else
			{
				//non multipart mesh, take the whole mesh as entry

				entries.resize(entries.size() + 1);
				entries.back().id = mesh->getUniqueID();
				auto bbox = mesh->getBoundingBox();
				if (bbox.size() == 2)
				{
					entries.back().min[0] = bbox[0].x;
					entries.back().min[1] = bbox[0].y;
					entries.back().min[2] = bbox[0].z;
					entries.back().max[0] = bbox[1].x;
					entries.back().max[1] = bbox[1].y;
					entries.back().max[2] = bbox[1].z;
				}
				else
				{
					repoWarning << "Could not find bounding box for " << UUIDtoString(mesh->getUniqueID()) << ". Skipping...";
					entries.pop_back();
				}
				
				
			}
		}
		else
		{
			repoWarning << "Failed to dynamically cast a mesh node, scene partitioning may be incomplete!";
		}
	}
	repoTrace << "mesh entries : " << entries.size();
	return entries;
}

std::shared_ptr<PartitioningTree> RDTreeSpatialPartitioner::createPartition(
	const std::vector<MeshEntry> &meshes,
	const PartitioningTreeType   &axis,
	const uint32_t               &depthCount)
{
	if (meshes.size() == 1 || (depthCount == maxDepth && maxDepth != 0))
	{
		/*
			If there is only one mesh left, or if we hit max depth count
			Create a leaf node
			*/
		std::vector<repoUUID> meshIDs;
		meshIDs.reserve(meshes.size());
		for (const auto mesh : meshes)
			meshIDs.push_back(mesh.id);

		return std::make_shared<PartitioningTree>(meshIDs);
	}

	//non leaf node, partition the meshes via the given axis and recurse
	float median;
	std::vector<MeshEntry> lMeshes, rMeshes;
	sortMeshes(meshes, axis, median, lMeshes, rMeshes);

	PartitioningTreeType nextAxis;
	//Enum classes are not guaranteed to be contiguous
	switch (axis)
	{
	case PartitioningTreeType::PARTITION_X:
		nextAxis = PartitioningTreeType::PARTITION_Y;
		break;
	case PartitioningTreeType::PARTITION_Y:
		nextAxis = PartitioningTreeType::PARTITION_Z;
		break;
	case PartitioningTreeType::PARTITION_Z:
		nextAxis = PartitioningTreeType::PARTITION_X;
		break;
	default:
		//Only thing left is LEAF, which should never be passed in in the first place
		repoError << "Unexpected Partition Tree type!";

	}
	
	return std::make_shared<PartitioningTree>(nextAxis, median,
		createPartition(lMeshes, nextAxis, depthCount + 1),
		createPartition(rMeshes, nextAxis, depthCount + 1)
		);
	

}

std::shared_ptr<PartitioningTree> RDTreeSpatialPartitioner::partitionScene()
{
	std::shared_ptr<PartitioningTree> pTree(nullptr);

	if (scene)
	{

		if (scene->hasRoot(gType))
		{
			//sort the meshes
			//FIXME: Using vector because i need different comparison functions for different axis. is this sane?
			std::vector<MeshEntry> meshEntries = createMeshEntries();
			//starts with a X partitioning
			pTree = createPartition(meshEntries, PartitioningTreeType::PARTITION_X, 0);
		}
		else
		{
			repoError << "Failed to perform spatial partitioning: optimised graph not found";
		}

	}
	else
	{
		repoError << "Failed to perform spatial partitioning: nullptr to scene.";
	}

	return pTree;
}

void RDTreeSpatialPartitioner::sortMeshes(
	const std::vector<MeshEntry> &meshes,
	const PartitioningTreeType   &axis,
	float                        &median,
	std::vector<MeshEntry>       &lMeshes,
	std::vector<MeshEntry>       &rMeshes
	) 
{
	std::vector<MeshEntry> sortedMeshes = meshes;
	uint32_t axisIdx;

	//Sort the meshes and determine axis index
	switch (axis){
	case PartitioningTreeType::PARTITION_X:
		axisIdx = 0;
		std::sort(sortedMeshes.begin(), sortedMeshes.end(),
			[](MeshEntry const& a, MeshEntry const& b) 
				{ return a.min[0] < b.min[0]; }
		);		
		break;
	case PartitioningTreeType::PARTITION_Y:
		axisIdx = 1;
		std::sort(sortedMeshes.begin(), sortedMeshes.end(),
			[](MeshEntry const& a, MeshEntry const& b)
		{ return a.min[1] < b.min[1]; }
		);
		break;
	case PartitioningTreeType::PARTITION_Z:
		axisIdx = 2;
		std::sort(sortedMeshes.begin(), sortedMeshes.end(),
			[](MeshEntry const& a, MeshEntry const& b)
		{ return a.min[2] < b.min[2]; }
		);
		break;
	default:
		//only thing left is leaf node, which we do not expect it to ever come in here
		repoError << "Unexpected error: sorting Meshes for partitioning with an undefined axis.";
	}
	std::string axisStr[3] = {"X", "Y", "Z"};
	repoTrace << "Splitting @ " << axisStr[axisIdx];
	repoTrace << "Sorted meshes ("<< sortedMeshes.size() << "):";

	for (const auto mesh : sortedMeshes)
		repoTrace << "\t" << UUIDtoString(mesh.id) << " [" << mesh.min[0] << "," << mesh.min[1] << "," << mesh.min[2] << "] "
		<< " [" << mesh.max[0] << "," << mesh.max[1] << "," << mesh.max[2] << "] ";

	//figure out the median value
	if (sortedMeshes.size() % 2)
	{
		//odd number
		median = sortedMeshes[sortedMeshes.size() / 2].min[axisIdx];
	}
	else
	{
		//even - take the mean of the 2 middle number
		median = (sortedMeshes[sortedMeshes.size() / 2 - 1].min[axisIdx] + sortedMeshes[sortedMeshes.size() / 2 ].min[axisIdx]) / 2.;
	}


	//put the meshes into left and right mesh vectors
	for (const auto &entry : sortedMeshes)
	{
		if (entry.min[axisIdx] <= median)
			lMeshes.push_back(entry);
		if (entry.max[axisIdx] > median)
			rMeshes.push_back(entry);
	}

	repoTrace << "Median is: " << median << " left entry size: " << lMeshes.size() << " right entry size: "<<  rMeshes.size();
}
