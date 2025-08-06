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
#include "repo/manipulator/modelutility/repo_scene_builder.h"
#include "repo_ifc_utils.h"
#include "repo/error_codes.h"
#include <boost/filesystem.hpp>

using namespace repo::manipulator::modelconvertor;

IFCModelImport::IFCModelImport(const ModelImportConfig &settings) :
	AbstractModelImport(settings),
	partialFailure(false),
	scene(nullptr)
{
	modelUnits = ModelUnits::METRES;
}

IFCModelImport::~IFCModelImport()
{
}

repo::core::model::RepoScene* IFCModelImport::importModel(std::string filePath, std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler, uint8_t& err)
{
	repoInfo << "IMPORT [" << filePath << "]";
	repoInfo << "=== IMPORTING MODEL WITH IFC OPEN SHELL ===";

	auto sceneBuilder = std::make_unique<repo::manipulator::modelutility::RepoSceneBuilder>(
		handler,
		settings.getDatabaseName(),
		settings.getProjectName(),
		settings.getRevisionId()
		);
	sceneBuilder->createIndexes();

	auto serialiser = ifcUtils::IfcUtils::CreateSerialiser(filePath);

	serialiser->setNumThreads(settings.getNumThreads());
	serialiser->setLevelOfDetail(settings.getLevelOfDetail());

	serialiser->import(sceneBuilder.get());

	sceneBuilder->finalise();

	scene = new repo::core::model::RepoScene(
		settings.getDatabaseName(),
		settings.getProjectName()
	);
	scene->setRevision(settings.getRevisionId());
	scene->setOriginalFiles({ filePath });
	scene->loadRootNode(handler.get());
	scene->setWorldOffset(sceneBuilder->getWorldOffset());

	if (sceneBuilder->hasMissingTextures()) {
		scene->setMissingTexture();
	}

	return scene;
}