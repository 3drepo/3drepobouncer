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
#include "../../../lib/repo_log.h"
#include "../../../core/model/bson/repo_bson_factory.h"

using namespace repo::manipulator::modelconvertor;

static const bool cesiumHack = true; //FIXME: turn on to check on generic viewer - to remove eventually

const static size_t GLTF_MAX_VERTEX_LIMIT = 65535;

static const std::string GLTF_LABEL_ACCESSORS       = "accessors";
static const std::string GLTF_LABEL_AMBIENT         = "ambient";
static const std::string GLTF_LABEL_ASSET           = "asset";
static const std::string GLTF_LABEL_ATTRIBUTES      = "attributes";
static const std::string GLTF_LABEL_BUFFER          = "buffer";
static const std::string GLTF_LABEL_BUFFERS         = "buffers";
static const std::string GLTF_LABEL_BUFFER_VIEW     = "bufferView";
static const std::string GLTF_LABEL_BUFFER_VIEWS    = "bufferViews";
static const std::string GLTF_LABEL_BYTE_LENGTH     = "byteLength";
static const std::string GLTF_LABEL_BYTE_OFFSET     = "byteOffset";
static const std::string GLTF_LABEL_BYTE_STRIDE     = "byteStride";
static const std::string GLTF_LABEL_CAMERAS         = "cameras";
static const std::string GLTF_LABEL_CHILDREN        = "children";
static const std::string GLTF_LABEL_COMP_TYPE       = "componentType";
static const std::string GLTF_LABEL_COUNT           = "count";
static const std::string GLTF_LABEL_DIFFUSE         = "diffuse";
static const std::string GLTF_LABEL_EMISSIVE        = "emission";
static const std::string GLTF_LABEL_ENABLE          = "enable";
static const std::string GLTF_LABEL_EXTRA           = "extras";
static const std::string GLTF_LABEL_FILTER_MAG      = "magFilter";
static const std::string GLTF_LABEL_FILTER_MIN      = "minFilter";
static const std::string GLTF_LABEL_FORMAT          = "format";
static const std::string GLTF_LABEL_GENERATOR       = "generator";
static const std::string GLTF_LABEL_IMAGES          = "images";
static const std::string GLTF_LABEL_INDICES         = "indices";
static const std::string GLTF_LABEL_INTERNAL_FORMAT = "internalFormat";
static const std::string GLTF_LABEL_MATERIAL        = "material";
static const std::string GLTF_LABEL_MATERIALS       = "materials";
static const std::string GLTF_LABEL_MATRIX          = "matrix";
static const std::string GLTF_LABEL_MAX             = "max";
static const std::string GLTF_LABEL_MESHES          = "meshes";
static const std::string GLTF_LABEL_MIN             = "min";
static const std::string GLTF_LABEL_NAME            = "name";
static const std::string GLTF_LABEL_NODES           = "nodes";
static const std::string GLTF_LABEL_NORMAL          = "NORMAL";
static const std::string GLTF_LABEL_PARAMETERS      = "parameters";
static const std::string GLTF_LABEL_POSITION        = "POSITION";
static const std::string GLTF_LABEL_PRIMITIVE       = "mode";
static const std::string GLTF_LABEL_PRIMITIVES      = "primitives";
static const std::string GLTF_LABEL_PROGRAM         = "program";
static const std::string GLTF_LABEL_PROGRAMS        = "programs";
static const std::string GLTF_LABEL_SAMPLER         = "sampler";
static const std::string GLTF_LABEL_SAMPLERS        = "samplers";
static const std::string GLTF_LABEL_SCENE           = "scene";
static const std::string GLTF_LABEL_SCENES          = "scenes";
static const std::string GLTF_LABEL_SEMANTIC        = "semantic";
static const std::string GLTF_LABEL_SHADERS         = "shaders";
static const std::string GLTF_LABEL_SHADER_FRAG     = "fragmentShader";
static const std::string GLTF_LABEL_SHADER_VERT     = "vertexShader";
static const std::string GLTF_LABEL_SHININESS       = "shininess";
static const std::string GLTF_LABEL_SOURCE          = "source";
static const std::string GLTF_LABEL_SPECULAR        = "specular";
static const std::string GLTF_LABEL_STATES          = "states";
static const std::string GLTF_LABEL_TARGET          = "target";
static const std::string GLTF_LABEL_TECHNIQUE       = "technique";
static const std::string GLTF_LABEL_TECHNIQUES      = "techniques";
static const std::string GLTF_LABEL_TEXCOORD        = "TEXCOORD";
static const std::string GLTF_LABEL_TEXTURES        = "textures";
static const std::string GLTF_LABEL_TRANSPARENCY    = "transparency";
static const std::string GLTF_LABEL_TWO_SIDED       = "twoSided";
static const std::string GLTF_LABEL_TYPE            = "type";
static const std::string GLTF_LABEL_UNIFORMS        = "uniforms";
static const std::string GLTF_LABEL_URI             = "uri";
static const std::string GLTF_LABEL_VALUES          = "values";
static const std::string GLTF_LABEL_VERSION         = "version";
static const std::string GLTF_LABEL_WRAP_S          = "wrapS";
static const std::string GLTF_LABEL_WRAP_T          = "wrapT";

static const std::string GLTF_PREFIX_ACCESSORS    =  "acc";
static const std::string GLTF_PREFIX_BUFFERS      = "buf";
static const std::string GLTF_PREFIX_BUFFER_VIEWS = "bufv";
static const std::string GLTF_PREFIX_TEXTURE      = "img";

static const std::string GLTF_SUFFIX_FACES = "f";
static const std::string GLTF_SUFFIX_NORMALS = "n";
static const std::string GLTF_SUFFIX_POSITION = "p";
static const std::string GLTF_SUFFIX_TEX_COORD = "uv";

static const uint32_t GLTF_PRIM_TYPE_TRIANGLE = 4;
static const uint32_t GLTF_PRIM_TYPE_ARRAY_BUFFER = 34962;
static const uint32_t GLTF_PRIM_TYPE_ELEMENT_ARRAY_BUFFER = 34963;

static const uint32_t GLTF_COMP_TYPE_USHORT = 5123;
static const uint32_t GLTF_COMP_TYPE_FLOAT  = 5126;
static const uint32_t GLTF_COMP_TYPE_FLOAT_VEC3 = 35665;
static const uint32_t GLTF_COMP_TYPE_FLOAT_VEC4 = 35666;
static const uint32_t GLTF_COMP_TYPE_FLOAT_MAT3 = 35675;
static const uint32_t GLTF_COMP_TYPE_FLOAT_MAT4 = 35676;

static const uint32_t GLTF_FILTER_TYPE_LINEAR                = 9729;
static const uint32_t GLTF_FILTER_TYPE_NEAREST_MIPMAP_LINEAR = 9987;
static const uint32_t GLTF_WRAP_MODE_REPEAT                  = 10497;

static const uint32_t GLTF_FORMAT_RGBA = 6408;
static const uint32_t GLTF_TYPE_TEXTURE_2D = 3553;
static const uint32_t GLTF_TYPE_UNSIGNED_BYTES = 5121;


static const uint32_t GLTF_SHADER_TYPE_FRAGMENT = 35632;
static const uint32_t GLTF_SHADER_TYPE_VERTEX   = 35633;

static const uint32_t GLTF_STATE_BLEND                    = 3042;
static const uint32_t GLTF_STATE_CULL_FACE                = 2884;
static const uint32_t GLTF_STATE_DEPTH_TEST               = 2929;
static const uint32_t GLTF_STATE_POLYGON_OFFSET_FILL      = 32823;
static const uint32_t GLTF_STATE_SAMPLE_ALPHA_TO_COVERAGE = 32926;
static const uint32_t GLTF_STATE_SCISSOR_TEST             = 3089;


static const std::string GLTF_ARRAY_BUFFER = "arraybuffer";
static const std::string GLTF_TYPE_SCALAR  = "SCALAR";
static const std::string GLTF_TYPE_VEC2    = "VEC2";
static const std::string GLTF_TYPE_VEC3    = "VEC3";

static const std::string GLTF_VERSION = "1.0";

//Default shader properties
static const std::string REPO_GLTF_DEFAULT_PROGRAM         = "default_program";
static const std::string REPO_GLTF_DEFAULT_SAMPLER         = "default_sampler";
static const std::string REPO_GLTF_DEFAULT_SHADER_FRAG     = "default_fshader";
static const std::string REPO_GLTF_DEFAULT_SHADER_FRAG_URI = "fragShader.glsl";
static const std::string REPO_GLTF_DEFAULT_SHADER_VERT     = "default_vshader";
static const std::string REPO_GLTF_DEFAULT_SHADER_VERT_URI = "vertShader.glsl";
static const std::string REPO_GLTF_DEFAULT_TECHNIQUE       = "default_technique";

static const std::string REPO_GLTF_LABEL_REF_ID = "refID";




GLTFModelExport::GLTFModelExport(
	const repo::core::model::RepoScene *scene
	) : AbstractModelExport()
	, scene(scene)
{
	if (convertSuccess = scene)
	{
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
	const std::string              &refId)
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
		startFaceIdx * sizeof(*faces.data()), 0, GLTF_COMP_TYPE_USHORT, 
		GLTF_TYPE_SCALAR, min, max, refId);
}

void GLTFModelExport::addAccessors(
	const std::string                  &accName,
	const std::string                  &buffViewName,
	repo::lib::PropertyTree            &tree,
	const std::vector<repo_vector2d_t> &buffer,
	const uint32_t                     &addrFrom,
	const uint32_t                     &addrTo,
	const std::string                  &refId)
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
		addrFrom * sizeof(*buffer.data()), sizeof(*buffer.data()),
		GLTF_COMP_TYPE_FLOAT, GLTF_TYPE_VEC2, min, max, refId);
}

void GLTFModelExport::addAccessors(
	const std::string                &accName,
	const std::string                &buffViewName,
	repo::lib::PropertyTree          &tree,
	const std::vector<repo_vector_t> &buffer,
	const uint32_t                   &addrFrom,
	const uint32_t                   &addrTo,
	const std::string                &refId)
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

		for (uint32_t i = addrFrom+1; i < addrTo; ++i)
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
		addrFrom * sizeof(*buffer.data()), sizeof(*buffer.data()), 
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
	const std::string              &refId)
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
	if (scene)
	{
		repo::core::model::RepoNode* root;
		if (root = scene->getRoot(repo::core::model::RepoScene::GraphType::OPTIMIZED))
		{
			gType = repo::core::model::RepoScene::GraphType::OPTIMIZED;
		}
		else if (root = scene->getRoot(repo::core::model::RepoScene::GraphType::DEFAULT))
		{
			gType = repo::core::model::RepoScene::GraphType::DEFAULT;
		}
		else
		{
			repoError << "Failed to export to glTF : Failed to find root node within the scene!";
			return false;
		}
			
		std::string sceneName = "defaultScene";
		tree.addToTree(GLTF_LABEL_SCENE, sceneName);
		std::vector<std::string> treeNodes = { UUIDtoString(root->getUniqueID()) };
		tree.addToTree(GLTF_LABEL_SCENES + ".defaultScene." + GLTF_LABEL_NODES, treeNodes);

		auto splitMeshes = populateWithMeshes(tree);
		populateWithNodes(tree, splitMeshes);
		populateWithMaterials(tree);
		populateWithTextures(tree);

	}
	else
	{
		return false;
	}
}


bool GLTFModelExport::generateTreeRepresentation()
{
	if (!scene)
	{
		//Sanity check, shouldn't be calling this function with nullptr anyway
		return false;
	}

	repo::lib::PropertyTree tree;

	std::stringstream ss;
	ss << "3D Repo Bouncer v" << BOUNCER_VMAJOR << "." << BOUNCER_VMINOR;
	tree.addToTree(GLTF_LABEL_ASSET + "." + GLTF_LABEL_GENERATOR, ss.str());
	tree.addToTree(GLTF_LABEL_ASSET + "." + GLTF_LABEL_VERSION  , GLTF_VERSION);
	//SHADER- Premultiplied alpha?

	constructScene(tree);
	writeBuffers(tree);

	trees["test"] = tree;

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
			//default technique takes on a 4d vector, we store 3d vectors
			matStruct.diffuse.push_back(1);
			tree.addToTree(valuesLabel + "." + GLTF_LABEL_DIFFUSE, matStruct.diffuse);
		}

		if (matStruct.emissive.size())
		{
			//default technique takes on a 4d vector, we store 3d vectors
			matStruct.emissive.push_back(1);
			tree.addToTree(valuesLabel + "." + GLTF_LABEL_EMISSIVE, matStruct.emissive);
		}
		if (matStruct.specular.size())
		{
			//default technique takes on a 4d vector, we store 3d vectors
			matStruct.specular.push_back(1);
			tree.addToTree(valuesLabel + "." + GLTF_LABEL_SPECULAR, matStruct.specular);
		}

		if (matStruct.shininess == matStruct.shininess)
		{
			tree.addToTree(valuesLabel + "." + GLTF_LABEL_SHININESS, matStruct.shininess);
		}

		if (matStruct.opacity == matStruct.opacity)
		{
			tree.addToTree(valuesLabel + "." + GLTF_LABEL_TRANSPARENCY, 1.0 - matStruct.opacity);
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

		auto normals = node->getNormals();
		auto vertices = node->getVertices();
		auto UVs = node->getUVChannelsSeparated();


		size_t vStart = addToDataBuffer(meshUUID, vertices);
		size_t nStart = addToDataBuffer(meshUUID, vertices);

		if (mappings.size() > 1)
		{
			//This is a multipart mesh node, the mesh may be too big for 
			//webGL, split the mesh into sub meshes

			std::vector<uint16_t> newFaces;
			std::vector<std::vector<float>> idMapBuf;
			std::unordered_map<repoUUID, std::vector<uint32_t>, RepoUUIDHasher> splitMap;
			std::vector<std::vector<repo_mesh_mapping_t>> matMap;

			repo::core::model::MeshNode splitMesh = node->cloneAndRemapMeshMapping(GLTF_MAX_VERTEX_LIMIT,
				newFaces,
				idMapBuf,
				splitMap,
				matMap);


			size_t fStart = addToDataBuffer(meshUUID, newFaces);
		
			auto newMappings = splitMesh.getMeshMapping();

			splitSizes[node->getUniqueID()] = newMappings.size();			

			std::string binFileName = meshUUID;
			for (size_t i = 0; i < newMappings.size(); ++i)
			{
				//every mapping is a mesh
				std::string meshId = meshUUID + "_" + std::to_string(i);
				std::string label = GLTF_LABEL_MESHES + "." + meshId;
				std::vector<repo::lib::PropertyTree> primitives;
				size_t count = 0;

				std::string faceBufferName = meshId + "_" + GLTF_SUFFIX_FACES;
				std::string normBufferName = meshId + "_" + GLTF_SUFFIX_NORMALS;
				std::string posBufferName = meshId + "_" + GLTF_SUFFIX_POSITION;

				size_t vCount = newMappings[i].vertTo - newMappings[i].vertFrom;
				size_t fCount = newMappings[i].triTo - newMappings[i].triFrom;

				//for each mesh we need to add a bufferView for each buffer
				addBufferView(normBufferName, meshUUID, tree, normals , nStart, vCount, meshId);
				addBufferView(posBufferName , meshUUID, tree, vertices, vStart, vCount, meshId);
				addBufferView(faceBufferName, meshUUID, tree, newFaces, fStart, fCount, meshId);

				for (size_t i = 0; i < UVs.size(); ++i)
				{
					size_t uvStart = addToDataBuffer(meshUUID, UVs[i]);
					std::string uvBufferName = meshId + "_" + GLTF_SUFFIX_TEX_COORD + "_" + std::to_string(i);
					addBufferView(uvBufferName, meshUUID, tree, UVs[i], uvStart, vCount, meshId);
				}

				for (const repo_mesh_mapping_t & meshMap : matMap[i])
				{
					std::string subMeshID = UUIDtoString(meshMap.mesh_id);

					primitives.push_back(repo::lib::PropertyTree());
					primitives.back().addToTree(GLTF_LABEL_MATERIAL, UUIDtoString(meshMap.material_id));
					primitives.back().addToTree(GLTF_LABEL_PRIMITIVE, GLTF_PRIM_TYPE_TRIANGLE);

					std::string subMeshName = meshId + "_m" + std::to_string(count++);

					/*
					  ===== NOTE =====
					  Generic viewers (like Cesium) expects the faces to be indexed in respective of
					  the accessor range of vertices, meaning we would have to remap the whole face indices
					  for multipart models, or have every multipart submeshes to have access to the full
					  vertex buffer.  The code under "cesiumHack" below would perform the latter, which works
					  on Cesium.
					*/


					if (newFaces.size())
					{
						std::string accessorName = subMeshName + "_" + GLTF_SUFFIX_FACES;
						primitives.back().addToTree(GLTF_LABEL_INDICES, GLTF_PREFIX_ACCESSORS + "_" + accessorName);
						addAccessors(accessorName, faceBufferName, tree, newFaces, meshMap.triFrom, meshMap.triTo, subMeshID);
					}

				
					if (normals.size())
					{
						std::string accessorName = subMeshName + "_" + GLTF_SUFFIX_NORMALS;
						primitives.back().addToTree(GLTF_LABEL_ATTRIBUTES + "." + GLTF_LABEL_NORMAL, GLTF_PREFIX_ACCESSORS + "_" + accessorName);
						if (cesiumHack)
							addAccessors(accessorName, posBufferName, tree, normals, 0, normals.size(), subMeshID);
						else
							addAccessors(accessorName, normBufferName, tree, normals, meshMap.vertFrom, meshMap.vertTo, subMeshID);
					}

					if (vertices.size())
					{
						std::string accessorName = subMeshName + "_" + GLTF_SUFFIX_POSITION;
						primitives.back().addToTree(GLTF_LABEL_ATTRIBUTES + "." + GLTF_LABEL_POSITION, GLTF_PREFIX_ACCESSORS + "_" + accessorName);
						if (cesiumHack)
							addAccessors(accessorName, posBufferName, tree, vertices, 0, vertices.size(), subMeshID);
						else
							addAccessors(accessorName, posBufferName, tree, vertices, meshMap.vertFrom, meshMap.vertTo, subMeshID);					
					}


					if (UVs.size())
					{
						for (uint32_t iUV = 0; iUV < UVs.size(); ++iUV)
						{
							std::string accessorName = subMeshName + "_" + GLTF_SUFFIX_TEX_COORD + "_" + std::to_string(iUV);
							std::string uvBufferName = meshUUID + "_" + GLTF_SUFFIX_TEX_COORD + "_" + std::to_string(iUV);
							primitives.back().addToTree(GLTF_LABEL_ATTRIBUTES + "." + GLTF_LABEL_TEXCOORD + "_" + std::to_string(iUV),
								GLTF_PREFIX_ACCESSORS + "_" + accessorName);							
							if (cesiumHack)
								addAccessors(accessorName, posBufferName, tree, UVs[iUV], 0, UVs[iUV].size(), subMeshID);
							else
								addAccessors(accessorName, posBufferName, tree, UVs[iUV], meshMap.vertFrom, meshMap.vertTo, subMeshID);
						}
					}
					primitives.back().addToTree(GLTF_LABEL_EXTRA + "." + REPO_GLTF_LABEL_REF_ID, subMeshID);

				}
				tree.addArrayObjects(label + "." + GLTF_LABEL_PRIMITIVES, primitives);

			}
		}
		else
		{

			bool hasMapping = mappings.size();
			std::string meshId = hasMapping ? UUIDtoString(mappings[0].mesh_id) : UUIDtoString(node->getUniqueID());
			std::string label = GLTF_LABEL_MESHES + "." + UUIDtoString(node->getUniqueID());
			std::string name = node->getName();
			if (!name.empty())
				tree.addToTree(label + "." + GLTF_LABEL_NAME, node->getName());

			auto faces = node->getFaces();
			std::vector<uint16_t> sFaces = serialiseFaces(faces);
			size_t fStart = addToDataBuffer(meshUUID, sFaces);

			std::string faceBufferName = meshId + "_" + GLTF_SUFFIX_FACES;
			std::string normBufferName = meshId + "_" + GLTF_SUFFIX_NORMALS;
			std::string posBufferName = meshId + "_" + GLTF_SUFFIX_POSITION;

			//for each mesh we need to add a bufferView for each buffer
			addBufferView(normBufferName, meshUUID, tree, normals, nStart, normals.size(), meshId);
			addBufferView(posBufferName, meshUUID, tree, vertices, vStart, vertices.size(), meshId);
			addBufferView(faceBufferName, meshUUID, tree, sFaces, fStart, faces.size(), meshId);

			for (size_t i = 0; i < UVs.size(); ++i)
			{
				size_t uvStart = addToDataBuffer(meshUUID, UVs[i]);
				std::string uvBufferName = meshId + "_" + GLTF_SUFFIX_TEX_COORD + "_" + std::to_string(i);
				addBufferView(uvBufferName, meshUUID, tree, UVs[i], uvStart, UVs[i].size(), meshId);
			}


			std::vector<repo::lib::PropertyTree> primitives;
			primitives.push_back(repo::lib::PropertyTree());

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

			

			std::string binFileName = UUIDtoString(scene->getRevisionID());
			
			if (hasMat)
			{
				primitives[0].addToTree(GLTF_LABEL_MATERIAL, UUIDtoString(matID));
				primitives[0].addToTree(GLTF_LABEL_PRIMITIVE, GLTF_PRIM_TYPE_TRIANGLE);

				if (faces.size())
				{
					std::string bufferName = meshId + "_" + GLTF_SUFFIX_FACES;
					primitives[0].addToTree(GLTF_LABEL_INDICES, GLTF_PREFIX_ACCESSORS + "_" + bufferName);
					addAccessors(bufferName, bufferName, tree, sFaces, 0, faces.size(), meshId);
				}

				//attributes
				auto normals = node->getNormals();
				if (normals.size())
				{
					std::string bufferName = meshId + "_" + GLTF_SUFFIX_NORMALS;
					primitives[0].addToTree(GLTF_LABEL_ATTRIBUTES + "." + GLTF_LABEL_NORMAL, GLTF_PREFIX_ACCESSORS + "_" + bufferName);
					addAccessors(bufferName, bufferName, tree, normals, 0, normals.size(), meshId);
				}
				auto vertices = node->getVertices();
				if (vertices.size())
				{
					std::string bufferName = meshId + "_" + GLTF_SUFFIX_POSITION;
					primitives[0].addToTree(GLTF_LABEL_ATTRIBUTES + "." + GLTF_LABEL_POSITION, GLTF_PREFIX_ACCESSORS + "_" + bufferName);
					addAccessors(bufferName, bufferName, tree, vertices, 0, vertices.size(), meshId);
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
			tree.addToTree(label + "." + GLTF_LABEL_MATRIX, transNode->getTransMatrix());
		}
		processNodeChildren(node, tree, subMeshCounts);
	}	
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
		std::string bufferLabel = GLTF_LABEL_BUFFERS + "." + pair.first;
		tree.addToTree(bufferLabel + "." + GLTF_LABEL_BYTE_LENGTH, pair.second.size()  * sizeof(*pair.second.data()));
		tree.addToTree(bufferLabel + "." + GLTF_LABEL_TYPE, GLTF_ARRAY_BUFFER);
		tree.addToTree(bufferLabel + "." + GLTF_LABEL_URI, pair.first + ".bin");
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
	const std::string paramlabel = label + "." +  GLTF_LABEL_PARAMETERS;

	tree.addToTree(paramlabel + ".modelViewMatrix." + GLTF_LABEL_SEMANTIC, "MODELVIEW");
	tree.addToTree(paramlabel + ".modelViewMatrix." + GLTF_LABEL_TYPE, GLTF_COMP_TYPE_FLOAT_MAT4);

	tree.addToTree(paramlabel + ".normal." + GLTF_LABEL_SEMANTIC, GLTF_LABEL_NORMAL);
	tree.addToTree(paramlabel + ".normal." + GLTF_LABEL_TYPE, GLTF_COMP_TYPE_FLOAT_VEC3);

	tree.addToTree(paramlabel + ".normalMatrix." + GLTF_LABEL_SEMANTIC, "MODELVIEWINVERSETRANSPOSE");
	tree.addToTree(paramlabel + ".normalMatrix." + GLTF_LABEL_TYPE, GLTF_COMP_TYPE_FLOAT_MAT3);

	tree.addToTree(paramlabel + ".position." + GLTF_LABEL_SEMANTIC, GLTF_LABEL_POSITION);
	tree.addToTree(paramlabel + ".position." + GLTF_LABEL_TYPE, GLTF_COMP_TYPE_FLOAT_VEC3);

	tree.addToTree(paramlabel + ".projectionMatrix." + GLTF_LABEL_SEMANTIC, "PROJECTION");
	tree.addToTree(paramlabel + ".projectionMatrix." + GLTF_LABEL_TYPE, GLTF_COMP_TYPE_FLOAT_MAT4);

	tree.addToTree(paramlabel + "." + GLTF_LABEL_SHININESS + "." + GLTF_LABEL_TYPE, GLTF_COMP_TYPE_FLOAT);
	tree.addToTree(paramlabel + "." + GLTF_LABEL_DIFFUSE + "."  + GLTF_LABEL_TYPE , GLTF_COMP_TYPE_FLOAT_VEC4);
	tree.addToTree(paramlabel + "." + GLTF_LABEL_SPECULAR + "." + GLTF_LABEL_TYPE , GLTF_COMP_TYPE_FLOAT_VEC4);

	tree.addToTree(label + "." + GLTF_LABEL_PROGRAM, REPO_GLTF_DEFAULT_PROGRAM);

	const std::string stateslabel = label + "." + GLTF_LABEL_STATES;
	std::vector<uint32_t> states = { GLTF_STATE_DEPTH_TEST, GLTF_STATE_CULL_FACE };
	tree.addToTree(stateslabel + "." + GLTF_LABEL_ENABLE, states);

	const std::string uniformLabel = label + "." +  GLTF_LABEL_UNIFORMS;

	tree.addToTree(uniformLabel + ".u_diffuse"         , GLTF_LABEL_DIFFUSE);
	tree.addToTree(uniformLabel + ".u_modelViewMatrix" , "modelViewMatrix");
	tree.addToTree(uniformLabel + ".u_normalMatrix"    , "normalMatrix");
	tree.addToTree(uniformLabel + ".u_projectionMatrix", "projectionMatrix");
	tree.addToTree(uniformLabel + ".u_shininess"       , GLTF_LABEL_SHININESS);
	tree.addToTree(uniformLabel + ".u_specular"        , GLTF_LABEL_SPECULAR);

	const std::string attriLabel = label + "." + GLTF_LABEL_ATTRIBUTES;
	tree.addToTree(attriLabel + ".a_normal", "normal");
	tree.addToTree(attriLabel + ".a_position", "position");

	//========== DEFAULT PROGRAM =========
	const std::string programLabel = GLTF_LABEL_PROGRAMS + "." + REPO_GLTF_DEFAULT_PROGRAM;
	std::vector<std::string> programAttributes = { "a_normal", "a_position" };
	tree.addToTree(programLabel + "." + GLTF_LABEL_ATTRIBUTES , programAttributes);
	tree.addToTree(programLabel + "." + GLTF_LABEL_SHADER_FRAG, REPO_GLTF_DEFAULT_SHADER_FRAG);
	tree.addToTree(programLabel + "." + GLTF_LABEL_SHADER_VERT, REPO_GLTF_DEFAULT_SHADER_VERT);
	
	//========== DEFAULT SHADERS =========
	tree.addToTree(GLTF_LABEL_SHADERS + "." + REPO_GLTF_DEFAULT_SHADER_FRAG + "." + GLTF_LABEL_TYPE, GLTF_SHADER_TYPE_FRAGMENT);
	tree.addToTree(GLTF_LABEL_SHADERS + "." + REPO_GLTF_DEFAULT_SHADER_FRAG + "." + GLTF_LABEL_URI, REPO_GLTF_DEFAULT_SHADER_FRAG_URI);

	tree.addToTree(GLTF_LABEL_SHADERS + "." + REPO_GLTF_DEFAULT_SHADER_VERT + "." + GLTF_LABEL_TYPE, GLTF_SHADER_TYPE_VERTEX);
	tree.addToTree(GLTF_LABEL_SHADERS + "." + REPO_GLTF_DEFAULT_SHADER_VERT + "." + GLTF_LABEL_URI , REPO_GLTF_DEFAULT_SHADER_VERT_URI);


}

void GLTFModelExport::debug() const
{
	//temporary function to debug gltf. to remove once done

	std::stringstream ss;
	std::string fname = "test";
	auto it = trees.find(fname);
	it->second.write_json(ss);
	
	std::string jsonFile = ss.str();

	std::string dir = "D:\\gltfTest\\redBox\\";
	std::string filePath = dir + "redBox.gltf";
	FILE *fp = fopen(filePath.c_str(), "w");
	fwrite(jsonFile.c_str(), 1, jsonFile.size(), fp);
	fclose(fp);

	auto it2 = fullDataBuffer.begin();
	
	filePath = dir + it2->first + ".bin";
	fopen(filePath.c_str(), "wb");
	std::vector<uint8_t> buffer = it2->second;
	fwrite(it2->second.data(), 1,it2->second.size(), fp);
	fclose(fp);
}