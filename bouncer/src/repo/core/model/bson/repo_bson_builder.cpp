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

#include <bsoncxx/types.hpp>
#include <bsoncxx/types/value.hpp>
#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>

using namespace repo::core::model;
using namespace bsoncxx::builder;

RepoBSONBuilder::RepoBSONBuilder()
	:core(false)
{
}

RepoBSONBuilder::~RepoBSONBuilder()
{
}

void RepoBSONBuilder::appendUUID(
	const std::string &label,
	const repo::lib::RepoUUID &uuid)
{
	append(label, uuid);
}

void repo::core::model::RepoBSONBuilder::appendRepoVariant(const std::string& label, const repo::lib::RepoVariant& item)
{
	// Apply visitor to handle the variant.
	boost::apply_visitor(AppendVisitor(*this, label), item);
}

RepoBSON RepoBSONBuilder::obj()
{
	return RepoBSON(core::extract_document(), binMapping);
}

void repo::core::model::RepoBSONBuilder::append(const repo::lib::RepoUUID& uuid)
{
	auto uuidData = uuid.data();
	bsoncxx::types::b_binary binary{
		bsoncxx::binary_sub_type::k_uuid,
		uuidData.size(),
		uuidData.data()
	};
	append(binary);
}

void repo::core::model::RepoBSONBuilder::append(const repo::lib::RepoBounds& bounds)
{
	open_array();
	append(bounds.min().toStdVector());
	append(bounds.max().toStdVector());
	close_array();
}

void repo::core::model::RepoBSONBuilder::append(const repo::lib::RepoVector3D& vec)
{
	append(vec.toStdVector());
}

void repo::core::model::RepoBSONBuilder::append(const repo::lib::RepoMatrix& mat)
{
	bsoncxx::builder::basic::array rows{};
	auto data = mat.getData();
	for (uint32_t i = 0; i < 4; ++i)
	{
		bsoncxx::builder::basic::array columns;
		for (uint32_t j = 0; j < 4; ++j) {
			columns.append(data[i * 4 + j]);
		}
		rows.append(columns.extract());
	}
	core::append(rows.extract());
}

void repo::core::model::RepoBSONBuilder::append(const RepoBSON& obj)
{
	append(obj.view());
}

void RepoBSONBuilder::appendVector3DObject(
	const std::string& label,
	const repo::lib::RepoVector3D& vec
)
{
	using namespace bsoncxx::builder::basic;
	document objBuilder;
	objBuilder.append(kvp("x", vec.x));
	objBuilder.append(kvp("y", vec.y));
	objBuilder.append(kvp("z", vec.z));
	append(label, objBuilder.extract());
}

void RepoBSONBuilder::appendLargeArray(std::string name, const void* data, size_t size)
{
	binMapping[name] = std::vector<uint8_t>();
	auto& buf = binMapping[name];
	buf.resize(size);
	memcpy(buf.data(), data, size);
}

void RepoBSONBuilder::appendTime(std::string label, const int64_t& ts)
{
	bsoncxx::types::b_date date(std::chrono::milliseconds(ts * 1000));
	key_owned(label);
	append(date);
}

void RepoBSONBuilder::appendTime(std::string label, const tm& t) {
	tm tmCpy = t; // Copy because mktime can alter the struct
	int64_t time = static_cast<int64_t>(mktime(&tmCpy));

	// Check for a unsuccessful conversion
	if (time == -1)
	{
		throw repo::lib::RepoException("Failed converting date to mongo compatible format. tm malformed or date pre 1970?");
	}

	// Append time
	appendTime(label, time);
}

void RepoBSONBuilder::appendTimeStamp(std::string label) {
	appendTime(label, time(NULL));
}

void RepoBSONBuilder::appendElements(RepoBSON bson) {
	core::concatenate(bson.view());
}

void RepoBSONBuilder::appendElementsUnique(RepoBSON bson) {
	auto view = core::view_document();
	for (auto& element : bson) {
		if (view.find(element.key()) == view.end()) {
			key_view(element.key());
			append(element.get_value());
		}
	}
}