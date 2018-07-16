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
	auto meshes = geoCollector.getMeshes();
	if (meshes.size()) {
		auto mats = geoCollector.getMaterialMappings();
		std::unordered_map<uint32_t, std::vector<repo::lib::RepoUUID>> matCodeToMeshIDs;
		const repo::core::model::RepoNodeSet dummy;
		repo::core::model::RepoNodeSet meshSet;
		repo::core::model::RepoNodeSet transSet;
		repo::core::model::RepoNodeSet matSet;
		
		auto root = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode());
		
		transSet.insert(root);
		auto rootID = root->getSharedID();
		std::vector<repo::lib::RepoUUID> meshIDs;
		for (int i = 0; i < meshes.size(); ++i) {
			auto mesh = meshes[i];			
			meshSet.insert(new repo::core::model::MeshNode(mesh.cloneAndAddParent(rootID)));
			meshIDs.push_back(mesh.getSharedID());

			if (mats.size() > i)
			{
				auto code = mats[i];
				if (matCodeToMeshIDs.find(code) == matCodeToMeshIDs.end()) {
					matCodeToMeshIDs[code] = std::vector<repo::lib::RepoUUID>();
				}

				matCodeToMeshIDs[code].push_back(mesh.getSharedID());
			}
		}

		
		auto codeToMat = geoCollector.getMaterialNodes();
		for (const auto &pair : codeToMat)
		{
			if (matCodeToMeshIDs.find(pair.first) != matCodeToMeshIDs.end())
			{
				matSet.insert(new repo::core::model::MaterialNode(pair.second.cloneAndAddParent(matCodeToMeshIDs[pair.first])));
			}
		}

		scene = new repo::core::model::RepoScene({ filePath }, dummy, meshSet, matSet, dummy, dummy, transSet);
		scene->setWorldOffset(geoCollector.getModelOffset());
	}
	return scene;
}


bool DgnModelImport::importModel(std::string filePath, uint8_t &err)
{
	
#ifdef ODA_SUPPORT
	this->filePath = filePath;
	OdaFileProcessor odaProcessor(filePath, &geoCollector);
	bool success = false;
	success = odaProcessor.readFile() == 0;
	if (!success) {		
		err = REPOERR_LOAD_SCENE_FAIL;
	}
	return success;
#else
	//ODA support not compiled in. 
	err = REPOERR_ODA_UNAVAILABLE;
	return false;
#endif
}