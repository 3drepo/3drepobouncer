/**
*  Copyright (C) 2026 3D Repo Ltd
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

#include "repo_model_import_point_cloud_abstract.h"

#include <fstream>

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			class XYZModelImport : public AbstractPointCloudImport
			{
			public:
				/**
				* Create XYZModelImport with specific settings
				* @param settings
				*/
				XYZModelImport(const ModelImportConfig& settings);

				/**
				* Default Deconstructor
				*/
				virtual ~XYZModelImport();

				/**
				* Import model from a given file
				* @param path to the file
				* @param database handler
				* @param error message if failed
				* @return returns a populated RepoScene upon success
				*/
				repo::core::model::RepoScene* importModel(std::string filePath, std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler, uint8_t& errMsg);



			protected:

				bool getNextPoint(repo::core::model::PointData& point);
				void resetReader();
				uint8_t loadFile(std::string filePath);

			private:
				std::ifstream fileStream;

				enum ColumnType {PosX, PosY, PosZ, ColR, ColG, ColB, ColA, UNKNOWN};
				std::vector<ColumnType> schema;
			};
		} // namespace modelconvertor
	} // namespace manipulator
} // namespace repo