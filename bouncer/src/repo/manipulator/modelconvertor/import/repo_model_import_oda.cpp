/**
*  Copyright (C) 2025 3D Repo Ltd
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

#include "repo_model_import_oda.h"
#include "repo/core/model/bson/repo_bson_factory.h"
#include "repo/error_codes.h"

#ifdef ODA_SUPPORT
#include <OdaCommon.h>
#include <Gs/GsBaseInclude.h>
#include "odaHelper/helper_functions.h"
#endif

using namespace repo::lib;
using namespace repo::manipulator::modelconvertor;

const std::string OdaModelImport::supportedExtensions = ".dgn.rvt.rfa.dwg.dxf.nwd.nwc";

OdaModelImport::~OdaModelImport()
{
}

static repo_material_t createDefaultMaterial() {
	repo_material_t matStruct;
	matStruct.diffuse = { 1, 1, 1 };
	matStruct.opacity = 1;
}

bool OdaModelImport::isSupportedExts(const std::string &testExt)
{
	std::string lowerExt(testExt);
	std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(), ::tolower);

	return supportedExtensions.find(lowerExt) != std::string::npos;
}

repo::core::model::RepoScene* OdaModelImport::importModel(std::string filePath, std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler, uint8_t& err)
{
#ifdef ODA_SUPPORT
	this->filePath = filePath;
	repoInfo << " ==== Importing with Teigha Library [" << filePath << "] using RepoSceneBuilder ====";
	this->handler = handler;
	sceneBuilder = std::make_unique<repo::manipulator::modelutility::RepoSceneBuilder>(
		handler,
		settings.getDatabaseName(),
		settings.getProjectName(),
		settings.getRevisionId()
	);
	sceneBuilder->createIndexes(false);

	odaProcessor = odaHelper::FileProcessor::getFileProcessor(filePath, sceneBuilder.get(), settings);
	auto result = odaProcessor->readFile();

	if (result != REPOERR_OK) {
		throw repo::lib::RepoImportException(result);
	}

	sceneBuilder->finalise();

	repoInfo << "Initialising Repo Scene...";

	this->modelUnits = sceneBuilder->getUnits();

	repo::core::model::RepoScene* scene = new repo::core::model::RepoScene(
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
#else
	throw repo::lib::RepoImporterUnavailable("ODA support has not been compiled in. Please rebuild with ODA_SUPPORT ON", REPOERR_ODA_UNAVAILABLE);
#endif
}