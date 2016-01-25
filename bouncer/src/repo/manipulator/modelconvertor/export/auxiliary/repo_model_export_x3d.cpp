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

using namespace repo::manipulator::modelconvertor;

//DOMs
static const std::string X3D_LABEL          = "X3D";
static const std::string X3D_LABEL_APP      = "Appearance";
static const std::string X3D_LABEL_EXT_GEO  = "ExternalGeometry";
static const std::string X3D_LABEL_GROUP    = "group";
static const std::string X3D_LABEL_MAT      = "Material";
static const std::string X3D_LABEL_SCENE    = "Scene";
static const std::string X3D_LABEL_SHAPE    = "Shape";

//Attribute Names
static const std::string X3D_ATTR_BBOX_CENTRE   = "bboxCenter";
static const std::string X3D_ATTR_BBOX_SIZE     = "bboxSize";
static const std::string X3D_ATTR_DEF           = "def";
static const std::string X3D_ATTR_DO_PICK_PASS  = "dopickpass";
static const std::string X3D_ATTR_ID            = "id";
static const std::string X3D_ATTR_ON_MOUSE_MOVE = "onmouseover";
static const std::string X3D_ATTR_ON_MOUSE_OVER = "onmousemove";
static const std::string X3D_ATTR_ON_CLICK      = "onclick";
static const std::string X3D_ATTR_ON_LOAD       = "onload";
static const std::string X3D_ATTR_RENDER        = "render";
static const std::string X3D_ATTR_URL           = "url";
static const std::string X3D_ATTR_XMLNS         = "xmlns";

//Values
static const std::string X3D_SPEC_URL      = "http://www.web3d.org/specification/x3d-namespace";
static const std::string X3D_ON_CLICK      = "clickObject(event);";
static const std::string X3D_ON_LOAD       = "onLoaded(event);";
static const std::string X3D_ON_MOUSE_MOVE = "onMouseOver(event);";
static const std::string X3D_ON_MOUSE_OVER = "onMouseMove(event);";


X3DModelExport::X3DModelExport(
	const repo::core::model::MeshNode &mesh,
	const repo::core::model::RepoScene *scene
	)
{
	tree.disableJSONWorkaround();
	convertSuccess = populateTree(mesh, scene);
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
