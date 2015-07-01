#include "repo_bson_builder.h"

using namespace repo::core::model::bson;

RepoBSONBuilder::RepoBSONBuilder()
	:mongo::BSONObjBuilder()
{
}


RepoBSONBuilder::~RepoBSONBuilder()
{
}

void RepoBSONBuilder::append(
	const std::string &label,
	const repo_uuid &uuid)
{
	appendBinData(label, uuid.size(), mongo::bdtUUID, (char*)uuid.data);

}

RepoBSON RepoBSONBuilder::obj()
{
	mongo::BSONObjBuilder build;
	return RepoBSON(mongo::BSONObjBuilder::obj());
}
