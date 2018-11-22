#include "repo_model_import_dgn.h"
#include "../../../core/model/bson/repo_bson_factory.h"
#include "../../../error_codes.h"

#ifdef ODA_SUPPORT
#include "odaHelper/file_processor.h"
#endif


using namespace repo::manipulator::modelconvertor;


DgnModelImport::DgnModelImport()
{
}


DgnModelImport::~DgnModelImport()
{
}

static repo_material_t createDefaultMaterial() {
	repo_material_t matStruct;
	matStruct.diffuse = { 1, 1, 1 };
	matStruct.opacity = 1;
}

repo::core::model::RepoScene* DgnModelImport::generateRepoScene()
{
	repo::core::model::RepoScene *scene = nullptr;
#ifdef ODA_SUPPORT
	repoInfo << "Constructing Repo Scene...";
	const repo::core::model::RepoNodeSet dummy;
	auto meshSet = geoCollector.getMeshNodes();
	if (!meshSet.size()) {
		repoInfo << "Get material nodes... ";
		auto matSet =  geoCollector.getMaterialNodes();
		auto transSet = geoCollector.getTransformationNodes();
		scene = new repo::core::model::RepoScene({ filePath }, dummy, meshSet, matSet, dummy, dummy, transSet);
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


bool DgnModelImport::importModel(std::string filePath, uint8_t &err)
{
	
#ifdef ODA_SUPPORT
	this->filePath = filePath;
	repoInfo << " ==== Importing with Teigha Library [" << filePath << "] ====";
	odaHelper::FileProcessor odaProcessor(filePath, &geoCollector);
	bool success = false;
	try {
		success = odaProcessor.readFile() == 0;
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