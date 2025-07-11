/**
*  Copyright (C) 2019 3D Repo Ltd
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

#include "repo_model_import_manager.h"
#include "repo/lib/repo_utils.h"
#include "repo/error_codes.h"
#include "repo_model_import_assimp.h"
#include "repo_model_import_ifc.h"
#include "repo_model_import_3drepo.h"
#include "repo_model_import_oda.h"
#include "repo_model_import_synchro.h"
#include "repo_model_units.h"
#include <boost/filesystem.hpp>

using namespace repo::manipulator::modelconvertor;

repo::core::model::RepoScene* ModelImportManager::ImportFromFile(
	const std::string &file,
	const repo::manipulator::modelconvertor::ModelImportConfig &config,
	std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler,
	uint8_t &error
) const {
	if (!repo::lib::doesFileExist(file)) {
		error = REPOERR_MODEL_FILE_READ;
		repoError << "Cannot find file: " << file;
		return nullptr;
	}
	auto modelConvertor = chooseModelConvertor(file, config);
	if (!modelConvertor) {
		error = REPOERR_FILE_TYPE_NOT_SUPPORTED;
		repoError << "Cannot find file: " << file;
		return nullptr;
	}

	repoTrace << "Importing model and generating Repo Scene";
	repo::core::model::RepoScene* scene = modelConvertor->importModel(file, handler, error);
	if (scene) {

		scene->setDatabaseAndProjectName(config.getDatabaseName(), config.getProjectName());

		auto fileUnits = modelConvertor->getUnits();
		auto targetUnits = config.getTargetUnits();

		auto unitsScale = determineScaleFactor(fileUnits, targetUnits);
		repoInfo << "File units: " << toUnitsString(fileUnits) << "\tTarget units: " << toUnitsString(targetUnits);

		if (unitsScale != 1.0) {
			repoInfo << "Applying scaling factor of " << unitsScale << " to convert " << toUnitsString(fileUnits) << " to " << toUnitsString(targetUnits);
			scene->applyScaleFactor(unitsScale);
		}

		if (!scene->hasRoot(repo::core::model::RepoScene::GraphType::DEFAULT)) {
			delete scene;
			scene = nullptr;
			error = REPOERR_NO_MESHES;
		}
		else {
			if (modelConvertor->requireReorientation()) {
				repoTrace << "rotating model by 270 degress on the x axis...";
				scene->reorientateDirectXModel();
			}

			error = REPOERR_OK;
		}
		
	}

	return scene;
}

std::shared_ptr<AbstractModelImport> ModelImportManager::chooseModelConvertor(
	const std::string &file,
	const repo::manipulator::modelconvertor::ModelImportConfig &config
) const {
	std::string fileExt = repo::lib::getExtension(file);
	repo::lib::toUpper(fileExt);

	std::shared_ptr<AbstractModelImport> modelConvertor = nullptr;

	//NOTE: IFC and ODA checks needs to be done before assimp otherwise assimp will try to import IFC/DXF
	if (fileExt == ".IFC")
		modelConvertor = std::shared_ptr<AbstractModelImport>(new repo::manipulator::modelconvertor::IFCModelImport(config));
	else if (repo::manipulator::modelconvertor::OdaModelImport::isSupportedExts(fileExt))
		modelConvertor = std::shared_ptr<AbstractModelImport>(new repo::manipulator::modelconvertor::OdaModelImport(config));
	else if (repo::manipulator::modelconvertor::AssimpModelImport::isSupportedExts(fileExt))
		modelConvertor = std::shared_ptr<AbstractModelImport>(new repo::manipulator::modelconvertor::AssimpModelImport(config));
	else if (fileExt == ".BIM")
		modelConvertor = std::shared_ptr<AbstractModelImport>(new repo::manipulator::modelconvertor::RepoModelImport(config));
	else if (fileExt == ".SPM")
		modelConvertor = std::shared_ptr<AbstractModelImport>(new repo::manipulator::modelconvertor::SynchroModelImport(config));

	return modelConvertor;
}