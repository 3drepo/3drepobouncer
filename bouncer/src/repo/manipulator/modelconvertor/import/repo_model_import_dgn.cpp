#include "repo_model_import_dgn.h"
#include "dgnHelper/oda_file_processor.h"

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
	std::cout << " Importing model file: " << filePath << std::endl;
	OdaFileProcessor odaProcessor(filePath);
	odaProcessor.readFile();

	return true;
}