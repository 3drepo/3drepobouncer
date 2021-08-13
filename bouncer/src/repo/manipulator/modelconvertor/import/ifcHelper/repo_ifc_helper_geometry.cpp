/**
*  Copyright (C) 2015 3D Repo Ltd
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
* Allows geometry creation using ifcopenshell
*/

#include <ifcUtils/repo_ifc_utils_ifc2x3.h>
#include <ifcUtils/repo_ifc_utils_ifc4.h>
#include "repo_ifc_helper_common.h"
#include "repo_ifc_helper_geometry.h"
#include "../../../../core/model/bson/repo_bson_factory.h"
#include <boost/filesystem.hpp>
#include "../repo_model_import_config_default_values.h"

using namespace repo::manipulator::modelconvertor::ifcHelper;

IFCUtilsGeometry::IFCUtilsGeometry(const std::string &file, const modelConverter::ModelImportConfig &settings) :
	file(file)
{
}

IFCUtilsGeometry::~IFCUtilsGeometry()
{
}

bool IFCUtilsGeometry::generateGeometry(
	std::string &errMsg,
	bool        &partialFailure)
{
	partialFailure = false;

	std::vector<std::vector<repo_face_t>> allFaces;
	std::vector<std::vector<double>> allVertices;
	std::vector<std::vector<double>> allNormals;
	std::vector<std::vector<double>> allUVs;
	std::vector<std::string> allIds, allNames, allMaterials;
	std::unordered_map<std::string, repo_material_t> matNameToMaterials;

	switch (getIFCSchema(file)) {
	case IfcSchemaVersion::IFC2x3:
		if (!IfcUtils::Schema_Ifc2x3::GeometryHandler::retrieveGeometry(file,
			allVertices, allFaces, allNormals, allUVs, allIds, allNames, allMaterials, matNameToMaterials, offset, errMsg))
			return false;
		break;
	case IfcSchemaVersion::IFC4:
		if (!IfcUtils::Schema_Ifc4::GeometryHandler::retrieveGeometry(file,
			allVertices, allFaces, allNormals, allUVs, allIds, allNames, allMaterials, matNameToMaterials, offset, errMsg))
			return false;
		break;
	default:
		errMsg = "Unsupported IFC Version";
		return false;
	}

	//now we have found all meshes, take the minimum bounding box of the scene as offset
	//and create repo meshes
	repoTrace << "Finished iterating. number of meshes found: " << allVertices.size();
	repoTrace << "Finished iterating. number of materials found: " << materials.size();

	std::map<std::string, std::vector<repo::lib::RepoUUID>> materialParent;
	std::string defaultMaterialName = "_3DREPO_DEFAULT_MAT";
	std::vector<repo::lib::RepoUUID> parentsOfDefault;
	for (int i = 0; i < allVertices.size(); ++i)
	{
		std::vector<repo::lib::RepoVector3D> vertices, normals;
		std::vector<repo::lib::RepoVector2D> uvs;
		std::vector<repo_face_t> faces;
		std::vector<std::vector<float>> boundingBox;
		for (int j = 0; j < allVertices[i].size(); j += 3)
		{
			vertices.push_back({ (float)(allVertices[i][j] - offset[0]), (float)(allVertices[i][j + 1] - offset[1]), (float)(allVertices[i][j + 2] - offset[2]) });
			if (allNormals[i].size())
				normals.push_back({ (float)allNormals[i][j], (float)allNormals[i][j + 1], (float)allNormals[i][j + 2] });

			auto vertex = vertices.back();
			if (j == 0)
			{
				boundingBox.push_back({ vertex.x, vertex.y, vertex.z });
				boundingBox.push_back({ vertex.x, vertex.y, vertex.z });
			}
			else
			{
				boundingBox[0][0] = boundingBox[0][0] > vertex.x ? vertex.x : boundingBox[0][0];
				boundingBox[0][1] = boundingBox[0][1] > vertex.y ? vertex.y : boundingBox[0][1];
				boundingBox[0][2] = boundingBox[0][2] > vertex.z ? vertex.z : boundingBox[0][2];

				boundingBox[1][0] = boundingBox[1][0] < vertex.x ? vertex.x : boundingBox[1][0];
				boundingBox[1][1] = boundingBox[1][1] < vertex.y ? vertex.y : boundingBox[1][1];
				boundingBox[1][2] = boundingBox[1][2] < vertex.z ? vertex.z : boundingBox[1][2];
			}
		}

		for (int j = 0; j < allUVs[i].size(); j += 2)
		{
			uvs.push_back({ (float)allUVs[i][j], (float)allUVs[i][j + 1] });
		}

		std::vector < std::vector<repo::lib::RepoVector2D>> uvChannels;
		if (uvs.size())
			uvChannels.push_back(uvs);

		auto mesh = repo::core::model::RepoBSONFactory::makeMeshNode(vertices, allFaces[i], normals, boundingBox, uvChannels,
			std::vector<repo_color4d_t>(), std::vector<std::vector<float>>());

		if (meshes.find(allIds[i]) == meshes.end())
		{
			meshes[allIds[i]] = std::vector<repo::core::model::MeshNode*>();
		}
		meshes[allIds[i]].push_back(new repo::core::model::MeshNode(mesh));

		if (allMaterials[i] != "")
		{
			if (materialParent.find(allMaterials[i]) == materialParent.end())
			{
				materialParent[allMaterials[i]] = std::vector<repo::lib::RepoUUID>();
			}

			materialParent[allMaterials[i]].push_back(mesh.getSharedID());
		}
		else
		{
			//This mesh has no material, assigning a default
			if (matNameToMaterials.find(defaultMaterialName) == matNameToMaterials.end())
			{
				repo_material_t matProp;
				matProp.diffuse = { 0.5, 0.5, 0.5, 1 };
				matNameToMaterials[defaultMaterialName] = matProp;
			}
			if (materialParent.find(defaultMaterialName) == materialParent.end())
			{
				materialParent[defaultMaterialName] = std::vector<repo::lib::RepoUUID>();
			}
			materialParent[defaultMaterialName].push_back(mesh.getSharedID());
		}
	}

	repoTrace << "Meshes constructed. Wiring materials to parents...";

	for (const auto &pair : materialParent)
	{
		auto matIt = matNameToMaterials.find(pair.first);
		if (matIt != matNameToMaterials.end())
		{
			auto matNode = (new repo::core::model::MaterialNode(repo::core::model::RepoBSONFactory::makeMaterialNode(matIt->second, pair.first, pair.second)));
			materials[pair.first] = matNode;
		}
	}

	return true;
}