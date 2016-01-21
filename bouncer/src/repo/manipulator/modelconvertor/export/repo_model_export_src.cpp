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

/**
* Allows Export functionality from 3D Repo World to SRC
*/


#include "repo_model_export_src.h"
#include "../../../lib/repo_log.h"
#include "../../../core/model/bson/repo_bson_factory.h"

using namespace repo::manipulator::modelconvertor;

const static uint32_t SRC_MAGIC_BIT      = 23;
const static uint32_t SRC_VERSION        = 42;
const static size_t SRC_MAX_VERTEX_LIMIT = 65535;
const static size_t SRC_X3DOM_FLOAT      = 5126;
const static size_t SRC_X3DOM_USHORT     = 5123;
const static size_t SRC_X3DOM_TRIANGLE   = 4;
const static std::string SRC_VECTOR_2D   = "VEC2";
const static std::string SRC_VECTOR_3D   = "VEC3";
const static std::string SRC_SCALAR      = "SCALAR";


const static std::string SRC_LABEL_ACCESSORS            = "accessors";
const static std::string SRC_LABEL_ACCESSORS_ATTR_VIEWS = "attributeViews";
const static std::string SRC_LABEL_BUFFER_CHUNKS        = "bufferChunks";
const static std::string SRC_LABEL_BUFF_VIEWS           = "bufferViews";
const static std::string SRC_LABEL_INDEX_VIEWS          = "indexViews";

const static std::string SRC_LABEL_ATTRS                = "attributes";
const static std::string SRC_LABEL_BUFFVIEW             = "bufferView";
const static std::string SRC_LABEL_BYTE_LENGTH          = "byteLength";
const static std::string SRC_LABEL_BYTE_OFFSET          = "byteOffset";
const static std::string SRC_LABEL_BYTE_STRIDE          = "byteStride";
const static std::string SRC_LABEL_COMP_TYPE            = "componentType";
const static std::string SRC_LABEL_TYPE                 = "type";
const static std::string SRC_LABEL_CHUNKS               = "chunks";
const static std::string SRC_LABEL_COUNT                = "count";
const static std::string SRC_LABEL_DECODE_OFFSET        = "decodeOffset";
const static std::string SRC_LABEL_DECODE_SCALE         = "decodeScale";
const static std::string SRC_LABEL_ID                   = "id";
const static std::string SRC_LABEL_INDICIES             = "indices";
const static std::string SRC_LABEL_PRIMITIVE            = "primitive";
const static std::string SRC_LABEL_MESHES               = "meshes";
const static std::string SRC_LABEL_NORMAL               = "normal";
const static std::string SRC_LABEL_POSITION             = "position";
const static std::string SRC_LABEL_TEX_COORD            = "texcoord";

const static std::string SRC_PREFIX_POSITION_ATTR_VIEW = "p";
const static std::string SRC_PREFIX_NORMAL_ATTR_VIEW   = "n";
const static std::string SRC_PREFIX_UV_ATTR_VIEW       = "u";
const static std::string SRC_PREFIX_IDMAP_ATTR_VIEW    = "id";
const static std::string SRC_PREFIX_IDX_VIEW           = "i";

const static std::string SRC_PREFIX_POSITION_BUFF_VIEW = "pb";
const static std::string SRC_PREFIX_NORMAL_BUFF_VIEW   = "nb";
const static std::string SRC_PREFIX_TEX_BUFF_VIEW      = "tb";
const static std::string SRC_PREFIX_UV_BUFF_VIEW       = "ub";
const static std::string SRC_PREFIX_IDX_BUFF_VIEW      = "ib";
const static std::string SRC_PREFIX_IDMAP_BUFF_VIEW    = "idb";

const static std::string SRC_PREFIX_POSITION_BUFF_CHK  = "pc";
const static std::string SRC_PREFIX_IDX_BUFF_CHK       = "ic";
const static std::string SRC_PREFIX_NORMAL_BUFF_CHK    = "nc";
const static std::string SRC_PREFIX_TEX_BUFF_CHK       = "tc";
const static std::string SRC_PREFIX_UV_BUFF_CHK        = "uc";
const static std::string SRC_PREFIX_IDMAP_BUFF_CHK     = "idc";

const static std::string SRC_PREFIX_IDMAP              = "idMap";



struct repo_src_mesh_info
{
	std::string meshId;
	uint32_t offset;
	uint32_t vCount;
	uint32_t vFrom;
	uint32_t vTo;
	uint32_t fCount;
	uint32_t fFrom;
	uint32_t fTo;
	repo_vector_t bbox[2];
	std::vector<uint32_t> idxList;
	std::vector<float> idMapBuf;
};


SRCModelExport::SRCModelExport(
	const repo::core::model::RepoScene *scene
	) : AbstractModelExport()
	, scene(scene)
{
	//Considering all newly imported models should have a stash graph, we only need to support stash graph?
	if (scene)
	{
		if (scene->hasRoot(repo::core::model::RepoScene::GraphType::OPTIMIZED))
		{
			gType = repo::core::model::RepoScene::GraphType::OPTIMIZED;
			convertSuccess = generateTreeRepresentation();
		}
		else
		{
			repoError << "Scene has no optimised graph. SRC Exporter relies on this.";
			convertSuccess = false;
		}
		
	}
	else
	{
		repoError << "Unable to export to SRC : Empty scene graph!";
	}

}

SRCModelExport::~SRCModelExport()
{
}

std::unordered_map<std::string, std::vector<uint8_t>> SRCModelExport::getFileAsBuffer()
{
	std::unordered_map < std::string, std::vector<uint8_t> > fileBuffers;

	for (const auto &treePair : trees)
	{
		std::vector<uint8_t> buffer;
		std::string fName = treePair.first;		

		std::stringstream ss;
		boost::property_tree::write_json(ss, treePair.second);
		std::string jsonStr = ss.str();

		//one char is one byte, 12bytes for Magic Bit(4), SRC Version (4), Header Length(4)	
		size_t jsonByteSize = jsonStr.size()*sizeof(*jsonStr.c_str());
		size_t headerSize = 12 + jsonByteSize;

		size_t buffPtr = 0; //BufferPointer in bytes

		buffer.resize(headerSize);

		uint32_t* bufferAsUInt = (uint32_t*)buffer.data();

		//Header ints
		bufferAsUInt[0] = SRC_MAGIC_BIT;
		bufferAsUInt[1] = SRC_VERSION;
		bufferAsUInt[2] = jsonByteSize;

		buffPtr += 3 * sizeof(uint32_t);

		//write json
		memcpy(&buffer[buffPtr], jsonStr.c_str(), jsonByteSize);

		buffPtr += jsonByteSize;
		
		if (fullDataBuffer.find(fName) != fullDataBuffer.end())
		{
			//Add data buffer to the full buffer
			buffer.insert(buffer.end(), fullDataBuffer[fName].begin(), fullDataBuffer[fName].end());
			fileBuffers[fName] = buffer;
		}
		else
		{
			repoError << " Failed to find data buffer for " << fName;
		}

	}

	return fileBuffers;
}

bool SRCModelExport::generateTreeRepresentation(
	)
{
	bool success;
	if (success = scene->hasRoot(gType))
	{
		auto meshes = scene->getAllMeshes(gType);
		size_t index = 0;
		//Every mesh is a new SRC file
		fullDataBuffer.reserve(meshes.size());
		repoTrace << "#Meshes = " << meshes.size();
		for (const repo::core::model::RepoNode* mesh : meshes)
		{	
			std::string textureID = scene->getTextureIDForMesh(gType, mesh->getSharedID());
			std::vector<uint16_t> facebuf;
			std::vector<std::vector<float>> idMapBuf;
			repo::core::model::MeshNode splittedMesh =
				((repo::core::model::MeshNode*)mesh)->cloneAndRemapMeshMapping(SRC_MAX_VERTEX_LIMIT, facebuf, idMapBuf);
			repoTrace << " Mapping before: " << ((repo::core::model::MeshNode*)mesh)->getMeshMapping().size() << " Mapping after: " << splittedMesh.getMeshMapping().size();

			std::string ext = ".src";
			if (((repo::core::model::MeshNode*)mesh)->getMeshMapping().size() > 1)
			{
				ext += ".mpc";
			}

			if (!textureID.empty())
			{
				ext += "?tex_uuid=" + textureID;
			}

			addMeshToExport(splittedMesh, index++, facebuf, idMapBuf, ext);
			

		}		
	}

	return success;


}

std::string SRCModelExport::getSupportedFormats()
{
	return ".src";
}
template <>
void SRCModelExport::addToTree<std::string>(
	boost::property_tree::ptree &tree,
	const std::string           &label,
	const std::string           &value)
{
	if (label.empty())
		tree.put(label, value, stringTranslator());
	else
		tree.add(label, value, stringTranslator());
}

void SRCModelExport::addMeshToExport(
	const repo::core::model::MeshNode      &mesh,
	const size_t                           &idx,
	const std::vector<uint16_t>            &faceBuf,
	const std::vector<std::vector<float>>  &idMapBuf,
	const std::string                      &fileExt
	)
{
	std::vector<repo_mesh_mapping_t> mapping = mesh.getMeshMapping();
	

	auto vertices = mesh.getVertices();
	auto normals = mesh.getNormals();
	auto uvs = mesh.getUVChannels();
	
	//Define starting position of buffers
	size_t bufPos = 0; //In bytes
	size_t vertexWritePosition = bufPos;

	bufPos += vertices->size()*sizeof(*vertices->data());

	size_t normalWritePosition = bufPos;
	bufPos += normals->size()*sizeof(*normals->data());

	size_t facesWritePosition = bufPos;
	bufPos += faceBuf.size() * sizeof(*faceBuf.data());

	size_t idMapWritePosition = bufPos;
	bufPos += vertices->size() * sizeof(float); //idMap array is of floats

	size_t uvWritePosition = bufPos;
	size_t nSubMeshes = mapping.size();

	std::string meshId = UUIDtoString(mesh.getUniqueID());

	boost::property_tree::ptree tree;

	repoTrace << "Looping Through submeshes (#submeshes : " << nSubMeshes << ")";
	for (size_t subMeshIdx = 0; subMeshIdx < nSubMeshes; ++subMeshIdx)
	{
		// ------------------------------------------------------------------------
		// In SRC each attribute has an associated attributeView.
		// Each attributeView has an associated bufferView
		// Each bufferView is composed of several chunks.
		// ------------------------------------------------------------------------

		std::string meshIDX = std::to_string(idx) + "_" + std::to_string(subMeshIdx);

		std::string positionAttributeView = SRC_PREFIX_POSITION_ATTR_VIEW + meshIDX;
		std::string normalAttributeView = SRC_PREFIX_NORMAL_ATTR_VIEW + meshIDX;
		std::string uvAttributeView = SRC_PREFIX_UV_ATTR_VIEW + meshIDX;
		std::string idMapAttributeView = SRC_PREFIX_IDMAP_ATTR_VIEW + meshIDX;
		std::string indexView = SRC_PREFIX_IDX_VIEW + meshIDX;

		std::string positionBufferView = SRC_PREFIX_POSITION_BUFF_VIEW + meshIDX;
		std::string normalBufferView = SRC_PREFIX_NORMAL_BUFF_VIEW + meshIDX;
		std::string texBufferView = SRC_PREFIX_TEX_BUFF_VIEW + meshIDX;
		std::string uvBufferView = SRC_PREFIX_UV_BUFF_VIEW + meshIDX;
		std::string indexBufferView = SRC_PREFIX_IDX_BUFF_VIEW + meshIDX;
		std::string idMapBufferView = SRC_PREFIX_IDMAP_BUFF_VIEW + meshIDX;

		std::string positionBufferChunk = SRC_PREFIX_POSITION_BUFF_CHK + meshIDX;
		std::string indexBufferChunk = SRC_PREFIX_IDX_BUFF_CHK + meshIDX;
		std::string normalBufferChunk = SRC_PREFIX_NORMAL_BUFF_CHK + meshIDX;
		std::string texBufferChunk = SRC_PREFIX_TEX_BUFF_CHK + meshIDX;
		std::string uvBufferChunk = SRC_PREFIX_UV_BUFF_CHK + meshIDX;
		std::string idMapBufferChunk = SRC_PREFIX_IDMAP_BUFF_CHK + meshIDX;

		std::string idMapID = SRC_PREFIX_IDMAP + meshIDX;

		std::string meshID = meshId + "_" + std::to_string(subMeshIdx);

		std::string srcAccessors_AttributeViews = SRC_LABEL_ACCESSORS + "." + SRC_LABEL_ACCESSORS_ATTR_VIEWS;
		std::string srcMesh_MeshID = SRC_LABEL_MESHES + "." + meshID + ".";


		size_t vCount = mapping[subMeshIdx].vertTo - mapping[subMeshIdx].vertFrom;
		size_t fCount = mapping[subMeshIdx].triTo - mapping[subMeshIdx].triFrom;

		// SRC Header for this mesh
		if (vertices->size())
		{
			std::string srcAccessors_AttrViews_positionAttrView = srcAccessors_AttributeViews + "." + positionAttributeView + ".";
			addToTree(tree, srcAccessors_AttrViews_positionAttrView + SRC_LABEL_BUFFVIEW, positionBufferView);
			addToTree(tree, srcAccessors_AttrViews_positionAttrView + SRC_LABEL_BYTE_OFFSET, 0);
			addToTree(tree, srcAccessors_AttrViews_positionAttrView + SRC_LABEL_BYTE_STRIDE, 12);
			addToTree(tree, srcAccessors_AttrViews_positionAttrView + SRC_LABEL_COMP_TYPE, SRC_X3DOM_FLOAT);
			addToTree(tree, srcAccessors_AttrViews_positionAttrView + SRC_LABEL_TYPE, SRC_VECTOR_3D);
			addToTree(tree, srcAccessors_AttrViews_positionAttrView + SRC_LABEL_COUNT, vCount);

			std::vector<uint32_t> offsetArr = { 0, 0, 0 };
			std::vector<uint32_t> scaleArr = { 1, 1, 1 };
			tree.add_child(srcAccessors_AttrViews_positionAttrView + SRC_LABEL_DECODE_OFFSET, createPTArray(offsetArr));
			tree.add_child(srcAccessors_AttrViews_positionAttrView + SRC_LABEL_DECODE_SCALE, createPTArray(scaleArr));


			std::string srcBufferChunks_positionBufferChunks = SRC_LABEL_BUFFER_CHUNKS + "." + positionBufferChunk + ".";
			size_t verticeBufferLength = vCount * sizeof(*vertices->data());

			addToTree(tree, srcBufferChunks_positionBufferChunks + SRC_LABEL_BYTE_OFFSET, vertexWritePosition);
			addToTree(tree, srcBufferChunks_positionBufferChunks + SRC_LABEL_BYTE_LENGTH, verticeBufferLength); 

			vertexWritePosition += verticeBufferLength;

			std::string srcBufferViews_positionBufferView = SRC_LABEL_BUFF_VIEWS + "." + positionBufferView + ".";

			std::vector<std::string> chunksArray = { positionBufferChunk };
			tree.add_child(srcBufferViews_positionBufferView + SRC_LABEL_CHUNKS, createPTArray(chunksArray));

			addToTree(tree, srcMesh_MeshID + SRC_LABEL_ATTRS + "." + SRC_LABEL_POSITION, positionAttributeView);
		}

		//Normal Attribute View
		if (normals->size())
		{
			std::string srcAccessors_AttrViews_normalAttrView = srcAccessors_AttributeViews + "." + normalAttributeView + ".";
			addToTree(tree, srcAccessors_AttrViews_normalAttrView + SRC_LABEL_BUFFVIEW, normalBufferView);
			addToTree(tree, srcAccessors_AttrViews_normalAttrView + SRC_LABEL_BYTE_OFFSET, 0);
			addToTree(tree, srcAccessors_AttrViews_normalAttrView + SRC_LABEL_BYTE_STRIDE, 12);
			addToTree(tree, srcAccessors_AttrViews_normalAttrView + SRC_LABEL_COMP_TYPE, SRC_X3DOM_FLOAT);
			addToTree(tree, srcAccessors_AttrViews_normalAttrView + SRC_LABEL_TYPE, SRC_VECTOR_3D);
			addToTree(tree, srcAccessors_AttrViews_normalAttrView + SRC_LABEL_COUNT, vCount);

			std::vector<uint32_t> offsetArr = { 0, 0, 0 };
			std::vector<uint32_t> scaleArr = { 1, 1, 1 };
			tree.add_child(srcAccessors_AttrViews_normalAttrView + SRC_LABEL_DECODE_OFFSET, createPTArray(offsetArr));
			tree.add_child(srcAccessors_AttrViews_normalAttrView + SRC_LABEL_DECODE_SCALE, createPTArray(scaleArr));


			std::string srcBufferChunks_positionBufferChunks = SRC_LABEL_BUFFER_CHUNKS + "." + normalBufferChunk + ".";
			size_t verticeBufferLength = vCount * sizeof(*normals->data());

			addToTree(tree, srcBufferChunks_positionBufferChunks + SRC_LABEL_BYTE_OFFSET, normalWritePosition);
			addToTree(tree, srcBufferChunks_positionBufferChunks + SRC_LABEL_BYTE_LENGTH, verticeBufferLength); 

			normalWritePosition += verticeBufferLength;


			std::string srcBufferViews_normalBufferView = SRC_LABEL_BUFF_VIEWS + "." + normalBufferView + ".";

			std::vector<std::string> chunksArray = { normalBufferChunk };
			tree.add_child(srcBufferViews_normalBufferView + SRC_LABEL_CHUNKS, createPTArray(chunksArray));

			addToTree(tree, srcMesh_MeshID + SRC_LABEL_ATTRS + "." + SRC_LABEL_NORMAL, normalAttributeView);

		}

		// Index View
		if (faceBuf.size())
		{
			std::string srcAccessors_indexViews = SRC_LABEL_ACCESSORS + "." + SRC_LABEL_INDEX_VIEWS + "." + indexView + ".";

			addToTree(tree, srcAccessors_indexViews + SRC_LABEL_BUFFVIEW, indexBufferView);
			addToTree(tree, srcAccessors_indexViews + SRC_LABEL_BYTE_OFFSET, 0);
			addToTree(tree, srcAccessors_indexViews + SRC_LABEL_COMP_TYPE, SRC_X3DOM_USHORT);
			addToTree(tree, srcAccessors_indexViews + SRC_LABEL_COUNT, fCount * 3);

			std::string srcBufferChunks_indexBufferChunk = SRC_LABEL_BUFFER_CHUNKS + "." + indexBufferChunk + ".";
			size_t facesBufferLength = fCount * 3 * sizeof(*faceBuf.data()); //3 shorts for face index

			addToTree(tree, srcBufferChunks_indexBufferChunk + SRC_LABEL_BYTE_OFFSET, facesWritePosition);
			addToTree(tree, srcBufferChunks_indexBufferChunk + SRC_LABEL_BYTE_LENGTH, facesBufferLength);

			facesWritePosition += facesBufferLength;

			std::string srcBufferViews_indexBufferView = SRC_LABEL_BUFF_VIEWS + "." + indexBufferView + ".";

			std::vector<std::string> chunksArray = { indexBufferChunk };
			tree.add_child(srcBufferViews_indexBufferView + SRC_LABEL_CHUNKS, createPTArray(chunksArray));

			addToTree(tree, srcMesh_MeshID + SRC_LABEL_INDICIES, indexView);
			addToTree(tree, srcMesh_MeshID + SRC_LABEL_PRIMITIVE, SRC_X3DOM_TRIANGLE);

		}

		if (idMapBuf.size() && idMapBuf[subMeshIdx].size())
		{
			std::string srcAccessors_AttrViews_idMapAttributeView = srcAccessors_AttributeViews + "." + idMapAttributeView + ".";

			addToTree(tree, srcAccessors_AttrViews_idMapAttributeView + SRC_LABEL_BUFFVIEW, idMapBufferView);
			addToTree(tree, srcAccessors_AttrViews_idMapAttributeView + SRC_LABEL_BYTE_OFFSET, 0);
			addToTree(tree, srcAccessors_AttrViews_idMapAttributeView + SRC_LABEL_BYTE_STRIDE, 4);
			addToTree(tree, srcAccessors_AttrViews_idMapAttributeView + SRC_LABEL_COMP_TYPE, SRC_X3DOM_FLOAT);
			addToTree(tree, srcAccessors_AttrViews_idMapAttributeView + SRC_LABEL_TYPE, SRC_SCALAR);
			addToTree(tree, srcAccessors_AttrViews_idMapAttributeView + SRC_LABEL_COUNT, vCount);

			std::vector<uint32_t> offsetArr = { 0 };
			std::vector<uint32_t> scaleArr = { 1 };
			tree.add_child(srcAccessors_AttrViews_idMapAttributeView + SRC_LABEL_DECODE_OFFSET, createPTArray(offsetArr));
			tree.add_child(srcAccessors_AttrViews_idMapAttributeView + SRC_LABEL_DECODE_SCALE, createPTArray(scaleArr));

			std::string srcBufferChunks_idMapBufferChunks = SRC_LABEL_BUFFER_CHUNKS + "." + idMapBufferChunk + ".";
			size_t idMapBufferLength = idMapBuf[subMeshIdx].size() * sizeof(*idMapBuf[subMeshIdx].data());

			addToTree(tree, srcBufferChunks_idMapBufferChunks + SRC_LABEL_BYTE_OFFSET, idMapWritePosition);
			addToTree(tree, srcBufferChunks_idMapBufferChunks + SRC_LABEL_BYTE_LENGTH, idMapBufferLength);

			idMapWritePosition += idMapBufferLength;

			std::string srcBufferViews_idMapBufferView = SRC_LABEL_BUFF_VIEWS + "." + idMapBufferView + ".";

			std::vector<std::string> chunksArray = { idMapBufferChunk };
			tree.add_child(srcBufferViews_idMapBufferView + SRC_LABEL_CHUNKS, createPTArray(chunksArray));

			addToTree(tree, srcMesh_MeshID + SRC_LABEL_ATTRS + "." + SRC_LABEL_ID, idMapAttributeView);
		}

		if (uvs && uvs->size())
		{
			// UV coordinates
			std::string srcAccessors_AttrViews_uvAttrView = srcAccessors_AttributeViews + "." + uvAttributeView + ".";
			addToTree(tree, srcAccessors_AttrViews_uvAttrView + SRC_LABEL_BUFFVIEW, uvBufferView);
			addToTree(tree, srcAccessors_AttrViews_uvAttrView + SRC_LABEL_BYTE_OFFSET, 0);
			addToTree(tree, srcAccessors_AttrViews_uvAttrView + SRC_LABEL_BYTE_STRIDE, 8);
			addToTree(tree, srcAccessors_AttrViews_uvAttrView + SRC_LABEL_COMP_TYPE, SRC_X3DOM_FLOAT);
			addToTree(tree, srcAccessors_AttrViews_uvAttrView + SRC_LABEL_TYPE, SRC_VECTOR_2D);
			addToTree(tree, srcAccessors_AttrViews_uvAttrView + SRC_LABEL_COUNT, vCount);

			std::vector<uint32_t> offsetArr = { 0, 0 };
			std::vector<uint32_t> scaleArr = { 1, 1 };
			tree.add_child(srcAccessors_AttrViews_uvAttrView + SRC_LABEL_DECODE_OFFSET, createPTArray(offsetArr));
			tree.add_child(srcAccessors_AttrViews_uvAttrView + SRC_LABEL_DECODE_SCALE, createPTArray(scaleArr));

			std::string srcBufferChunks_uvBufferChunks = SRC_LABEL_BUFFER_CHUNKS + "." + uvBufferChunk + ".";
			size_t uvBufferLength = vCount * sizeof(*uvs->data());

			addToTree(tree, srcBufferChunks_uvBufferChunks + SRC_LABEL_BYTE_OFFSET, uvWritePosition);
			addToTree(tree, srcBufferChunks_uvBufferChunks + SRC_LABEL_BYTE_LENGTH, uvBufferLength);

			uvWritePosition += uvBufferLength;

			std::string srcBufferViews_uvBufferView = SRC_LABEL_BUFF_VIEWS + "." + uvBufferView + ".";

			std::vector<std::string> chunksArray = { uvBufferChunk };
			tree.add_child(srcBufferViews_uvBufferView + SRC_LABEL_CHUNKS, createPTArray(chunksArray));

			addToTree(tree, srcMesh_MeshID + SRC_LABEL_ATTRS + "." + SRC_LABEL_TEX_COORD, uvAttributeView);

		}

	}//for (size_t subMeshIdx = 0; subMeshIdx < nSubMeshes; ++subMeshIdx)

	repoTrace << "Generating output buffers for " << idx;

	std::vector<float> idMapBufFull;
	//create the full buffer
	for (const auto &subMesh : idMapBuf)
	{
		if (subMesh.size())
		{
			idMapBufFull.insert(idMapBufFull.end(), subMesh.begin(), subMesh.end());
		}
	}
	

	size_t bufferSize = (vertices->size() ? vertices->size() * sizeof(*vertices->data()) : 0)
		+ (normals->size() ? normals->size() * sizeof(*normals->data()) : 0)
		+ (faceBuf.size() ? faceBuf.size() * sizeof(*faceBuf.data()) : 0)
		+ (idMapBufFull.size() ? idMapBufFull.size() * sizeof(*idMapBufFull.data()) : 0)
		+ (uvs && uvs->size() ? uvs->size() *sizeof(*uvs->data()) : 0);

	std::vector<uint8_t> dataBuffer;
	dataBuffer.resize(bufferSize);

	size_t bufferPtr = 0;
	repoTrace << "Writing to buffer... expected Size is : " << bufferSize;
	// Output vertices
	if (vertices->size())
	{
		size_t byteSize = vertices->size() * sizeof(*vertices->data());
		memcpy(&dataBuffer[bufferPtr], vertices->data(), byteSize);
		bufferPtr += byteSize;

		repoTrace << "Written Vertices: byte Size " << byteSize << " bufferPtr is " << bufferPtr;
	}

	// Output normals
	if (normals->size())
	{
		size_t byteSize = normals->size() * sizeof(*normals->data());
		memcpy(&dataBuffer[bufferPtr], normals->data(), byteSize);
		bufferPtr += byteSize;
		repoTrace << "Written normals: byte Size " << byteSize << " bufferPtr is " << bufferPtr;
	}

	// Output faces
	if (faceBuf.size())
	{
		size_t byteSize = faceBuf.size() * sizeof(*faceBuf.data());
		memcpy(&dataBuffer[bufferPtr], faceBuf.data(), byteSize);
		bufferPtr += byteSize;
		repoTrace << "Written faces: byte Size " << byteSize << " bufferPtr is " << bufferPtr;
	}

	if (idMapBufFull.size())
	{
		size_t byteSize = idMapBufFull.size() * sizeof(*idMapBufFull.data());
		memcpy(&dataBuffer[bufferPtr], idMapBufFull.data(), byteSize);
		bufferPtr += byteSize;
		repoTrace << "Written idMapBuff: byte Size " << byteSize << " bufferPtr is " << bufferPtr;
	}


	if (uvs && uvs->size()) {
		size_t byteSize = uvs->size() * sizeof(*uvs->data());
		memcpy(&dataBuffer[bufferPtr], uvs->data(), byteSize);
		bufferPtr += byteSize;
		repoTrace << "Written UVs: byte Size " << byteSize << " bufferPtr is " << bufferPtr;
	}


	if (normals)
		delete normals;
	if (vertices)
		delete vertices;
	if (uvs)
		delete uvs;

	std::string fname = meshId + fileExt;
	
	trees[fname] = tree;
	fullDataBuffer[fname] = dataBuffer;

}