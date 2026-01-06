/**
*  Copyright (C) 2025 3D Repo Ltd
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU Affero General Public License as
*  published by the Free Software Foundation, either version 3 of the
*  License, or (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU Affero General Public License for more details.
*
*  You should have received a copy of the GNU Affero General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "clash_exceptions.h"

#include <repo/manipulator/modelutility/repo_clash_detection_config.h>

#include <sstream>

#define RAPIDJSON_HAS_STDSTRING 1
#include "repo/manipulator/modelutility/rapidjson/rapidjson.h"
#include "repo/manipulator/modelutility/rapidjson/document.h"
#include "repo/manipulator/modelutility/rapidjson/writer.h"
#include "repo/manipulator/modelutility/rapidjson/ostreamwrapper.h"

using namespace repo::manipulator::modelutility::clash;

void writeContainerParams(const repo::lib::Container& container, rapidjson::Writer<rapidjson::OStreamWrapper>& writer)
{
	writer.Key("teamspace");
	writer.String(container.teamspace.c_str());
	writer.Key("container");
	writer.String(container.container.c_str());
	writer.Key("revision");
	writer.String(container.revision.toString().c_str());
}

MeshBoundsException::MeshBoundsException(const repo::lib::Container& container, const repo::lib::RepoUUID& uniqueId)
	: container(container), uniqueId(uniqueId)
{
}

std::shared_ptr<ClashDetectionException> MeshBoundsException::clone() const {
	return std::make_shared<MeshBoundsException>(*this);
}

std::string MeshBoundsException::toJson() const {
	
	std::basic_ostringstream<char> os;
	rapidjson::OStreamWrapper osw(os);
	rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);

	writer.StartObject();
	writer.Key("type");
	writer.String("MeshBoundsException");
	writeContainerParams(container, writer);
	writer.Key("uniqueId");
	writer.String(uniqueId.toString());
	writer.EndObject();

	return os.str();
}

TransformBoundsException::TransformBoundsException(const repo::lib::Container& container, const repo::lib::RepoUUID& uniqueId)
	: container(container), uniqueId(uniqueId)
{
}

std::string TransformBoundsException::toJson() const {

	std::basic_ostringstream<char> os;
	rapidjson::OStreamWrapper osw(os);
	rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);

	writer.StartObject();
	writer.Key("type");
	writer.String("TransformBoundsException");
	writeContainerParams(container, writer);
	writer.Key("uniqueId");
	writer.String(uniqueId.toString());
	writer.EndObject();

	return os.str();
}

std::shared_ptr<ClashDetectionException> TransformBoundsException::clone() const {
	return std::make_shared<TransformBoundsException>(*this);
}

OverlappingSetsException::OverlappingSetsException(std::set<repo::lib::RepoUUID> overlappingCompositeIds)
	:compositeIds(overlappingCompositeIds)
{
}

std::string OverlappingSetsException::toJson() const {

	std::basic_ostringstream<char> os;
	rapidjson::OStreamWrapper osw(os);
	rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);

	writer.StartObject();
	writer.Key("type");
	writer.String("OverlappingSetsException");
	writer.Key("compositeIds");
	writer.StartArray();
	for (const auto& id : compositeIds) {
		writer.String(id.toString());
	}
	writer.EndArray();
	writer.EndObject();

	return os.str();
}

std::shared_ptr<ClashDetectionException> OverlappingSetsException::clone() const {
	return std::make_shared<OverlappingSetsException>(*this);
}

DuplicateMeshIdsException::DuplicateMeshIdsException(const repo::lib::RepoUUID& uniqueId)
	: uniqueId(uniqueId)
{
}

std::string DuplicateMeshIdsException::toJson() const {

	std::basic_ostringstream<char> os;
	rapidjson::OStreamWrapper osw(os);
	rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);

	writer.StartObject();
	writer.Key("type");
	writer.String("DuplicateMeshIdsException");
	writer.Key("uniqueId");
	writer.String(uniqueId.toString());
	writer.EndObject();

	return os.str();
}

std::shared_ptr<ClashDetectionException> DuplicateMeshIdsException::clone() const {
	return std::make_shared<DuplicateMeshIdsException>(*this);
}

DegenerateTestException::DegenerateTestException(const repo::lib::RepoUUID& compositeIdA,
	const repo::lib::RepoUUID& compositeIdB,
	const char* what)
	: compositeIdA(compositeIdA),
	compositeIdB(compositeIdB),
	message(what)
{
}

std::shared_ptr<ClashDetectionException> DegenerateTestException::clone() const {
	return std::make_shared<DegenerateTestException>(*this);
}

std::string DegenerateTestException::toJson() const {

	std::basic_ostringstream<char> os;
	rapidjson::OStreamWrapper osw(os);
	rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);

	writer.StartObject();
	writer.Key("type");
	writer.String("DegenerateTestException");
	writer.Key("compositeIdA");
	writer.String(compositeIdA.toString());
	writer.Key("compositeIdB");
	writer.String(compositeIdB.toString());
	writer.Key("reason");
	writer.String(message);
	writer.EndObject();

	return os.str();
}