#include "repo_model_import_dgn.h"
#include "../../../core/model/bson/repo_bson_factory.h"
#include "../../../error_codes.h"

#ifdef ODA_SUPPORT
#include "odaHelper/oda_file_processor.h"
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
	auto meshes = geoCollector.getMeshes();
	repoInfo << "mesh size: " << meshes.size();
	if (meshes.size()) {
		auto mats = geoCollector.getMaterialMappings();
		const repo::core::model::RepoNodeSet dummy;
		repo::core::model::RepoNodeSet meshSet;
		repo::core::model::RepoNodeSet transSet;		
		
		auto root = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode());
		
		transSet.insert(root);
		auto rootID = root->getSharedID();
		std::vector<repo::lib::RepoUUID> meshIDs;
		for (int i = 0; i < meshes.size(); ++i) {
			auto mesh = meshes[i];			
			meshSet.insert(new repo::core::model::MeshNode(mesh.cloneAndAddParent(rootID)));
			meshIDs.push_back(mesh.getSharedID());			
		}

		auto matSet =  geoCollector.getMaterialNodes();
		repoInfo << "# materials: " << matSet.size();
		scene = new repo::core::model::RepoScene({ filePath }, dummy, meshSet, matSet, dummy, dummy, transSet);
		scene->setWorldOffset(geoCollector.getModelOffset());
	}
#endif
	return scene;
}


bool DgnModelImport::importModel(std::string filePath, uint8_t &err)
{
	
#ifdef ODA_SUPPORT
	this->filePath = filePath;
	odaHelper::OdaFileProcessor odaProcessor(filePath, &geoCollector);
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