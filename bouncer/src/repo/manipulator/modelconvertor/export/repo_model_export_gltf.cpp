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

static const std::string GLTF_LABEL_ACCESSORS    = "accessors";
static const std::string GLTF_LABEL_ASSET        = "asset";
static const std::string GLTF_LABEL_ATTRIBUTES   = "attributes";
static const std::string GLTF_LABEL_BUFFER       = "buffer";
static const std::string GLTF_LABEL_BUFFERS      = "buffers";
static const std::string GLTF_LABEL_BUFFER_VIEW  = "bufferView";
static const std::string GLTF_LABEL_BUFFER_VIEWS = "bufferViews";
static const std::string GLTF_LABEL_BYTE_LENGTH  = "byteLength";
static const std::string GLTF_LABEL_BYTE_OFFSET  = "byteOffset";
static const std::string GLTF_LABEL_BYTE_STRIDE  = "byteStride";
static const std::string GLTF_LABEL_CAMERAS      = "cameras";
static const std::string GLTF_LABEL_CHILDREN     = "children";
static const std::string GLTF_LABEL_COMP_TYPE    = "componentType";
static const std::string GLTF_LABEL_COUNT        = "count";
static const std::string GLTF_LABEL_GENERATOR    = "generator";
static const std::string GLTF_LABEL_INDICES      = "indices";
static const std::string GLTF_LABEL_MATERIAL     = "material";
static const std::string GLTF_LABEL_MATRIX       = "matrix";
static const std::string GLTF_LABEL_MESHES       = "meshes";
static const std::string GLTF_LABEL_NAME         = "name";
static const std::string GLTF_LABEL_NODES        = "nodes";
static const std::string GLTF_LABEL_NORMAL       = "NORMAL";
static const std::string GLTF_LABEL_POSITION     = "POSITION";
static const std::string GLTF_LABEL_TEXCOORD     = "TEXCOORD";
static const std::string GLTF_LABEL_PRIMITIVE    = "primitive";
static const std::string GLTF_LABEL_PRIMITIVES   = "primitives";
static const std::string GLTF_LABEL_SCENE        = "scene";
static const std::string GLTF_LABEL_SCENES       = "scenes";
static const std::string GLTF_LABEL_TARGET       = "target";
static const std::string GLTF_LABEL_TYPE         = "type";
static const std::string GLTF_LABEL_URI          = "uri";
static const std::string GLTF_LABEL_VERSION      = "version";

static const std::string GLTF_PREFIX_ACCESSORS   =  "acc";
static const std::string GLTF_PREFIX_BUFFERS      = "buf";
static const std::string GLTF_PREFIX_BUFFER_VIEWS = "bufv";

static const std::string GLTF_SUFFIX_FACES = "f";
static const std::string GLTF_SUFFIX_NORMALS = "n";
static const std::string GLTF_SUFFIX_POSITION = "p";
static const std::string GLTF_SUFFIX_TEX_COORD = "uv";

static const uint32_t GLTF_PRIM_TYPE_TRIANGLE = 4;
static const uint32_t GLTF_PRIM_TYPE_ARRAY_BUFFER = 34962;
static const uint32_t GLTF_PRIM_TYPE_ELEMENT_ARRAY_BUFFER = 34963;

static const uint32_t GLTF_COMP_TYPE_USHORT = 5123;
static const uint32_t GLTF_COMP_TYPE_FLOAT  = 5126;

static const std::string GLTF_ARRAY_BUFFER = "arraybuffer";
static const std::string GLTF_TYPE_SCALAR  = "SCALAR";
static const std::string GLTF_TYPE_VEC2    = "VEC2";
static const std::string GLTF_TYPE_VEC3    = "VEC3";

static const std::string GLTF_VERSION = "1.0";



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

void GLTFModelExport::addBuffer(
	const std::string              &name,
	const std::string              &fileName,
	repo::lib::PropertyTree        &tree,
	const std::vector<uint16_t>    &buffer)
{
	addBuffer(name, fileName, tree, (uint8_t*)buffer.data(), buffer.size(), buffer.size() * sizeof(*buffer.data()), 
		0, GLTF_PRIM_TYPE_ELEMENT_ARRAY_BUFFER, GLTF_COMP_TYPE_USHORT, GLTF_TYPE_SCALAR);
}

void GLTFModelExport::addBuffer(
	const std::string                   &name,
	const std::string                   &fileName,
	repo::lib::PropertyTree             &tree,
	const std::vector<repo_vector_t>    &buffer)
{
	addBuffer(name, fileName, tree, (uint8_t*)buffer.data(), buffer.size(), buffer.size() * sizeof(*buffer.data()),
		sizeof(repo_vector_t), GLTF_PRIM_TYPE_ARRAY_BUFFER, GLTF_COMP_TYPE_FLOAT, GLTF_TYPE_VEC3);
}

void GLTFModelExport::addBuffer(
	const std::string                   &name,
	const std::string                   &fileName,
	repo::lib::PropertyTree             &tree,
	const std::vector<repo_vector2d_t>    &buffer)
{
	addBuffer(name, fileName, tree, (uint8_t*)buffer.data(), buffer.size(), buffer.size() * sizeof(*buffer.data()),
		sizeof(repo_vector2d_t), GLTF_PRIM_TYPE_ARRAY_BUFFER, GLTF_COMP_TYPE_FLOAT, GLTF_TYPE_VEC2);
}


void GLTFModelExport::addBuffer(
	const std::string              &name,
	const std::string              &fileName,
	repo::lib::PropertyTree        &tree,
	const uint8_t                  *buffer,
	const size_t                   &count,
	const size_t                   &byteLength,
	const size_t                   &stride,
	const uint32_t                 &bufferTarget,
	const uint32_t                 &componentType,
	const std::string              &bufferType)
{
	std::string bufferName = GLTF_PREFIX_BUFFERS + "_" + name;
	std::string bufferViewName = GLTF_PREFIX_BUFFER_VIEWS + "_" + name;	

	auto mapIt = fullDataBuffer.find(fileName);

	if (mapIt == fullDataBuffer.end())
	{
		//current no such buffer, create one
		fullDataBuffer[fileName] = std::vector<uint8_t>();
	}
	//Append buffer into the dataBuffer
	size_t offset = fullDataBuffer.size();
	fullDataBuffer[fileName].resize(offset + byteLength);
	memcpy(&fullDataBuffer[fileName][offset], buffer, byteLength);

	//declare buffer view
	std::string bufferViewLabel = GLTF_LABEL_BUFFER_VIEWS + "." + bufferViewName;
	tree.addToTree(bufferViewLabel + "." + GLTF_LABEL_BUFFER, bufferName);
	tree.addToTree(bufferViewLabel + "." + GLTF_LABEL_BYTE_LENGTH, byteLength);
	tree.addToTree(bufferViewLabel + "." + GLTF_LABEL_BYTE_OFFSET, offset);
	tree.addToTree(bufferViewLabel + "." + GLTF_LABEL_TARGET, bufferTarget); //if it is a uint16_t its an indices buffer...?

	//declare accessor
	std::string accLabel = GLTF_LABEL_ACCESSORS + "." + GLTF_PREFIX_ACCESSORS + "_" + name;
	tree.addToTree(accLabel + "." + GLTF_LABEL_BUFFER_VIEW, bufferViewName);
	tree.addToTree(accLabel + "." + GLTF_LABEL_BYTE_OFFSET, 0);
	tree.addToTree(accLabel + "." + GLTF_LABEL_BYTE_STRIDE, stride);
	tree.addToTree(accLabel + "." + GLTF_LABEL_COMP_TYPE, componentType);
	tree.addToTree(accLabel + "." + GLTF_LABEL_COUNT, count);
	tree.addToTree(accLabel + "." + GLTF_LABEL_TYPE, bufferType);

}

void GLTFModelExport::addFaceBuffer(
	const std::string              &name,
	const std::string              &fileName,
	repo::lib::PropertyTree        &tree,
	const std::vector<repo_face_t> &faces)
{
	//We first need to serialise this then pass it into the generic add buffer function
	//Faces should also be uint16_t
	std::vector<uint16_t> serialisedFaces;
	serialisedFaces.reserve(faces.size() * 3); //faces should be triangulated

	for (uint32_t i = 0; i < faces.size(); ++i)
	{
		if (faces[i].size() == 3)
		{
			serialisedFaces.push_back(faces[i][0]);
			serialisedFaces.push_back(faces[i][1]);
			serialisedFaces.push_back(faces[i][2]);
		}
		else
		{
			repoError << "Error during glTF export: found non triangulated face. This may not visualise correctly.";
		}		
	}

	addBuffer(name, fileName, tree, serialisedFaces);
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

		populateWithNode(root, tree);

		populateWithMeshes(tree);

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
	const repo::core::model::RepoNode *node,
	repo::lib::PropertyTree          &tree
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
			meshes.push_back(UUIDtoString(child->getUniqueID()));
			break;
		case repo::core::model::NodeType::TRANSFORMATION:
			trans.push_back(UUIDtoString(child->getUniqueID())); 
			//Recursive call to construct transformation node
			populateWithNode(child, tree);
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

void GLTFModelExport::populateWithMeshes(
	repo::lib::PropertyTree           &tree)
{
	repo::core::model::RepoNodeSet meshes = scene->getAllMeshes(gType);

	for (const auto &mesh : meshes)
	{
		const repo::core::model::MeshNode *node = (const repo::core::model::MeshNode *)mesh;
		std::string meshId = UUIDtoString(node->getUniqueID());
		std::string label = GLTF_LABEL_MESHES + "." + meshId;
		std::string name = node->getName();
		if (!name.empty())
			tree.addToTree(label + "." + GLTF_LABEL_NAME, node->getName());

		std::vector<repo::lib::PropertyTree> primitives;
		primitives.push_back(repo::lib::PropertyTree());


		auto children = scene->getChildrenNodesFiltered(gType, node->getSharedID(), repo::core::model::NodeType::MATERIAL);

		std::string binFileName = UUIDtoString(scene->getRevisionID()) + ".bin";
		//TODO: need multiple primitives for multiple materials, check for subMeshes
		if (children.size())
		{
			primitives[0].addToTree(GLTF_LABEL_MATERIAL, UUIDtoString(children[0]->getUniqueID()));	
			primitives[0].addToTree(GLTF_LABEL_PRIMITIVE, GLTF_PRIM_TYPE_TRIANGLE);
			auto faces = node->getFaces();
			if (faces.size())
			{
				std::string bufferName = meshId + "_" + GLTF_SUFFIX_FACES;
				primitives[0].addToTree(GLTF_LABEL_INDICES, GLTF_PREFIX_ACCESSORS + "_" + bufferName);
				//FIXME: fill in filename properly
				addFaceBuffer(bufferName, binFileName, tree, faces);
			}
				
			std::string accessorPrefix = GLTF_PREFIX_ACCESSORS + "_" + meshId;
			//attributes
			auto normals = node->getNormals();
			if (normals.size())
			{
				std::string bufferName = meshId + "_" + GLTF_SUFFIX_NORMALS;
				primitives[0].addToTree(GLTF_LABEL_ATTRIBUTES + "." + GLTF_LABEL_NORMAL, GLTF_PREFIX_ACCESSORS + "_" + bufferName);
				addBuffer(bufferName, binFileName, tree, normals);
			}
			auto vertices = node->getVertices();
			if (vertices.size())
			{
				std::string bufferName = meshId + "_" + GLTF_SUFFIX_POSITION;
				primitives[0].addToTree(GLTF_LABEL_ATTRIBUTES + "." + GLTF_LABEL_POSITION, GLTF_PREFIX_ACCESSORS + "_" + bufferName);
				addBuffer(bufferName, binFileName, tree, vertices);
			}
			
			auto UVs = node->getUVChannelsSeparated();
			if (UVs.size())
			{
				for (uint32_t i = 0; i < UVs.size(); ++i)
				{
					std::string bufferName = meshId + "_" + GLTF_SUFFIX_TEX_COORD + "_" + std::to_string(i);
					primitives[0].addToTree(GLTF_LABEL_ATTRIBUTES + "." + GLTF_LABEL_TEXCOORD + "_" + std::to_string(i), 
						GLTF_PREFIX_ACCESSORS + "_" + bufferName);
					addBuffer(bufferName, binFileName, tree, UVs[i]);
				}
			}

		}

		tree.addArrayObjects(label + "." + GLTF_LABEL_PRIMITIVES, primitives);

	}

	
	
}

void GLTFModelExport::populateWithNode(
	const repo::core::model::RepoNode* node,
	repo::lib::PropertyTree          &tree)
{
	if (node)
	{
		if (node->getTypeAsEnum() == repo::core::model::NodeType::TRANSFORMATION)
		{
			//add to list of nodes
			std::string label = GLTF_LABEL_NODES + "." + UUIDtoString(node->getUniqueID());
			std::string name = node->getName();
			if (!name.empty())
				tree.addToTree(label+ "." + GLTF_LABEL_NAME, node->getName());
			const repo::core::model::TransformationNode *transNode = (const repo::core::model::TransformationNode*) node;
			if (!transNode->isIdentity())
			{
				tree.addToTree(label + "." + GLTF_LABEL_MATRIX, transNode->getTransMatrix());
			}
			processNodeChildren(node, tree);
		}
		else
		{
			//Not expecting anything but transformation to be passed into this function
			repoWarning << "Trying to create a node that is not of type transformation! (node type: " 
				<< node->getType() << ")";
		}
	}
}

void GLTFModelExport::writeBuffers(
	repo::lib::PropertyTree &tree)
{
	for (const auto &pair : fullDataBuffer)
	{
		std::string bufferLabel = GLTF_LABEL_BUFFERS + "." + pair.first;
		tree.addToTree(bufferLabel + "." + GLTF_LABEL_BYTE_LENGTH, pair.second.size()  * sizeof(*pair.second.data()));
		tree.addToTree(bufferLabel + "." + GLTF_LABEL_TYPE, GLTF_ARRAY_BUFFER);
		tree.addToTree(bufferLabel + "." + GLTF_LABEL_URI, pair.first);
	}
}

void GLTFModelExport::debug() const
{
	//temporary function to debug gltf. to remove once done

	std::stringstream ss;
	std::string fname = "test";
	auto it = trees.find(fname);
	if (it != trees.end())
	{
		it->second.write_json(ss);
		repoDebug << ss.str();
	}
}