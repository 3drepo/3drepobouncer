/**
*  Copyright (C) 2015 3D Repo Ltd
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

#include <string>
#include <map>
#include <vector>
#include <stdint.h>
#include <repo_log.h>
#include "repo/lib/datastructure/repo_uuid.h"
#include "repo/repo_bouncer_global.h"
#include "repo_model_units.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			class REPO_API_EXPORT ModelImportConfig
			{
			public:
				bool importAnimations;
				int lod;
				std::string timeZone;
				ModelUnits targetUnits;
				repo::lib::RepoUUID revisionId;
				std::string databaseName;
				std::string projectName;
				int numThreads;
				std::string viewName;
				std::string viewStyle;

				ModelImportConfig();

				ModelImportConfig(
					bool importAnimations,
					ModelUnits targetUnits,
					std::string timeZone,
					int lod,
					repo::lib::RepoUUID revisionId,
					std::string database,
					std::string collection);

				ModelImportConfig(
					repo::lib::RepoUUID revisionId,
					std::string database,
					std::string collection);

				~ModelImportConfig() {}

				bool shouldImportAnimations() const { return importAnimations; }
				std::string getTimeZone() const { return timeZone; }
				ModelUnits getTargetUnits() const { return targetUnits; }
				int getLevelOfDetail() const { return lod; }
				repo::lib::RepoUUID getRevisionId() const { return revisionId; }
				std::string getDatabaseName() const { return databaseName; }
				std::string getProjectName() const { return projectName; }
				int getNumThreads() const { return numThreads; }
				std::string getViewName() const { return viewName; }

				std::string prettyPrint();
			};
		}//namespace modelconvertor
	}//namespace manipulator
}//namespace repo
