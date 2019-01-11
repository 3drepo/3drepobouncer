#include "repo_model_import_oda.h"
#include "repo/core/model/bson/repo_bson_factory.h"
#include "repo/error_codes.h"

#ifdef ODA_SUPPORT
#include "repo/manipulator/modelconvertor/import/odaHelper/file_processor_rvt.h"
#endif

using namespace repo::manipulator::modelconvertor;

const std::string OdaModelImport::DgnExt = ".dgn";
const std::string OdaModelImport::RvtExt = ".rvt";
const std::string OdaModelImport::RfaExt = ".rfa";

OdaModelImport::OdaModelImport()
{
}

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
	std::string extensions = DgnExt + RvtExt + RfaExt;

	std::string lowerExt(testExt);
	std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(), ::tolower);

	return extensions.find(lowerExt) != std::string::npos;
}

repo::core::model::RepoScene* OdaModelImport::generateRepoScene()
{
    repo::core::model::RepoScene *scene = nullptr;
#ifdef ODA_SUPPORT
    repoInfo << "Constructing Repo Scene...";
    const repo::core::model::RepoNodeSet dummy;
    auto meshSet = geoCollector.getMeshNodes();
    if (meshSet.size()) {
        repoInfo << "Get material nodes... ";
        auto matSet = geoCollector.getMaterialNodes();
		auto textSet = geoCollector.getTextureNodes();
        auto transSet = geoCollector.getTransformationNodes();
        scene = new repo::core::model::RepoScene({ filePath }, dummy, meshSet, matSet, dummy, textSet, transSet);
		if (geoCollector.hasMissingTextures())
			scene->setMissingTexture();
        scene->setWorldOffset(geoCollector.getModelOffset());
        repoInfo << "Repo Scene constructed.";
    }
    else {
        repoError << "No meshes generated";
        scene = new repo::core::model::RepoScene({ filePath }, dummy, dummy, dummy, dummy, dummy, dummy);
    }

#endif
    return scene;
}

bool OdaModelImport::importModel(std::string filePath, uint8_t &err)
{
#ifdef ODA_SUPPORT
    this->filePath = filePath;
    repoInfo << " ==== Importing with Teigha Library [" << filePath << "] ====";
    std::unique_ptr<odaHelper::FileProcessor> odaProcessor = odaHelper::FileProcessor::getFileProcessor(filePath, &geoCollector);
    bool success = false;
    try {
        success = odaProcessor != nullptr && odaProcessor->readFile() == 0;
    }
    catch (...)
    {
        success = false;
        repoError << "Process errored whilst ODA processor is trying to read the file";
    }
    if (!success) {
        err = REPOERR_LOAD_SCENE_FAIL;
        repoInfo << "Failed to read file";
    }
    return success;
#else
    //ODA support not compiled in. 
    err = REPOERR_ODA_UNAVAILABLE;
    return false;
#endif
}

