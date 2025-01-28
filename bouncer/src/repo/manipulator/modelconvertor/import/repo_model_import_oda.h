/**
*  Copyright (C) 2018 3D Repo Ltd
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

#pragma once

#include "repo_model_import_abstract.h"
#include "repo/manipulator/modelutility/repo_scene_builder.h"

#ifdef ODA_SUPPORT
#include "odaHelper/file_processor.h"
#endif

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			class OdaModelImport : public AbstractModelImport
			{
			public:
				static const std::string supportedExtensions;

				OdaModelImport(const ModelImportConfig &settings) : AbstractModelImport(settings) {}
				~OdaModelImport();

				/**
				* Returns true if format is supported
				*/
				static bool isSupportedExts(const std::string &testExt);

				/**
				* Generates a repo scene graph
				* an internal representation needs to have
				* been created before this call (e.g. by means of importModel())
				* @return returns a populated RepoScene upon success.
				*/
				virtual repo::core::model::RepoScene* generateRepoScene(uint8_t &errMsg);

				/**
				* Import model from a given file
				* This does not generate the Repo Scene Graph
				* Use getRepoScene() to generate a Repo Scene Graph.
				* @param path to the file
				* @param error message if failed
				* @return returns true upon success
				*/
				virtual bool importModel(std::string filePath, uint8_t& err) {
					throw std::runtime_error("Classic import is no longer supported for ODA. Use streaming model import instead.");
				}

				virtual bool importModel(std::string filePath, std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler, uint8_t& err);

				virtual bool applyReduction() const { return shouldReduce; }
				virtual bool requireReorientation() const { return true; }

			private:
				std::string filePath;
				bool shouldReduce = false;
				std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler;
				std::unique_ptr<repo::manipulator::modelutility::RepoSceneBuilder> sceneBuilder;
#ifdef ODA_SUPPORT
				std::unique_ptr<odaHelper::FileProcessor> odaProcessor;
#endif
			};
		}
	}
}
