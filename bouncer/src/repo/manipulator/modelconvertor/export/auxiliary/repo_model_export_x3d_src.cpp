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

#include "repo_model_export_x3d_src.h"
#include "x3dom_constants.h"
#include "../../../../lib/repo_log.h"
#include "../../../../core/model/bson/repo_bson_factory.h"

using namespace repo::manipulator::modelconvertor;

X3DSRCModelExport::X3DSRCModelExport(
	const repo::core::model::RepoScene *scene
	) : AbstractX3DModelExport(scene)
{	
}

X3DSRCModelExport::X3DSRCModelExport(
	const repo::core::model::MeshNode &mesh,
	const repo::core::model::RepoScene *scene
	)
	: AbstractX3DModelExport(mesh, scene)
{
}

std::string X3DSRCModelExport::populateTreeWithProperties(
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

		case repo::core::model::NodeType::CAMERA:
		{
			label = X3D_LABEL_VIEWPOINT;
			const repo::core::model::CameraNode *camNode = (const repo::core::model::CameraNode *)node;

			repo_vector_t camPos = camNode->getPosition();
			repo_vector_t camlookAt = camNode->getLookAt();

			tree.addFieldAttribute("", X3D_ATTR_ID, camNode->getName());
			tree.addFieldAttribute("", X3D_ATTR_DEF, UUIDtoString(camNode->getSharedID()));
			tree.addFieldAttribute("", X3D_ATTR_BIND, false);
			tree.addFieldAttribute("", X3D_ATTR_FOV, X3D_DEFAULT_FOV);
			tree.addFieldAttribute("", X3D_ATTR_POS, camPos);
			tree.addFieldAttribute("", X3D_ATTR_ZNEAR, -1);
			tree.addFieldAttribute("", X3D_ATTR_ZFAR, -1);

			repo_vector_t centre = { camPos.x + camlookAt.x, camPos.y + camlookAt.y, camPos.z + camlookAt.z };

			tree.addFieldAttribute("", X3D_ATTR_ROT_CENTRE, centre);
			tree.addFieldAttribute("", X3D_ATTR_ORIENTATION, camNode->getOrientation());

		}
		break;
		case repo::core::model::NodeType::MATERIAL:
		{
			const repo::core::model::MaterialNode *matNode = (const repo::core::model::MaterialNode *)node;
			label = X3D_LABEL_APP;

			repo_material_t matStruct = matNode->getMaterialStruct();

			if (matStruct.diffuse.size() > 0)
			{
				tree.addFieldAttribute(X3D_LABEL_TWOSIDEMAT, X3D_ATTR_COL_DIFFUSE, matStruct.diffuse, false);
				if (matStruct.isTwoSided)
					tree.addFieldAttribute(X3D_LABEL_TWOSIDEMAT, X3D_ATTR_COL_BK_DIFFUSE, matStruct.diffuse, false);

			}

			if (matStruct.emissive.size() > 0)
			{
				tree.addFieldAttribute(X3D_LABEL_TWOSIDEMAT, X3D_ATTR_COL_EMISSIVE, matStruct.emissive, false);
				if (matStruct.isTwoSided)
					tree.addFieldAttribute(X3D_LABEL_TWOSIDEMAT, X3D_ATTR_COL_BK_EMISSIVE, matStruct.emissive, false);

			}

			if (matStruct.shininess == matStruct.shininess)
			{
				tree.addFieldAttribute(X3D_LABEL_TWOSIDEMAT, X3D_ATTR_SHININESS, matStruct.shininess);
				if (matStruct.isTwoSided)
					tree.addFieldAttribute(X3D_LABEL_TWOSIDEMAT, X3D_ATTR_BK_SHININESS, matStruct.shininess);

			}

			if (matStruct.specular.size() > 0)
			{
				tree.addFieldAttribute(X3D_LABEL_TWOSIDEMAT, X3D_ATTR_COL_SPECULAR, matStruct.specular, false);
				if (matStruct.isTwoSided)
					tree.addFieldAttribute(X3D_LABEL_TWOSIDEMAT, X3D_ATTR_COL_BK_SPECULAR, matStruct.specular, false);

			}

			if (matStruct.opacity == matStruct.opacity)
			{
				float transparency = 1.0 - matStruct.opacity;
				if (transparency != 0.0)
				{
					tree.addFieldAttribute(X3D_LABEL_TWOSIDEMAT, X3D_ATTR_TRANSPARENCY, transparency);
					if (matStruct.isTwoSided)
						tree.addFieldAttribute(X3D_LABEL_TWOSIDEMAT, X3D_ATTR_BK_TRANSPARENCY, transparency);
				}


			}

			tree.addFieldAttribute(X3D_LABEL_TWOSIDEMAT, X3D_ATTR_ID, UUIDtoString(matNode->getUniqueID()));
			tree.addFieldAttribute(X3D_LABEL_TWOSIDEMAT, X3D_ATTR_DEF, UUIDtoString(matNode->getSharedID()));
		}
		break;
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
		case repo::core::model::NodeType::MESH:
		{
			const repo::core::model::MeshNode *meshNode = (const repo::core::model::MeshNode *)node;
			std::vector<repo_mesh_mapping_t> mappings = meshNode->getMeshMapping();


			std::string nodeID = mappings.size() == 1 ? UUIDtoString(mappings[0].mesh_id) : UUIDtoString(meshNode->getUniqueID());

			tree.addFieldAttribute("", X3D_ATTR_ID, nodeID);
			tree.addFieldAttribute("", X3D_ATTR_ON_CLICK, X3D_ON_CLICK);
			tree.addFieldAttribute("", X3D_ATTR_ON_MOUSE_OVER, X3D_ON_MOUSE_OVER);
			tree.addFieldAttribute("", X3D_ATTR_ON_MOUSE_MOVE, X3D_ON_MOUSE_MOVE);

			std::vector<repo_vector_t> bbox = meshNode->getBoundingBox();

			if (bbox.size() >= 2)
			{
				repo_vector_t boxCentre = getBoxCentre(bbox[0], bbox[1]);
				repo_vector_t boxSize = getBoxSize(bbox[0], bbox[1]);

				tree.addFieldAttribute("", X3D_ATTR_BBOX_CENTRE, boxCentre);
				tree.addFieldAttribute("", X3D_ATTR_BBOX_SIZE, boxSize);
			}


			if (mappings.size() > 1)
			{
				label = X3D_LABEL_MULTIPART;
				tree.addFieldAttribute("", X3D_ATTR_ON_LOAD, X3D_ON_LOAD);

				std::string baseURL = "/api/" + scene->getDatabaseName() + "/" + scene->getProjectName() + "/" + nodeID;

				tree.addFieldAttribute("", X3D_ATTR_URL, baseURL + ".x3d.mpc");
				tree.addFieldAttribute("", X3D_ATTR_URL_IDMAP, baseURL + ".json.mpc");
				tree.addFieldAttribute("", X3D_ATTR_SOLID, "false");
				tree.addFieldAttribute("", X3D_ATTR_NAMESPACE, nodeID);

				stopRecursing = true;

			}
			else
			{
				label = X3D_LABEL_SHAPE;
				tree.addFieldAttribute("", X3D_ATTR_DEF, nodeID);

				//Add material to shape
				std::string suffix;
				if (mappings.size() > 0)
					suffix = "#" + UUIDtoString(mappings[0].mesh_id);
				else
					suffix = "#" + nodeID;

				//Would we ever be working with unoptimized graph?
				std::vector<repo::core::model::RepoNode*> mats
					= scene->getChildrenNodesFiltered(gType, meshNode->getSharedID(), repo::core::model::NodeType::MATERIAL);
				if (mats.size())
				{
					//check for textures
					std::vector<repo::core::model::RepoNode*> textures
						= scene->getChildrenNodesFiltered(gType, mats[0]->getSharedID(), repo::core::model::NodeType::TEXTURE);
					if (textures.size())
					{
						suffix += "?tex_uuid=" + UUIDtoString(textures[0]->getUniqueID());
					}
				}

				std::string url = "/api/" + scene->getDatabaseName() + "/" + scene->getProjectName() + "/" + UUIDtoString(meshNode->getUniqueID()) + ".src" + suffix;

				tree.addFieldAttribute(X3D_LABEL_EXT_GEO, X3D_ATTR_URL, url);
			}

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

			tree.addFieldAttribute("", X3D_ATTR_ON_LOAD, X3D_ON_LOAD);
			tree.addFieldAttribute("", X3D_ATTR_URL, url);
			tree.addFieldAttribute("", X3D_ATTR_ID, UUIDtoString(refNode->getUniqueID()));
			tree.addFieldAttribute("", X3D_ATTR_DEF, UUIDtoString(refNode->getSharedID()));
			tree.addFieldAttribute("", X3D_ATTR_NAMESPACE, refNode->getDatabaseName() + "__" + refNode->getProjectName());

			//FIXME: Bounding box on reference nodes?
		}
		break;
		case repo::core::model::NodeType::TEXTURE:
		{
			label = X3D_LABEL_IMG_TEXTURE;
			const repo::core::model::TextureNode *texNode = (const repo::core::model::TextureNode *)node;
			std::string url = "/api/" + scene->getDatabaseName() + "/" + scene->getProjectName()
				+ "/" + UUIDtoString(texNode->getUniqueID()) + "." + texNode->getFileExtension();

			tree.addFieldAttribute("", X3D_ATTR_URL, url);
			tree.addFieldAttribute("", X3D_ATTR_ID, UUIDtoString(texNode->getUniqueID()));
			tree.addFieldAttribute("", X3D_ATTR_DEF, UUIDtoString(texNode->getSharedID()));
			tree.addFieldAttribute("", X3D_ATTR_XORIGIN, X3D_USE_CRED);

			tree.addFieldAttribute(X3D_LABEL_TEXT_PROP, X3D_ATTR_GEN_MIPMAPS, "true");
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

			tree.addFieldAttribute("", X3D_ATTR_ID, UUIDtoString(transNode->getUniqueID()));
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

bool X3DSRCModelExport::writeMultiPartMeshAsScene(
	const repo::core::model::MeshNode &mesh,
	const repo::core::model::RepoScene *scene)
{
	if (mesh.isEmpty())
	{
		repoError << "Failed to generate x3d representation for an empty multipart mesh";
		return false;
	}

	if (scene->getDatabaseName().empty() || scene->getProjectName().empty())
	{
		repoError << "Failed to generate x3d representation: Scene has no database/project name assigned.";
		return false;
	}


	std::string sceneLabel = X3D_LABEL + "." + X3D_LABEL_SCENE;

	//Set Scene Attributes
	tree.addFieldAttribute(sceneLabel, X3D_ATTR_ID, "scene");
	tree.addFieldAttribute(sceneLabel, X3D_ATTR_DO_PICK_PASS, "false");

	std::vector<repo_mesh_mapping_t> mappings = mesh.getMeshMapping();

	std::string meshIDStr = UUIDtoString(mesh.getUniqueID());
	fname = "/" + scene->getDatabaseName() + "/" + scene->getProjectName() + "/" + meshIDStr;

	repo::lib::PropertyTree groupSubTree(false);
	//Set root group attributes
	groupSubTree.addFieldAttribute("", X3D_ATTR_ON_LOAD, X3D_ON_LOAD);
	groupSubTree.addFieldAttribute("", X3D_ATTR_ID, "root");
	groupSubTree.addFieldAttribute("", X3D_ATTR_DEF, "root");
	groupSubTree.addFieldAttribute("", X3D_ATTR_RENDER, "true");

	for (size_t idx = 0; idx < mappings.size(); ++idx)
	{
		repo::lib::PropertyTree subTree(false);
		std::string shapeLabel = X3D_LABEL_SHAPE;
		std::string childId = meshIDStr + "_" + std::to_string(idx);

		//Shape Label for subMeshes
		subTree.addFieldAttribute("", X3D_ATTR_DEF, childId);

		repo_vector_t boxCentre = getBoxCentre(mappings[idx].min, mappings[idx].max);
		repo_vector_t boxSize = getBoxSize(mappings[idx].min, mappings[idx].max);

		subTree.addFieldAttribute("", X3D_ATTR_BBOX_CENTRE, boxCentre);
		subTree.addFieldAttribute("", X3D_ATTR_BBOX_SIZE, boxSize);

		//add empty material
		std::string materialLabel = X3D_LABEL_APP + "." + X3D_LABEL_MAT;
		subTree.addToTree(materialLabel, "");


		//add external geometry (path to SRC file)
		std::string extGeoLabel = X3D_LABEL_EXT_GEO;

		//FIXME: Using relative path, this will kill embedded viewer
		std::string srcFileURI = "/api" + fname + ".src.mpc#" + childId;
		subTree.addFieldAttribute(extGeoLabel, X3D_ATTR_URL, srcFileURI);

		groupSubTree.mergeSubTree(shapeLabel, subTree);
	}

	std::string rootGroupLabel = sceneLabel + "." + X3D_LABEL_GROUP;

	tree.mergeSubTree(rootGroupLabel, groupSubTree);

	return true;
}