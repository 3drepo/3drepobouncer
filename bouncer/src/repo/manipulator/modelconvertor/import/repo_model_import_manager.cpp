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
#include "../../../lib/repo_utils.h"
#include "../../../error_codes.h"
#include "repo_model_import_assimp.h"
#include "repo_model_import_ifc.h"
#include "repo_model_import_3drepo.h"
#include "repo_model_import_oda.h"
#include "repo_model_import_synchro.h"
#include "../../modeloptimizer/repo_optimizer_trans_reduction.h"
#include <boost/filesystem.hpp>


using namespace repo::manipulator::modelconvertor;

repo::core::model::RepoScene* ModelImportManager::ImportFromFile(
	const std::string &file, 
	const bool reorientateModel,
	const bool applyReduction,
	uint8_t &error
) const {

	if (!repo::lib::doesFileExist(file)) {
		error = REPOERR_MODEL_FILE_READ;
		repoError << "Cannot find file: " << file;
		return nullptr;
	}
	auto modelConvertor = chooseModelConvertor(file);
	if (!modelConvertor) {
		error = REPOERR_FILE_TYPE_NOT_SUPPORTED;
		repoError << "Cannot find file: " << file;
		return nullptr;
	}

	repo::core::model::RepoScene* scene = nullptr;
	repoTrace << "Importing model...";
	if (modelConvertor->importModel(file, error)) {
		repoTrace << "model Imported, generating Repo Scene";
		scene = modelConvertor->generateRepoScene();

		if (!scene) {
			error = REPOERR_UNKNOWN_ERR;
		} else {
			if (scene->exceedsMaximumNodes()) {
				delete scene;
				scene = nullptr;
				error = REPOERR_MAX_NODES_EXCEEDED;
			} else if (!scene->getAllMeshes(repo::core::model::RepoScene::GraphType::DEFAULT).size()) {
				delete scene;
				scene = nullptr;
				error = REPOERR_NO_MESHES;
			} else {
				if (reorientateModel || modelConvertor->requireReorientation()) {
					repoTrace << "rotating model by 270 degress on the x axis...";
					scene->reorientateDirectXModel();
				}

				if (applyReduction || modelConvertor->applyReduction()) {
					repoTrace << "Applying transformation reduction optimizer";
					repo::manipulator::modeloptimizer::TransformationReductionOptimizer optimizer;
					optimizer.apply(scene);
				}

				error = REPOERR_OK;
			}
		}
	}
	return scene;
}

std::shared_ptr<AbstractModelImport> ModelImportManager::chooseModelConvertor(
	const std::string &file
) const {
	boost::filesystem::path filePathP(file);
	std::string fileExt = filePathP.extension().string();
	std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::toupper);

	std::shared_ptr<AbstractModelImport> modelConvertor = nullptr;

	if (fileExt == ".IFC")
		modelConvertor = std::shared_ptr<AbstractModelImport>(new repo::manipulator::modelconvertor::IFCModelImport());
	else if (repo::manipulator::modelconvertor::AssimpModelImport::isSupportedExts(fileExt))
		modelConvertor = std::shared_ptr<AbstractModelImport>(new repo::manipulator::modelconvertor::AssimpModelImport());
	else if (fileExt == ".BIM")
		modelConvertor = std::shared_ptr<AbstractModelImport>(new repo::manipulator::modelconvertor::RepoModelImport());
	else if (repo::manipulator::modelconvertor::OdaModelImport::isSupportedExts(fileExt))
		modelConvertor = std::shared_ptr<AbstractModelImport>(new repo::manipulator::modelconvertor::OdaModelImport());
	else if (fileExt == ".SPM")
		modelConvertor = std::shared_ptr<AbstractModelImport>(new repo::manipulator::modelconvertor::SynchroModelImport());

	return modelConvertor;
}