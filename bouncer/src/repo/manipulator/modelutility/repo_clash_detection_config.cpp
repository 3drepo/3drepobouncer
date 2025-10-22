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

#include "repo_clash_detection_config.h"

#include <repo/manipulator/modelutility/repo_json_parser.h>

#include <fstream>

using namespace repo::manipulator::modelutility;
using namespace repo::manipulator::modelutility::json;

using CompositeObjectMap = std::unordered_map<repo::lib::RepoUUID, CompositeObject, repo::lib::RepoUUIDHasher>;

class ICompositeObjectSet
{
public:
	repo::lib::Container* container;
	virtual CompositeObject& getCompositeObject(const repo::lib::RepoUUID&) = 0;
};

class IContainerSet
{
public:
	virtual repo::lib::Container* getContainer(
		const std::string& teamspace,
		const std::string& container,
		const repo::lib::RepoUUID& revision) = 0;
};

struct MeshIdsParser : public Parser
{
	std::vector<MeshReference>& meshes;
	ICompositeObjectSet* set;

	MeshIdsParser(std::vector<MeshReference>& meshes, ICompositeObjectSet* set)
		:meshes(meshes),
		set(set)
	{
	}

	virtual void String(const std::string_view& string) override
	{
		meshes.push_back({ set->container, repo::lib::RepoUUID(std::string(string)) });
	}
};

struct CompositeObjectParser : public ObjectParser
{
	repo::lib::RepoUUID id;
	std::vector<MeshReference> meshIds;

	CompositeObjectParser(ICompositeObjectSet* set)
		:set(set)
	{
		parsers["id"] = new RepoUUIDParser(id);
		parsers["meshIds"] = new ArrayParser(new MeshIdsParser(meshIds, set));
	}

	ICompositeObjectSet* set;

	virtual void EndObject() override
	{
		auto& obj = set->getCompositeObject(id);
		if (obj.meshes.size()) {
			obj.meshes.insert(obj.meshes.end(), meshIds.begin(), meshIds.end());
		}
		else {
			obj.meshes = std::move(meshIds);
		}
		meshIds = {};
	}
};

struct CompositeObjectSetParser : public ObjectParser, public ICompositeObjectSet
{
	std::string teamspace;
	std::string containerName;
	repo::lib::RepoUUID revision;

	CompositeObjectMap& set;
	IContainerSet* containers;

	CompositeObjectSetParser(IContainerSet* containers, CompositeObjectMap& set)
		:set(set),
		containers(containers)
	{
		parsers["teamspace"] = new StringParser(teamspace);
		parsers["container"] = new StringParser(containerName);
		parsers["revision"] = new RepoUUIDParser(revision);
		parsers["objects"] = new ArrayParser(new CompositeObjectParser(this));
	}

	CompositeObject& getCompositeObject(const repo::lib::RepoUUID& id) override
	{
		auto& obj = set[id];
		obj.id = id;
		return obj;
	}

	virtual Parser* Key(const std::string_view& key) override
	{
		// The objects field should always follow the container parameters
		if (key == "objects") {
			container = containers->getContainer(teamspace, containerName, revision);
		}
		return ObjectParser::Key(key);
	}
};

struct ClashTypeParser : Parser
{
	ClashDetectionType& type;

	ClashTypeParser(ClashDetectionType& type)
		:type(type)
	{
	}

	virtual void String(const std::string_view& string) override
	{
		if (string == "clearance") {
			type = ClashDetectionType::Clearance;
		}
		else if (string == "hard") {
			type = ClashDetectionType::Hard;
		}
		else {
			throw std::runtime_error("Unknown clash detection type: " + std::string(string));
		}
	}
};

struct ClashConfigParser : public ObjectParser, public IContainerSet
{
	ClashDetectionConfig& config;
	std::unordered_map<std::string, repo::lib::Container*> containers;
	CompositeObjectMap mapA;
	CompositeObjectMap mapB;

	ClashConfigParser(ClashDetectionConfig& config)
		:config(config)
	{
		parsers["type"] = new ClashTypeParser(config.type);
		parsers["tolerance"] = new NumberParser<double>(config.tolerance);
		parsers["resultsFile"] = new StringParser(config.resultsFile);
		parsers["setA"] = new ArrayParser(new CompositeObjectSetParser(this, mapA));
		parsers["setB"] = new ArrayParser(new CompositeObjectSetParser(this, mapB));
	}

	virtual repo::lib::Container* getContainer(
		const std::string& teamspace,
		const std::string& container,
		const repo::lib::RepoUUID& revision) override
	{
		auto key = teamspace + ":" + container + ":" + revision.toString();
		auto it = containers.find(key);
		if (it == containers.end())
		{
			auto c = std::make_unique<repo::lib::Container>();
			c->teamspace = teamspace;
			c->container = container;
			c->revision = revision;
			auto ptr = c.get();
			config.containers.push_back(std::move(c));
			containers[key] = ptr;
			return ptr;
		}
		else {
			return it->second;
		}
	}

	virtual void EndObject() override
	{
		config.setA.clear();
		for (auto& [id, obj] : mapA) {
			config.setA.push_back(std::move(obj));
		}
		config.setB.clear();
		for (auto& [id, obj] : mapB) {
			config.setB.push_back(std::move(obj));
		}
	}

	static void ParseJson(const char* buf, ClashDetectionConfig& config)
	{
		ClashConfigParser parser(config);
		JsonParser::ParseJson(buf, &parser);
	}
};

void ClashDetectionConfig::ParseJsonFile(const std::string& jsonFilePath, ClashDetectionConfig& config)
{
	std::ifstream fileStream(jsonFilePath, std::ios::in | std::ios::binary);
	std::ostringstream contentStream;
	contentStream << fileStream.rdbuf();
	auto content = contentStream.str();
	ClashConfigParser::ParseJson(content.data(), config);
}