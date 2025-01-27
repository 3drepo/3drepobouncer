#include "repo_model_import_oda.h"
#include "../../../core/model/bson/repo_bson_factory.h"
#include "../../../error_codes.h"

#ifdef ODA_SUPPORT
#include <OdaCommon.h>
#include <Gs/GsBaseInclude.h>
#include "odaHelper/helper_functions.h"
#endif

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

repo::core::model::RepoScene* OdaModelImport::generateRepoScene(uint8_t &errMsg)
{
	// RepoSceneBuilder has already populated the collection with nodes having a
	// fixed revision id. This method creates a RepoScene instance that sits
	// those nodes to finalise the import.

	#ifdef ODA_SUPPORT

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

		delete sceneBuilder;
		sceneBuilder = nullptr;

		return scene;

	#else
		throw repo::lib::RepoImporterUnavailable("ODA support has not been compiled in. Please rebuild with ODA_SUPPORT ON", REPOERR_ODA_UNAVAILABLE);
	#endif
}

bool OdaModelImport::importModel(std::string filePath, std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler, uint8_t& err)
{
#ifdef ODA_SUPPORT
	this->filePath = filePath;
	repoInfo << " ==== Importing with Teigha Library [" << filePath << "] using RepoSceneBuilder ====";
	this->handler = handler;
	sceneBuilder = new repo::manipulator::modelutility::RepoSceneBuilder(
		handler,
		settings.getDatabaseName(),
		settings.getProjectName(),
		settings.getRevisionId()
	);

	odaProcessor = odaHelper::FileProcessor::getFileProcessor(filePath, sceneBuilder, settings);
	odaProcessor->readFile();
	sceneBuilder->finalise();

	return true;
#else
	throw repo::lib::RepoImporterUnavailable("ODA support has not been compiled in. Please rebuild with ODA_SUPPORT ON", REPOERR_ODA_UNAVAILABLE);
#endif
}