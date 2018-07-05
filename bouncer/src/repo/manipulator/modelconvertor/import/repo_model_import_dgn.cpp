#include <OdaCommon.h>
#include <StaticRxObject.h>
#include <RxInit.h>
#include <RxDynamicModule.h>

//#include "DynamicLinker.h"
//#include "DgDatabase.h"
//#include "RxDynamicModule.h"

//#include "ExSystemServices.h"
//#include "ExDgnServices.h"
//#include "ExDgnHostAppServices.h"

//#include "DgGiContext.h"
//#include "DgGsManager.h"

#include "repo_model_import_dgn.h"


using namespace repo::manipulator::modelconvertor;


DgnModelImport::DgnModelImport()
{
}


DgnModelImport::~DgnModelImport()
{
}


repo::core::model::RepoScene* DgnModelImport::generateRepoScene()
{
	return nullptr;
}


bool DgnModelImport::importModel(std::string filePath, uint8_t &err)
{
	OdString szSource = filePath.c_str();

	return true;
}