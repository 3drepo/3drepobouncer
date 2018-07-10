#include "repo_model_import_dgn.h"
#include "dgnHelper/oda_file_processor.h"
#include "../../../core/model/bson/repo_bson_factory.h"


using namespace repo::manipulator::modelconvertor;


DgnModelImport::DgnModelImport()
{
}


DgnModelImport::~DgnModelImport()
{
}


repo::core::model::RepoScene* DgnModelImport::generateRepoScene()
{
	repo::core::model::RepoScene *scene = nullptr;
	if (meshes.size()) {
		const repo::core::model::RepoNodeSet dummy;
		repo::core::model::RepoNodeSet meshSet;
		repo::core::model::RepoNodeSet transSet;
		
		auto root = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode());
		transSet.insert(root);
		auto rootID = root->getUniqueID();
		for (const auto &mesh : meshes) {
			
			meshSet.insert(new repo::core::model::MeshNode(mesh.cloneAndAddParent(rootID)));
		}
		scene = new repo::core::model::RepoScene({ filePath }, dummy, meshSet, dummy, dummy, dummy, transSet);
	}
	return scene;
}


bool DgnModelImport::importModel(std::string filePath, uint8_t &err)
{
	this->filePath = filePath;
	OdaFileProcessor odaProcessor(filePath);
	bool success = false;
	if (success = odaProcessor.readFile() == 0) {
		meshes = odaProcessor.getMeshes();
	}
	return success;
}