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
	const repoUUID &uuid)
{
	appendBinData(label, uuid.size(), mongo::bdtUUID, (char*)uuid.data);

}

//FIXME: this is silly. should just pass everything in as vectors
void RepoBSONBuilder::appendVector(
	const std::string    &label,
	const repo_vector_t vec
	)
{

	float vector[] = { vec.x, vec.y, vec.z };
	RepoBSONBuilder array;
	for (uint32_t i = 0; i < 3; ++i)
		array << boost::lexical_cast<std::string>(i) << vector[i];

	appendArray(label, array.obj());
}


RepoBSON RepoBSONBuilder::obj()
{
	mongo::BSONObjBuilder build;
	return RepoBSON(mongo::BSONObjBuilder::obj());
}
