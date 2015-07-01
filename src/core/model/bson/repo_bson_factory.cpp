#include "repo_bson_factory.h"

using namespace repo::core::model::bson;

RepoNode RepoBSONFactory::makeRepoNode(std::string type){
	return RepoNode::createRepoNode(type);
}
