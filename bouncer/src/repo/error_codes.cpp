#include <unordered_map>
#include "repo/error_codes.h"

using namespace repo;

std::unordered_map<uint8_t, std::string> errorsHashMap = 
{
	{ REPOERR_LOAD_SCENE_FAIL, "Error occured while ODA processor is trying to read the file" },
	{ REPOERR_VALID_3D_VIEW_NOT_FOUND, "Valid 3D view wasn't found in Revit file" }
};

const std::string NO_DESCRIPTION = "No description found for this error code";

std::string repo::get3DRepoErrorDescription(uint8_t errorCode)
{
	auto it = errorsHashMap.find(errorCode);
	return it != errorsHashMap.end() ? it->second : NO_DESCRIPTION;
}