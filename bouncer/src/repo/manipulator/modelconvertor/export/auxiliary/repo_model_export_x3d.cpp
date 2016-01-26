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

#include "repo_model_export_x3d.h"
#include "../../../../lib/repo_log.h"
#include "../../../../core/model/bson/repo_bson_factory.h"

#define _USE_MATH_DEFINES
#include <math.h>

using namespace repo::manipulator::modelconvertor;

//DOMs
static const std::string X3D_LABEL             = "X3D";
static const std::string X3D_LABEL_APP         = "Appearance";
static const std::string X3D_LABEL_EXT_GEO     = "ExternalGeometry";
static const std::string X3D_LABEL_GROUP       = "Group";
static const std::string X3D_LABEL_IMG_TEXTURE = "ImageTexutre";
static const std::string X3D_LABEL_INLINE      = "Inline";
static const std::string X3D_LABEL_MAT         = "Material";
static const std::string X3D_LABEL_MAT_TRANS   = "MatrixTransform";
static const std::string X3D_LABEL_SCENE       = "Scene";
static const std::string X3D_LABEL_SHAPE       = "Shape";
static const std::string X3D_LABEL_TEXT_PROP   = "TextureProperties";
static const std::string X3D_LABEL_TRANS       = "Transform";
static const std::string X3D_LABEL_TWOSIDEMAT  = "TwoSidedMaterial";
static const std::string X3D_LABEL_VIEWPOINT   = "Viewpoint";

//Attribute Names
static const std::string X3D_ATTR_BIND             = "bind";
static const std::string X3D_ATTR_BBOX_CENTRE      = "bboxCenter";
static const std::string X3D_ATTR_BBOX_SIZE        = "bboxSize";
static const std::string X3D_ATTR_COL_DIFFUSE      = "diffuseColor";
static const std::string X3D_ATTR_COL_BK_DIFFUSE   = "backDiffuseColor";
static const std::string X3D_ATTR_COL_EMISSIVE     = "emissiveColor";
static const std::string X3D_ATTR_COL_BK_EMISSIVE  = "backEmissiveColor";
static const std::string X3D_ATTR_COL_SPECULAR     = "specularColor";
static const std::string X3D_ATTR_COL_BK_SPECULAR  = "backSpecularColor";
static const std::string X3D_ATTR_DEF              = "def";
static const std::string X3D_ATTR_DO_PICK_PASS     = "dopickpass";
static const std::string X3D_ATTR_FOV              = "fieldOfView";
static const std::string X3D_ATTR_GEN_MIPMAPS      = "generateMipMaps";
static const std::string X3D_ATTR_ID               = "id";
static const std::string X3D_ATTR_INVISIBLE        = "invisible";
static const std::string X3D_ATTR_MAT              = "matrix";
static const std::string X3D_ATTR_NAMESPACE        = "nameSpaceName";
static const std::string X3D_ATTR_ON_MOUSE_MOVE    = "onmouseover";
static const std::string X3D_ATTR_ON_MOUSE_OVER    = "onmousemove";
static const std::string X3D_ATTR_ON_CLICK         = "onclick";
static const std::string X3D_ATTR_ON_LOAD          = "onload";
static const std::string X3D_ATTR_SCALE            = "scale";
static const std::string X3D_ATTR_ROTATION         = "rotation";
static const std::string X3D_ATTR_TRANSLATION      = "translation";
static const std::string X3D_ATTR_TRANSPARENCY     = "transparency";
static const std::string X3D_ATTR_BK_TRANSPARENCY  = "backTransparency";
static const std::string X3D_ATTR_ORIENTATION      = "orientation";
static const std::string X3D_ATTR_POS              = "position";
static const std::string X3D_ATTR_RENDER           = "render";
static const std::string X3D_ATTR_ROT_CENTRE       = "centreOfRotation";
static const std::string X3D_ATTR_SHININESS        = "shininess";
static const std::string X3D_ATTR_BK_SHININESS     = "backShininess";
static const std::string X3D_ATTR_URL              = "url";
static const std::string X3D_ATTR_XMLNS            = "xmlns";
static const std::string X3D_ATTR_XORIGIN          = "crossOrigin";
static const std::string X3D_ATTR_ZFAR             = "zFar";
static const std::string X3D_ATTR_ZNEAR            = "zNear";

//Values
static const std::string X3D_SPEC_URL      = "http://www.web3d.org/specification/x3d-namespace";
static const std::string X3D_ON_CLICK      = "clickObject(event);";
static const std::string X3D_ON_LOAD       = "onLoaded(event);";
static const std::string X3D_ON_MOUSE_MOVE = "onMouseOver(event);";
static const std::string X3D_ON_MOUSE_OVER = "onMouseMove(event);";
static const std::string X3D_USE_CRED      = "use-credentials";
static const float       X3D_DEFAULT_FOV   = 0.25 * M_PI;

static const size_t      GOOGLE_TILE_SIZE  = 640;


X3DModelExport::X3DModelExport(
	const repo::core::model::RepoScene *scene
	)
{
	tree.disableJSONWorkaround();
	convertSuccess = populateTree(scene);
}

X3DModelExport::X3DModelExport(
	const repo::core::model::MeshNode &mesh,
	const repo::core::model::RepoScene *scene
	)
{
	tree.disableJSONWorkaround();
	convertSuccess = populateTree(mesh, scene);
}

repo::lib::PropertyTree createGoogleMapSubTree(
	const repo::core::model::MapNode *mapNode)
{
	repo::lib::PropertyTree gmtree;

	if (mapNode)
	{
		repo::lib::PropertyTree yrotTrans, shapeTrans, tileGroup;
		gmtree.addFieldAttribute("", X3D_ATTR_ID, "mapPosition");
		gmtree.addFieldAttribute("", X3D_ATTR_TRANSLATION, mapNode->getTransformationMatrix());
		gmtree.addFieldAttribute("", X3D_ATTR_SCALE, "1,1,1");

		yrotTrans.addFieldAttribute("", X3D_ATTR_ID      , "mapRotation");
		yrotTrans.addFieldAttribute("", X3D_ATTR_ROTATION, "0,1,0," + std::to_string(mapNode->getYRot()));

		shapeTrans.addFieldAttribute("", X3D_ATTR_ROTATION, "1,0,0,4.7124");

		tileGroup.addFieldAttribute("", X3D_ATTR_ID, "tileGroup");
		tileGroup.addFieldAttribute("", X3D_ATTR_INVISIBLE, "true");

		uint32_t halfWidth = (mapNode->getWidth() + 1) / 2;

		for (uint32_t x = -halfWidth; x < halfWidth; ++x)
			for (uint32_t y = -halfWidth; y < halfWidth; ++y)
			{
				repo::lib::PropertyTree tileTree;

				float xPos = ((float)x + 0.5) * GOOGLE_TILE_SIZE;
				float yPos = ((float)y + 0.5) * GOOGLE_TILE_SIZE;

				float tileCentX = centX * nTiles + xPos;
				float tileCentY = centY * nTiles + yPos;

			}

		shapeTrans.mergeSubTree(X3D_LABEL_GROUP, tileGroup);
		yrotTrans.mergeSubTree(X3D_LABEL_TRANS, shapeTrans);
		gmtree.mergeSubTree(X3D_LABEL_TRANS, yrotTrans);
	}
	else
	{
		repoError << "Unable to generate google map info for x3d export: null pointer to Map Node!";
	}

	return gmtree;
}

repo::lib::PropertyTree X3DModelExport::generateSubTree(
	const repo::core::model::RepoNode             *node,
	const repo::core::model::RepoScene::GraphType &gtype,
	const repo::core::model::RepoScene            *scene)
{
	repo::lib::PropertyTree subTree;
	std::string label; //top subtree label depending on node types
	if (node)
	{
		populateTreeWithProperties(node, scene, subTree);

	}
	else
	{
		repoWarning << "Unexpected error occured in x3d file generation: Null pointer to child. The tree may not be complete"; 
	}
	return subTree;
}

repo_vector_t X3DModelExport::getBoxCentre(
	const repo_vector_t &min,
	const repo_vector_t &max) const
{
	repoDebug << "Centre of bbox of [" << min.x << "," << min.y << "," << min.z << "] and [" << max.x << "," << max.y << "," << max.z << "] is " 
		<< "[" << (min.x + max.x) / 2. << ", " << (min.y + max.y) / 2. << ", " << (min.z + max.z) / 2. << "]";

	return { (min.x + max.x) / 2., (min.y + max.y) / 2., (min.z + max.z) / 2. };
}

repo_vector_t X3DModelExport::getBoxSize(
	const repo_vector_t &min,
	const repo_vector_t &max) const
{
	repoDebug << "Size of bbox of [" << min.x << "," << min.y << "," << min.z << "] and [" << max.x << "," << max.y << "," << max.z << "] is "
		<< "[" << max.x - min.x << ", " << max.y - min.y << ", " << max.z - min.z << "]";
	return { max.x - min.x, max.y - min.y, max.z - min.z};
}

std::vector<uint8_t> X3DModelExport::getFileAsBuffer() const
{
	std::stringstream ss;
	tree.write_xml(ss);
	std::string xmlFile = ss.str();
	std::vector<uint8_t> xmlBuf;
	xmlBuf.resize(xmlFile.size());
	repoTrace << "FILE: " << fname << " : " << xmlFile;
	memcpy(xmlBuf.data(), xmlFile.c_str(), xmlFile.size());

	return xmlBuf;
}

std::string X3DModelExport::getFileName() const
{
	return fname + ".x3d.mpc";
}

bool X3DModelExport::includeHeader()
{
	tree.addFieldAttribute(X3D_LABEL, X3D_ATTR_XMLNS, X3D_SPEC_URL);
	tree.addFieldAttribute(X3D_LABEL, X3D_ATTR_ON_LOAD, X3D_ON_LOAD);

	return true;
}

std::string populateTreeWithProperties(
	const repo::core::model::RepoNode  *node,
	const repo::core::model::RepoScene *scene,
	repo::lib::PropertyTree            &tree
	)
{
	std::string label;
	if (node)
	{
		switch (node->getTypeAsEnum())
		{

		case repo::core::model::NodeType::CAMERA:
		{
			label = X3D_LABEL_VIEWPOINT;
			const repo::core::model::CameraNode *camNode = (const repo::core::model::CameraNode *)node;

			repo_vector_t camPos    = camNode->getPosition();
			repo_vector_t camlookAt = camNode->getLookAt();

			tree.addFieldAttribute("", X3D_ATTR_ID   , camNode->getName());
			tree.addFieldAttribute("", X3D_ATTR_DEF  , UUIDtoString(camNode->getSharedID()));
			tree.addFieldAttribute("", X3D_ATTR_BIND , false);
			tree.addFieldAttribute("", X3D_ATTR_FOV  , X3D_DEFAULT_FOV);
			tree.addFieldAttribute("", X3D_ATTR_POS  , camPos);
			tree.addFieldAttribute("", X3D_ATTR_ZNEAR, -1);
			tree.addFieldAttribute("", X3D_ATTR_ZFAR , -1);

			repo_vector_t centre = { camPos.x + camlookAt.x, camPos.y + camlookAt.y, camPos.z + camlookAt.z };

			tree.addFieldAttribute("", X3D_ATTR_ROT_CENTRE, centre);
			tree.addFieldAttribute("", X3D_ATTR_ORIENTATION, camNode->getOrientation());

		}
		break;
		case repo::core::model::NodeType::MATERIAL:
		{
			const repo::core::model::MaterialNode *matNode = (const repo::core::model::MaterialNode *)node;
			label = X3D_LABEL_APP + "." + X3D_LABEL_TWOSIDEMAT;

			repo_material_t matStruct = matNode->getMaterialStruct();

			if (matStruct.diffuse.size() > 0)
			{
				tree.addFieldAttribute("", X3D_ATTR_COL_DIFFUSE, matStruct.diffuse);
				if (matStruct.isTwoSided)
					tree.addFieldAttribute("", X3D_ATTR_COL_BK_DIFFUSE, matStruct.diffuse);

			}

			if (matStruct.emissive.size() > 0)
			{
				tree.addFieldAttribute("", X3D_ATTR_COL_EMISSIVE, matStruct.emissive);
				if (matStruct.isTwoSided)
					tree.addFieldAttribute("", X3D_ATTR_COL_BK_EMISSIVE, matStruct.emissive);

			}

			if (matStruct.shininess == matStruct.shininess)
			{
				tree.addFieldAttribute("", X3D_ATTR_SHININESS, matStruct.shininess);
				if (matStruct.isTwoSided)
					tree.addFieldAttribute("", X3D_ATTR_BK_SHININESS, matStruct.shininess);

			}

			if (matStruct.specular.size() > 0)
			{
				tree.addFieldAttribute("", X3D_ATTR_COL_SPECULAR, matStruct.specular);
				if (matStruct.isTwoSided)
					tree.addFieldAttribute("", X3D_ATTR_COL_BK_SPECULAR, matStruct.specular);

			}

			if (matStruct.opacity == matStruct.opacity)
			{
				float transparency = 1.0 - matStruct.opacity;
				tree.addFieldAttribute("", X3D_ATTR_TRANSPARENCY, transparency);
				if (matStruct.isTwoSided)
					tree.addFieldAttribute("", X3D_ATTR_BK_TRANSPARENCY, transparency);

			}

			tree.addFieldAttribute("", X3D_ATTR_ID , UUIDtoString(matNode->getUniqueID()));
			tree.addFieldAttribute("", X3D_ATTR_DEF, UUIDtoString(matNode->getSharedID()));
		}
		break;
		case repo::core::model::NodeType::MAP:
		{
			const repo::core::model::MapNode *mapNode = (const repo::core::model::MapNode *)node;
			label = X3D_LABEL_GROUP + "." + X3D_LABEL_TRANS;
			std::string mapType = mapNode->getMapType();
			if (mapType.empty())
				mapType = "satellite";

			tree.addFieldAttribute("", X3D_ATTR_ID, UUIDtoString(mapNode->getUniqueID()));
			tree.addFieldAttribute("", X3D_ATTR_DEF, UUIDtoString(mapNode->getSharedID()));

			repo::lib::PropertyTree gmST = createGoogleMapSubTree(mapNode);

		}
		break;
		case repo::core::model::NodeType::REFERENCE:
		{
			label = X3D_LABEL_INLINE;
			const repo::core::model::ReferenceNode *refNode = (const repo::core::model::ReferenceNode *)node;
			std::string revisionId = UUIDtoString(refNode->getRevisionID());
			std::string url = "/api/" + scene->getDatabaseName() + "/" + scene->getProjectName() + "/revision/";
			if (revisionId == REPO_HISTORY_MASTER_BRANCH)
			{
				//Load head of master
				url += "master/head";
			}
			else
			{
				//load specific revision
				//FIXME: this is weird. We don't need a branch specification for a specific revision loading
				//Also it may not be master this revision is from
				url += "master/" + revisionId;
			}

			url += ".x3d";

			tree.addFieldAttribute("", X3D_ATTR_ON_LOAD  , X3D_ON_LOAD);
			tree.addFieldAttribute("", X3D_ATTR_URL      , url);
			tree.addFieldAttribute("", X3D_ATTR_ID       , UUIDtoString(refNode->getUniqueID()));
			tree.addFieldAttribute("", X3D_ATTR_DEF      , UUIDtoString(refNode->getSharedID()));
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
			tree.addFieldAttribute("", X3D_ATTR_ID , UUIDtoString(texNode->getUniqueID()));
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

			tree.addFieldAttribute("", X3D_ATTR_ID,  UUIDtoString(transNode->getUniqueID()));
			tree.addFieldAttribute("", X3D_ATTR_DEF, UUIDtoString(transNode->getSharedID()));
		}
		break;
		default:
			repoError << "Unsupported node type: " << node->getType();
		}
	}

	return label;
}


bool X3DModelExport::writeMultiPartMeshAsScene(
	const repo::core::model::MeshNode  &mesh,
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
	tree.addFieldAttribute(sceneLabel, X3D_ATTR_ID          , "scene");
	tree.addFieldAttribute(sceneLabel, X3D_ATTR_DO_PICK_PASS, "false");

	std::vector<repo_mesh_mapping_t> mappings = mesh.getMeshMapping();

	std::string meshIDStr = UUIDtoString(mesh.getUniqueID());
	fname = "/" + scene->getDatabaseName() + "/" + scene->getProjectName() + "/" + meshIDStr;

	repo::lib::PropertyTree groupSubTree;
	groupSubTree.disableJSONWorkaround();
	//Set root group attributes
	groupSubTree.addFieldAttribute("", X3D_ATTR_ON_LOAD, X3D_ON_LOAD);
	groupSubTree.addFieldAttribute("", X3D_ATTR_ID, "root");
	groupSubTree.addFieldAttribute("", X3D_ATTR_DEF, "root");
	groupSubTree.addFieldAttribute("", X3D_ATTR_RENDER, "true");

	for (size_t idx = 0; idx < mappings.size(); ++idx)
	{
		repo::lib::PropertyTree subTree;
		subTree.disableJSONWorkaround();
		std::string shapeLabel = X3D_LABEL_SHAPE;
		std::string childId    = meshIDStr + "_" + std::to_string(idx);

		//Shape Label for subMeshes
		subTree.addFieldAttribute("", X3D_ATTR_DEF, childId); //FIXME: it's "DEF" instead of "def" in Repo io

		repo_vector_t boxCentre = getBoxCentre(mappings[idx].min, mappings[idx].max);
		repo_vector_t boxSize   = getBoxSize  (mappings[idx].min, mappings[idx].max);

		subTree.addFieldAttribute("", X3D_ATTR_BBOX_CENTRE, boxCentre);
		subTree.addFieldAttribute("", X3D_ATTR_BBOX_SIZE, boxSize);

		//add empty material
		std::string materialLabel = X3D_LABEL_APP + "." + X3D_LABEL_MAT;
		subTree.addToTree(materialLabel, "");


		//add external geometry (path to SRC file)
		std::string extGeoLabel = X3D_LABEL_EXT_GEO;

		//FIXME: Using relative path, this will kill embedded viewer
		std::string srcFileURI = "/api" + fname+ ".src.mpc#" + childId;
		subTree.addFieldAttribute(extGeoLabel, X3D_ATTR_URL, srcFileURI);

		groupSubTree.mergeSubTree(shapeLabel, subTree);
	}

	std::string rootGroupLabel = sceneLabel + "." + X3D_LABEL_GROUP;

	tree.mergeSubTree(rootGroupLabel, groupSubTree);
}

bool X3DModelExport::writeScene(
	const repo::core::model::RepoScene *scene)
{
	if (!scene)
	{
		repoError << "Failed to generate x3d representation: Null pointer to scene!";
		return false;
	}

	if (scene->getDatabaseName().empty() || scene->getProjectName().empty())
	{
		repoError << "Failed to generate x3d representation: Scene has no database/project name assigned.";
		return false;
	}

	//We only support stash graph generation
	repo::core::model::RepoScene::GraphType gType = 
		repo::core::model::RepoScene::GraphType::OPTIMIZED;

	if (!scene->hasRoot(gType))
	{
		repoError << "Failed to generate x3d representation: Cannot find optimised graph representation!";
		return false;
	}

	std::string sceneLabel = X3D_LABEL + "." + X3D_LABEL_SCENE;

	//Set Scene Attributes
	tree.addFieldAttribute(sceneLabel, X3D_ATTR_ID, "scene");
	tree.addFieldAttribute(sceneLabel, X3D_ATTR_DO_PICK_PASS, "false");

	repo::lib::PropertyTree rootGrpST;
	rootGrpST.disableJSONWorkaround();
	//Set root group attributes
	rootGrpST.addFieldAttribute("", X3D_ATTR_ON_LOAD, X3D_ON_LOAD);
	rootGrpST.addFieldAttribute("", X3D_ATTR_ID     , "root");
	rootGrpST.addFieldAttribute("", X3D_ATTR_DEF    , "root");
	rootGrpST.addFieldAttribute("", X3D_ATTR_RENDER , "true");

	repo::lib::PropertyTree subTree = generateSubTree(scene->getRoot(gType), gType, scene);
	std::string rootGroupLabel = sceneLabel + "." + X3D_LABEL_GROUP;
	rootGrpST.mergeSubTree(rootGroupLabel, subTree);
}

bool X3DModelExport::populateTree(
	const repo::core::model::RepoScene *scene)
{
	//Write xml header
	bool success = includeHeader();

	success &= writeScene(scene);
	return success;
}

bool X3DModelExport::populateTree(
	const repo::core::model::MeshNode &mesh,
	const repo::core::model::RepoScene *scene)
{
	//Write xml header
	bool success = includeHeader();

	success &= writeMultiPartMeshAsScene(mesh, scene);
	return success;
}

X3DModelExport::~X3DModelExport()
{
}
