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

	for (const auto &node : meshes)
	{
		const auto mesh = dynamic_cast<repo::core::model::MeshNode*>(node);
		if (mesh)
		{
			auto meshMaps = mesh->getMeshMapping();
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

	return entries;
}

std::shared_ptr<PartitioningTree> RDTreeSpatialPartitioner::createPartition(
	const std::vector<MeshEntry>              &meshes,
	const PartitioningTreeType                &axis,
	const uint32_t                            &depthCount,
	const uint32_t                            &failCount,
	const std::vector<std::vector<float>>     &currentSection)
{
	if (meshes.size() <= 1 || (depthCount == maxDepth && maxDepth != 0) || failCount == 3)
	{
		/*
			Create a leaf node

			There are 3 possible scenario to hit this condition:
			1. If there is only one mesh left 
			2. If we hit max depth count
			3. If failCount is 3, meaning we tried all 3 axis and none of them manage to split the meshes up further (we have probably hit the overlapping regions)


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
	sortMeshes(meshes, axis, currentSection, median, lMeshes, rMeshes);


	PartitioningTreeType nextAxis;
	int32_t axisIdx;
	//Enum classes are not guaranteed to be contiguous
	switch (axis)
	{
	case PartitioningTreeType::PARTITION_X:
		axisIdx = 0;
		nextAxis = PartitioningTreeType::PARTITION_Y;
		break;
	case PartitioningTreeType::PARTITION_Y:
		axisIdx = 1;
		nextAxis = PartitioningTreeType::PARTITION_Z;
		break;
	case PartitioningTreeType::PARTITION_Z:
		axisIdx = 2;
		nextAxis = PartitioningTreeType::PARTITION_X;
		break;
	default:
		//Only thing left is LEAF, which should never be passed in in the first place
		repoError << "Unexpected Partition Tree type!";

	}
	

	if (lMeshes.size() == rMeshes.size() == meshes.size())
	{
		//If this partitioning did absolutely nothing, skip this node all together
		return createPartition(meshes, nextAxis, depthCount, failCount +1, currentSection);
	}
	else
	{
		auto newSectionRight = currentSection;
		auto newSectionLeft = currentSection;
		newSectionRight[0][axisIdx] = median;
		newSectionLeft[1][axisIdx] = median;

		//repoTrace << " right bounding box: [" << newSectionLeft[0][0] << "," << newSectionLeft[0][1] << "," << newSectionLeft[0][2] << "] "
		//	<< " [" << newSectionLeft[1][0] << "," << newSectionLeft[1][1] << "," << newSectionLeft[1][2] << "] ";

		//repoTrace << " left bounding box: [" << newSectionRight[0][0] << "," << newSectionRight[0][1] << "," << newSectionRight[0][2] << "] "
		//	<< " [" << newSectionRight[1][0] << "," << newSectionRight[1][1] << "," << newSectionRight[1][2] << "] ";

		return std::make_shared<PartitioningTree>(axis, median,
			createPartition(lMeshes, nextAxis, depthCount + 1, 0, newSectionLeft),
			createPartition(rMeshes, nextAxis, depthCount + 1, 0, newSectionRight)
			);

	}

	

}

std::shared_ptr<PartitioningTree> RDTreeSpatialPartitioner::partitionScene()
{
	std::shared_ptr<PartitioningTree> pTree(nullptr);
	repoInfo << "Generating spatial partitioning...";
	if (scene)
	{

		if (scene->hasRoot(gType))
		{
			//sort the meshes
			//FIXME: Using vector because i need different comparison functions for different axis. is this sane?
			std::vector<MeshEntry> meshEntries = createMeshEntries();
			//starts with a X partitioning
			
			auto bbox = scene->getSceneBoundingBox();

			std::vector<std::vector<float>> currentSection = {
																{bbox[0].x, bbox[0].y, bbox[0].z},
																{bbox[1].x, bbox[1].y, bbox[1].z}
															};

			pTree = createPartition(meshEntries, PartitioningTreeType::PARTITION_X, 0, 0, currentSection);
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
	const std::vector<std::vector<float>>    &currentSection,
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
		{ return ((a.max[0] + a.min[0]) / 2.)  < ((b.max[0] + b.min[0]) / 2.); }
		);		
		break;
	case PartitioningTreeType::PARTITION_Y:
		axisIdx = 1;
		std::sort(sortedMeshes.begin(), sortedMeshes.end(),
			[](MeshEntry const& a, MeshEntry const& b)
		{ return ((a.max[1] + a.min[1]) / 2.)  < ((b.max[1] + b.min[1]) / 2.); }
		);
		break;
	case PartitioningTreeType::PARTITION_Z:
		axisIdx = 2;
		std::sort(sortedMeshes.begin(), sortedMeshes.end(),
			[](MeshEntry const& a, MeshEntry const& b)
		{ return ((a.max[2] + a.min[2]) / 2.)  < ((b.max[2] + b.min[2]) / 2.); }
		);
		break;
	default:
		//only thing left is leaf node, which we do not expect it to ever come in here
		repoError << "Unexpected error: sorting Meshes for partitioning with an undefined axis.";
	}
	std::string axisStr[3] = {"X", "Y", "Z"};
	//repoTrace << "Splitting @ " << axisStr[axisIdx];
	//repoTrace << "Sorted meshes ("<< sortedMeshes.size() << "):";

	//for (const auto mesh : sortedMeshes)
	//	repoTrace << "\t" << UUIDtoString(mesh.id) << " [" << mesh.min[0] << "," << mesh.min[1] << "," << mesh.min[2] << "] "
	//	<< " [" << mesh.max[0] << "," << mesh.max[1] << "," << mesh.max[2] << "] ";

	//figure out the median value
	if (sortedMeshes.size() % 2)
	{
		//odd number
		auto medMesh = sortedMeshes[sortedMeshes.size() / 2];
		median = (medMesh.max[axisIdx] + medMesh.min[axisIdx]) / 2.;
	}
	else
	{
		//even - take the mean of the 2 middle number
		auto medMesh1 = sortedMeshes[sortedMeshes.size() / 2 - 1];
		auto medMesh2 = sortedMeshes[sortedMeshes.size() / 2];
		auto medPoint1 = (medMesh1.max[axisIdx] + medMesh1.min[axisIdx]) / 2.;
		auto medPoint2 = (medMesh2.max[axisIdx] + medMesh2.min[axisIdx]) / 2.;

		median = (medPoint1 + medPoint2) / 2.;

	}

	//put the meshes into left and right mesh vectors
	for (auto &entry_ : sortedMeshes)
	{
		
		if (entry_.min[axisIdx] <= median)
		{
			auto entry = entry_;
			if (entry.max[axisIdx] > median)
			{
				entry.max[axisIdx] = median;
			}
			lMeshes.push_back(entry);
		}
			
		if (entry_.max[axisIdx] > median)
		{
			auto entry = entry_;
			if (entry.min[axisIdx] < median)
			{
				entry.min[axisIdx] = median;
			}
			rMeshes.push_back(entry);
		}
			
	}
	
	if (!lMeshes.size() || !rMeshes.size())
	{
		//in no situation should the split happens where we're not getting anything on either side of the tree.
		repoWarning << "Terrible median choice: axis: " << axisIdx << " median: " << median;

	}

		
}
