#include "repo_bson_builder.h"

using namespace repo::core::model;

RepoBSONBuilder::RepoBSONBuilder()
	:mongo::BSONObjBuilder()
{
}

RepoBSONBuilder::~RepoBSONBuilder()
{
}

void RepoBSONBuilder::appendUUID(
	const std::string &label,
	const repoUUID &uuid)
{
	appendBinData(label, uuid.size(), mongo::bdtUUID, (char*)uuid.data);
}

void RepoBSONBuilder::appendArrayPair(
	const std::string &label,
	const std::list<std::pair<std::string, std::string> > &list,
	const std::string &fstLabel,
	const std::string &sndLabel
	)
{
	if (list.size() > 0)
	{
		mongo::BSONArrayBuilder arrBuilder;
		for (auto it = list.begin(); it != list.end(); ++it)
		{
			RepoBSONBuilder innerBuilder;
			innerBuilder << fstLabel << it->first;
			innerBuilder << sndLabel << it->second;
			arrBuilder.append(innerBuilder.obj());
		}
		append(label, arrBuilder.arr());
	}
	else
	{
		repoWarning << "Trying to append an empty array pair with label:" << label;
	}
}

RepoBSON RepoBSONBuilder::obj()
{
	mongo::BSONObjBuilder build;
	return RepoBSON(mongo::BSONObjBuilder::obj());
}

template<> void repo::core::model::RepoBSONBuilder::append < repoUUID >
	(
		const std::string &label,
		const repoUUID &uuid
		)
{
	appendUUID(label, uuid);
}

	template<> void repo::core::model::RepoBSONBuilder::append < repo_vector_t >
		(
			const std::string &label,
			const repo_vector_t &vec
			)
	{
		appendArray(label, std::vector<float>({ vec.x, vec.y, vec.z }));
	}