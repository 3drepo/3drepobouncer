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

#include "repo_bson_factory.h"

#include <boost/filesystem.hpp>

#include "repo/lib/repo_log.h"
#include "repo/lib/repo_exception.h"

using namespace repo::core::model;

MaterialNode RepoBSONFactory::makeMaterialNode(
	const repo_material_t& material,
	const std::string& name,
	const std::vector<repo::lib::RepoUUID>& parents,
	const int& apiLevel)
{
	MaterialNode node;
	node.setMaterialStruct(material);
	node.setSharedID(repo::lib::RepoUUID::createUUID());
	node.changeName(name, true);
	node.addParents(parents);
	return node;
}

MetadataNode RepoBSONFactory::makeMetaDataNode(
	const std::vector<std::string>& keys,
	const std::vector<repo::lib::RepoVariant>& values,
	const std::string& name,
	const std::vector<repo::lib::RepoUUID>& parents,
	const int& apiLevel)
{
	auto keysLen = keys.size();
	auto valLen = values.size();
	//check keys and values have the same sizes
	if (keysLen != valLen)
	{
		repoWarning << "makeMetaDataNode: number of keys (" << keys.size()
			<< ") does not match the number of values(" << values.size() << ")!";
	}

	std::unordered_map<std::string, repo::lib::RepoVariant> metadataMap;

	for (int i = 0; i < (keysLen < valLen ? keysLen : valLen); ++i) {
		metadataMap[keys[i]] = values[i];
	}

	return makeMetaDataNode(metadataMap, name, parents);
}

MetadataNode RepoBSONFactory::makeMetaDataNode(
	const std::unordered_map<std::string, repo::lib::RepoVariant>& data,
	const std::string& name,
	const std::vector<repo::lib::RepoUUID>& parents,
	const int& apiLevel)
{
	MetadataNode node;
	node.setSharedID(repo::lib::RepoUUID::createUUID()); // By convention, factory produced MetadataNodes have SharedIds
	node.setMetadata(data);
	node.changeName(name, true);
	node.addParents(parents);
	return node;
}

MeshNode RepoBSONFactory::makeMeshNode(
	const std::vector<repo::lib::RepoVector3D>& vertices,
	const std::vector<repo_face_t>& faces,
	const std::vector<repo::lib::RepoVector3D>& normals,
	const std::vector<std::vector<float>>& boundingBox,
	const std::vector<std::vector<repo::lib::RepoVector2D>>& uvChannels,
	const std::string& name,
	const std::vector<repo::lib::RepoUUID>& parents)
{
	MeshNode node;
	node.setSharedID(repo::lib::RepoUUID::createUUID());
	node.setVertices(vertices);
	node.setBoundingBox({
		repo::lib::RepoVector3D(boundingBox[0]),
		repo::lib::RepoVector3D(boundingBox[1]),
	});
	node.setFaces(faces);
	node.setNormals(normals);
	for (size_t i = 0; i < uvChannels.size(); i++)
	{
		node.setUVChannel(i, uvChannels[i]);
	}
	node.changeName(name, true);
	node.addParents(parents);
	return node;
}

SupermeshNode RepoBSONFactory::makeSupermeshNode(
	const std::vector<repo::lib::RepoVector3D>& vertices,
	const std::vector<repo_face_t>& faces,
	const std::vector<repo::lib::RepoVector3D>& normals,
	const std::vector<std::vector<float>>& boundingBox,
	const std::vector<std::vector<repo::lib::RepoVector2D>>& uvChannels,
	const std::string& name,
	const std::vector<repo_mesh_mapping_t>& mappings
)
{
	SupermeshNode node;
	node.setVertices(vertices);
	node.setFaces(faces);
	node.setNormals(normals);
	node.setBoundingBox({
		repo::lib::RepoVector3D(boundingBox[0]),
		repo::lib::RepoVector3D(boundingBox[1]),
	});
	for (size_t i = 0; i < uvChannels.size(); i++)
	{
		node.setUVChannel(i, uvChannels[i]);
	}
	node.changeName(name, true);
	node.setMeshMapping(mappings);
	return node;
}

SupermeshNode RepoBSONFactory::makeSupermeshNode(
	const std::vector<repo::lib::RepoVector3D>& vertices,
	const std::vector<repo_face_t>& faces,
	const std::vector<repo::lib::RepoVector3D>& normals,
	const std::vector<std::vector<float>>& boundingBox,
	const std::vector<std::vector<repo::lib::RepoVector2D>>& uvChannels,
	const std::vector<repo_mesh_mapping_t>& mappings,
	const repo::lib::RepoUUID& id,
	const repo::lib::RepoUUID& sharedId,
	const std::vector<float> mappingIds
)
{
	SupermeshNode node;
	node.setVertices(vertices);
	node.setFaces(faces);
	node.setNormals(normals);
	node.setBoundingBox({
		repo::lib::RepoVector3D(boundingBox[0]),
		repo::lib::RepoVector3D(boundingBox[1]),
		});
	for (size_t i = 0; i < uvChannels.size(); i++)
	{
		node.setUVChannel(i, uvChannels[i]);
	}
	node.setUniqueID(id);
	node.setSharedID(id);
	node.setMeshMapping(mappings);
	node.setSubmeshIds(mappingIds);
	return node;
}

template<typename IdType>
RepoRefT<IdType> RepoBSONFactory::makeRepoRef(
	const IdType &id,
	const RepoRef::RefType &type,
	const std::string &link,
	const uint32_t size,
	const RepoRef::Metadata &metadata)
{
	if (type != RepoRef::RefType::FS)
	{
		throw repo::lib::RepoException("New RefNodes must be FS");
	}

	return RepoRefT<IdType>(
		id,
		link,
		size,
		metadata
	);
}

// Explicit instantations for the supported key types

template RepoRefT<std::string> RepoBSONFactory::makeRepoRef(
	const std::string&,
	const RepoRef::RefType&,
	const std::string&,
	const uint32_t,
	const RepoRef::Metadata&
);

template RepoRefT<repo::lib::RepoUUID> RepoBSONFactory::makeRepoRef(
	const repo::lib::RepoUUID&,
	const RepoRef::RefType&,
	const std::string&,
	const uint32_t,
	const RepoRef::Metadata&
);

RepoAssets RepoBSONFactory::makeRepoBundleAssets(
	const repo::lib::RepoUUID& revisionID,
	const std::vector<std::string>& repoBundleFiles,
	const std::string& database,
	const std::string& model,
	const std::vector<double>& offset,
	const std::vector<std::string>& repoJsonFiles,
	const std::vector<RepoSupermeshMetadata> metadata)
{
	return RepoAssets(
		revisionID,
		database,
		model,
		offset,
		repoBundleFiles,
		repoJsonFiles,
		metadata
	);
}

RepoCalibration repo::core::model::RepoBSONFactory::makeRepoCalibration(
	const repo::lib::RepoUUID& projectId,
	const repo::lib::RepoUUID& drawingId,
	const repo::lib::RepoUUID& revisionId,
	const std::vector<repo::lib::RepoVector3D>& horizontal3d,
	const std::vector<repo::lib::RepoVector2D>& horizontal2d,
	const std::string& units)
{
	return RepoCalibration(
		projectId,
		drawingId,
		revisionId,
		horizontal3d,
		horizontal2d,
		units
	);
}

ReferenceNode RepoBSONFactory::makeReferenceNode(
	const std::string& database,
	const std::string& project,
	const repo::lib::RepoUUID& revisionID,
	const bool& isUniqueID,
	const std::string& name,
	const int& apiLevel)
{
	ReferenceNode node;
	node.setSharedID(repo::lib::RepoUUID::createUUID());
	node.changeName(name.empty() ? database + "." + project : name, true);
	node.setDatabaseName(database);
	node.setProjectId(project);
	node.setProjectRevision(revisionID);
	node.setUseSpecificRevision(isUniqueID);
	return node;
}

ModelRevisionNode RepoBSONFactory::makeRevisionNode(
	const std::string& user,
	const repo::lib::RepoUUID& branch,
	const repo::lib::RepoUUID& id,
	const std::vector<std::string>& files,
	const std::vector<repo::lib::RepoUUID>& parent,
	const std::vector<double>& worldOffset,
	const std::string& message,
	const std::string& tag,
	const int& apiLevel
)
{
	ModelRevisionNode node;
	node.setUniqueID(id);
	node.setSharedID(branch);
	node.setFiles(files);
	node.setAuthor(user);
	node.addParents(parent);
	node.setCoordOffset(worldOffset);
	node.setMessage(message);
	node.setTag(tag);
	node.setTimestamp(); // Set timestamp to now
	return node;
}

TextureNode RepoBSONFactory::makeTextureNode(
	const std::string& name,
	const char* data,
	const uint32_t& byteCount,
	const uint32_t& width,
	const uint32_t& height,
	const std::vector<repo::lib::RepoUUID>& parentIDs,
	const int& apiLevel)
{
	TextureNode node;
	std::string ext;
	if (!name.empty())
	{
		boost::filesystem::path file { name };
		ext = file.extension().string();
		if (!ext.empty())
		{
			ext = ext.substr(1, ext.size());
		}
	}
	node.setData(
		std::vector<uint8_t>((uint8_t*)data, (uint8_t*)data + byteCount),
		width,
		height,
		ext
	);
	node.setSharedID(repo::lib::RepoUUID::createUUID()); // By convention TextureNode's get SharedIds as default
	node.changeName(name, true);
	node.addParents(parentIDs);
	return node;
}

TransformationNode RepoBSONFactory::makeTransformationNode(
	const repo::lib::RepoMatrix& transMatrix,
	const std::string& name,
	const std::vector<repo::lib::RepoUUID>& parents,
	const int& apiLevel)
{
	TransformationNode node;
	node.setSharedID(repo::lib::RepoUUID::createUUID());
	node.changeName(name, true);
	node.addParents(parents);
	node.setTransformation(transMatrix);
	return node;
}

RepoTask RepoBSONFactory::makeTask(
	const std::string& name,
	const uint64_t& startTime,
	const uint64_t& endTime,
	const repo::lib::RepoUUID& sequenceID,
	const std::unordered_map<std::string, std::string>& data,
	const std::vector<repo::lib::RepoUUID>& resources,
	const repo::lib::RepoUUID& parent,
	const repo::lib::RepoUUID& id
) {
	return RepoTask(
		name,
		id,
		parent,
		sequenceID,
		startTime,
		endTime,
		resources,
		data
	);
}

RepoSequence RepoBSONFactory::makeSequence(
	const std::vector<repo::core::model::RepoSequence::FrameData>& frameData,
	const std::string& name,
	const repo::lib::RepoUUID& id,
	const uint64_t firstFrame,
	const uint64_t lastFrame
) {
	return RepoSequence(
		name,
		id,
		repo::lib::RepoUUID::defaultValue,
		firstFrame,
		lastFrame,
		frameData
	);
}