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

#include "repo_model_import_abstract.h"
#include "repo/core/model/bson/repo_node_point.h"
#include "repo/manipulator/modelutility/repo_scene_builder.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {

			#define REPO_PC_CHUNKING_DEPTH 7
			#define REPO_PC_CHUNKING_MAXPOINTS 1000000


			class AbstractPointCloudImport : public AbstractModelImport
			{
			public:

				/**
				* Create AbstractModelImport with specific settings
				* @param settings
				*/
				AbstractPointCloudImport(const ModelImportConfig& settings);

				/**
				* Default Deconstructor
				* NOTE: The destructor will destroy the settings object referenced
				* in this object!
				*/
				virtual ~AbstractPointCloudImport();

				/**
				* Import model from a given file
				* @param path to the file
				* @param database handler
				* @param error message if failed
				* @return returns a populated RepoScene upon success
				*/
				repo::core::model::RepoScene* importModel(std::string filePath, std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler, uint8_t& errMsg);

			protected:

				// To be implemented by the children
				virtual bool getNextPoint(repo::core::model::PointData& point) = 0;
				virtual void resetReader() = 0;
				virtual uint8_t loadFile(std::string filePath) = 0;

			private:

				int steps = 0;
				double stepLength = 0.f;

				repo::lib::RepoBounds bounds;

				repo::lib::RepoUUID rootNodeId;

				std::vector<int> counters;

				std::vector<std::unique_ptr<repo::core::model::PointNode>> nodes;
				std::vector<int> nodeIndices;

				int getIndexFromCellCoordinates(int xIndex, int yIndex, int zIndex);

				int projectPositionIntoCell(repo::lib::RepoVector3D64 position);

				int getIndexFromTreePosition(std::vector<uint8_t>& treePosition);

				repo::lib::RepoBounds getBoundsFromTreePosition(std::vector<uint8_t>& treePosition);

				void createNode(
					repo::lib::RepoBounds bounds,
					std::vector<uint8_t>& treePosition,
					int numPoints);

				void mergeCells(std::vector<uint8_t> treePosition, int& numPoints, bool& cellsMergedBelow);

				void mergeCells();

				void updateLeaves(
					std::vector<uint8_t>& treePosition,
					int nodeIndex,
					int pointCount);

				void createRootNode(repo::manipulator::modelutility::RepoSceneBuilder* builder);
			};
		} // namespace modelconvertor
	} // namespace manipulator
} // namespace repo