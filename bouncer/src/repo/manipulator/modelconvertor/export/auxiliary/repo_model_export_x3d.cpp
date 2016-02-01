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
#include <algorithm>

using namespace repo::manipulator::modelconvertor;

//DOMs
static const std::string X3D_LABEL                 = "X3D";
static const std::string X3D_LABEL_APP             = "Appearance";
static const std::string X3D_LABEL_COMPOSED_SHADER = "ComposedShader";
static const std::string X3D_LABEL_EXT_GEO         = "ExternalGeometry";
static const std::string X3D_LABEL_FIELD           = "Field";
static const std::string X3D_LABEL_GROUP           = "Group";
static const std::string X3D_LABEL_IMG_TEXTURE     = "ImageTexture";
static const std::string X3D_LABEL_INLINE          = "Inline";
static const std::string X3D_LABEL_MAT             = "Material";
static const std::string X3D_LABEL_MAT_TRANS       = "MatrixTransform";
static const std::string X3D_LABEL_MULTIPART       = "MultiPart";
static const std::string X3D_LABEL_PLANE           = "Plane";
static const std::string X3D_LABEL_SCENE           = "Scene";
static const std::string X3D_LABEL_SHADER_PART     = "ShaderPart";
static const std::string X3D_LABEL_SHAPE           = "Shape";
static const std::string X3D_LABEL_TEXT_PROP       = "TextureProperties";
static const std::string X3D_LABEL_TRANS           = "Transform";
static const std::string X3D_LABEL_TWOSIDEMAT      = "TwoSidedMaterial";
static const std::string X3D_LABEL_VIEWPOINT       = "Viewpoint";

//Attribute Names
static const std::string X3D_ATTR_BIND             = "bind";
static const std::string X3D_ATTR_BBOX_CENTRE      = "bboxCenter";
static const std::string X3D_ATTR_BBOX_SIZE        = "bboxSize";
static const std::string X3D_ATTR_CENTRE           = "center";
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
static const std::string X3D_ATTR_LIT              = "lit";
static const std::string X3D_ATTR_MAT              = "matrix";
static const std::string X3D_ATTR_NAME             = "name";
static const std::string X3D_ATTR_NAMESPACE        = "nameSpaceName";
static const std::string X3D_ATTR_ON_MOUSE_MOVE    = "onmousemove";
static const std::string X3D_ATTR_ON_MOUSE_OVER    = "onmouseover"; 
static const std::string X3D_ATTR_ON_CLICK         = "onclick";
static const std::string X3D_ATTR_ON_LOAD          = "onload";
static const std::string X3D_ATTR_SCALE            = "scale";
static const std::string X3D_ATTR_SIZE             = "size";
static const std::string X3D_ATTR_ROTATION         = "rotation";
static const std::string X3D_ATTR_TRANSLATION      = "translation";
static const std::string X3D_ATTR_TRANSPARENCY     = "transparency";
static const std::string X3D_ATTR_BK_TRANSPARENCY  = "backTransparency";
static const std::string X3D_ATTR_TYPE             = "type";
static const std::string X3D_ATTR_ORIENTATION      = "orientation";
static const std::string X3D_ATTR_POS              = "position";
static const std::string X3D_ATTR_RENDER           = "render";
static const std::string X3D_ATTR_ROT_CENTRE       = "centerOfRotation";
static const std::string X3D_ATTR_SHININESS        = "shininess";
static const std::string X3D_ATTR_BK_SHININESS     = "backShininess";
static const std::string X3D_ATTR_SOLID            = "solid";
static const std::string X3D_ATTR_URL              = "url";
static const std::string X3D_ATTR_URL_IDMAP        = "urlIDMap";
static const std::string X3D_ATTR_VALUE            = "value";
static const std::string X3D_ATTR_VALUES           = "values";
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

static const float      GOOGLE_TILE_SIZE  = 640;


X3DModelExport::X3DModelExport(
	const repo::core::model::RepoScene *scene
	) :
	fullScene(true)
{
	tree.disableJSONWorkaround();
	convertSuccess = populateTree(scene);
	fname = "/" + scene->getDatabaseName() + "/" + scene->getProjectName() + "/revision/" + UUIDtoString(scene->getRevisionID());
	
}

X3DModelExport::X3DModelExport(
	const repo::core::model::MeshNode &mesh,
	const repo::core::model::RepoScene *scene
	)
	:
	fullScene(false)
{
	tree.disableJSONWorkaround();
	convertSuccess = populateTree(mesh, scene);
	
}

repo::lib::PropertyTree X3DModelExport::createGoogleMapSubTree(
	const repo::core::model::MapNode *mapNode)
{
	repo::lib::PropertyTree gmtree(false);

	if (mapNode)
	{

		repo_vector_t centrePoint = mapNode->getCentre();
		repo::lib::PropertyTree yrotTrans(false), shapeTrans(false), tileGroup(false);
		gmtree.addFieldAttribute("", X3D_ATTR_ID, "mapPosition");
		gmtree.addFieldAttribute("", X3D_ATTR_TRANSLATION, centrePoint);
		gmtree.addFieldAttribute("", X3D_ATTR_SCALE, "1,1,1");

		yrotTrans.addFieldAttribute("", X3D_ATTR_ID      , "mapRotation");
		yrotTrans.addFieldAttribute("", X3D_ATTR_ROTATION, "0,1,0," + std::to_string(mapNode->getYRot()));

		shapeTrans.addFieldAttribute("", X3D_ATTR_ROTATION, "1,0,0,4.7124");

		tileGroup.addFieldAttribute("", X3D_ATTR_ID, "tileGroup");
		tileGroup.addFieldAttribute("", X3D_ATTR_INVISIBLE, "true");

		int32_t halfWidth = (mapNode->getWidth() + 1) / 2;
		float centX = 128.0f + mapNode->getLong() * (256.0f / 360.0f);
		float s = sinf(mapNode->getLat() * (M_PI / 180.0f));
		if (s < -0.9999)
			s = -0.9999;
		else if (s > 0.9999)
			s = 0.9999;

		float centY = 128.0 + 0.5 * log((1.0 + s) / (1.0 - s)) * (-256.0 / (2.0 * M_PI));


		size_t zoom = mapNode->getZoom();
		size_t nTiles = 1 << zoom;

		std::string mapType = mapNode->getMapType();
		
		if (mapType.empty()) mapType = "satellite";

		for (int32_t x = -halfWidth; x < halfWidth; ++x)
		{
			for (int32_t y = -halfWidth; y < halfWidth; ++y)
			{
				repo::lib::PropertyTree tileTree(false), tileTreeWithPlane(false);

				float xPos = (x + 0.5) * GOOGLE_TILE_SIZE;
				float yPos = -(y + 0.5) * GOOGLE_TILE_SIZE;

				float tileCentX = centX * nTiles + xPos;
				float tileCentY = centY * nTiles + yPos;

				float tileLat = (2.0 * atan(exp(((tileCentY / nTiles) - 128.0) / -(256.0 / (2.0 * M_PI)))) - M_PI / 2.0) / (M_PI / 180.0);
				
				float tileLong = ((tileCentX / nTiles) - 128.) / (256. / 360.);

				std::stringstream ss;
				ss << "https://maps.googleapis.com/maps/api/staticmap?center=" << tileLat << "," 
					<< tileLong << "&size=" << GOOGLE_TILE_SIZE << "x" << GOOGLE_TILE_SIZE
					<< "&zoom=" << zoom << "&key=" << mapNode->getAPIKey() << "&maptype=" << mapType;
				std::string googleMapsURL = ss.str();


				if (mapNode->isTwoSided())
				{
					repo::lib::PropertyTree shaderPt(false), alphaValuePt(false), textureIdPTree(false), vertShaderPt(false), fragShaderPt(false);
					shaderPt.addFieldAttribute("", X3D_ATTR_DEF, "TwoSidedShader");

					//Field element
					alphaValuePt.addFieldAttribute("", X3D_ATTR_NAME, "alpha");
					alphaValuePt.addFieldAttribute("", X3D_ATTR_TYPE, "SFFloat");
					alphaValuePt.addFieldAttribute("", X3D_ATTR_VALUE, mapNode->getTwoSidedValue());

					shaderPt.mergeSubTree(X3D_LABEL_FIELD, alphaValuePt);

					textureIdPTree.addFieldAttribute("", X3D_ATTR_NAME, "map");
					textureIdPTree.addFieldAttribute("", X3D_ATTR_TYPE, "SFInt32");
					textureIdPTree.addFieldAttribute("", X3D_ATTR_VALUES, 0);

					shaderPt.mergeSubTree(X3D_LABEL_FIELD, textureIdPTree);

					vertShaderPt.addFieldAttribute("", X3D_ATTR_TYPE, "VERTEX");
					vertShaderPt.addToTree("", "\n\
							attribute vec3 position;\n\
							attribute vec2 texcoord;\n\
							varying vec2 fragTexCoord;\n\
							uniform mat4 modelViewProjectionMatrix;\n\
							\n\
							void main()\n\
							{\n\
								fragTexCoord = vec2(texcoord.x, 1.0 - texcoord.y);\n\
								gl_Position = modelViewProjectionMatrix * vec4(position, 1.0);\n\
							}\n\
							")			;
					shaderPt.mergeSubTree(X3D_LABEL_SHADER_PART, vertShaderPt);

					fragShaderPt.addFieldAttribute("", X3D_ATTR_TYPE, "FRAGMENT");
					fragShaderPt.addToTree("", " \
							#ifdef GL_ES\n\
								precision highp float;\n\
							#endif\n\
							\n\
							varying vec2 fragTexCoord;\n\
							\n\
							uniform float alpha;\n\
							uniform sampler2D map;\n\
							\n\
							void main()\n\
							{\n\
								vec4 mapCol = texture2D(map, fragTexCoord);\n\
								gl_FragColor.rgba = vec4(mapCol.rgb, alpha);\n\
							}\n\
							");

					shaderPt.mergeSubTree(X3D_LABEL_SHADER_PART, fragShaderPt);
					tileTree.mergeSubTree(X3D_LABEL_COMPOSED_SHADER, shaderPt);
				}				

				tileTree.addFieldAttribute(X3D_LABEL_IMG_TEXTURE,  X3D_ATTR_URL , googleMapsURL);

				repo::lib::PropertyTree planePt(false);
				float worldTileSize = mapNode->getTileSize();
				planePt.addFieldAttribute("", X3D_ATTR_CENTRE, std::to_string(x*worldTileSize) + "," + std::to_string(y*worldTileSize));
				planePt.addFieldAttribute("", X3D_ATTR_SIZE, std::to_string(worldTileSize) + "," + std::to_string(worldTileSize));
				planePt.addFieldAttribute("", X3D_ATTR_SOLID, "false");
				planePt.addFieldAttribute("", X3D_ATTR_LIT, "false");

				tileTreeWithPlane.mergeSubTree(X3D_LABEL_APP, tileTree);
				tileTreeWithPlane.mergeSubTree(X3D_LABEL_PLANE, planePt);

				tileGroup.mergeSubTree(X3D_LABEL_SHAPE, tileTreeWithPlane);

			}
		}
		repoDebug << "Exited...";

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

repo_vector_t X3DModelExport::getBoxCentre(
	const repo_vector_t &min,
	const repo_vector_t &max) const
{
	repoDebug << "Centre of bbox of [" << min.x << "," << min.y << "," << min.z << "] and [" << max.x << "," << max.y << "," << max.z << "] is " 
		<< "[" << (min.x + max.x) / 2. << ", " << (min.y + max.y) / 2. << ", " << (min.z + max.z) / 2. << "]";

	return { (min.x + max.x) / 2.0f, (min.y + max.y) / 2.0f, (min.z + max.z) / 2.0f };
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
	//repoTrace << "FILE: " << fname << " : " << xmlFile;
	memcpy(xmlBuf.data(), xmlFile.c_str(), xmlFile.size());

	return xmlBuf;
}

std::string X3DModelExport::getFileName() const
{
	std::string fullName =  fname + ".x3d.mp";
	if (!fullScene)
		fullName += "c";

	return fullName;
}

bool X3DModelExport::includeHeader()
{
	tree.addFieldAttribute(X3D_LABEL, X3D_ATTR_XMLNS, X3D_SPEC_URL);
	tree.addFieldAttribute(X3D_LABEL, X3D_ATTR_ON_LOAD, X3D_ON_LOAD);

	return true;
}

std::string X3DModelExport::populateTreeWithProperties(
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
			tree.addFieldAttribute("", X3D_ATTR_ON_CLICK     , X3D_ON_CLICK);
			tree.addFieldAttribute("", X3D_ATTR_ON_MOUSE_OVER, X3D_ON_MOUSE_OVER);
			tree.addFieldAttribute("", X3D_ATTR_ON_MOUSE_MOVE, X3D_ON_MOUSE_MOVE);

			std::vector<repo_vector_t> bbox = meshNode->getBoundingBox();

			if (bbox.size() >= 2)
			{
				repo_vector_t boxCentre = getBoxCentre(bbox[0], bbox[1]);
				repo_vector_t boxSize   = getBoxSize(bbox[0], bbox[1]);

				tree.addFieldAttribute("", X3D_ATTR_BBOX_CENTRE, boxCentre);
				tree.addFieldAttribute("", X3D_ATTR_BBOX_SIZE, boxSize);
			}
		

			if (mappings.size() > 1)
			{
				label = X3D_LABEL_MULTIPART;
				tree.addFieldAttribute("", X3D_ATTR_ON_LOAD, X3D_ON_LOAD);
				
				std::string baseURL = "/api/" + scene->getDatabaseName() + "/" + scene->getProjectName() + "/" + nodeID;

				tree.addFieldAttribute("", X3D_ATTR_URL      , baseURL + ".x3d.mpc");
				tree.addFieldAttribute("", X3D_ATTR_URL_IDMAP, baseURL + ".json.mpc");
				tree.addFieldAttribute("", X3D_ATTR_SOLID    , "false");
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

	return true;
}

bool X3DModelExport::writeScene(
	const repo::core::model::RepoScene *scene)
{

	repoDebug << "Writing scene...";
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

	
	gType = repo::core::model::RepoScene::GraphType::OPTIMIZED;

	if (!scene->hasRoot(gType))
	{
		gType = repo::core::model::RepoScene::GraphType::DEFAULT;
		if (!scene->hasRoot(gType))
		{
			repoError << "Failed to generate x3d representation: Cannot find optimised graph representation!";
			return false;
		}
	}

	repo::lib::PropertyTree rootGrpST(false), sceneST(false);

	std::string sceneLabel = X3D_LABEL + "." + X3D_LABEL_SCENE;

	//Set Scene Attributes
	sceneST.addFieldAttribute("", X3D_ATTR_ID, "scene");
	sceneST.addFieldAttribute("", X3D_ATTR_DO_PICK_PASS, "false");


	//Set root group attributes
	rootGrpST.addFieldAttribute("", X3D_ATTR_ON_LOAD, X3D_ON_LOAD);
	rootGrpST.addFieldAttribute("", X3D_ATTR_ID     , "root");
	rootGrpST.addFieldAttribute("", X3D_ATTR_DEF    , "root");
	rootGrpST.addFieldAttribute("", X3D_ATTR_RENDER , "true");

	repo::lib::PropertyTree subTree(false);


	std::string label = populateTreeWithProperties(scene->getRoot(gType), scene, subTree);


	rootGrpST.mergeSubTree(label, subTree);
	sceneST.mergeSubTree(X3D_LABEL_GROUP, rootGrpST);

	//FIXME: node JS gets the bounding box from the root node, root node is always a transformation?
	//Faking a bounding box for now.
	repo_vector_t bboxCentre = { 0, 0, 0 };
	repo_vector_t bboxSize = { 2, 2, 2 };
	repo_vector_t vpos = bboxCentre;

	float max_dim = (bboxSize.x > bboxSize.y? bboxSize.x : bboxSize.y)*0.5f;

	float fov = 40 * (M_PI / 180); //Field of View in radians (40 degrees)

	// Move back in the z direction such that the model takes
	// up half the center of the screen.

	vpos.z += bboxSize.z * 0.5 + max_dim / tanf(0.5*fov);

	repo::lib::PropertyTree vpTree(false);
	vpTree.addFieldAttribute("", X3D_ATTR_ID, scene->getDatabaseName() + "_" + scene->getProjectName() + "_origin");
	vpTree.addFieldAttribute("", X3D_ATTR_POS, vpos);
	vpTree.addFieldAttribute("", X3D_ATTR_ROT_CENTRE, bboxCentre);
	vpTree.addFieldAttribute("", X3D_ATTR_ORIENTATION, "0 0 -1 0");
	vpTree.addFieldAttribute("", X3D_ATTR_FOV, fov);
	vpTree.addToTree("", " ");
	sceneST.mergeSubTree(X3D_LABEL_VIEWPOINT, vpTree);

	tree.mergeSubTree(sceneLabel, sceneST);



	return true;
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
