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

using namespace repo::manipulator::modelconvertor;

ModelImportConfig::ModelImportConfig() :
	importAnimations(true),
	targetUnits(ModelUnits::UNKNOWN),
	revisionId(repo::lib::RepoUUID::defaultValue),
	lod(0),
	numThreads(0)
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
		+ " target units: " + toUnitsString(targetUnits)
		+ " importAnimations: " + (importAnimations ? "true" : "false")
		+ " lod: " + std::to_string(lod)
		+ " revisionId: " + revisionId.toString()
		+ " num threads: " + std::to_string(numThreads)
		+ " view name: " + (viewName.empty() ? "NONE" : viewName)
	);
}