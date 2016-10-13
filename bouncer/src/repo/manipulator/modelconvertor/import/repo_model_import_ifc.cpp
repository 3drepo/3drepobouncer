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
* Allows Import/Export functionality into/output Repo world using IFCOpenShell
*/

#include "repo_model_import_ifc.h"
#include "repo_ifc_utils_geometry.h"
#include "repo_ifc_utils_parser.h"
#include "../../../core/model/bson/repo_bson_factory.h"
#include <boost/filesystem.hpp>

using namespace repo::manipulator::modelconvertor;

IFCModelImport::IFCModelImport(const ModelImportConfig *settings) :
AbstractModelImport(settings)
{
	if (settings)
	{
		repoWarning << "IFC importer currently ignores all import settings. Any bespoke configurations will be ignored.";
	}
}

IFCModelImport::~IFCModelImport()
{
}

repo::core::model::RepoScene* IFCModelImport::generateRepoScene()
{
	IFCUtilsParser parserUtil(ifcFile);
	std::string errMsg;
	auto scene = parserUtil.generateRepoScene(errMsg, meshes, materials, offset);
	if (!scene)
		repoError << "Failed to generate Repo Scene: " << errMsg;
	return scene;
}

bool IFCModelImport::importModel(std::string filePath, std::string &errMsg)
{
	ifcFile = filePath;
	std::string fileName = getFileName(filePath);

	repoInfo << "IMPORT [" << fileName << "]";
	repoInfo << "=== IMPORTING MODEL WITH IFC OPEN SHELL ===";
	bool success = false;

	IFCUtilsGeometry geoUtil(filePath);
	if (success = geoUtil.generateGeometry(errMsg))
	{
		//generate tree;
		repoInfo << "Geometry generated successfully";
		meshes = geoUtil.getGeneratedMeshes();
		materials = geoUtil.getGeneratedMaterials();
		offset = geoUtil.getGeometryOffset();
	}
	else
	{
		repoError << "Failed to generate geometry: " << errMsg;
	}

	return success;
}