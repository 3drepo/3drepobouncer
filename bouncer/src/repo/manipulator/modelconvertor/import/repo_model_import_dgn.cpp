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
	auto meshSet = geoCollector.getMeshNodes();
	if (meshSet.size()) {
		const repo::core::model::RepoNodeSet dummy;

		auto matSet =  geoCollector.getMaterialNodes();
		auto transSet = geoCollector.getTransformationNodes();
		scene = new repo::core::model::RepoScene({ filePath }, dummy, meshSet, matSet, dummy, dummy, transSet);
		scene->setWorldOffset(geoCollector.getModelOffset());
	}
	else {
		repoError << "No meshes generated";
	}
	
#endif
	return scene;
}


bool DgnModelImport::importModel(std::string filePath, uint8_t &err)
{
	
#ifdef ODA_SUPPORT
	this->filePath = filePath;
	odaHelper::FileProcessor odaProcessor(filePath, &geoCollector);
	bool success = false;
	success = odaProcessor.readFile() == 0;
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