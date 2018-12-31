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

#include "repo/manipulator/modelconvertor/import/repo_model_import_abstract.h"
#include "repo/core/model/bson/repo_node_mesh.h"
#include <vector>

#ifdef ODA_SUPPORT
#include "repo/manipulator/modelconvertor/import/odaHelper/geometry_collector.h"
#endif

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			class OdaModelImport : public AbstractModelImport
			{
			public:
				static const std::string DgnExt;
				static const std::string RvtExt;
				static const std::string RfaExt;

				OdaModelImport();
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
				virtual repo::core::model::RepoScene* generateRepoScene();

				/**
				* Import model from a given file
				* This does not generate the Repo Scene Graph
				* Use getRepoScene() to generate a Repo Scene Graph.
				* @param path to the file
				* @param error message if failed
				* @return returns true upon success
				*/
				virtual bool importModel(std::string filePath, uint8_t &err);
			private:
				std::string filePath;
#ifdef ODA_SUPPORT
				odaHelper::GeometryCollector geoCollector;
#endif
			};

		}
	}
}

