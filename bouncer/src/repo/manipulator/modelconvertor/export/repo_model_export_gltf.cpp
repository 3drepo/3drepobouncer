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

static const std::string GLTF_LABEL_CAMERAS = "cameras";
static const std::string GLTF_LABEL_CHILDREN = "children";
static const std::string GLTF_LABEL_MATRIX = "matrix";
static const std::string GLTF_LABEL_MESHES = "meshes";
static const std::string GLTF_LABEL_NAME = "name";
static const std::string GLTF_LABEL_NODES = "nodes";
static const std::string GLTF_LABEL_SCENE = "scene";
static const std::string GLTF_LABEL_SCENES = "scenes";


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
			
		tree.addToTree(GLTF_LABEL_SCENE, "defaultScene");
		std::vector<std::string> treeNodes = { UUIDtoString(root->getUniqueID()) };
		tree.addToTree(GLTF_LABEL_SCENES + ".defaultScene." + GLTF_LABEL_NODES, treeNodes);

		populateWithNode(root, tree);

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
	constructScene(tree);
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