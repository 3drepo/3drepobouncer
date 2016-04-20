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

#include "repo_model_export_x3d_abstract.h"
#include "x3dom_constants.h"
#include "../../../../lib/repo_log.h"
#include "../../../../core/model/bson/repo_bson_factory.h"

using namespace repo::manipulator::modelconvertor;



AbstractX3DModelExport::AbstractX3DModelExport(
	const repo::core::model::RepoScene *scene
	) : 
	fullScene(true),
	scene(scene),
	mesh(repo::core::model::MeshNode()),
	initialised(false),
	convertSuccess(false)
	
{
	tree.disableJSONWorkaround();
	fname = "/" + scene->getDatabaseName() + "/" + scene->getProjectName() + "/revision/" + UUIDtoString(scene->getRevisionID());
	
}

AbstractX3DModelExport::AbstractX3DModelExport(
	const repo::core::model::MeshNode &mesh,
	const repo::core::model::RepoScene *scene
	)
	:
	fullScene(false),
	scene(scene),
	mesh(mesh),
	initialised(false),
	convertSuccess(false)
{
	tree.disableJSONWorkaround();	
}

repo::lib::PropertyTree AbstractX3DModelExport::createGoogleMapSubTree(
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

repo_vector_t AbstractX3DModelExport::getBoxCentre(
	const repo_vector_t &min,
	const repo_vector_t &max) const
{
	return { (min.x + max.x) / 2.0f, (min.y + max.y) / 2.0f, (min.z + max.z) / 2.0f };
}

repo_vector_t AbstractX3DModelExport::getBoxSize(
	const repo_vector_t &min,
	const repo_vector_t &max) const
{
	return { max.x - min.x, max.y - min.y, max.z - min.z};
}

std::vector<uint8_t> AbstractX3DModelExport::getFileAsBuffer()
{
	std::vector<uint8_t> xmlBuf;
	if (!initialised) initialize();

	if (convertSuccess)
	{
		std::stringstream ss;
		tree.write_xml(ss);
		std::string xmlFile = ss.str();
		xmlBuf.resize(xmlFile.size());
		//repoTrace << "FILE: " << fname << " : " << xmlFile;
		memcpy(xmlBuf.data(), xmlFile.c_str(), xmlFile.size());

	}
	
	return xmlBuf;
}

std::string AbstractX3DModelExport::getFileName() const
{
	std::string fullName =  fname + ".x3d.mp";
	if (!fullScene)
		fullName += "c";

	return fullName;
}


repo::lib::PropertyTree AbstractX3DModelExport::generateDefaultViewPointTree()
{
	repo::lib::PropertyTree vpTree(false);
	
	auto bbox = scene->getSceneBoundingBox();

	repo_vector_t bboxCentre = { (bbox[1].x + bbox[0].x) / 2., (bbox[1].y + bbox[0].y) / 2., (bbox[1].z + bbox[0].z) / 2. };
	repo_vector_t bboxSize = { bbox[1].x - bbox[0].x, bbox[1].y - bbox[0].y, bbox[1].z - bbox[0].z };
	repo_vector_t vpos = bboxCentre;

	float max_dim = (bboxSize.x > bboxSize.y ? bboxSize.x : bboxSize.y)*0.5f;

	float fov = 40 * (M_PI / 180); //Field of View in radians (40 degrees)

	// Move back in the z direction such that the model takes
	// up half the center of the screen.

	vpos.z += bboxSize.z * 0.5 + max_dim / tanf(0.5*fov);

	vpTree.addFieldAttribute("", X3D_ATTR_ID, scene->getDatabaseName() + "_" + scene->getProjectName() + "_origin");
	vpTree.addFieldAttribute("", X3D_ATTR_POS, vpos);
	vpTree.addFieldAttribute("", X3D_ATTR_ROT_CENTRE, bboxCentre);
	vpTree.addFieldAttribute("", X3D_ATTR_ORIENTATION, "0 0 -1 0");
	vpTree.addFieldAttribute("", X3D_ATTR_FOV, fov);
	vpTree.addToTree("", " ");

	return vpTree;
}

bool AbstractX3DModelExport::includeHeader()
{
	tree.addFieldAttribute(X3D_LABEL, X3D_ATTR_XMLNS, X3D_SPEC_URL);
	tree.addFieldAttribute(X3D_LABEL, X3D_ATTR_ON_LOAD, X3D_ON_LOAD);

	return true;
}

bool AbstractX3DModelExport::sceneValid()
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

	return true;
}

bool AbstractX3DModelExport::writeScene(
	const repo::core::model::RepoScene *scene)
{

	repoDebug << "Writing scene...";

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

	auto vpTree = generateDefaultViewPointTree();

	sceneST.mergeSubTree(X3D_LABEL_VIEWPOINT, vpTree);

	tree.mergeSubTree(sceneLabel, sceneST);



	return true;
}


bool AbstractX3DModelExport::populateTree(
	const repo::core::model::RepoScene *scene)
{
	//Write xml header
	bool success = includeHeader();

	success &= writeScene(scene);


	return success;
}

bool AbstractX3DModelExport::populateTree(
	const repo::core::model::MeshNode &mesh,
	const repo::core::model::RepoScene *scene)
{
	//Write xml header
	bool success = includeHeader();

	success &= writeMultiPartMeshAsScene(mesh, scene);	


	return success;
}
