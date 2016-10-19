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
* Allows Export functionality from 3D Repo World to glTF
*/

#include "repo_model_export_gltf.h"

#include <cmath>

#include "../../../core/model/bson/repo_bson_factory.h"
#include "../../../lib/repo_log.h"
#include "../../modelutility/repo_mesh_map_reorganiser.h"
#include "../../modelutility/spatialpartitioning/repo_spatial_partitioner_rdtree.h"
#include "auxiliary/x3dom_constants.h"

using namespace repo::manipulator::modelconvertor;

const static int lodLimit = 15; //Hack to test pop buffers, i should not be committing this!

const static size_t GLTF_MAX_VERTEX_LIMIT = 65535;
//#define DEBUG //FIXME: to remove
//#define LODLIMIT

static const std::string GLTF_LABEL_ACCESSORS = "accessors";
static const std::string GLTF_LABEL_AMBIENT = "ambient";
static const std::string GLTF_LABEL_ASP_RATIO = "aspectRatio";
static const std::string GLTF_LABEL_ASSET = "asset";
static const std::string GLTF_LABEL_ATTRIBUTES = "attributes";
static const std::string GLTF_LABEL_BUFFER = "buffer";
static const std::string GLTF_LABEL_BUFFERS = "buffers";
static const std::string GLTF_LABEL_BUFFER_VIEW = "bufferView";
static const std::string GLTF_LABEL_BUFFER_VIEWS = "bufferViews";
static const std::string GLTF_LABEL_BYTE_LENGTH = "byteLength";
static const std::string GLTF_LABEL_BYTE_OFFSET = "byteOffset";
static const std::string GLTF_LABEL_BYTE_STRIDE = "byteStride";
static const std::string GLTF_LABEL_CAMERAS = "cameras";
static const std::string GLTF_LABEL_CHILDREN = "children";
static const std::string GLTF_LABEL_COMP_TYPE = "componentType";
static const std::string GLTF_LABEL_COUNT = "count";
static const std::string GLTF_LABEL_DIFFUSE = "diffuse";
static const std::string GLTF_LABEL_EMISSIVE = "emission";
static const std::string GLTF_LABEL_ENABLE = "enable";
static const std::string GLTF_LABEL_EXTRA = "extras";
static const std::string GLTF_LABEL_FAR_CP = "zfar";
static const std::string GLTF_LABEL_FILTER_MAG = "magFilter";
static const std::string GLTF_LABEL_FILTER_MIN = "minFilter";
static const std::string GLTF_LABEL_FORMAT = "format";
static const std::string GLTF_LABEL_FOV = "yfov";
static const std::string GLTF_LABEL_GENERATOR = "generator";
static const std::string GLTF_LABEL_IMAGES = "images";
static const std::string GLTF_LABEL_INDICES = "indices";
static const std::string GLTF_LABEL_INTERNAL_FORMAT = "internalFormat";
static const std::string GLTF_LABEL_MATERIAL = "material";
static const std::string GLTF_LABEL_MATERIALS = "materials";
static const std::string GLTF_LABEL_MATRIX = "matrix";
static const std::string GLTF_LABEL_MAX = "max";
static const std::string GLTF_LABEL_MESHES = "meshes";
static const std::string GLTF_LABEL_MIN = "min";
static const std::string GLTF_LABEL_NAME = "name";
static const std::string GLTF_LABEL_NEAR_CP = "znear";
static const std::string GLTF_LABEL_NODES = "nodes";
static const std::string GLTF_LABEL_NORMAL = "NORMAL";
static const std::string GLTF_LABEL_PARAMETERS = "parameters";
static const std::string GLTF_LABEL_POSITION = "POSITION";
static const std::string GLTF_LABEL_PRIMITIVE = "mode";
static const std::string GLTF_LABEL_PRIMITIVES = "primitives";
static const std::string GLTF_LABEL_PROGRAM = "program";
static const std::string GLTF_LABEL_PROGRAMS = "programs";
static const std::string GLTF_LABEL_SAMPLER = "sampler";
static const std::string GLTF_LABEL_SAMPLERS = "samplers";
static const std::string GLTF_LABEL_SCENE = "scene";
static const std::string GLTF_LABEL_SCENES = "scenes";
static const std::string GLTF_LABEL_SEMANTIC = "semantic";
static const std::string GLTF_LABEL_SHADERS = "shaders";
static const std::string GLTF_LABEL_SHADER_FRAG = "fragmentShader";
static const std::string GLTF_LABEL_SHADER_VERT = "vertexShader";
static const std::string GLTF_LABEL_SHININESS = "shininess";
static const std::string GLTF_LABEL_SOURCE = "source";
static const std::string GLTF_LABEL_SPECULAR = "specular";
static const std::string GLTF_LABEL_STATES = "states";
static const std::string GLTF_LABEL_TARGET = "target";
static const std::string GLTF_LABEL_TECHNIQUE = "technique";
static const std::string GLTF_LABEL_TECHNIQUES = "techniques";
static const std::string GLTF_LABEL_TEXCOORD = "TEXCOORD";
static const std::string GLTF_LABEL_TEXTURES = "textures";
static const std::string GLTF_LABEL_TRANSPARENCY = "transparency";
static const std::string GLTF_LABEL_TWO_SIDED = "twoSided";
static const std::string GLTF_LABEL_TYPE = "type";
static const std::string GLTF_LABEL_UNIFORMS = "uniforms";
static const std::string GLTF_LABEL_URI = "uri";
static const std::string GLTF_LABEL_VALUE = "value";
static const std::string GLTF_LABEL_VALUES = "values";
static const std::string GLTF_LABEL_VERSION = "version";
static const std::string GLTF_LABEL_WRAP_S = "wrapS";
static const std::string GLTF_LABEL_WRAP_T = "wrapT";

static const std::string GLTF_PREFIX_ACCESSORS = "acc";
static const std::string GLTF_PREFIX_BUFFERS = "buf";
static const std::string GLTF_PREFIX_BUFFER_VIEWS = "bufv";
static const std::string GLTF_PREFIX_TEXTURE = "img";

static const std::string GLTF_SUFFIX_FACES = "f";
static const std::string GLTF_SUFFIX_IDMAP = "id";
static const std::string GLTF_SUFFIX_NORMALS = "n";
static const std::string GLTF_SUFFIX_POSITION = "p";
static const std::string GLTF_SUFFIX_TEX_COORD = "uv";

static const uint32_t GLTF_PRIM_TYPE_TRIANGLE = 4;
static const uint32_t GLTF_PRIM_TYPE_ARRAY_BUFFER = 34962;
static const uint32_t GLTF_PRIM_TYPE_ELEMENT_ARRAY_BUFFER = 34963;

static const uint32_t GLTF_COMP_TYPE_USHORT = 5123;
static const uint32_t GLTF_COMP_TYPE_FLOAT = 5126;
static const uint32_t GLTF_COMP_TYPE_FLOAT_VEC2 = 35664;
static const uint32_t GLTF_COMP_TYPE_FLOAT_VEC3 = 35665;
static const uint32_t GLTF_COMP_TYPE_FLOAT_VEC4 = 35666;
static const uint32_t GLTF_COMP_TYPE_FLOAT_MAT3 = 35675;
static const uint32_t GLTF_COMP_TYPE_FLOAT_MAT4 = 35676;

static const uint32_t GLTF_FILTER_TYPE_LINEAR = 9729;
static const uint32_t GLTF_FILTER_TYPE_NEAREST_MIPMAP_LINEAR = 9987;
static const uint32_t GLTF_WRAP_MODE_REPEAT = 10497;

static const uint32_t GLTF_FORMAT_RGBA = 6408;
static const uint32_t GLTF_TYPE_TEXTURE_2D = 3553;
static const uint32_t GLTF_TYPE_UNSIGNED_BYTES = 5121;

static const uint32_t GLTF_SHADER_TYPE_FRAGMENT = 35632;
static const uint32_t GLTF_SHADER_TYPE_VERTEX = 35633;

static const uint32_t GLTF_STATE_BLEND = 3042;
static const uint32_t GLTF_STATE_CULL_FACE = 2884;
static const uint32_t GLTF_STATE_DEPTH_TEST = 2929;
static const uint32_t GLTF_STATE_POLYGON_OFFSET_FILL = 32823;
static const uint32_t GLTF_STATE_SAMPLE_ALPHA_TO_COVERAGE = 32926;
static const uint32_t GLTF_STATE_SCISSOR_TEST = 3089;

static const std::string GLTF_ARRAY_BUFFER = "arraybuffer";
static const std::string GLTF_CAM_TYPE_PERSPECTIVE = "perspective";
static const std::string GLTF_TYPE_SCALAR = "SCALAR";
static const std::string GLTF_TYPE_VEC2 = "VEC2";
static const std::string GLTF_TYPE_VEC3 = "VEC3";

static const std::string GLTF_VERSION = "1.0";

static const std::string        REPO_GLTF_LABEL_IDMAP = "IDMAP";

//Default shader properties
static const std::string        REPO_GLTF_DEFAULT_PROGRAM = "default_program";
static const std::string        REPO_GLTF_DEFAULT_SAMPLER = "default_sampler";
static const std::string        REPO_GLTF_DEFAULT_SHADER_FRAG = "default_fshader";
#ifdef DEBUG
static const std::string        REPO_GLTF_DEFAULT_SHADER_FRAG_URI = "fragShader.glsl";
static const std::string        REPO_GLTF_DEFAULT_X3DSHADER_FRAG_URI = "x3domFragShader.glsl";
#else
static const std::string        REPO_GLTF_DEFAULT_SHADER_FRAG_URI = "/public/shader/gltfStockShaders/fragShader.glsl";
static const std::string        REPO_GLTF_DEFAULT_X3DSHADER_FRAG_URI = "/public/shader/gltfStockShaders/x3domFragShader.glsl";
#endif
static const std::string        REPO_GLTF_DEFAULT_SHADER_VERT = "default_vshader";
#ifdef DEBUG
static const std::string        REPO_GLTF_DEFAULT_SHADER_VERT_URI = "vertShader.glsl";
static const std::string        REPO_GLTF_DEFAULT_X3DSHADER_VERT_URI = "x3domVertShader.glsl";
#else
static const std::string        REPO_GLTF_DEFAULT_SHADER_VERT_URI = "/public/shader/gltfStockShaders/vertShader.glsl";
static const std::string        REPO_GLTF_DEFAULT_X3DSHADER_VERT_URI = "/public/shader/gltfStockShaders/x3domVertShader.glsl";
#endif
static const std::string        REPO_GLTF_DEFAULT_TECHNIQUE = "default_technique";
static const float              REPO_GLTF_DEFAULT_SHININESS = 50;
static const std::vector<float> REPO_GLTF_DEFAULT_SPECULAR = { 0, 0, 0, 0 };

static const std::string REPO_GLTF_LABEL_REF_ID = "refID";
static const std::string REPO_GLTF_LABEL_LOD = "lodRef";
static const std::string REPO_LABEL_X3D_MATERIAL = "x3dmaterial";

GLTFModelExport::GLTFModelExport(
	const repo::core::model::RepoScene *scene
	) : WebModelExport(scene)
{
	if (convertSuccess)
	{
		//We only need a GLTF representation if there are meshes or cameras
		if (scene->getAllMeshes(gType).size() || scene->getAllCameras(gType).size())
			convertSuccess = generateTreeRepresentation();
	}
	else
	{
		repoError << "Failed to export to glTF: null pointer to scene!";
	}
}

GLTFModelExport::~GLTFModelExport()
{
}

void GLTFModelExport::addAccessors(
	const std::string              &accName,
	const std::string              &buffViewName,
	repo::lib::PropertyTree        &tree,
	const std::vector<uint16_t>    &faces,
	const uint32_t                 &addrFrom,
	const uint32_t                 &addrTo,
	const std::string              &refId,
	const std::vector<uint16_t>    &lod,
	const size_t                   &offset)
{
	std::vector<float> min, max;
	uint32_t startFaceIdx = addrFrom * 3;
	uint32_t endFaceIdx = addrTo * 3;
	if (faces.size() >= startFaceIdx)
	{
		min.push_back(faces[startFaceIdx]);
		max.push_back(faces[startFaceIdx]);
	}
	for (size_t i = startFaceIdx + 1; i < endFaceIdx; ++i)
	{
		if (min[0] > faces[i]) min[0] = faces[i];
		if (max[0] < faces[i]) max[0] = faces[i];
	}
	addAccessors(accName, buffViewName, tree, endFaceIdx - startFaceIdx,
		(startFaceIdx - offset * 3) * sizeof(*faces.data()), 0, GLTF_COMP_TYPE_USHORT,
		GLTF_TYPE_SCALAR, min, max, refId, lod);
}

void GLTFModelExport::addAccessors(
	const std::string              &accName,
	const std::string              &buffViewName,
	repo::lib::PropertyTree        &tree,
	const std::vector<float>       &data,
	const uint32_t                 &addrFrom,
	const uint32_t                 &addrTo,
	const std::string              &refId,
	const size_t                   &offset)
{
	std::vector<float> min, max;
	size_t offsetIdx = addrFrom - offset;
	if (data.size() >= addrTo - offset)
	{
		//This is idmapbuff accessor, it's already offseted to the submesh
		min.push_back(data[offsetIdx]);
		max.push_back(data[offsetIdx]);
	}
	else
	{
		repoError << "Failed to add accessor for " << accName << " #data (" << data.size() << ") does not match it's range (" << addrFrom << "," << addrTo << " ," << offset << ", " << offsetIdx << ")!";
		return;
	}
	for (size_t i = offsetIdx + 1; i < addrTo - addrFrom; ++i)
	{
		if (min[0] > data[i]) min[0] = data[i];
		if (max[0] < data[i]) max[0] = data[i];
	}
	addAccessors(accName, buffViewName, tree, addrTo - addrFrom,
		(addrFrom - offset) * sizeof(*data.data()), sizeof(*data.data()), GLTF_COMP_TYPE_FLOAT,
		GLTF_TYPE_SCALAR, min, max, refId);
}

void GLTFModelExport::addAccessors(
	const std::string                  &accName,
	const std::string                  &buffViewName,
	repo::lib::PropertyTree            &tree,
	const std::vector<repo_vector2d_t> &buffer,
	const uint32_t                     &addrFrom,
	const uint32_t                     &addrTo,
	const std::string                  &refId,
	const size_t                       &offset)
{
	std::vector<float> min, max;
	//find maximum and minimum values

	if (buffer.size())
	{
		min.push_back(buffer[addrFrom].x);
		min.push_back(buffer[addrFrom].y);
		max.push_back(buffer[addrFrom].x);
		max.push_back(buffer[addrFrom].y);

		for (uint32_t i = addrFrom + 1; i < addrTo; ++i)
		{
			if (buffer[i].x < min[0])
				min[0] = buffer[i].x;
			if (buffer[i].x > max[0])
				max[0] = buffer[i].x;

			if (buffer[i].y < min[1])
				min[1] = buffer[i].y;
			if (buffer[i].y > max[1])
				max[1] = buffer[i].y;
		}
	}
	addAccessors(accName, buffViewName, tree, addrTo - addrFrom,
		(addrFrom - offset) * sizeof(*buffer.data()), sizeof(*buffer.data()),
		GLTF_COMP_TYPE_FLOAT, GLTF_TYPE_VEC2, min, max, refId);
}

void GLTFModelExport::addAccessors(
	const std::string                &accName,
	const std::string                &buffViewName,
	repo::lib::PropertyTree          &tree,
	const std::vector<repo_vector_t> &buffer,
	const uint32_t                   &addrFrom,
	const uint32_t                   &addrTo,
	const std::string                &refId,
	const size_t                     &offset)
{
	std::vector<float> min, max;
	//find maximum and minimum values
	if (buffer.size())
	{
		min.push_back(buffer[addrFrom].x);
		min.push_back(buffer[addrFrom].y);
		min.push_back(buffer[addrFrom].z);
		max.push_back(buffer[addrFrom].x);
		max.push_back(buffer[addrFrom].y);
		max.push_back(buffer[addrFrom].z);

		for (uint32_t i = addrFrom + 1; i < addrTo; ++i)
		{
			if (buffer[i].x < min[0])
				min[0] = buffer[i].x;
			if (buffer[i].x > max[0])
				max[0] = buffer[i].x;

			if (buffer[i].y < min[1])
				min[1] = buffer[i].y;
			if (buffer[i].y > max[1])
				max[1] = buffer[i].y;

			if (buffer[i].z < min[2])
				min[2] = buffer[i].z;
			if (buffer[i].z > max[2])
				max[2] = buffer[i].z;
		}
	}
	addAccessors(accName, buffViewName, tree, addrTo - addrFrom,
		(addrFrom - offset) * sizeof(*buffer.data()), sizeof(*buffer.data()),
		GLTF_COMP_TYPE_FLOAT, GLTF_TYPE_VEC3, min, max, refId);
}

void GLTFModelExport::addAccessors(
	const std::string              &accName,
	const std::string              &buffViewName,
	repo::lib::PropertyTree        &tree,
	const size_t                   &count,
	const size_t                   &offset,
	const size_t                   &stride,
	const uint32_t                 &componentType,
	const std::string              &bufferType,
	const std::vector<float>       &min,
	const std::vector<float>       &max,
	const std::string              &refId,
	const std::vector<uint16_t>    &lod)
{
	//declare accessor
	std::string accLabel = GLTF_LABEL_ACCESSORS + "." + GLTF_PREFIX_ACCESSORS + "_" + accName;
	tree.addToTree(accLabel + "." + GLTF_LABEL_BUFFER_VIEW, GLTF_PREFIX_BUFFER_VIEWS + "_" + buffViewName);
	tree.addToTree(accLabel + "." + GLTF_LABEL_BYTE_OFFSET, offset);
	tree.addToTree(accLabel + "." + GLTF_LABEL_BYTE_STRIDE, stride);
	tree.addToTree(accLabel + "." + GLTF_LABEL_COMP_TYPE, componentType);
	tree.addToTree(accLabel + "." + GLTF_LABEL_COUNT, count);
	if (min.size())
	{
		tree.addToTree(accLabel + "." + GLTF_LABEL_MIN, min);
	}
	if (max.size())
	{
		tree.addToTree(accLabel + "." + GLTF_LABEL_MAX, max);
	}
	tree.addToTree(accLabel + "." + GLTF_LABEL_TYPE, bufferType);

	if (!refId.empty())
	{
		tree.addToTree(accLabel + "." + GLTF_LABEL_EXTRA + "." + REPO_GLTF_LABEL_REF_ID, refId);
	}
	if (lod.size())
	{
		tree.addToTree(accLabel + "." + GLTF_LABEL_EXTRA + "." + REPO_GLTF_LABEL_LOD, lod);
	}
}

void GLTFModelExport::addBufferView(
	const std::string              &name,
	const std::string              &fileName,
	repo::lib::PropertyTree        &tree,
	const std::vector<uint16_t>    &buffer,
	const size_t                   &offset,
	const size_t                   &count,
	const std::string              &refId)
{
	addBufferView(name, fileName, tree, count * 3 * sizeof(*buffer.data()), offset, GLTF_PRIM_TYPE_ELEMENT_ARRAY_BUFFER, refId);
}

void GLTFModelExport::addBufferView(
	const std::string              &name,
	const std::string              &fileName,
	repo::lib::PropertyTree        &tree,
	const std::vector<float>       &buffer,
	const size_t                   &offset,
	const size_t                   &count,
	const std::string              &refId)
{
	addBufferView(name, fileName, tree, count * sizeof(*buffer.data()), offset, GLTF_PRIM_TYPE_ARRAY_BUFFER, refId);
}

void GLTFModelExport::addBufferView(
	const std::string                   &name,
	const std::string                   &fileName,
	repo::lib::PropertyTree             &tree,
	const std::vector<repo_vector_t>    &buffer,
	const size_t                        &offset,
	const size_t                        &count,
	const std::string                   &refId)
{
	addBufferView(name, fileName, tree, count * sizeof(*buffer.data()), offset, GLTF_PRIM_TYPE_ARRAY_BUFFER, refId);
}

void GLTFModelExport::addBufferView(
	const std::string                   &name,
	const std::string                   &fileName,
	repo::lib::PropertyTree             &tree,
	const std::vector<repo_vector2d_t>  &buffer,
	const size_t                        &offset,
	const size_t                        &count,
	const std::string                   &refId)
{
	addBufferView(name, fileName, tree, count * sizeof(*buffer.data()), offset, GLTF_PRIM_TYPE_ARRAY_BUFFER, refId);
}

void GLTFModelExport::addBufferView(
	const std::string              &name,
	const std::string              &fileName,
	repo::lib::PropertyTree        &tree,
	const size_t                   &byteLength,
	const size_t                   &offset,
	const uint32_t                 &bufferTarget,
	const std::string              &refId)
{
	std::string bufferViewName = GLTF_PREFIX_BUFFER_VIEWS + "_" + name;

	//declare buffer view
	std::string bufferViewLabel = GLTF_LABEL_BUFFER_VIEWS + "." + bufferViewName;
	tree.addToTree(bufferViewLabel + "." + GLTF_LABEL_BUFFER, fileName);
	tree.addToTree(bufferViewLabel + "." + GLTF_LABEL_BYTE_LENGTH, byteLength);
	tree.addToTree(bufferViewLabel + "." + GLTF_LABEL_BYTE_OFFSET, offset);
	tree.addToTree(bufferViewLabel + "." + GLTF_LABEL_TARGET, bufferTarget);

	if (!refId.empty())
	{
		tree.addToTree(bufferViewLabel + "." + GLTF_LABEL_EXTRA + "." + REPO_GLTF_LABEL_REF_ID, refId);
	}
}
size_t GLTFModelExport::addToDataBuffer(
	const std::string              &bufferName,
	const uint8_t                  *buffer,
	const size_t                   &byteLength
	)
{
	auto mapIt = fullDataBuffer.find(bufferName);

	if (mapIt == fullDataBuffer.end())
	{
		//current no such buffer, create one
		fullDataBuffer[bufferName] = std::vector<uint8_t>();
	}
	//Append buffer into the dataBuffer
	size_t offset = fullDataBuffer[bufferName].size();
	fullDataBuffer[bufferName].resize(offset + byteLength);
	memcpy(&fullDataBuffer[bufferName][offset], buffer, byteLength);
	return offset;
}

bool GLTFModelExport::constructScene(
	repo::lib::PropertyTree &tree)
{
	repo::core::model::RepoNode* root = scene->getRoot(gType);
	bool success = false;
	if (scene && root)
	{
		std::string sceneName = "defaultScene";
		tree.addToTree(GLTF_LABEL_SCENE, sceneName);
		std::vector<std::string> treeNodes = { UUIDtoString(root->getUniqueID()) };
		tree.addToTree(GLTF_LABEL_SCENES + ".defaultScene." + GLTF_LABEL_NODES, treeNodes);

		auto splitMeshes = populateWithMeshes(tree);
		if (success = splitMeshes.size())
		{
			populateWithNodes(tree, splitMeshes);
			populateWithMaterials(tree);
			populateWithTextures(tree);
			populateWithCameras(tree);

			repo::lib::PropertyTree spatialPartTree = generateSpatialPartitioning();

#ifdef DEBUG
			std::string jsonFilePrefix = "/";
#else
			std::string jsonFilePrefix = "/" + scene->getDatabaseName() + "/" + scene->getProjectName() + "/";
#endif
			std::string jsonFileName = jsonFilePrefix + "revision/" + UUIDtoString(scene->getRevisionID()) + "/partitioning.json";
			tree.addToTree(GLTF_LABEL_SCENES + ".defaultScene." + GLTF_LABEL_EXTRA + ".partitioning." + GLTF_LABEL_URI, "/api" + jsonFileName);

			jsonTrees[jsonFileName] = spatialPartTree;
		}
	}
	return success;
}

repo::lib::PropertyTree GLTFModelExport::generateSpatialPartitioning()
{
	//TODO: We could take in a spatial partitioner in the constructor to allow flexibility
	repo::manipulator::modelutility::RDTreeSpatialPartitioner rdTreePartitioner(scene);
	return rdTreePartitioner.generatePropertyTreeForPartitioning();
}

bool GLTFModelExport::generateTreeRepresentation()
{
	bool success = false;
	if (!scene)
	{
		//Sanity check, shouldn't be calling this function with nullptr anyway
		return false;
	}

	repo::lib::PropertyTree tree;

	std::stringstream ss;
	ss << "3D Repo Bouncer v" << BOUNCER_VMAJOR << "." << BOUNCER_VMINOR;
	tree.addToTree(GLTF_LABEL_ASSET + "." + GLTF_LABEL_GENERATOR, ss.str());
	tree.addToTree(GLTF_LABEL_ASSET + "." + GLTF_LABEL_VERSION, GLTF_VERSION);
	//FIXME: SHADER- Premultiplied alpha?

	success = constructScene(tree);
	writeBuffers(tree);

	std::string fname = "/" + scene->getDatabaseName() + "/" + scene->getProjectName() + "/revision/" + UUIDtoString(scene->getRevisionID()) + ".gltf";
	trees[fname] = tree;

	return success;
}

repo_web_buffers_t GLTFModelExport::getAllFilesExportedAsBuffer() const
{
	return{ getGLTFFilesAsBuffer(), getJSONFilesAsBuffer() };
}

std::unordered_map<std::string, std::vector<uint8_t>> GLTFModelExport::getGLTFFilesAsBuffer() const
{
	std::unordered_map<std::string, std::vector<uint8_t>> files;
	//GLTF files
	for (const auto &pair : trees)
	{
		std::stringstream ss;
		pair.second.write_json(ss);
		std::string jsonString = ss.str();
		if (!jsonString.empty())
		{
			files[pair.first] = std::vector<uint8_t>();
			size_t byteLength = jsonString.size() * sizeof(*jsonString.data());
			files[pair.first].resize(byteLength);
			memcpy(files[pair.first].data(), jsonString.data(), byteLength);
		}
		else
		{
			repoError << "Failed to write " << pair.first << " into the buffer: JSON string is empty.";
		}
	}

	//bin files
	for (const auto &pair : fullDataBuffer)
	{
		std::string bufferFilePrefix = "/" + scene->getDatabaseName() + "/" + scene->getProjectName() + "/";
		std::string fileName = bufferFilePrefix + pair.first + ".bin";
		if (pair.second.size())
		{
			auto it = files.find(fileName);
			//None of the gltf should ever share the same name, this is a sanity check
			if (it == files.end())
			{
				files[fileName] = std::vector<uint8_t>();
				size_t byteLength = pair.second.size() * sizeof(*pair.second.data());
				files[fileName].resize(byteLength);
				memcpy(files[fileName].data(), pair.second.data(), byteLength);
			}
			else
			{
				repoError << "Multiple files are named " << fileName << ". This is not allowed.";
			}
		}
		else
		{
			repoError << "Failed to write " << fileName << " into the buffer: data buffer is empty.";
		}
	}

	return files;
}

/**
* TODO: this should really be done whilst the mesh is being split
* But since we still need the non re-Indexed faces for SRC export,
* This is temporarily done here.
* Once we stop support for SRC, the faces should be remapped during the
* meshsplit functionality within MeshNode.
*/
bool GLTFModelExport::reIndexFaces(
	const std::vector<std::vector<repo_mesh_mapping_t>> &matMap,
	std::vector<uint16_t>                               &faces)
{
	size_t verticesOffset = 0;
	size_t verticesLastIndex = 0;
	size_t facesLastIndex = 0;
	for (const auto &subMeshMap : matMap)
	{
		//Faces given are already offset by subMeshes. we need to know the start of the vertices
		verticesOffset = subMeshMap.front().vertFrom;
		for (const auto &mapping : subMeshMap)
		{
			if (mapping.vertFrom > verticesLastIndex)
			{
				repoError << "Backtracking on vertices detected on sub mesh " << mapping.mesh_id << ". This is not expected";
				return false;
			}

			if (mapping.triFrom > facesLastIndex)
			{
				repoError << "Backtracking on vertices detected on sub mesh " << mapping.mesh_id << ". This is not expected";
				return false;
			}

			auto offset = mapping.vertFrom - verticesOffset;
			auto maximum = mapping.vertTo - mapping.vertFrom;

			if (offset < 0 || maximum < 0)
			{
				repoError << "Negative offset within " << mapping.mesh_id << " - This is not expected.";
				return false;
			}
			//This will totally fall apart if the faces are not triangulated
			//But at this stage all faces should be triangulated.
			for (size_t i = mapping.triFrom * 3; i < mapping.triTo * 3; ++i)
			{
				faces[i] -= offset;
			}

			verticesLastIndex = mapping.vertTo;
			facesLastIndex = mapping.triTo;
		}
	}
	return true;
}

void GLTFModelExport::processNodeChildren(
	const repo::core::model::RepoNode                            *node,
	repo::lib::PropertyTree                                      &tree,
	const std::unordered_map<repoUUID, uint32_t, RepoUUIDHasher> &subMeshCounts
	)
{
	std::vector<std::string> trans, meshes, cameras;
	std::vector<repo::core::model::RepoNode*> children = scene->getChildrenAsNodes(gType, node->getSharedID());

	for (const repo::core::model::RepoNode* child : children)
	{
		switch (child->getTypeAsEnum())
		{
		case repo::core::model::NodeType::CAMERA:
			cameras.push_back(UUIDtoString(child->getUniqueID()));
			break;
		case repo::core::model::NodeType::MESH:
		{
			repoUUID meshId = child->getUniqueID();
			auto meshIt = subMeshCounts.find(meshId);
			if (meshIt != subMeshCounts.end())
			{
				std::string meshIdStr = UUIDtoString(meshId);
				for (uint32_t i = 0; i < meshIt->second; ++i)
				{
					meshes.push_back(meshIdStr + "_" + std::to_string(i));
				}
			}
			else
			{
				meshes.push_back(UUIDtoString(child->getUniqueID()));
			}
		}
		break;
		case repo::core::model::NodeType::TRANSFORMATION:
			trans.push_back(UUIDtoString(child->getUniqueID()));
			break;
		}
	}
	std::string prefix = GLTF_LABEL_NODES + "." + UUIDtoString(node->getUniqueID()) + ".";
	//Add the children arrays into the node
	if (trans.size())
		tree.addToTree(prefix + GLTF_LABEL_CHILDREN, trans);
	if (meshes.size())
		tree.addToTree(prefix + GLTF_LABEL_MESHES, meshes);
	if (cameras.size())
		tree.addToTree(prefix + GLTF_LABEL_CAMERAS, cameras);
}

void GLTFModelExport::populateWithCameras(
	repo::lib::PropertyTree           &tree)
{
	repo::core::model::RepoNodeSet cameras = scene->getAllCameras(gType);
	for (const auto &cam : cameras)
	{
		const repo::core::model::CameraNode *node = (const repo::core::model::CameraNode *)cam;
		const std::string label = GLTF_LABEL_CAMERAS + "." + UUIDtoString(node->getUniqueID());
		//All our viewpoints are perspective..?
		tree.addToTree(label + "." + GLTF_LABEL_TYPE, GLTF_CAM_TYPE_PERSPECTIVE);
		std::string name = node->getName();
		if (!name.empty())
			tree.addToTree(label + "." + GLTF_LABEL_NAME, name);

		const std::string perspectLabel = label + "." + GLTF_CAM_TYPE_PERSPECTIVE;

		tree.addToTree(perspectLabel + "." + GLTF_LABEL_ASP_RATIO, node->getAspectRatio());
		tree.addToTree(perspectLabel + "." + GLTF_LABEL_FOV, node->getFieldOfView());
		tree.addToTree(perspectLabel + "." + GLTF_LABEL_FAR_CP, node->getFarClippingPlane());
		tree.addToTree(perspectLabel + "." + GLTF_LABEL_NEAR_CP, node->getNearClippingPlane());
	}
}

void GLTFModelExport::populateWithMaterials(
	repo::lib::PropertyTree           &tree)
{
	writeDefaultTechnique(tree);

	repo::core::model::RepoNodeSet mats = scene->getAllMaterials(gType);

	for (const auto &mat : mats)
	{
		const repo::core::model::MaterialNode *node = (const repo::core::model::MaterialNode *)mat;
		repo_material_t matStruct = node->getMaterialStruct();
		std::string matLabel = GLTF_LABEL_MATERIALS + "." + UUIDtoString(node->getUniqueID());
		tree.addToTree(matLabel + "." + GLTF_LABEL_TECHNIQUE, REPO_GLTF_DEFAULT_TECHNIQUE);

		std::string valuesLabel = matLabel + "." + GLTF_LABEL_VALUES;
		if (matStruct.ambient.size())
		{
			//default technique takes on a 4d vector, we store 3d vectors
			matStruct.ambient.push_back(1);
			tree.addToTree(valuesLabel + "." + GLTF_LABEL_AMBIENT, matStruct.ambient);
		}

		//find if there are any diffuse texture within the model, add it onto the material if so
		std::vector<repo::core::model::RepoNode*> childrenNodes = scene->getChildrenNodesFiltered(gType, node->getSharedID(), repo::core::model::NodeType::TEXTURE);
		if (childrenNodes.size())
		{
			//should only ever have 1 texture to a material
			tree.addToTree(valuesLabel + "." + GLTF_LABEL_DIFFUSE, UUIDtoString(childrenNodes[0]->getUniqueID()));
		}
		else if (matStruct.diffuse.size())
		{
			tree.addToTree(matLabel + "." + GLTF_LABEL_EXTRA + "." + REPO_LABEL_X3D_MATERIAL + "." + X3D_ATTR_COL_DIFFUSE, matStruct.diffuse, false);
			if (matStruct.isTwoSided)
				tree.addToTree(matLabel + "." + GLTF_LABEL_EXTRA + "." + REPO_LABEL_X3D_MATERIAL + "." + X3D_ATTR_COL_BK_DIFFUSE, matStruct.diffuse, false);
			//default technique takes on a 4d vector, we store 3d vectors
			matStruct.diffuse.push_back(1);
			tree.addToTree(valuesLabel + "." + GLTF_LABEL_DIFFUSE, matStruct.diffuse);
		}

		if (matStruct.emissive.size())
		{
			tree.addToTree(matLabel + "." + GLTF_LABEL_EXTRA + "." + REPO_LABEL_X3D_MATERIAL + "." + X3D_ATTR_COL_EMISSIVE, matStruct.emissive, false);
			if (matStruct.isTwoSided)
				tree.addToTree(matLabel + "." + GLTF_LABEL_EXTRA + "." + REPO_LABEL_X3D_MATERIAL + "." + X3D_ATTR_COL_BK_EMISSIVE, matStruct.emissive, false);
			//default technique takes on a 4d vector, we store 3d vectors
			matStruct.emissive.push_back(1);
			tree.addToTree(valuesLabel + "." + GLTF_LABEL_EMISSIVE, matStruct.emissive);
		}
		if (matStruct.specular.size())
		{
			tree.addToTree(matLabel + "." + GLTF_LABEL_EXTRA + "." + REPO_LABEL_X3D_MATERIAL + "." + X3D_ATTR_COL_SPECULAR, matStruct.specular, false);
			if (matStruct.isTwoSided)
				tree.addToTree(matLabel + "." + GLTF_LABEL_EXTRA + "." + REPO_LABEL_X3D_MATERIAL + "." + X3D_ATTR_COL_BK_SPECULAR, matStruct.specular, false);
			//default technique takes on a 4d vector, we store 3d vectors
			matStruct.specular.push_back(1);
			tree.addToTree(valuesLabel + "." + GLTF_LABEL_SPECULAR, matStruct.specular);
		}

		if (matStruct.shininess == matStruct.shininess)
		{
			tree.addToTree(matLabel + "." + GLTF_LABEL_EXTRA + "." + REPO_LABEL_X3D_MATERIAL + "." + X3D_ATTR_SHININESS, matStruct.shininess);
			if (matStruct.isTwoSided)
				tree.addToTree(matLabel + "." + GLTF_LABEL_EXTRA + "." + REPO_LABEL_X3D_MATERIAL + "." + X3D_ATTR_BK_SHININESS, matStruct.shininess);
			tree.addToTree(valuesLabel + "." + GLTF_LABEL_SHININESS, matStruct.shininess);
		}

		if (matStruct.opacity == matStruct.opacity)
		{
			float transparency = 1.0 - matStruct.opacity;
			tree.addToTree(matLabel + "." + GLTF_LABEL_EXTRA + "." + REPO_LABEL_X3D_MATERIAL + "." + X3D_ATTR_TRANSPARENCY, transparency);
			if (matStruct.isTwoSided)
				tree.addToTree(matLabel + "." + GLTF_LABEL_EXTRA + "." + REPO_LABEL_X3D_MATERIAL + "." + X3D_ATTR_BK_TRANSPARENCY, transparency);
			tree.addToTree(valuesLabel + "." + GLTF_LABEL_TRANSPARENCY, transparency);
		}

		if (matStruct.isTwoSided)
			tree.addToTree(valuesLabel + "." + GLTF_LABEL_TWO_SIDED, "true");

		std::string matName = node->getName();
		if (!matName.empty())
			tree.addToTree(matLabel + "." + GLTF_LABEL_NAME, matName);
	}
}

std::unordered_map<repoUUID, uint32_t, RepoUUIDHasher> GLTFModelExport::populateWithMeshes(
	repo::lib::PropertyTree           &tree)
{
	repo::core::model::RepoNodeSet meshes = scene->getAllMeshes(gType);
	std::unordered_map<repoUUID, uint32_t, RepoUUIDHasher> splitSizes;
	for (const auto &mesh : meshes)
	{
		const repo::core::model::MeshNode *node = (const repo::core::model::MeshNode *)mesh;
		const std::vector<repo_mesh_mapping_t> mappings = node->getMeshMapping();

		std::string meshUUID = UUIDtoString(node->getUniqueID());

		std::vector<repo_vector_t> normals;
		std::vector<repo_vector_t> vertices = node->getVertices();
		std::vector<std::vector<repo_vector2d_t>> UVs;

		if (mappings.size() > 1 || vertices.size() > GLTF_MAX_VERTEX_LIMIT)
		{
			//This is a multipart mesh node, the mesh may be too big for
			//webGL, split the mesh into sub meshes
			std::string bufferFileName = UUIDtoString(mesh->getUniqueID());
			repo::manipulator::modelutility::MeshMapReorganiser *reSplitter =
				new repo::manipulator::modelutility::MeshMapReorganiser(node, GLTF_MAX_VERTEX_LIMIT);

			repo::core::model::MeshNode splitMesh = reSplitter->getRemappedMesh();
			if (splitMesh.isEmpty())
			{
				repoError << "Failed to generate remappings for mesh: " << mesh->getUniqueID();
				splitSizes.clear();
				delete reSplitter;
				return splitSizes;
			}
			std::vector<uint16_t> newFaces = reSplitter->getSerialisedFaces();
			std::vector<std::vector<float>> idMapBuf = reSplitter->getIDMapArrays();
			std::vector<std::vector<repo_mesh_mapping_t>> matMap = reSplitter->getMappingsPerSubMesh();

			delete reSplitter;

			auto normals = splitMesh.getNormals();
			auto vertices = splitMesh.getVertices();

			if (!vertices.size())
			{
				repoError << "Mesh " << mesh->getUniqueID() << " has no vertices after remapping!";
				splitSizes.clear();
				return splitSizes;
			}

			if (!newFaces.size())
			{
				//If there is no faces, just ignore this.
				repoWarning << "Mesh has no faces after remapping. Skipping...";
				continue;
			}
			repoTrace << "Reindexing Faces...";
			//reindex the face buffer also check validity of the indices
			if (!reIndexFaces(matMap, newFaces))
			{
				splitSizes.clear();
				return splitSizes;
			}
			repoTrace << "Reordering Faces...";
			auto lods = reorderFaces(newFaces, vertices, matMap);
#if defined(DEBUG) && defined(LODLIMIT)
			for (size_t i = 0; i < matMap.size(); ++i)
				for (size_t j = 0; j < matMap[i].size(); ++j)
				{
					repo_mesh_mapping_t mapping = matMap[i][j];
					size_t maxBits = 16;
					const float maxQuant = pow(2, maxBits) - 1;
					const size_t vCount = mapping.vertTo - mapping.vertFrom;
					uint32_t dim = pow(2, (maxBits - lodLimit));
					uint32_t shift = maxBits - lodLimit;
					repo_vector_t *vRaw = &vertices[mapping.vertFrom];
					repo_vector_t bboxMin = mapping.min;
					repo_vector_t bboxSize = { mapping.max.x - bboxMin.x, mapping.max.y - bboxMin.y, mapping.max.z - bboxMin.z };
					for (size_t vertId = 0; vertId < vCount; ++vertId)
					{
						uint32_t vertXNormal = floorf(((vRaw[vertId].x - bboxMin.x) / bboxSize.x) * maxQuant + 0.5);
						uint32_t vertYNormal = floorf(((vRaw[vertId].y - bboxMin.y) / bboxSize.y) * maxQuant + 0.5);
						uint32_t vertZNormal = floorf(((vRaw[vertId].z - bboxMin.z) / bboxSize.z) * maxQuant + 0.5);

						uint32_t vertX = (vertXNormal >> shift) << shift;
						uint32_t vertY = (vertYNormal >> shift) << shift;
						uint32_t vertZ = (vertZNormal >> shift) << shift;

						//						repoDebug << "Before: (" << vRaw[vertId].x << "," << vRaw[vertId].y << ", " << vRaw[vertId].z << ")";
						vRaw[vertId].x = ((float)vertX) / maxQuant * bboxSize.x + bboxMin.x;
						vRaw[vertId].y = ((float)vertY) / maxQuant * bboxSize.y + bboxMin.y;
						vRaw[vertId].z = ((float)vertZ) / maxQuant * bboxSize.z + bboxMin.z;
						//					repoDebug << "After: (" << vRaw[vertId].x << "," << vRaw[vertId].y << ", " << vRaw[vertId].z << ")";
					}
				}
#endif

			auto newMappings = splitMesh.getMeshMapping();

			splitSizes[node->getUniqueID()] = newMappings.size();

			size_t vStart = addToDataBuffer(bufferFileName, vertices);
			size_t nStart = addToDataBuffer(bufferFileName, normals);
			size_t fStart = addToDataBuffer(bufferFileName, newFaces);

			std::vector<size_t> idMapStart;
			idMapStart.reserve(idMapBuf.size());

			size_t offset = 0;
			for (size_t idMapBufIdx = 0; idMapBufIdx < idMapBuf.size(); ++idMapBufIdx)
			{
				if (offset > 0)
				{
					for (size_t idMapIdx = 0; idMapIdx < idMapBuf[idMapBufIdx].size(); idMapIdx++)
					{
						//remove offset on subsequent supermeshes
						idMapBuf[idMapBufIdx][idMapIdx] -= offset;
					}
				}

				idMapStart.push_back(addToDataBuffer(bufferFileName, idMapBuf[idMapBufIdx]));
				offset += matMap[idMapBufIdx].size();
			}

			std::vector<size_t> uvStart;
			for (size_t i = 0; i < UVs.size(); ++i)
			{
				uvStart.push_back(addToDataBuffer(bufferFileName, UVs[i]));
			}

			for (size_t i = 0; i < newMappings.size(); ++i)
			{
				//every mapping is a mesh
				std::string meshId = meshUUID + "_" + std::to_string(i);
				std::string label = GLTF_LABEL_MESHES + "." + meshId;
				repoTrace << "Generatinng GLTF entry for mapping : " << label;
				std::vector<repo::lib::PropertyTree> primitives;
				size_t count = 0;

				std::string faceBufferName = meshId + "_" + GLTF_SUFFIX_FACES;
				std::string normBufferName = meshId + "_" + GLTF_SUFFIX_NORMALS;
				std::string posBufferName = meshId + "_" + GLTF_SUFFIX_POSITION;
				std::string idBufferName = meshId + "_" + GLTF_SUFFIX_IDMAP;

				size_t vcount = newMappings[i].vertTo - newMappings[i].vertFrom;
				size_t fcount = newMappings[i].triTo - newMappings[i].triFrom;

				//for each mesh we need to add a bufferView for each buffer
				addBufferView(normBufferName, bufferFileName, tree, normals, nStart, vcount, meshId);
				nStart += vcount * sizeof(repo_vector_t);

				addBufferView(posBufferName, bufferFileName, tree, vertices, vStart, vcount, meshId);
				vStart += vcount * sizeof(repo_vector_t);

				addBufferView(faceBufferName, bufferFileName, tree, newFaces, fStart, fcount, meshId);
				fStart += fcount * 3 * sizeof(uint16_t); //faces are triangulated

				addBufferView(idBufferName, bufferFileName, tree, idMapBuf[i], idMapStart[i], vcount, meshId);

				for (size_t iUV = 0; iUV < UVs.size(); ++iUV)
				{
					std::string uvBufferName = meshId + "_" + GLTF_SUFFIX_TEX_COORD + "_" + std::to_string(iUV);
					addBufferView(uvBufferName, bufferFileName, tree, UVs[iUV], uvStart[iUV], vcount, meshId);
					uvStart[iUV] += vcount*sizeof(repo_vector2d_t);
				}

				size_t subMeshOffset_v = newMappings[i].vertFrom;
				size_t subMeshOffset_f = newMappings[i].triFrom;

				auto lodIterator = lods[i].begin();
				size_t nVertices = newMappings[i].vertTo - newMappings[i].vertFrom;
				if (nVertices != idMapBuf[i].size())
				{
					repoError << "Mismatched nvertices (" << nVertices << ") != idmapbuf ( " << idMapBuf[i].size() << "). Skipping...";
				}

				//For each sub mesh...
				for (const repo_mesh_mapping_t & meshMap : matMap[i])
				{
					std::string subMeshID = UUIDtoString(meshMap.mesh_id);

					primitives.push_back(repo::lib::PropertyTree());
					primitives.back().addToTree(GLTF_LABEL_MATERIAL, UUIDtoString(meshMap.material_id));
					primitives.back().addToTree(GLTF_LABEL_PRIMITIVE, GLTF_PRIM_TYPE_TRIANGLE);

					std::string subMeshName = meshId + "_m" + std::to_string(count++);

					if (newFaces.size())
					{
						std::string accessorName = subMeshName + "_" + GLTF_SUFFIX_FACES;
						primitives.back().addToTree(GLTF_LABEL_INDICES, GLTF_PREFIX_ACCESSORS + "_" + accessorName);
						std::vector<uint16_t> lodVec = *lodIterator;
#if defined(DEBUG) && defined(LODLIMIT)
						size_t triTo = meshMap.triFrom + (lodVec.size() < lodLimit ? lodVec.back() : lodVec[lodLimit - 1]) / 3;
#else
						size_t triTo = meshMap.triTo;
#endif
						addAccessors(accessorName, faceBufferName, tree, newFaces, meshMap.triFrom, triTo, subMeshID, *lodIterator, subMeshOffset_f);
					}
					++lodIterator;

					if (normals.size())
					{
						std::string accessorName = subMeshName + "_" + GLTF_SUFFIX_NORMALS;
						primitives.back().addToTree(GLTF_LABEL_ATTRIBUTES + "." + GLTF_LABEL_NORMAL, GLTF_PREFIX_ACCESSORS + "_" + accessorName);
						addAccessors(accessorName, normBufferName, tree, normals, meshMap.vertFrom, meshMap.vertTo, subMeshID, subMeshOffset_v);
					}

					if (vertices.size())
					{
						std::string accessorName = subMeshName + "_" + GLTF_SUFFIX_POSITION;
						primitives.back().addToTree(GLTF_LABEL_ATTRIBUTES + "." + GLTF_LABEL_POSITION, GLTF_PREFIX_ACCESSORS + "_" + accessorName);
						addAccessors(accessorName, posBufferName, tree, vertices, meshMap.vertFrom, meshMap.vertTo, subMeshID, subMeshOffset_v);
					}

					if (idMapBuf[i].size())
					{
						std::string accessorName = subMeshName + "_" + GLTF_SUFFIX_IDMAP;
						primitives.back().addToTree(GLTF_LABEL_ATTRIBUTES + "." + REPO_GLTF_LABEL_IDMAP, GLTF_PREFIX_ACCESSORS + "_" + accessorName);
						addAccessors(accessorName, idBufferName, tree, idMapBuf[i], meshMap.vertFrom, meshMap.vertTo, subMeshID, subMeshOffset_v);
					}

					if (UVs.size())
					{
						for (uint32_t iUV = 0; iUV < UVs.size(); ++iUV)
						{
							std::string accessorName = subMeshName + "_" + GLTF_SUFFIX_TEX_COORD + "_" + std::to_string(iUV);
							std::string uvBufferName = meshId + "_" + GLTF_SUFFIX_TEX_COORD + "_" + std::to_string(iUV);
							primitives.back().addToTree(GLTF_LABEL_ATTRIBUTES + "." + GLTF_LABEL_TEXCOORD + "_" + std::to_string(iUV),
								GLTF_PREFIX_ACCESSORS + "_" + accessorName);
							addAccessors(accessorName, uvBufferName, tree, UVs[iUV], meshMap.vertFrom, meshMap.vertTo, subMeshID, subMeshOffset_v);
						}
					}
					primitives.back().addToTree(GLTF_LABEL_EXTRA + "." + REPO_GLTF_LABEL_REF_ID, subMeshID);
				}
				tree.addArrayObjects(label + "." + GLTF_LABEL_PRIMITIVES, primitives);
			}
		}
		else
		{
			normals = node->getNormals();
			vertices = node->getVertices();
			UVs = node->getUVChannelsSeparated();

			bool hasMapping = mappings.size();
			std::string meshId = hasMapping ? UUIDtoString(mappings[0].mesh_id) : UUIDtoString(node->getUniqueID());
			std::string label = GLTF_LABEL_MESHES + "." + UUIDtoString(node->getUniqueID());

			repoTrace << "Generatinng GLTF entry for : " << label;
			std::string name = node->getName();
			if (!name.empty())
				tree.addToTree(label + "." + GLTF_LABEL_NAME, node->getName());

			auto faces = node->getFaces();
			std::vector<uint16_t> sFaces = serialiseFaces(faces);

			bool hasMat = false;
			repoUUID matID;
			if (hasMapping)
			{
				hasMat = true;
				matID = mappings[0].material_id;
			}
			else
			{
				auto children = scene->getChildrenNodesFiltered(gType, node->getSharedID(), repo::core::model::NodeType::MATERIAL);
				if (children.size())
				{
					hasMat = true;
					matID = children[0]->getUniqueID();
				}
			}

			std::vector < std::vector<repo_mesh_mapping_t>> matMap;
			matMap.resize(1);
			matMap[0].resize(1);
			matMap[0][0].material_id = matID;
			matMap[0][0].mesh_id = hasMapping ? mappings[0].mesh_id : node->getUniqueID();
			auto bbox = node->getBoundingBox();
			matMap[0][0].max = bbox[1];
			matMap[0][0].min = bbox[0];
			matMap[0][0].triFrom = 0;
			matMap[0][0].triTo = faces.size();
			matMap[0][0].vertFrom = 0;
			matMap[0][0].vertTo = vertices.size();

			auto lods = reorderFaces(sFaces, vertices, matMap);
#if defined(DEBUG) && defined(LODLIMIT)
			for (size_t i = 0; i < matMap.size(); ++i)
				for (size_t j = 0; j < matMap[i].size(); ++j)
				{
					repo_mesh_mapping_t mapping = matMap[i][j];
					size_t maxBits = 16;
					const float maxQuant = pow(2, maxBits) - 1;
					const size_t vCount = mapping.vertTo - mapping.vertFrom;
					uint32_t dim = pow(2, (maxBits - lodLimit));
					uint32_t shift = maxBits - lodLimit;
					repo_vector_t *vRaw = &vertices[mapping.vertFrom];
					repo_vector_t bboxMin = mapping.min;
					repo_vector_t bboxSize = { mapping.max.x - bboxMin.x, mapping.max.y - bboxMin.y, mapping.max.z - bboxMin.z };
					for (size_t vertId = 0; vertId < vCount; ++vertId)
					{
						uint32_t vertXNormal = floorf(((vRaw[vertId].x - bboxMin.x) / bboxSize.x) * maxQuant + 0.5);
						uint32_t vertYNormal = floorf(((vRaw[vertId].y - bboxMin.y) / bboxSize.y) * maxQuant + 0.5);
						uint32_t vertZNormal = floorf(((vRaw[vertId].z - bboxMin.z) / bboxSize.z) * maxQuant + 0.5);

						uint32_t vertX = (vertXNormal >> shift) << shift;
						uint32_t vertY = (vertYNormal >> shift) << shift;
						uint32_t vertZ = (vertZNormal >> shift) << shift;

						//						repoDebug << "Before: (" << vRaw[vertId].x << "," << vRaw[vertId].y << ", " << vRaw[vertId].z << ")";
						vRaw[vertId].x = ((float)vertX) / maxQuant * bboxSize.x + bboxMin.x;
						vRaw[vertId].y = ((float)vertY) / maxQuant * bboxSize.y + bboxMin.y;
						vRaw[vertId].z = ((float)vertZ) / maxQuant * bboxSize.z + bboxMin.z;
						//					repoDebug << "After: (" << vRaw[vertId].x << "," << vRaw[vertId].y << ", " << vRaw[vertId].z << ")";
					}
				}
#endif
			std::string bufferFileName = UUIDtoString(scene->getRevisionID());

			size_t vStart = addToDataBuffer(bufferFileName, vertices);
			size_t nStart = addToDataBuffer(bufferFileName, vertices);
			size_t fStart = addToDataBuffer(bufferFileName, sFaces);

			std::string faceBufferName = meshId + "_" + GLTF_SUFFIX_FACES;
			std::string normBufferName = meshId + "_" + GLTF_SUFFIX_NORMALS;
			std::string posBufferName = meshId + "_" + GLTF_SUFFIX_POSITION;

			//for each mesh we need to add a bufferView for each buffer
			addBufferView(normBufferName, bufferFileName, tree, normals, nStart, normals.size(), meshId);
			addBufferView(posBufferName, bufferFileName, tree, vertices, vStart, vertices.size(), meshId);
			addBufferView(faceBufferName, bufferFileName, tree, sFaces, fStart, faces.size(), meshId);

			for (size_t i = 0; i < UVs.size(); ++i)
			{
				size_t uvStart = addToDataBuffer(bufferFileName, UVs[i]);
				std::string uvBufferName = meshId + "_" + GLTF_SUFFIX_TEX_COORD + "_" + std::to_string(i);
				addBufferView(uvBufferName, bufferFileName, tree, UVs[i], uvStart, UVs[i].size(), meshId);
			}

			std::vector<repo::lib::PropertyTree> primitives;
			primitives.push_back(repo::lib::PropertyTree());

			std::string binFileName = UUIDtoString(scene->getRevisionID());

			if (hasMat)
			{
				primitives[0].addToTree(GLTF_LABEL_MATERIAL, UUIDtoString(matID));
				primitives[0].addToTree(GLTF_LABEL_PRIMITIVE, GLTF_PRIM_TYPE_TRIANGLE);

				if (faces.size())
				{
					std::string bufferName = meshId + "_" + GLTF_SUFFIX_FACES;
					primitives[0].addToTree(GLTF_LABEL_INDICES, GLTF_PREFIX_ACCESSORS + "_" + bufferName);
#if defined(DEBUG) && defined(LODLIMIT)
					size_t triTo = (lods[0][0].size() < lodLimit ? lods[0][0].back() : lods[0][0][lodLimit - 1]) / 3;
#else
					size_t triTo = faces.size();
#endif
					addAccessors(bufferName, faceBufferName, tree, sFaces, 0, faces.size(), meshId);
				}

				//attributes
				auto normals = node->getNormals();
				if (normals.size())
				{
					std::string bufferName = meshId + "_" + GLTF_SUFFIX_NORMALS;
					primitives[0].addToTree(GLTF_LABEL_ATTRIBUTES + "." + GLTF_LABEL_NORMAL, GLTF_PREFIX_ACCESSORS + "_" + bufferName);
					addAccessors(bufferName, normBufferName, tree, normals, 0, normals.size(), meshId);
				}
				auto vertices = node->getVertices();
				if (vertices.size())
				{
					std::string bufferName = meshId + "_" + GLTF_SUFFIX_POSITION;
					primitives[0].addToTree(GLTF_LABEL_ATTRIBUTES + "." + GLTF_LABEL_POSITION, GLTF_PREFIX_ACCESSORS + "_" + bufferName);
					addAccessors(bufferName, posBufferName, tree, vertices, 0, vertices.size(), meshId);
				}

				auto UVs = node->getUVChannelsSeparated();
				if (UVs.size())
				{
					for (uint32_t i = 0; i < UVs.size(); ++i)
					{
						std::string bufferName = meshId + "_" + GLTF_SUFFIX_TEX_COORD + "_" + std::to_string(i);
						primitives[0].addToTree(GLTF_LABEL_ATTRIBUTES + "." + GLTF_LABEL_TEXCOORD + "_" + std::to_string(i),
							GLTF_PREFIX_ACCESSORS + "_" + bufferName);
						addAccessors(bufferName, bufferName, tree, UVs[i], 0, UVs[i].size(), meshId);
					}
				}
			}

			tree.addArrayObjects(label + "." + GLTF_LABEL_PRIMITIVES, primitives);
		}
	}
	return splitSizes;
}

void GLTFModelExport::populateWithTextures(
	repo::lib::PropertyTree           &tree)
{
	repo::core::model::RepoNodeSet textures = scene->getAllTextures(gType);

	writeDefaultSampler(tree);

	for (const auto &texture : textures)
	{
		const repo::core::model::TextureNode *node = (const repo::core::model::TextureNode *)texture;
		const std::string textureID = UUIDtoString(node->getUniqueID());
		const std::string label = GLTF_LABEL_TEXTURES + "." + textureID;
		const std::string imageName = GLTF_PREFIX_TEXTURE + "_" + textureID;
		tree.addToTree(label + "." + GLTF_LABEL_FORMAT, GLTF_FORMAT_RGBA);
		tree.addToTree(label + "." + GLTF_LABEL_INTERNAL_FORMAT, GLTF_FORMAT_RGBA);
		tree.addToTree(label + "." + GLTF_LABEL_SOURCE, imageName);
		tree.addToTree(label + "." + GLTF_LABEL_SAMPLER, REPO_GLTF_DEFAULT_SAMPLER);
		tree.addToTree(label + "." + GLTF_LABEL_TARGET, GLTF_TYPE_TEXTURE_2D);
		tree.addToTree(label + "." + GLTF_LABEL_TYPE, GLTF_TYPE_UNSIGNED_BYTES);
		tree.addToTree(label + "." + GLTF_LABEL_NAME, node->getName());

		const std::string imageLabel = GLTF_LABEL_IMAGES + "." + imageName;

		//FIXME: this should be an api or something to drag the image out
		tree.addToTree(imageLabel + "." + GLTF_LABEL_URI, node->getName());
		tree.addToTree(imageLabel + "." + GLTF_LABEL_NAME, node->getName());
	}
}

void GLTFModelExport::populateWithNodes(
	repo::lib::PropertyTree          &tree,
	const std::unordered_map<repoUUID, uint32_t, RepoUUIDHasher> &subMeshCounts)
{
	repo::core::model::RepoNodeSet trans = scene->getAllTransformations(gType);
	for (const auto &tran : trans)
	{
		const repo::core::model::TransformationNode *node = (const repo::core::model::TransformationNode *)tran;
		//add to list of nodes
		std::string label = GLTF_LABEL_NODES + "." + UUIDtoString(node->getUniqueID());
		std::string name = node->getName();
		if (!name.empty())
			tree.addToTree(label + "." + GLTF_LABEL_NAME, node->getName());
		const repo::core::model::TransformationNode *transNode = (const repo::core::model::TransformationNode*) node;
		if (!transNode->isIdentity())
		{
			tree.addToTree(label + "." + GLTF_LABEL_MATRIX, transNode->getTransMatrix(false));
		}
		processNodeChildren(node, tree, subMeshCounts);
	}
}

std::vector<std::vector<std::vector<uint16_t>>> GLTFModelExport::reorderFaces(
	std::vector<uint16_t>                         &faces,
	const std::vector<repo_vector_t>                    &vertices,
	const std::vector<std::vector<repo_mesh_mapping_t>> &mapping)
{
	std::vector<std::vector<std::vector<uint16_t>>> lods;
	for (size_t i = 0; i < mapping.size(); ++i)
	{
		lods.resize(lods.size() + 1);
		lods.back().clear();
		for (size_t j = 0; j < mapping[i].size(); ++j)
		{
			lods[i].resize(lods[i].size() + 1);
			lods[i].back().clear();
			std::vector<uint16_t> newFaces = reorderFaces(faces, vertices, mapping[i][j], lods[i].back());
			std::copy(newFaces.begin(), newFaces.end(), faces.begin() + mapping[i][j].triFrom * 3);
		}
	}
	return lods;
}

std::vector<uint16_t> GLTFModelExport::reorderFaces(
	const std::vector<uint16_t>      &faces,
	const std::vector<repo_vector_t> &vertices,
	const repo_mesh_mapping_t        &mapping,
	std::vector<uint16_t>      &lods) const
{
	const uint32_t maxBits = 16;
	const float maxQuant = pow(2, maxBits) - 1;

	const repo_vector_t *vRaw = &vertices[mapping.vertFrom];
	const uint16_t      *fRaw = &faces[mapping.triFrom * 3];

	const size_t vCount = mapping.vertTo - mapping.vertFrom;
	const size_t fCount = mapping.triTo - mapping.triFrom;

	//use int32_t because we need to represent all values in uint16_t and also -1
	std::vector<int32_t> vertexMap;
	vertexMap.resize(vCount);

	//Instantiate with -1s
	std::fill(vertexMap.begin(), vertexMap.end(), -1);

	std::vector<bool> validFaces;
	validFaces.resize(fCount);

	//Instantiate with false
	std::fill(validFaces.begin(), validFaces.end(), false);

	std::vector<uint16_t> reOrderedFaces;
	reOrderedFaces.reserve(fCount * 3);

	repo_vector_t bboxMin = mapping.min;
	repo_vector_t bboxSize = { mapping.max.x - bboxMin.x, mapping.max.y - bboxMin.y, mapping.max.z - bboxMin.z };

	std::vector<uint32_t> quantIndex;
	quantIndex.resize(vCount);
	/**
	* Every level of detail, we need to identify the quantized vertices
	* see which face should be degenerated.
	* for all faces that are not degenerated, put them into the new face buffer
	*/
	for (uint32_t lod = 0; lod < maxBits; ++lod)
	{
		uint32_t dim = pow(2, (maxBits - lod));
		uint32_t shift = maxBits - lodLimit;

		// For all non mapped vertices compute quantization
		for (size_t vertId = 0; vertId < vCount; ++vertId)
		{
			if (vertexMap[vertId] == -1)
			{
				uint32_t vertXNormal = floorf(((vRaw[vertId].x - bboxMin.x) / bboxSize.x) * maxQuant + 0.5);
				uint32_t vertYNormal = floorf(((vRaw[vertId].y - bboxMin.y) / bboxSize.y) * maxQuant + 0.5);
				uint32_t vertZNormal = floorf(((vRaw[vertId].z - bboxMin.z) / bboxSize.z) * maxQuant + 0.5);

				uint32_t vertX = (vertXNormal >> shift) << shift;
				uint32_t vertY = (vertYNormal >> shift) << shift;
				uint32_t vertZ = (vertZNormal >> shift) << shift;

				quantIndex[vertId] = vertX + vertY * dim + vertZ * dim * dim;
			}
		}

		//check if any faces appear in this quantization
		for (size_t triIdx = 0; triIdx < fCount; ++triIdx)
		{
			size_t startIdx = triIdx * 3;
			//If this face has not been added from previous LODs
			if (!validFaces[triIdx])
			{
				//the face should not be rendered if more than 1 vertex fall into the same quantized region
				uint32_t currQuantX = quantIndex[fRaw[startIdx]];
				uint32_t currQuantY = quantIndex[fRaw[startIdx + 1]];
				uint32_t currQuantZ = quantIndex[fRaw[startIdx + 2]];

				if (currQuantX != currQuantY && currQuantX != currQuantY && currQuantY != currQuantZ || lod == maxBits - 1)
				{
					//Add this face to the new face buffer
					reOrderedFaces.push_back(fRaw[startIdx]);
					reOrderedFaces.push_back(fRaw[startIdx + 1]);
					reOrderedFaces.push_back(fRaw[startIdx + 2]);

					validFaces[triIdx] = true;
				}
			}
		}

		lods.push_back(reOrderedFaces.size());

		if (reOrderedFaces.size() == fCount * 3)
			break;
	}

	if (reOrderedFaces.size() != fCount * 3)
	{
		//sanity check
		repoError << "Reordered faces (" << reOrderedFaces.size() << ") != fCount(" << fCount * 3 << ")!!!";
	}

	return reOrderedFaces;
}

std::vector<uint16_t> GLTFModelExport::serialiseFaces(
	const std::vector<repo_face_t> &faces) const
{
	std::vector<uint16_t> sFaces;

	for (uint32_t i = 0; i < faces.size(); ++i)
	{
		if (faces[i].size() == 3)
		{
			sFaces.push_back(faces[i][0]);
			sFaces.push_back(faces[i][1]);
			sFaces.push_back(faces[i][2]);
		}
		else
		{
			repoError << "Error during glTF export: found non triangulated face. This may not visualise correctly.";
		}
	}

	return sFaces;
}

void GLTFModelExport::writeBuffers(
	repo::lib::PropertyTree &tree)
{
	for (const auto &pair : fullDataBuffer)
	{
#ifdef DEBUG
		std::string bufferFilePrefix = "/";
#else
		std::string bufferFilePrefix = "/" + scene->getDatabaseName() + "/" + scene->getProjectName() + "/";
#endif
		std::string bufferLabel = GLTF_LABEL_BUFFERS + "." + pair.first;
		tree.addToTree(bufferLabel + "." + GLTF_LABEL_BYTE_LENGTH, pair.second.size()  * sizeof(*pair.second.data()));
		tree.addToTree(bufferLabel + "." + GLTF_LABEL_TYPE, GLTF_ARRAY_BUFFER);
		tree.addToTree(bufferLabel + "." + GLTF_LABEL_URI, "/api" + bufferFilePrefix + pair.first + ".bin");
	}
}

void GLTFModelExport::writeDefaultSampler(
	repo::lib::PropertyTree &tree)
{
	const std::string label = GLTF_LABEL_SAMPLERS + "." + REPO_GLTF_DEFAULT_SAMPLER;
	tree.addToTree(label + "." + GLTF_LABEL_FILTER_MAG, GLTF_FILTER_TYPE_LINEAR);
	tree.addToTree(label + "." + GLTF_LABEL_FILTER_MIN, GLTF_FILTER_TYPE_NEAREST_MIPMAP_LINEAR);
	tree.addToTree(label + "." + GLTF_LABEL_WRAP_S, GLTF_WRAP_MODE_REPEAT);
	tree.addToTree(label + "." + GLTF_LABEL_WRAP_T, GLTF_WRAP_MODE_REPEAT);
}

void GLTFModelExport::writeDefaultTechnique(
	repo::lib::PropertyTree &tree)
{
	//========== DEFAULT TECHNIQUE =========
	const std::string label = GLTF_LABEL_TECHNIQUES + "." + REPO_GLTF_DEFAULT_TECHNIQUE;
	const std::string paramlabel = label + "." + GLTF_LABEL_PARAMETERS;

	tree.addToTree(paramlabel + ".modelViewMatrix." + GLTF_LABEL_SEMANTIC, "MODELVIEW");
	tree.addToTree(paramlabel + ".modelViewMatrix." + GLTF_LABEL_TYPE, GLTF_COMP_TYPE_FLOAT_MAT4);

	tree.addToTree(paramlabel + ".normal." + GLTF_LABEL_SEMANTIC, GLTF_LABEL_NORMAL);
	tree.addToTree(paramlabel + ".normal." + GLTF_LABEL_TYPE, GLTF_COMP_TYPE_FLOAT_VEC3);

	tree.addToTree(paramlabel + ".normalMatrix." + GLTF_LABEL_SEMANTIC, "MODELVIEWINVERSETRANSPOSE");
	tree.addToTree(paramlabel + ".normalMatrix." + GLTF_LABEL_TYPE, GLTF_COMP_TYPE_FLOAT_MAT4);

	tree.addToTree(paramlabel + ".position." + GLTF_LABEL_SEMANTIC, GLTF_LABEL_POSITION);
	tree.addToTree(paramlabel + ".position." + GLTF_LABEL_TYPE, GLTF_COMP_TYPE_FLOAT_VEC3);

	tree.addToTree(paramlabel + ".texcoord0." + GLTF_LABEL_SEMANTIC, GLTF_LABEL_TEXCOORD + "_0");
	tree.addToTree(paramlabel + ".texcoord0." + GLTF_LABEL_TYPE, GLTF_COMP_TYPE_FLOAT_VEC2);

	tree.addToTree(paramlabel + ".projectionMatrix." + GLTF_LABEL_SEMANTIC, "PROJECTION");
	tree.addToTree(paramlabel + ".projectionMatrix." + GLTF_LABEL_TYPE, GLTF_COMP_TYPE_FLOAT_MAT4);

	tree.addToTree(paramlabel + "." + GLTF_LABEL_SHININESS + "." + GLTF_LABEL_TYPE, GLTF_COMP_TYPE_FLOAT);
	tree.addToTree(paramlabel + "." + GLTF_LABEL_SHININESS + "." + GLTF_LABEL_VALUE, REPO_GLTF_DEFAULT_SHININESS);
	tree.addToTree(paramlabel + "." + GLTF_LABEL_DIFFUSE + "." + GLTF_LABEL_TYPE, GLTF_COMP_TYPE_FLOAT_VEC4);
	tree.addToTree(paramlabel + "." + GLTF_LABEL_SPECULAR + "." + GLTF_LABEL_TYPE, GLTF_COMP_TYPE_FLOAT_VEC4);
	tree.addToTree(paramlabel + "." + GLTF_LABEL_SPECULAR + "." + GLTF_LABEL_VALUE, REPO_GLTF_DEFAULT_SPECULAR);

	tree.addToTree(label + "." + GLTF_LABEL_PROGRAM, REPO_GLTF_DEFAULT_PROGRAM);

	const std::string stateslabel = label + "." + GLTF_LABEL_STATES;
	std::vector<uint32_t> states = { GLTF_STATE_DEPTH_TEST, GLTF_STATE_CULL_FACE };
	tree.addToTree(stateslabel + "." + GLTF_LABEL_ENABLE, states);

	const std::string uniformLabel = label + "." + GLTF_LABEL_UNIFORMS;

	tree.addToTree(uniformLabel + ".u_diffuse", GLTF_LABEL_DIFFUSE);
	tree.addToTree(uniformLabel + ".modelViewMatrix", "modelViewMatrix");
	tree.addToTree(uniformLabel + ".normalMatrix", "normalMatrix");
	tree.addToTree(uniformLabel + ".projectionMatrix", "projectionMatrix");
	tree.addToTree(uniformLabel + ".u_shininess", GLTF_LABEL_SHININESS);
	tree.addToTree(uniformLabel + ".u_specular", GLTF_LABEL_SPECULAR);

	const std::string attriLabel = label + "." + GLTF_LABEL_ATTRIBUTES;
	tree.addToTree(attriLabel + ".normal", "normal");
	tree.addToTree(attriLabel + ".position", "position");
	tree.addToTree(attriLabel + ".a_texcoord0", "texcoord0");

	const std::string extraLabel = label + "." + GLTF_LABEL_EXTRA;
	tree.addToTree(extraLabel + ".varyings.v_normal." + GLTF_LABEL_TYPE, GLTF_COMP_TYPE_FLOAT_VEC3);

	//========== DEFAULT PROGRAM =========
	const std::string programLabel = GLTF_LABEL_PROGRAMS + "." + REPO_GLTF_DEFAULT_PROGRAM;
	std::vector<std::string> programAttributes = { "normal", "position" };
	tree.addToTree(programLabel + "." + GLTF_LABEL_ATTRIBUTES, programAttributes);
	tree.addToTree(programLabel + "." + GLTF_LABEL_SHADER_FRAG, REPO_GLTF_DEFAULT_SHADER_FRAG);
	tree.addToTree(programLabel + "." + GLTF_LABEL_SHADER_VERT, REPO_GLTF_DEFAULT_SHADER_VERT);

	//========== DEFAULT SHADERS =========
	tree.addToTree(GLTF_LABEL_SHADERS + "." + REPO_GLTF_DEFAULT_SHADER_FRAG + "." + GLTF_LABEL_TYPE, GLTF_SHADER_TYPE_FRAGMENT);
	tree.addToTree(GLTF_LABEL_SHADERS + "." + REPO_GLTF_DEFAULT_SHADER_FRAG + "." + GLTF_LABEL_URI, REPO_GLTF_DEFAULT_SHADER_FRAG_URI);
	//x3d shader
	std::string x3dShaderFragLabel = GLTF_LABEL_SHADERS + "." + REPO_GLTF_DEFAULT_SHADER_FRAG + "." + GLTF_LABEL_EXTRA + ".x3domShaderFunction";
	tree.addToTree(x3dShaderFragLabel + "." + GLTF_LABEL_URI, REPO_GLTF_DEFAULT_X3DSHADER_FRAG_URI);
	tree.addToTree(x3dShaderFragLabel + "." + GLTF_LABEL_NAME, "x3dmain");
	std::vector<std::string> fragParameters = { "v_normal", "u_diffuse", "u_specular", "u_shininess" };
	tree.addToTree(x3dShaderFragLabel + "." + GLTF_LABEL_PARAMETERS, fragParameters);
	tree.addToTree(x3dShaderFragLabel + "." + "returns", "gl_FragColor");

	tree.addToTree(GLTF_LABEL_SHADERS + "." + REPO_GLTF_DEFAULT_SHADER_VERT + "." + GLTF_LABEL_TYPE, GLTF_SHADER_TYPE_VERTEX);
	tree.addToTree(GLTF_LABEL_SHADERS + "." + REPO_GLTF_DEFAULT_SHADER_VERT + "." + GLTF_LABEL_URI, REPO_GLTF_DEFAULT_SHADER_VERT_URI);
	//x3d shader
	std::string x3dShaderVertLabel = GLTF_LABEL_SHADERS + "." + REPO_GLTF_DEFAULT_SHADER_VERT + "." + GLTF_LABEL_EXTRA + ".x3domShaderFunction";
	tree.addToTree(x3dShaderVertLabel + "." + GLTF_LABEL_URI, REPO_GLTF_DEFAULT_X3DSHADER_VERT_URI);
	tree.addToTree(x3dShaderVertLabel + "." + GLTF_LABEL_NAME, "x3dmain");
	std::vector<std::string> vertParameters = { "position", "normal", "v_normal", "normalMatrix", "modelViewMatrix", "projectionMatrix" };
	tree.addToTree(x3dShaderVertLabel + "." + GLTF_LABEL_PARAMETERS, vertParameters);
	tree.addToTree(x3dShaderVertLabel + "." + "returns", "gl_Position");
}