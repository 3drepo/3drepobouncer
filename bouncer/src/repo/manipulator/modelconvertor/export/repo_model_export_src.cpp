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
		}
		else
		{
			gType = repo::core::model::RepoScene::GraphType::OPTIMIZED;
		}


		convertSuccess = generateTreeRepresentation();
	}
	else
	{
		repoError << "Unable to export to SRC : Stash graph does not exist";
	}

}

SRCModelExport::~SRCModelExport()
{
}

void SRCModelExport::convertMesh(
	const repo::core::model::MeshNode* mesh,
	const size_t &idx,
	const std::string &textureID
	)
{
	std::vector<repo_mesh_mapping_t> mapping =  mesh->getMeshMapping();
	// First sort the combined map in order of vertex ID
	std::sort(mapping.begin(), mapping.end(), 
		[](repo_mesh_mapping_t const& a, repo_mesh_mapping_t const& b) { return a.vertFrom < b.vertFrom; });

	uint32_t runningVertTotal = 0;
	uint32_t runningFaceTotal = 0;
	int32_t subMeshIndex = -1;
	uint32_t origIndexPtr = 0;
	bool startLargeMeshSplit = false;
	bool useIDMap = true; //only false if it
	uint32_t runningIdx = 0;

	std::vector<std::vector<std::string>> subMeshKeys;
	std::vector<std::vector<repo_vector_t>> subMeshBBoxCenters;
	std::vector<std::vector<repo_vector_t>> subMeshBBoxSizes;
	std::vector<repo_src_mesh_info> subMeshArray;


	std::vector<repo_face_t>     *faces    = mesh->getFaces();
	std::vector<repo_vector_t>   *vertices = mesh->getVertices();
	std::vector<repo_vector2d_t> *uvs      = mesh->getUVChannels();


	std::vector<uint16_t> faceBuf;
	faceBuf.reserve(faces->size() * 3); //triangulated faces

	boost::property_tree::ptree tree;

	std::string fname;
	//TODO: not mpc if textured
	if (textureID.empty())
	{ 
		if (mapping.size() > 1)
			fname = UUIDtoString(mesh->getUniqueID()) + ".src.mpc";
		else
			fname = UUIDtoString(mesh->getUniqueID()) + ".src";
		
	}
	else
	{
		fname = UUIDtoString(mesh->getUniqueID()) + ".src?tex_uuid=" + textureID;
	}

	repoTrace << "#mappings = " << mapping.size();
	for (size_t i = 0; i < mapping.size(); ++i)
	{
		//For each submesh...
		uint32_t  currentMeshVFrom = mapping[i].vertFrom;
		uint32_t  currentMeshVTo = mapping[i].vertTo;
		uint32_t  currentMeshTFrom = mapping[i].triFrom;
		uint32_t  currentMeshTTo = mapping[i].triTo;

		uint32_t currentMeshNumVertices = currentMeshVTo - currentMeshVFrom;
		uint32_t currentMeshNumFaces = currentMeshTTo - currentMeshTFrom;
	
		//If the max. number of vertices exceeds the limit or it is the first sub mesh
		if ((runningVertTotal + currentMeshNumVertices) > SRC_MAX_VERTEX_LIMIT || subMeshIndex == -1)
		{
			//If (runningVertTotal + subMeshNumVertices) > SRC_MAX_VERTEX_LIMIT
			if ((subMeshIndex != -1) && !startLargeMeshSplit)
			{
				subMeshArray[subMeshIndex].offset = 0;
				subMeshArray[subMeshIndex].vCount = runningVertTotal;
				subMeshArray[subMeshIndex].fCount = runningFaceTotal;

			}

			startLargeMeshSplit = false;
			++subMeshIndex;
			subMeshArray.push_back(repo_src_mesh_info());
			subMeshKeys.push_back(std::vector<std::string>());
			subMeshBBoxCenters.push_back(std::vector<repo_vector_t>());
			subMeshBBoxSizes.push_back(std::vector<repo_vector_t>());



			subMeshArray[subMeshIndex].meshId = UUIDtoString(mesh->getUniqueID()) + "_" + std::to_string(subMeshIndex);
			subMeshArray[subMeshIndex].vFrom = currentMeshVFrom;
			subMeshArray[subMeshIndex].fFrom = currentMeshTFrom;

			// Reset runnning values
			runningVertTotal = 0;
			runningFaceTotal = 0;

		}


		// Now we've started a new mesh is the mesh that we're trying to add greater than
		// the limit itself. In the case that it is, this will always flag as above.
		if (currentMeshNumVertices > SRC_MAX_VERTEX_LIMIT)
		{
			repoTrace << "Splitting large meshes into smaller meshes";

			// Perform quick and dirty splitting algorithm
			std::unordered_map<uint32_t, uint32_t> reIndexMap;
			std::vector<repo_vector_t> splitBBox;
			std::vector<repo_vector_t> newVertexBuffer;
			newVertexBuffer.reserve(currentMeshNumVertices);


			for (uint32_t faceIdx = 0; faceIdx < currentMeshNumFaces; ++faceIdx) {
				uint32_t nComp = faces->at(origIndexPtr).size();
				if (nComp != 3)
				{
					repoError << "Non triangulated face with " << nComp << " vertices.";
					--runningFaceTotal;
					--subMeshArray[subMeshIndex].fTo;
				}
				else if (runningVertTotal + nComp > SRC_MAX_VERTEX_LIMIT || !startLargeMeshSplit)
				{
					// If new meshes has at least one in, then we are updating
					// an old one
					if (startLargeMeshSplit) {
						subMeshArray[subMeshIndex].vTo = mapping[i].vertFrom;
						subMeshArray[subMeshIndex].fTo = mapping[i].triFrom + faceIdx;

						subMeshArray[subMeshIndex].fCount = mapping[i].triTo - mapping[i].triFrom;
						subMeshArray[subMeshIndex].vCount = runningVertTotal;

						subMeshArray[subMeshIndex].bbox[0] = splitBBox[0];
						subMeshArray[subMeshIndex].bbox[1] = splitBBox[1];

						if (useIDMap) {
							subMeshArray[subMeshIndex].idMapBuf.reserve(runningVertTotal);

							for (uint32_t k = 0; k < runningVertTotal; k++) {
								float runningIdx_f = runningIdx;
								subMeshArray[subMeshIndex].idMapBuf.push_back(runningIdx_f);
							}
						}

						++runningIdx;

						
						auto bboxMin = splitBBox[0];
						auto bboxMax = splitBBox[1];
						repo_vector_t bboxCenter = { (bboxMin.x + bboxMax.x) / 2, (bboxMin.y + bboxMax.y) / 2, (bboxMin.z + bboxMax.z) / 2 };
						repo_vector_t bboxSize = { (bboxMax.x - bboxMin.x), (bboxMax.y - bboxMin.y), (bboxMax.z - bboxMin.z) };
						++subMeshIndex;


						subMeshBBoxCenters.push_back({ bboxCenter });
						subMeshBBoxSizes.push_back({ bboxSize });

						subMeshKeys.push_back(std::vector<std::string>());
						subMeshArray.push_back(repo_src_mesh_info());


						reIndexMap.clear();
					}

					subMeshArray[subMeshIndex].meshId = UUIDtoString(mesh->getUniqueID()) + "_" + std::to_string(subMeshIndex);
					subMeshKeys[subMeshIndex].push_back(subMeshArray[subMeshIndex].meshId);
					
					currentMeshVFrom                += runningVertTotal;
					subMeshArray[subMeshIndex].vFrom = currentMeshVFrom;
					subMeshArray[subMeshIndex].fFrom = currentMeshTFrom + faceIdx;
					runningVertTotal = 0;
					repoInfo << " running Vertice total reset!!!!!!!!";
					startLargeMeshSplit = true;
				}

				for (uint32_t compIdx = 0; compIdx < nComp; ++compIdx)
				{
					uint32_t indexVal = faces->at(origIndexPtr)[compIdx];

					if (reIndexMap.find(indexVal) == reIndexMap.end())
					{
						repoDebug << "New index : " << indexVal << " will be mapped to " << runningVertTotal;
						reIndexMap[indexVal] = runningVertTotal;
						faceBuf.push_back(runningVertTotal);

						//Update Bounding box
						repo_vector_t vertex = vertices->at(indexVal);
						if (splitBBox.size() < 2)
						{
							//no bounding box yet, initialise
							//both min and max to be this vertex
							splitBBox.push_back(vertex);
							splitBBox.push_back(vertex);
							
						}
						else
						{
							//FIXME: 0 is max and 1 is min?!
							if (splitBBox[0].x < vertex.x)
								splitBBox[0].x = vertex.x;

							if (splitBBox[0].y < vertex.y)
								splitBBox[0].y = vertex.y;

							if (splitBBox[0].z < vertex.z)
								splitBBox[0].z = vertex.z;

							if (splitBBox[1].x > vertex.x)
								splitBBox[1].x = vertex.x;

							if (splitBBox[1].y > vertex.y)
								splitBBox[1].y = vertex.y;

							if (splitBBox[1].z > vertex.z)
								splitBBox[1].z = vertex.z;

						}
						++runningVertTotal;
						newVertexBuffer.push_back(vertex);


					}
					else
					{
						repoDebug << "Using existing index for idx " << indexVal << " : " << reIndexMap[indexVal];
						//FIXME: faceBuf.writeUInt16LE(reindexMap[idx_val]); - no offset?
						repoWarning << " //FIXME: faceBuf.writeUInt16LE(reindexMap[idx_val]);";
						faceBuf.push_back(reIndexMap[indexVal]);
						exit(0);
					}
					
				}

				++origIndexPtr;
			}

			//FIXME: - no idea what i'm doing really.
			//std::copy(newVertexBuffer.begin(), newVertexBuffer.end(), vertices->begin() + mapping[i].vertFrom);


			subMeshArray[subMeshIndex].vTo = currentMeshVFrom;
			subMeshArray[subMeshIndex].fTo = currentMeshTFrom + currentMeshNumFaces;

			subMeshArray[subMeshIndex].fCount = subMeshArray[subMeshIndex].fTo - subMeshArray[subMeshIndex].fFrom;
			subMeshArray[subMeshIndex].vCount = runningVertTotal;
			subMeshArray[subMeshIndex].bbox[0] = splitBBox[0];
			subMeshArray[subMeshIndex].bbox[1] = splitBBox[1];

			if (useIDMap)
			{
				subMeshArray[subMeshIndex].idMapBuf.resize(runningVertTotal);
				float runningIdx_f = runningIdx;
				std::fill(subMeshArray[subMeshIndex].idMapBuf.begin(), subMeshArray[subMeshIndex].idMapBuf.end(), runningIdx_f);

				++runningIdx;
			}

			runningVertTotal = 0;
			repoInfo << " running Vertice total reset!!!!!!!!";

		}
		else //currentMeshNumVertices > SRC_MAX_VERTEX_LIMIT
		{
			//FIXME: duplicated code
			for (uint32_t faceIdx = 0; faceIdx < currentMeshNumFaces; ++faceIdx)
			{
				uint32_t nComp = faces->at(origIndexPtr).size();

				if (nComp != 3)
				{
					repoError << "Non triangulated face with " << nComp << " vertices.";
				}
				else
				{
					for (uint32_t compIdx = 0; compIdx < nComp; ++compIdx)
					{
						uint32_t indexVal = faces->at(origIndexPtr)[compIdx];
						// Take currentMeshVFrom from Index Value to reset to zero start,
						// then add back in the current running total to append after
						// pervious mesh.

						indexVal += (runningVertTotal - currentMeshVFrom);
						faceBuf.push_back(indexVal);
					}
				}

				++origIndexPtr;
			}//for (uint32_t faceIdx = 0; faceIdx < currentMeshNumFaces; ++faceIdx)

			subMeshKeys[subMeshIndex].push_back(UUIDtoString(mapping[i].mesh_id));

			subMeshArray[subMeshIndex].vTo = currentMeshVTo;
			subMeshArray[subMeshIndex].fTo = currentMeshTTo;

			std::vector<repo_vector_t> bbox = mesh->getBoundingBox();
			repo_vector_t bboxCenter, bboxSize;
			if (bbox.size() >= 2)
			{
				repo_vector_t bboxMin = bbox[0];
				repo_vector_t bboxMax = bbox[1];

				bboxCenter.x = (bboxMin.x + bboxMax.x) / 2;
				bboxCenter.y = (bboxMin.y + bboxMax.y) / 2;
				bboxCenter.z = (bboxMin.z + bboxMax.z) / 2;

				bboxSize.x = (bboxMax.x - bboxMin.x);
				bboxSize.y = (bboxMax.y - bboxMin.y);
				bboxSize.z = (bboxMax.z - bboxMin.z);

			}

			subMeshBBoxCenters[subMeshIndex].push_back(bboxCenter);
			subMeshBBoxSizes[subMeshIndex].push_back(bboxSize);

			if (useIDMap)
			{
				uint32_t idMapLength = subMeshArray[subMeshIndex].idMapBuf.size();

				subMeshArray[subMeshIndex].idMapBuf.resize(idMapLength + currentMeshNumVertices);

				float runningIdx_f = runningIdx;
				std::fill(subMeshArray[subMeshIndex].idMapBuf.begin() + idMapLength, subMeshArray[subMeshIndex].idMapBuf.end(), runningIdx_f);

				++runningIdx;

			}
			
		}//else //currentMeshNumVertices > SRC_MAX_VERTEX_LIMIT

		runningVertTotal += currentMeshNumVertices;
		runningFaceTotal += currentMeshNumFaces;
	}//for (size_t i = 0; i < mapping.size(); ++i)


	if (!startLargeMeshSplit)
	{
		subMeshArray[subMeshIndex].offset = 0;
		subMeshArray[subMeshIndex].vCount = runningVertTotal;
		subMeshArray[subMeshIndex].fCount = runningFaceTotal;
		
	}

	size_t bufPos = 0; //In bytes
	size_t vertexWritePosition = bufPos;
	
	bufPos += vertices->size()*sizeof(*vertices->data());

	size_t normalWritePosition = bufPos;
	auto normals = mesh->getNormals();
	bufPos += normals->size()*sizeof(*normals->data());

	size_t facesWritePosition = bufPos;
	bufPos += faceBuf.size() * sizeof(*faceBuf.data());

	size_t idMapWritePosition = bufPos;
	bufPos += vertices->size() * sizeof(float); //idMap array is of floats

	size_t uvWritePosition = bufPos;
	size_t nSubMeshes = subMeshArray.size();

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
		std::string normalAttributeView   = SRC_PREFIX_NORMAL_ATTR_VIEW   + meshIDX;
		std::string uvAttributeView       = SRC_PREFIX_UV_ATTR_VIEW       + meshIDX;
		std::string idMapAttributeView    = SRC_PREFIX_IDMAP_ATTR_VIEW    + meshIDX;
		std::string indexView             = SRC_PREFIX_IDX_VIEW           + meshIDX;

		std::string positionBufferView    = SRC_PREFIX_POSITION_BUFF_VIEW + meshIDX;
		std::string normalBufferView      = SRC_PREFIX_NORMAL_BUFF_VIEW   + meshIDX;
		std::string texBufferView         = SRC_PREFIX_TEX_BUFF_VIEW      + meshIDX;
		std::string uvBufferView          = SRC_PREFIX_UV_BUFF_VIEW       + meshIDX;
		std::string indexBufferView       = SRC_PREFIX_IDX_BUFF_VIEW      + meshIDX;
		std::string idMapBufferView       = SRC_PREFIX_IDMAP_BUFF_VIEW    + meshIDX;

		std::string positionBufferChunk   = SRC_PREFIX_POSITION_BUFF_CHK  + meshIDX;
		std::string indexBufferChunk      = SRC_PREFIX_IDX_BUFF_CHK       + meshIDX;
		std::string normalBufferChunk     = SRC_PREFIX_NORMAL_BUFF_CHK    + meshIDX;
		std::string texBufferChunk        = SRC_PREFIX_TEX_BUFF_CHK       + meshIDX;
		std::string uvBufferChunk         = SRC_PREFIX_UV_BUFF_CHK        + meshIDX;
		std::string idMapBufferChunk      = SRC_PREFIX_IDMAP_BUFF_CHK     + meshIDX;

		std::string idMapID               = SRC_PREFIX_IDMAP              + meshIDX;

		std::string meshID = subMeshArray[subMeshIdx].meshId;

		std::string srcAccessors_AttributeViews = SRC_LABEL_ACCESSORS + "." + SRC_LABEL_ACCESSORS_ATTR_VIEWS;
		std::string srcMesh_MeshID = SRC_LABEL_MESHES + "." + meshID + ".";


		// SRC Header for this mesh
		if (vertices->size())
		{		
			std::string srcAccessors_AttrViews_positionAttrView = srcAccessors_AttributeViews + "." + positionAttributeView + ".";
			addToTree(tree, srcAccessors_AttrViews_positionAttrView + SRC_LABEL_BUFFVIEW     , positionBufferView);
			addToTree(tree, srcAccessors_AttrViews_positionAttrView + SRC_LABEL_BYTE_OFFSET  , 0);
			addToTree(tree, srcAccessors_AttrViews_positionAttrView + SRC_LABEL_BYTE_STRIDE  , 12);
			addToTree(tree, srcAccessors_AttrViews_positionAttrView + SRC_LABEL_COMP_TYPE    , SRC_X3DOM_FLOAT);
			addToTree(tree, srcAccessors_AttrViews_positionAttrView + SRC_LABEL_TYPE         , SRC_VECTOR_3D);
			addToTree(tree, srcAccessors_AttrViews_positionAttrView + SRC_LABEL_COUNT        , subMeshArray[subMeshIdx].vCount);
			
			std::vector<uint32_t> offsetArr = { 0, 0, 0 };
			std::vector<uint32_t> scaleArr  = { 1, 1, 1 };
			tree.add_child(srcAccessors_AttrViews_positionAttrView + SRC_LABEL_DECODE_OFFSET, createPTArray(offsetArr));
			tree.add_child(srcAccessors_AttrViews_positionAttrView + SRC_LABEL_DECODE_SCALE, createPTArray(scaleArr));
			
			
			std::string srcBufferChunks_positionBufferChunks = SRC_LABEL_BUFFER_CHUNKS + "." + positionBufferChunk + ".";
			size_t verticeBufferLength = subMeshArray[subMeshIdx].vCount * 3 * 4;

			addToTree(tree, srcBufferChunks_positionBufferChunks + SRC_LABEL_BYTE_OFFSET, vertexWritePosition);
			addToTree(tree, srcBufferChunks_positionBufferChunks + SRC_LABEL_BYTE_LENGTH, verticeBufferLength); // 3 floats to a vertex

			vertexWritePosition += verticeBufferLength;

			std::string srcBufferViews_positionBufferView = SRC_LABEL_BUFF_VIEWS + "." + positionBufferView + ".";

			std::vector<std::string> chunksArray =  { positionBufferChunk } ;
			tree.add_child(srcBufferViews_positionBufferView + SRC_LABEL_CHUNKS, createPTArray(chunksArray));

			addToTree(tree, srcMesh_MeshID + SRC_LABEL_ATTRS + "." + SRC_LABEL_POSITION, positionAttributeView);
		}

		//Normal Attribute View
		if (normals->size())
		{
			std::string srcAccessors_AttrViews_normalAttrView = srcAccessors_AttributeViews + "." + normalAttributeView + ".";
			addToTree(tree, srcAccessors_AttrViews_normalAttrView + SRC_LABEL_BUFFVIEW   , normalBufferView);
			addToTree(tree, srcAccessors_AttrViews_normalAttrView + SRC_LABEL_BYTE_OFFSET, 0);
			addToTree(tree, srcAccessors_AttrViews_normalAttrView + SRC_LABEL_BYTE_STRIDE, 12);
			addToTree(tree, srcAccessors_AttrViews_normalAttrView + SRC_LABEL_COMP_TYPE  , SRC_X3DOM_FLOAT);
			addToTree(tree, srcAccessors_AttrViews_normalAttrView + SRC_LABEL_TYPE       , SRC_VECTOR_3D);
			addToTree(tree, srcAccessors_AttrViews_normalAttrView + SRC_LABEL_COUNT      , subMeshArray[subMeshIdx].vCount);

			std::vector<uint32_t> offsetArr = { 0, 0, 0 };
			std::vector<uint32_t> scaleArr = { 1, 1, 1 };
			tree.add_child(srcAccessors_AttrViews_normalAttrView + SRC_LABEL_DECODE_OFFSET, createPTArray(offsetArr));
			tree.add_child(srcAccessors_AttrViews_normalAttrView + SRC_LABEL_DECODE_SCALE , createPTArray(scaleArr));


			std::string srcBufferChunks_positionBufferChunks = SRC_LABEL_BUFFER_CHUNKS + "." + normalBufferChunk + ".";
			size_t verticeBufferLength = subMeshArray[subMeshIdx].vCount * 3 * 4;

			addToTree(tree, srcBufferChunks_positionBufferChunks + SRC_LABEL_BYTE_OFFSET, normalWritePosition);
			addToTree(tree, srcBufferChunks_positionBufferChunks + SRC_LABEL_BYTE_LENGTH, verticeBufferLength); // 3 floats to a vertex

			normalWritePosition += verticeBufferLength;


			std::string srcBufferViews_normalBufferView = SRC_LABEL_BUFF_VIEWS + "." + normalBufferView + ".";

			std::vector<std::string> chunksArray =  { normalBufferChunk } ;
			tree.add_child(srcBufferViews_normalBufferView + SRC_LABEL_CHUNKS, createPTArray(chunksArray));

			addToTree(tree, srcMesh_MeshID + SRC_LABEL_ATTRS + "." + SRC_LABEL_NORMAL, normalAttributeView);

		}

		// Index View
		if (faces->size())
		{
			std::string srcAccessors_indexViews = SRC_LABEL_ACCESSORS + "." + SRC_LABEL_INDEX_VIEWS + "." + indexView + ".";

			addToTree(tree, srcAccessors_indexViews + SRC_LABEL_BUFFVIEW   , indexBufferView);
			addToTree(tree, srcAccessors_indexViews + SRC_LABEL_BYTE_OFFSET, 0);
			addToTree(tree, srcAccessors_indexViews + SRC_LABEL_COMP_TYPE  , SRC_X3DOM_USHORT);
			addToTree(tree, srcAccessors_indexViews + SRC_LABEL_COUNT      , subMeshArray[subMeshIdx].fCount * 3);

			std::string srcBufferChunks_indexBufferChunk = SRC_LABEL_BUFFER_CHUNKS + "." + indexBufferChunk + ".";
			size_t facesBufferLength = subMeshArray[subMeshIdx].fCount * 3 * 2; //3 shorts for face index

			addToTree(tree, srcBufferChunks_indexBufferChunk + SRC_LABEL_BYTE_OFFSET, facesWritePosition);
			addToTree(tree, srcBufferChunks_indexBufferChunk + SRC_LABEL_BYTE_LENGTH, facesBufferLength);

			facesWritePosition += facesBufferLength;

			std::string srcBufferViews_indexBufferView = SRC_LABEL_BUFF_VIEWS + "." + indexBufferView + ".";

			std::vector<std::string> chunksArray = { indexBufferChunk };
			tree.add_child(srcBufferViews_indexBufferView + SRC_LABEL_CHUNKS, createPTArray(chunksArray));

			addToTree(tree, srcMesh_MeshID + SRC_LABEL_INDICIES , indexView);
			addToTree(tree, srcMesh_MeshID + SRC_LABEL_PRIMITIVE, SRC_X3DOM_TRIANGLE);

		}

		if (useIDMap && subMeshArray[subMeshIdx].idMapBuf.size())
		{
			std::string srcAccessors_AttrViews_idMapAttributeView = srcAccessors_AttributeViews + "." + idMapAttributeView + ".";
			
			addToTree(tree, srcAccessors_AttrViews_idMapAttributeView + SRC_LABEL_BUFFVIEW, idMapBufferView);
			addToTree(tree, srcAccessors_AttrViews_idMapAttributeView + SRC_LABEL_BYTE_OFFSET, 0);
			addToTree(tree, srcAccessors_AttrViews_idMapAttributeView + SRC_LABEL_BYTE_STRIDE, 4);
			addToTree(tree, srcAccessors_AttrViews_idMapAttributeView + SRC_LABEL_COMP_TYPE, SRC_X3DOM_FLOAT);
			addToTree(tree, srcAccessors_AttrViews_idMapAttributeView + SRC_LABEL_TYPE, SRC_SCALAR);
			addToTree(tree, srcAccessors_AttrViews_idMapAttributeView + SRC_LABEL_COUNT, subMeshArray[subMeshIdx].vCount);

			std::vector<uint32_t> offsetArr = { 0};
			std::vector<uint32_t> scaleArr = { 1};
			tree.add_child(srcAccessors_AttrViews_idMapAttributeView + SRC_LABEL_DECODE_OFFSET, createPTArray(offsetArr));
			tree.add_child(srcAccessors_AttrViews_idMapAttributeView + SRC_LABEL_DECODE_SCALE , createPTArray(scaleArr));

			std::string srcBufferChunks_idMapBufferChunks = SRC_LABEL_BUFFER_CHUNKS + "." + idMapBufferChunk + ".";
			size_t idMapBufferLength = subMeshArray[subMeshIdx].idMapBuf.size() * sizeof(*subMeshArray[subMeshIdx].idMapBuf.data());

			addToTree(tree, srcBufferChunks_idMapBufferChunks + SRC_LABEL_BYTE_OFFSET, idMapWritePosition);
			addToTree(tree, srcBufferChunks_idMapBufferChunks + SRC_LABEL_BYTE_LENGTH, idMapBufferLength); 

			idMapWritePosition += idMapBufferLength;

			std::string srcBufferViews_idMapBufferView = SRC_LABEL_BUFF_VIEWS + "." + idMapBufferView + ".";

			std::vector<std::string> chunksArray = { idMapBufferChunk};
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
			addToTree(tree, srcAccessors_AttrViews_uvAttrView + SRC_LABEL_COUNT, subMeshArray[subMeshIdx].vCount);

			std::vector<uint32_t> offsetArr = { 0, 0 };
			std::vector<uint32_t> scaleArr = { 1, 1};
			tree.add_child(srcAccessors_AttrViews_uvAttrView + SRC_LABEL_DECODE_OFFSET, createPTArray(offsetArr));
			tree.add_child(srcAccessors_AttrViews_uvAttrView + SRC_LABEL_DECODE_SCALE, createPTArray(scaleArr));

			std::string srcBufferChunks_uvBufferChunks = SRC_LABEL_BUFFER_CHUNKS + "." + uvBufferChunk + ".";
			size_t uvBufferLength = subMeshArray[subMeshIdx].vCount * 2 * 4;

			addToTree(tree, srcBufferChunks_uvBufferChunks + SRC_LABEL_BYTE_OFFSET, uvWritePosition);
			addToTree(tree, srcBufferChunks_uvBufferChunks + SRC_LABEL_BYTE_LENGTH, uvBufferLength); 

			uvWritePosition += uvBufferLength;

			std::string srcBufferViews_uvBufferView = SRC_LABEL_BUFF_VIEWS + "." + uvBufferView + ".";

			std::vector<std::string> chunksArray = { uvBufferChunk };
			tree.add_child(srcBufferViews_uvBufferView + SRC_LABEL_CHUNKS, createPTArray(chunksArray));

			addToTree(tree, srcMesh_MeshID + SRC_LABEL_ATTRS + "." + SRC_LABEL_TEX_COORD, uvAttributeView);

		}

	}//for (size_t subMeshIdx = 0; subMeshIdx < nSubMeshes; ++subMeshIdx)

	repoTrace << "Generating output buffers";

	std::vector<float> idMapBuf;
	//create the full buffer
	if (useIDMap)
	{
		for (const auto &subMesh : subMeshArray)
		{
			if (subMesh.idMapBuf.size())
			{
				idMapBuf.insert(idMapBuf.end(), subMesh.idMapBuf.begin(), subMesh.idMapBuf.end());
			}
		}
	}

	size_t bufferSize = (vertices->size() ? vertices->size() * 3 * 4 : 0)
						+ (normals->size() ? normals->size() * 3 * 4 : 0)
						+ (faces->size() ? faces->size() * 3 * 2 : 0)
						+ ((useIDMap && idMapBuf.size()) ? idMapBuf.size() * sizeof(*idMapBuf.data()) : 0)
						+ (uvs && uvs->size() ? (uvs->size() * 4 * 2) : 0);

	std::vector<uint8_t> dataBuffer;
	dataBuffer.resize(bufferSize);

	size_t bufferPtr = 0;
	repoTrace << "Writing to buffer... expected Size is : " << bufferSize;
	// Output vertices
	if (vertices->size())
	{
		repoTrace << "Vertices #: " << vertices->size();
		size_t byteSize = vertices->size() * sizeof(*vertices->data());
		memcpy(&dataBuffer[bufferPtr], vertices->data(), byteSize);
		bufferPtr += byteSize;

		repoTrace << "Written Vertices: byte Size " << byteSize << " bufferPtr is " << bufferPtr;
	}

	// Output normals
	if (normals->size())
	{
		repoTrace << "Normals #: " << normals->size();
		size_t byteSize = normals->size() * sizeof(*normals->data());
		memcpy(&dataBuffer[bufferPtr], normals->data(), byteSize);
		bufferPtr += byteSize;
		repoTrace << "Written normals: byte Size " << byteSize << " bufferPtr is " << bufferPtr;
	}

	// Output faces
	if (faceBuf.size())
	{
		repoTrace << "Faces #: " << faceBuf.size();
		size_t byteSize = faceBuf.size() * sizeof(*faceBuf.data());
		memcpy(&dataBuffer[bufferPtr], faceBuf.data(), byteSize);
		bufferPtr += byteSize;
		repoTrace << "Written faces: byte Size " << byteSize << " bufferPtr is " << bufferPtr;
	}

	if (useIDMap && idMapBuf.size())
	{
		repoTrace << "idMap #: " << idMapBuf.size();
		size_t byteSize = idMapBuf.size() * sizeof(*idMapBuf.data());
		memcpy(&dataBuffer[bufferPtr], idMapBuf.data(), byteSize);
		bufferPtr += byteSize;
		repoTrace << "Written idMapBuff: byte Size " << byteSize << " bufferPtr is " << bufferPtr;
	}

	
	if (uvs && uvs->size()) {
		repoTrace << "UV #: " << uvs->size();
		size_t byteSize = uvs->size() * sizeof(*uvs->data());
		memcpy(&dataBuffer[bufferPtr], uvs->data(), byteSize);
		bufferPtr += byteSize;
		repoTrace << "Written UVs: byte Size " << byteSize << " bufferPtr is " << bufferPtr;
	}


	if (normals)
		delete normals;
	if (faces)
		delete faces;
	if (vertices)
		delete vertices;
	if (uvs)
		delete uvs;

	trees[fname] = tree;
	fullDataBuffer[fname] = dataBuffer;

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

		buffer.resize(headerSize);

		size_t buffPtr = 0; //BufferPointer in bytes
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
			convertMesh((repo::core::model::MeshNode*)mesh, index++, scene->getTextureIDForMesh(gType, mesh->getSharedID()));

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