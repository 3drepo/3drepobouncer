/*
*  Copyright(C) 2015 3D Repo Ltd
*
*  This program is free software : you can redistribute it and / or modify
*  it under the terms of the GNU Affero General Public License as
*  published by the Free Software Foundation, either version 3 of the
*  License, or(at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
*  GNU Affero General Public License for more details.
*
*  You should have received a copy of the GNU Affero General Public License
*  along with this program.If not, see <http://www.gnu.org/licenses/>.
*/

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
	const repo::lib::RepoUUID &uuid)
{
	auto uuidData = uuid.data();
	appendBinData(label, uuidData.size(), mongo::bdtUUID, (char*)uuidData.data());
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
			arrBuilder.append(innerBuilder.mongoObj());
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

template<> void repo::core::model::RepoBSONBuilder::append<repo::lib::RepoUUID>
(
	const std::string &label,
	const repo::lib::RepoUUID &uuid
)
{
	appendUUID(label, uuid);
}

template<> void repo::core::model::RepoBSONBuilder::append<repo::lib::RepoVector3D>
(
	const std::string &label,
	const repo::lib::RepoVector3D &vec
)
{
	appendArray(label, vec.toStdVector());
}

template<> void repo::core::model::RepoBSONBuilder::append<repo::lib::RepoMatrix>
(
	const std::string &label,
	const repo::lib::RepoMatrix &mat
)
{
	RepoBSONBuilder rows;
	auto data = mat.getData();
	for (uint32_t i = 0; i < 4; ++i)
	{
		RepoBSONBuilder columns;
		for (uint32_t j = 0; j < 4; ++j){
			columns << std::to_string(j) << data[i * 4 + j];
		}
		rows.appendArray(std::to_string(i), columns.obj());
	}
	appendArray(label, rows.obj());;
}

void RepoBSONBuilder::appendVector3DObject(
	const std::string& label,
	const repo::lib::RepoVector3D& vec
)
{
	mongo::BSONObjBuilder objBuilder;
	objBuilder.append("x", vec.x);
	objBuilder.append("y", vec.y);
	objBuilder.append("z", vec.z);
	append(label, objBuilder.obj());
}