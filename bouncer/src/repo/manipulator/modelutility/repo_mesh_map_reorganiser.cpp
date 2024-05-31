/**
*  Copyright (C) 2016 3D Repo Ltd
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
#include "repo_mesh_map_reorganiser.h"
#include "../../core/model/bson/repo_bson_builder.h"
#include "../../core/model/bson/repo_bson_factory.h"

using namespace repo::manipulator::modelutility;

MeshMapReorganiser::MeshMapReorganiser(
	const repo::core::model::SupermeshNode *mesh,
	const size_t                    &vertThreshold,
	const size_t					&faceThreshold) :
	mesh(mesh),
	maxVertices(vertThreshold),
	maxFaces(faceThreshold),
	oldFaces(mesh->getFaces()),
	oldVertices(mesh->getVertices()),
	oldNormals(mesh->getNormals()),
	oldUVs(mesh->getUVChannelsSeparated()),
	reMapSuccess(false)
{
	if (mesh && mesh->getMeshMapping().size())
	{
		newVertices = oldVertices;
		newNormals = oldNormals;
		newUVs = oldUVs;
		newFaces.reserve(oldFaces.size());
		serialisedFaces.reserve(oldFaces.size() * static_cast<int>(mesh->getPrimitive()));

		if (!(reMapSuccess = performSplitting()))
		{
			//mission failed clear up the memory
			newVertices.clear();
			newNormals.clear();
			newUVs.clear();
			newFaces.clear();
			matMap.clear();
			serialisedFaces.clear();
			reMappedMappings.clear();
			splitMap.clear();
			idMapBuf.clear();
		}
	}
	else
	{
		repoError << "Failed to remap mesh.";
		repoDebug << "Failed @ MeshMapRerganiser:  Null ptr to mesh or trying to remap a mesh without mesh mapping!";
	}
}

MeshMapReorganiser::~MeshMapReorganiser() {}

void MeshMapReorganiser::finishSubMesh(
	repo_mesh_mapping_t &mapping,
	std::vector<float>  &minBox,
	std::vector<float>  &maxBox,
	const size_t        &nVertices,
	const size_t        &nFaces
)
{
	mapping.vertTo = mapping.vertFrom + nVertices;
	mapping.triTo = mapping.triFrom + nFaces;

	if (minBox.size() == 3)
	{
		mapping.min = { minBox[0], minBox[1], minBox[2] };
	}
	else
	{
		repoError << "MeshMapReorganiser: Failed to find minimum bounding box value : Information is incomplete";
	}

	if (maxBox.size() == 3)
	{
		mapping.max = { maxBox[0], maxBox[1], maxBox[2] };
	}
	else
	{
		repoError << "MeshMapReorganiser: Failed to find maximum bounding box value : Information is incomplete";
	}

	minBox.clear();
	maxBox.clear();
}

void MeshMapReorganiser::newMatMapEntry(
	const repo_mesh_mapping_t &mapping,
	const size_t        &sVertices,
	const size_t        &sFaces
)
{
	matMap.back().push_back(mapping);

	matMap.back().back().vertFrom = sVertices;
	matMap.back().back().triFrom = sFaces;
}

void MeshMapReorganiser::completeLastMatMapEntry(
	const size_t        &eVertices,
	const size_t        &eFaces,
	const std::vector<float>  &minBox,
	const std::vector<float>  &maxBox
)
{
	matMap.back().back().vertTo = eVertices;
	matMap.back().back().triTo = eFaces;
	if (minBox.size())
		matMap.back().back().min = { minBox[0], minBox[1], minBox[2] };
	if (maxBox.size())
		matMap.back().back().max = { maxBox[0], maxBox[1], maxBox[2] };
}

std::vector<std::vector<float>> MeshMapReorganiser::getIDMapArrays() const {
	return reMapSuccess ? idMapBuf : std::vector<std::vector<float>>();
}

std::vector<std::vector<repo_mesh_mapping_t>>
MeshMapReorganiser::getMappingsPerSubMesh() const {
	return reMapSuccess ? matMap : std::vector<std::vector<repo_mesh_mapping_t>>();
}

std::vector<uint16_t> MeshMapReorganiser::getSerialisedFaces() const {
	return reMapSuccess ? serialisedFaces : std::vector<uint16_t>();
}

std::unordered_map<repo::lib::RepoUUID, std::vector<uint32_t>, repo::lib::RepoUUIDHasher>
MeshMapReorganiser::getSplitMapping() const {
	return reMapSuccess ? splitMap : std::unordered_map<repo::lib::RepoUUID, std::vector<uint32_t>, repo::lib::RepoUUIDHasher>();
}

repo::core::model::SupermeshNode MeshMapReorganiser::getRemappedMesh() const
{
	if (reMapSuccess)
	{
		auto bbox = mesh->getBoundingBox();
		std::vector<std::vector<float>> bboxArr;
		if (bbox.size())
		{
			bboxArr.push_back({ bbox[0].x, bbox[0].y, bbox[0].z });
			bboxArr.push_back({ bbox[1].x, bbox[1].y, bbox[1].z });
		}

		auto submeshIds = getIDMapArrays();
		std::vector<float> newIds;
		for (auto& buf : submeshIds)
		{
			newIds.insert(newIds.end(), buf.begin(), buf.end());
		}

		auto newMesh = repo::core::model::RepoBSONFactory::makeSupermeshNode(
			newVertices,
			newFaces,
			newNormals,
			bboxArr,
			newUVs,
			newIds,
			reMappedMappings,
			mesh->getUniqueID(),
			mesh->getSharedID());

		return newMesh;
	}
	else
	{
		return repo::core::model::SupermeshNode();
	}
}

bool MeshMapReorganiser::performSplitting()
{
	std::vector<repo_mesh_mapping_t> newMappings;
	std::vector<repo_mesh_mapping_t> orgMappings = mesh->getMeshMapping();

	std::vector<repo_face_t> faces;

	size_t subMeshVertexCount = 0;
	size_t subMeshFaceCount = 0;

	size_t totalVertexCount = 0;
	size_t totalFaceCount = 0;

	size_t idMapIdx = 0;
	size_t orgFaceIdx = 0;

	bool finishedSubMesh = true;

	std::vector<float> bboxMin;
	std::vector<float> bboxMax;

	//Resources
	repo::lib::RepoUUID superMeshID = mesh->getUniqueID();

	repoTrace << "Performing splitting on mesh: " << mesh->getUniqueID();
	size_t nMappings = orgMappings.size();
	size_t tenths = orgMappings.size() / 10;
	if (!tenths)
	{
		tenths = nMappings;
	}
	size_t count = 0;
	for (const auto &currentSubMesh : orgMappings)
	{
		if (++count % tenths == 0)
			repoTrace << "Progress: " << (count / tenths) << "0%";
		splitMap[currentSubMesh.mesh_id] = std::vector < uint32_t >();

		auto currentMeshVFrom = currentSubMesh.vertFrom;
		auto currentMeshVTo = currentSubMesh.vertTo;
		auto currentMeshTFrom = currentSubMesh.triFrom;
		auto currentMeshTTo = currentSubMesh.triTo;

		auto currentMeshNumVertices = currentMeshVTo - currentMeshVFrom;
		auto currentMeshNumFaces = currentMeshTTo - currentMeshTFrom;

		// If the current cumulative count of vertices is greater than the
		// vertex limit then start a new mesh.
		// If newMappings === 0 we need to initialize the first mesh
		if (((subMeshVertexCount + currentMeshNumVertices) > maxVertices) || ((subMeshFaceCount + currentMeshNumFaces) > maxFaces) || finishedSubMesh) {
			// Close off the previous sub mesh
			if (!finishedSubMesh) // Have we already finished this one
			{
				finishSubMesh(newMappings.back(), bboxMin, bboxMax, subMeshVertexCount, subMeshFaceCount);
			}

			//Reset the counters for the submesh vertex and face count
			// and begin a new sub mesh with a new ID
			subMeshVertexCount = 0;
			subMeshFaceCount = 0;
			finishedSubMesh = false;

			newMappings.resize(newMappings.size() + 1);
			startSubMesh(newMappings.back(), superMeshID, mesh->getSharedID(), currentSubMesh.material_id, totalVertexCount, totalFaceCount);
		}

		// Now we've started a new mesh is the mesh that we're trying to add greater than
		// the limit itself. In the case that it is, this will always flag as above.
		if ((currentMeshNumVertices > maxVertices) || (currentMeshNumFaces > maxFaces)) {
			size_t retTotalVCount, retTotalFCount;
			newMatMapEntry(currentSubMesh, totalVertexCount, totalFaceCount);
			if (!splitLargeMesh(currentSubMesh, newMappings, idMapIdx, orgFaceIdx, totalVertexCount, totalFaceCount))
			{
				return false;
			}

			++idMapIdx;

			finishedSubMesh = true;
		}
		else
		{
			newMatMapEntry(currentSubMesh, totalVertexCount, totalFaceCount);

			for (uint32_t fIdx = 0; fIdx < currentMeshNumFaces; fIdx++)
			{
				repo_face_t currentFace = oldFaces[orgFaceIdx++];
				repo_face_t newFace;
				for (const auto& indexValue : currentFace)
				{
					// Take currentMeshVFrom from Index Value to reset to zero start,
					// then add back in the current running total to append after
					// previous mesh.
					auto newIdx = indexValue + (subMeshVertexCount - currentMeshVFrom);
					newFace.push_back(newIdx);
					serialisedFaces.push_back(newIdx);
				}
				newFaces.push_back(newFace);
				++subMeshFaceCount;
				++totalFaceCount;
			}

			updateIDMapArray(currentMeshNumVertices, idMapIdx++);

			updateBoundingBoxes(bboxMin, bboxMax, currentSubMesh.min, currentSubMesh.max);

			subMeshVertexCount += currentMeshNumVertices;
			totalVertexCount += currentMeshNumVertices;

			splitMap[currentSubMesh.mesh_id].push_back(newMappings.size() - 1);
			completeLastMatMapEntry(totalVertexCount, totalFaceCount);
		}
	}

	if (subMeshVertexCount) {
		finishSubMesh(newMappings.back(), bboxMin, bboxMax, subMeshVertexCount, subMeshFaceCount);
	}

	reMappedMappings = newMappings;
	return true;
}

bool MeshMapReorganiser::splitLargeMesh(
	const repo_mesh_mapping_t        &currentSubMesh,
	std::vector<repo_mesh_mapping_t> &newMappings,
	size_t                           &idMapIdx,
	size_t                           &orgFaceIdx,
	size_t                           &totalVertexCount,
	size_t                           &totalFaceCount)
{
	std::unordered_map<uint32_t, uint32_t> reIndexMap;
	std::vector<repo::lib::RepoVector3D> reMappedVertices, reMappedNormals;
	std::vector<repo_color4d_t> reMappedCols;
	std::vector<std::vector<repo::lib::RepoVector2D>> reMappedUVs;
	if (oldUVs.size())
		reMappedUVs.resize(oldUVs.size());

	auto currentMeshVFrom = currentSubMesh.vertFrom;
	auto currentMeshVTo = currentSubMesh.vertTo;
	auto currentMeshTFrom = currentSubMesh.triFrom;
	auto currentMeshTTo = currentSubMesh.triTo;

	auto currentMeshNumVertices = currentMeshVTo - currentMeshVFrom;
	auto currentMeshNumFaces = currentMeshTTo - currentMeshTFrom;

	//When split mesh is called, it's guaranteed that a new mesh mapping has been created for it
	auto newVerticesVFrom = newMappings.back().vertFrom;

	const bool hasNormal = oldNormals.size();
	const bool hasUV = oldUVs.size();

	// Split mesh information
	bool startedLargeMeshSplit = true;
	size_t splitMeshVertexCount = 0;
	size_t splitMeshFaceCount = 0;
	std::vector<float> bboxMin;
	std::vector<float> bboxMax;

	size_t totalLargeMeshVertexCount = 0;

	// Perform quick and dirty splitting algorithm
	// Loop over all faces in the giant mesh
	for (uint32_t fIdx = 0; fIdx < currentMeshNumFaces; ++fIdx) {
		repo_face_t currentFace = oldFaces[orgFaceIdx++];
		auto        nSides = currentFace.size();

		// If we haven't started yet, or the current number of vertices that we have
		// split is greater than the limit we need to start a new subMesh
		if (((splitMeshVertexCount + nSides) > maxVertices) || (splitMeshFaceCount >= maxFaces) || !startedLargeMeshSplit) {
			// If we have started we must be here because we have created a split mesh
			// greater than the required number of vertices

			if (fIdx)
				splitMap[currentSubMesh.mesh_id].push_back(newMappings.size() - 1);

			if (startedLargeMeshSplit) {
				updateIDMapArray(splitMeshVertexCount, idMapIdx);
				finishSubMesh(newMappings.back(), bboxMin, bboxMax, splitMeshVertexCount, splitMeshFaceCount);

				completeLastMatMapEntry(matMap.back().back().vertFrom + splitMeshVertexCount,
					matMap.back().back().triFrom + splitMeshFaceCount, bboxMin, bboxMax);
			}

			totalVertexCount += splitMeshVertexCount;
			totalFaceCount += splitMeshFaceCount;
			totalLargeMeshVertexCount += splitMeshVertexCount;
			startedLargeMeshSplit = true;
			newMappings.resize(newMappings.size() + 1);

			startSubMesh(newMappings.back(), mesh->getUniqueID(), mesh->getSharedID(), currentSubMesh.material_id, totalVertexCount, totalFaceCount);
			newMatMapEntry(currentSubMesh, totalVertexCount, totalFaceCount);
			splitMeshVertexCount = 0;
			splitMeshFaceCount = 0;
			reIndexMap.clear();
		}//if (((splitMeshVertexCount + nSides) > maxVertices) || ((splitMeshFaceCount + 1) > maxFaces) || !startedLargeMeshSplit)

		repo_face_t newFace;
		for (const auto& indexValue : currentFace)
		{
			const auto it = reIndexMap.find(indexValue);
			if (it == reIndexMap.end())
			{
				reIndexMap[indexValue] = splitMeshVertexCount;
				repo::lib::RepoVector3D vertex = oldVertices[indexValue];
				reMappedVertices.push_back(vertex);

				if (hasNormal)
				{
					reMappedNormals.push_back(oldNormals[indexValue]);
				}

				if (hasUV)
				{
					for (int iUV = 0; iUV < oldUVs.size(); ++iUV)
						reMappedUVs[iUV].push_back(oldUVs[iUV][indexValue]);
				}

				updateBoundingBoxes(bboxMin, bboxMax, vertex, vertex);
				splitMeshVertexCount++;
			}
			if (splitMeshVertexCount < reIndexMap[indexValue])
			{
				repoError << "SplitMesh Vertex count (" << splitMeshVertexCount << ") "
					<< "is smaller than remapped index value (" << reIndexMap[indexValue] << ") -  potentially out of range.";
				return false;
			}

			newFace.push_back(reIndexMap[indexValue]);
			serialisedFaces.push_back(reIndexMap[indexValue]);
		}//for (const auto &indexValue : currentFace)

		newFaces.push_back(newFace);
		splitMeshFaceCount++;
	}//for (uint32_t fIdx = 0; fIdx < currentMeshNumFaces; ++fIdx)

	updateIDMapArray(splitMeshVertexCount, idMapIdx);
	totalVertexCount += splitMeshVertexCount;
	totalLargeMeshVertexCount += splitMeshVertexCount;
	totalFaceCount += splitMeshFaceCount;

	if (currentMeshNumVertices > totalLargeMeshVertexCount)
	{
		auto leftOverVertices = currentMeshNumVertices - totalLargeMeshVertexCount;

		auto startingPos = newVertices.begin() + newMappings.back().vertFrom + splitMeshVertexCount;

		//Chop out the unwanted vertices
		newVertices.erase(startingPos, startingPos + leftOverVertices);
		if (hasNormal)
		{
			auto startingPosN = newNormals.begin() + newMappings.back().vertFrom + splitMeshVertexCount;
			newNormals.erase(startingPosN, startingPosN + leftOverVertices);
		}

		if (hasUV)
		{
			for (int iUV = 0; iUV < oldUVs.size(); ++iUV)
			{
				auto startingPosN = newUVs[iUV].begin() + newMappings.back().vertFrom + splitMeshVertexCount;
				newUVs[iUV].erase(startingPosN, startingPosN + leftOverVertices);
			}
		}
	}
	else if (currentMeshNumVertices < totalLargeMeshVertexCount)
	{
		auto extraVertices = totalLargeMeshVertexCount - currentMeshNumVertices;
		auto startingPos = newVertices.begin() + newVerticesVFrom;

		//Expand newVertices to include the extra vertices
		std::vector<repo::lib::RepoVector3D> extraVs;
		extraVs.resize(extraVertices);
		newVertices.insert(startingPos, extraVs.begin(), extraVs.end());
		if (hasNormal)
		{
			auto startingPosN = newNormals.begin() + newVerticesVFrom;
			newNormals.insert(startingPosN, extraVs.begin(), extraVs.end());
		}

		if (hasUV)
		{
			for (int iUV = 0; iUV < oldUVs.size(); ++iUV)
			{
				auto startingPosN = newUVs[iUV].begin() + newVerticesVFrom;
				std::vector<repo::lib::RepoVector2D> extras;
				extras.resize(extraVertices);
				newUVs[iUV].insert(startingPosN, extras.begin(), extras.end());
			}
		}
	}

	//Modify the vertices as it may be rearranged within this region
	std::copy(reMappedVertices.begin(), reMappedVertices.end(), newVertices.begin() + newVerticesVFrom);
	if (hasNormal)
		std::copy(reMappedNormals.begin(), reMappedNormals.end(), newNormals.begin() + newVerticesVFrom);

	if (hasUV)
		for (int iUV = 0; iUV < oldUVs.size(); ++iUV)
			std::copy(reMappedUVs[iUV].begin(), reMappedUVs[iUV].end(), newUVs[iUV].begin() + newVerticesVFrom);

	splitMap[currentSubMesh.mesh_id].push_back(newMappings.size() - 1);
	finishSubMesh(newMappings.back(), bboxMin, bboxMax, splitMeshVertexCount, splitMeshFaceCount);
	completeLastMatMapEntry(matMap.back().back().vertFrom + splitMeshVertexCount,
		matMap.back().back().triFrom + splitMeshFaceCount, bboxMin, bboxMax);
	repoTrace << "Completed Large Mesh Split";
	return true;
}

void MeshMapReorganiser::startSubMesh(
	repo_mesh_mapping_t &mapping,
	const repo::lib::RepoUUID      &meshID,
	const repo::lib::RepoUUID      &sharedID,
	const repo::lib::RepoUUID      &matID,
	const size_t        &sVertices,
	const size_t        &sFaces
)
{
	mapping.vertFrom = sVertices;
	mapping.triFrom = sFaces;

	// The split mappings are not bijective with respect to the originals,
	// so make sure these are initialised to invalid values so its obvious
	// if they are being misused.

	mapping.material_id = repo::lib::RepoUUID::defaultValue;
	mapping.mesh_id = repo::lib::RepoUUID::defaultValue;
	mapping.shared_id = repo::lib::RepoUUID::defaultValue;

	idMapBuf.resize(idMapBuf.size() + 1);
	idMapBuf.back().clear();
	matMap.resize(matMap.size() + 1);
	matMap.back().clear();
}

void MeshMapReorganiser::updateBoundingBoxes(
	std::vector<float>  &min,
	std::vector<float>  &max,
	const repo::lib::RepoVector3D &smMin,
	const repo::lib::RepoVector3D &smMax)
{
	//Update Bounding box
	if (min.size())
	{
		if (min[0] > smMin.x)
			min[0] = smMin.x;

		if (min[1] > smMin.y)
			min[1] = smMin.y;

		if (min[2] > smMin.z)
			min[2] = smMin.z;
	}
	else
	{
		min = { smMin.x, smMin.y, smMin.z };
	}

	if (max.size())
	{
		if (max[0] < smMax.x)
			max[0] = smMax.x;

		if (max[1] < smMax.y)
			max[1] = smMax.y;

		if (max[2] < smMax.z)
			max[2] = smMax.z;
	}
	else
	{
		max = { smMax.x, smMax.y, smMax.z };
	}
}

void MeshMapReorganiser::updateIDMapArray(
	const size_t &n,
	const size_t &value)
{
	auto idMapLength = idMapBuf.back().size();
	idMapBuf.back().resize(idMapLength + n);

	float value_f = value;
	std::fill(idMapBuf.back().begin() + idMapLength, idMapBuf.back().end(), value_f);
	auto bufferIdx = idMapBuf.size() - 1;
}