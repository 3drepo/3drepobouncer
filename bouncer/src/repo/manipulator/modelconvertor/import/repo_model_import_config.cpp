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
/**
* General Model Convertor Config
* used to track all settings required for Model Convertor
*/

#pragma once

#include "repo_model_import_config.h"
#include "repo/lib/repo_exception.h"
#include <repo/lib/rapidjson/document.h>
#include <fstream>
#include <sstream>

using namespace repo::manipulator::modelconvertor;
using namespace repo::lib;

ModelImportConfig::ModelImportConfig() :
	importAnimations(true),
	targetUnits(ModelUnits::UNKNOWN),
	revisionId(repo::lib::RepoUUID::defaultValue),
	lod(0),
	numThreads(0),
	splitByFloor(true)
{}

ModelImportConfig::ModelImportConfig(
	bool importAnimations,
	ModelUnits targetUnits,
	std::string timeZone,
	int lod,
	repo::lib::RepoUUID revisionId,
	std::string database,
	std::string collection):
	ModelImportConfig()
{
	this->importAnimations = importAnimations;
	this->targetUnits = targetUnits;
	this->timeZone = timeZone;
	this->lod = lod;
	this->revisionId = revisionId;
	this->databaseName = database;
	this->projectName = collection;
}

ModelImportConfig::ModelImportConfig(
	repo::lib::RepoUUID revisionId,
	std::string database,
	std::string collection) :
	ModelImportConfig()
{
	this->revisionId = revisionId;
	this->databaseName = database;
	this->projectName = collection;
}

std::string ModelImportConfig::prettyPrint()
{
	return (" database: " + databaseName
		+ " project: " + projectName
		+ " target units: " + units::toUnitsString(targetUnits)
		+ " importAnimations: " + (importAnimations ? "true" : "false")
		+ " lod: " + std::to_string(lod)
		+ " revisionId: " + revisionId.toString()
		+ " num threads: " + std::to_string(numThreads)
		+ " view name: " + (viewName.empty() ? "NONE" : viewName)
		+ " split by floor: " + (splitByFloor ? "true" : "false")
	);
}

ModelImportConfig ModelImportConfig::ParseJsonFile(std::string filename)
{
	std::ifstream fileStream(filename, std::ios::in | std::ios::binary);
	if (!fileStream.good())
	{
		throw repo::lib::RepoInvalidConfigException("Failed to open config file: " + filename);
	}

	std::ostringstream contentStream;
	contentStream << fileStream.rdbuf();
	auto content = contentStream.str();

	rapidjson::Document jsonTree;
	if (jsonTree.Parse(content.c_str()).HasParseError())
	{
		throw repo::lib::RepoInvalidConfigException("Failed to parse config file: " + filename);
	}

	ModelImportConfig config;

	// Client should be backwards compatible with v4 (database & project) and v5
	// (teamspace & container) terminology. Teamspace/database are the terms that
	// do not conflict, so we use them to distinguish which version is being used.

	if (jsonTree.HasMember("teamspace") && jsonTree["teamspace"].IsString()) {
		config.databaseName = jsonTree["teamspace"].GetString();
	}
	if (jsonTree.HasMember("container") && jsonTree["container"].IsString()) {
		config.projectName = jsonTree["container"].GetString();
	}

	if (config.databaseName.empty()) {
		if (jsonTree.HasMember("database") && jsonTree["database"].IsString()) {
			config.databaseName = jsonTree["database"].GetString();
		}
		if (jsonTree.HasMember("project") && jsonTree["project"].IsString()) {
			config.projectName = jsonTree["project"].GetString();
		}
	}

	if (jsonTree.HasMember("timezone") && jsonTree["timezone"].IsString()) {
		config.timeZone = jsonTree["timezone"].GetString();
	}

	if (jsonTree.HasMember("units") && jsonTree["units"].IsString()) {
		config.targetUnits = units::fromString(jsonTree["units"].GetString());
	}

	if (jsonTree.HasMember("lod") && jsonTree["lod"].IsInt()) {
		config.lod = jsonTree["lod"].GetInt();
	}

	if (jsonTree.HasMember("importAnimations") && jsonTree["importAnimations"].IsBool()) {
		config.importAnimations = jsonTree["importAnimations"].GetBool();
	}

	if (jsonTree.HasMember("view") && jsonTree["view"].IsString()) {
		config.viewName = jsonTree["view"].GetString();
	}

	if (jsonTree.HasMember("style") && jsonTree["style"].IsString()) {
		config.viewStyle = jsonTree["style"].GetString();
	}

	if (jsonTree.HasMember("splitByFloor") && jsonTree["splitByFloor"].IsBool()) {
		config.splitByFloor = jsonTree["splitByFloor"].GetBool();
	}

	if (jsonTree.HasMember("revId") && jsonTree["revId"].IsString()) {
		config.revisionId = repo::lib::RepoUUID(jsonTree["revId"].GetString());
	}

	if (jsonTree.HasMember("numThreads") && jsonTree["numThreads"].IsInt()) {
		config.numThreads = jsonTree["numThreads"].GetInt();
	}

	return config;
}

void ModelImportConfig::validate() 
{
	if (databaseName.empty() || projectName.empty() || !revisionId)
	{
		throw repo::lib::RepoInvalidConfigException("Invalid configuration: database, project and revisionId must be specified");
	}
}