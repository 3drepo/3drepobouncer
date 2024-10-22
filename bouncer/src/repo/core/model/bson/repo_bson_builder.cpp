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

void repo::core::model::RepoBSONBuilder::appendRepoVariant(const std::string& label, const repo::lib::RepoVariant& item)
{
	// Apply visitor to handle the variant.
	boost::apply_visitor(AppendVisitor(*this, label), item);
}

RepoBSON RepoBSONBuilder::obj()
{
	mongo::BSONObjBuilder build;
	return RepoBSON(mongo::BSONObjBuilder::obj(), binMapping);
}

template<> void repo::core::model::RepoBSONBuilder::append<repo::lib::RepoUUID>
(
	const std::string &label,
	const repo::lib::RepoUUID &uuid
)
{
	appendUUID(label, uuid);
}

template<> void repo::core::model::RepoBSONBuilder::append<tm>
	(
		const std::string& label,
		const tm& time
		)
{
	appendTime(label, time);
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

void RepoBSONBuilder::appendLargeArray(std::string name, const void* data, size_t size)
{
	binMapping[name] = std::vector<uint8_t>();
	auto& buf = binMapping[name];
	buf.resize(size);
	memcpy(buf.data(), data, size);
}

void RepoBSONBuilder::appendTime(std::string label, const int64_t& ts) {
	mongo::Date_t date = mongo::Date_t(ts * 1000); // Mongo expects time in ms
	mongo::BSONObjBuilder::append(label, date);
}

void RepoBSONBuilder::appendTime(std::string label, const tm& t) {
	tm tmCpy = t; // Copy because mktime can alter the struct
	int64_t time = static_cast<int64_t>(mktime(&tmCpy));

	// Check for a unsuccessful conversion
	if (time == -1)
	{
		throw repo::lib::RepoException("Failed converting date to mongo compatible format. tm malformed or date pre 1970?");
	}

	// Convert from seconds to milliseconds
	time = time * 1000;

	// Append time
	appendTime(label, time);
}

void RepoBSONBuilder::appendTimeStamp(std::string label) {
	appendTime(label, time(NULL));
}

void RepoBSONBuilder::appendElements(RepoBSON bson) {
	mongo::BSONObjBuilder::appendElements(bson);
}

void RepoBSONBuilder::appendElementsUnique(RepoBSON bson) {
	mongo::BSONObjBuilder::appendElementsUnique(bson);
}

void RepoBSONBuilder::appendArray(
	const std::string& label,
	const RepoBSON& bson)
{
	mongo::BSONObjBuilder::appendArray(label, bson);
}