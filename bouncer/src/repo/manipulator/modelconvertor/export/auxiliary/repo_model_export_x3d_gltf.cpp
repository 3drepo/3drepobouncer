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

#include "repo_model_export_x3d_gltf.h"
#include "x3dom_constants.h"
#include "../../../../lib/repo_log.h"
#include "../../../../core/model/bson/repo_bson_factory.h"

using namespace repo::manipulator::modelconvertor;

X3DGLTFModelExport::X3DGLTFModelExport(
	const repo::core::model::RepoScene *scene
	) : X3DModelExport(scene)
{	
}

X3DGLTFModelExport::X3DGLTFModelExport(
	const repo::core::model::MeshNode &mesh,
	const repo::core::model::RepoScene *scene
	)
	: X3DModelExport(mesh, scene)
{
}

std::string X3DGLTFModelExport::populateTreeWithProperties(
	const repo::core::model::RepoNode  *node,
	const repo::core::model::RepoScene *scene,
	repo::lib::PropertyTree            &tree
	)
{
	std::string label;
	bool stopRecursing = false;
	if (node)
	{
		switch (node->getTypeAsEnum())
		{		
		case repo::core::model::NodeType::MAP:
		{
			const repo::core::model::MapNode *mapNode = (const repo::core::model::MapNode *)node;
			label = X3D_LABEL_GROUP;
			std::string mapType = mapNode->getMapType();
			if (mapType.empty())
				mapType = "satellite";

			tree.addFieldAttribute("", X3D_ATTR_ID, UUIDtoString(mapNode->getUniqueID()));
			tree.addFieldAttribute("", X3D_ATTR_DEF, UUIDtoString(mapNode->getSharedID()));
			
			auto subTree = createGoogleMapSubTree(mapNode);
			tree.mergeSubTree(X3D_LABEL_TRANS, subTree);
			stopRecursing = true;
		}
		break;	
		case repo::core::model::NodeType::REFERENCE:
		{
			label = X3D_LABEL_INLINE;
			const repo::core::model::ReferenceNode *refNode = (const repo::core::model::ReferenceNode *)node;
			std::string revisionId = UUIDtoString(refNode->getRevisionID());
			std::string url = "/api/" + refNode->getDatabaseName() + "/" + refNode->getProjectName() + "/revision/";
			if (revisionId == REPO_HISTORY_MASTER_BRANCH)
			{
				//Load head of master
				url += "master/head";
			}
			else
			{
				//load specific revision
				url += revisionId;
			}

			url += ".x3d.mp";

			tree.addFieldAttribute("", X3D_ATTR_ON_LOAD  , X3D_ON_LOAD);
			tree.addFieldAttribute("", X3D_ATTR_URL      , url);
			tree.addFieldAttribute("", X3D_ATTR_ID       , UUIDtoString(refNode->getUniqueID()));
			tree.addFieldAttribute("", X3D_ATTR_DEF      , UUIDtoString(refNode->getSharedID()));
			tree.addFieldAttribute("", X3D_ATTR_NAMESPACE, refNode->getDatabaseName() + "__" + refNode->getProjectName());

			//FIXME: Bounding box on reference nodes?
		}
		break;
		case repo::core::model::NodeType::TRANSFORMATION:
		{
			const repo::core::model::TransformationNode *transNode = (const repo::core::model::TransformationNode *)node;
			if (transNode->isIdentity())
			{
				label = X3D_LABEL_GROUP;
			}
			else
			{
				label = X3D_LABEL_MAT_TRANS;
				tree.addFieldAttribute("", X3D_ATTR_MAT, transNode->getTransMatrix());
			}

			tree.addFieldAttribute("", X3D_ATTR_ID,  UUIDtoString(transNode->getUniqueID()));
			tree.addFieldAttribute("", X3D_ATTR_DEF, UUIDtoString(transNode->getSharedID()));
		}
		break;
		default:
			repoError << "Unsupported node type: " << node->getType();
		}
	}
	if (!stopRecursing)
	{
		//Get subtrees of children
		std::vector<repo::core::model::RepoNode*> children = scene->getChildrenAsNodes(gType, node->getSharedID());
		for (const auto & child : children)
		{
			repo::lib::PropertyTree childTree(false);
			std::string childLabel = populateTreeWithProperties(child, scene, childTree);
			tree.mergeSubTree(childLabel, childTree);
		}
	}
	return label;
}

bool X3DGLTFModelExport::writeScene(
	const repo::core::model::RepoScene *scene)
{
	//if this is a reference graph, follow the status quo
	//otherwise, just put a gltf tag for external scene information
	//This will break down if we have a mix scene/ref scene situation, which
	//is currently not expected

	if (scene->getAllReferences(gType).size())
	{
		//This is a reference scene graph
		if (scene->getAllMeshes(gType).size())
		{
			repoError << "A reference scene with meshes is currently not supported";
			return false;
		}

		return X3DModelExport::writeScene(scene);
	}
	else
	{
		//normal graphs, return a gltf dom
		repo::lib::PropertyTree gltfST(false), sceneST(false);

		std::string sceneLabel = X3D_LABEL + "." + X3D_LABEL_SCENE;

		//Set Scene Attributes
		sceneST.addFieldAttribute("", X3D_ATTR_ID, "scene");
		sceneST.addFieldAttribute("", X3D_ATTR_DO_PICK_PASS, "false");

		std::string gltfURL = "/api/" + scene->getDatabaseName() + "/" 
			+ scene->getProjectName() + "/" + UUIDtoString(scene->getRevisionID()) + ".gltf";
		gltfST.addFieldAttribute("", X3D_ATTR_URL, gltfURL);
		
		sceneST.mergeSubTree(X3D_LABEL_GLTF, gltfST);

		tree.mergeSubTree(sceneLabel, sceneST);

		return true;
	}
}

bool X3DGLTFModelExport::writeMultiPartMeshAsScene(
	const repo::core::model::MeshNode &mesh,
	const repo::core::model::RepoScene *scene)
{
	//currently not supported
	return false;
}