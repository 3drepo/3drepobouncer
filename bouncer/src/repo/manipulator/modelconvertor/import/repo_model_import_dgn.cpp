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
		repo::core::model::RepoNodeSet matSet;
		
		auto root = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode());
		
		transSet.insert(root);
		auto rootID = root->getSharedID();
		std::vector<repo::lib::RepoUUID> meshIDs;
		for (const auto &mesh : meshes) {
			
			meshSet.insert(new repo::core::model::MeshNode(mesh.cloneAndAddParent(rootID)));
			meshIDs.push_back(mesh.getSharedID());
		}

		repo_material_t matStruct;
		matStruct.diffuse = { 1, 1, 1 };
		matStruct.opacity = 1;

		auto mat = repo::core::model::RepoBSONFactory::makeMaterialNode(matStruct);

		matSet.insert(new repo::core::model::MaterialNode(mat.cloneAndAddParent(meshIDs)));
		scene = new repo::core::model::RepoScene({ filePath }, dummy, meshSet, matSet, dummy, dummy, transSet);
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