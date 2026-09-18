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

#include "../../core/model/collection/repo_scene.h"
#include <repo/core/model/bson/repo_bson.h>
#include <repo/manipulator/modelconvertor/export/repo_point_cloud_export_abstract.h>
#include "../../core/handler/database/repo_query.h"
#include "../../lib/datastructure/repo_structs.h"
#include "../../core/handler/fileservice/repo_blob_files_handler.h"
#include <repo/core/model/bson/repo_node_streaming_point.h>

namespace repo {
	namespace manipulator {
		namespace modeloptimizer {
			
			#define REPO_PC_OPTIMISATION_SPLITS 5
			#define REPO_PC_SUBSAMPLE_SPLITS 6 // Equivalent to Schuetz et al's 7, since we do children individually, which would be another split


			class OctreeNode 
			{
			public:

				OctreeNode()
				{
					//  Used for unit tests
				}

				OctreeNode(int splits, std::vector<uint8_t> treePosition, repo::lib::RepoBounds bounds);

				void insert(std::vector<uint8_t> treePosition, const repo::core::model::PointData& point);

				void addNode(std::vector<uint8_t> treePosition, std::unique_ptr<OctreeNode> node);

				OctreeNode* getNode(std::vector<uint8_t> treePosition);

				bool isLeaf()
				{
					for (int i = 0; i < 8; i++)
						if (children[i] != nullptr)
							return false;
					return true;
				}

				std::vector<uint8_t> nodeTreePosition;
				repo::lib::RepoBounds nodeBounds;
				std::array<std::unique_ptr<OctreeNode>, 8> children;

				// Point data
				std::unique_ptr<std::vector<repo::core::model::PointData>> points;
			};

			class Octree
			{
			public:
				Octree(int splits, repo::lib::RepoBounds bounds, std::vector<uint8_t> rootPos);

				void project(const repo::core::model::PointData& point);

				void subsampleTree(repo::manipulator::modelconvertor::AbstractPointCloudExport* exporter);

				void addNode(std::unique_ptr<OctreeNode> node, std::vector<uint8_t> nodePosition);

				OctreeNode* getRoot() {
					return root.get();
				}

				OctreeNode* getNode(std::vector<uint8_t> treePosition);

				std::unique_ptr<OctreeNode> moveRoot() {
					return std::move(this->root);
				}

				std::vector<uint8_t> getRootPosition() {
					return rootPosition;
				}

			private:
				repo::lib::RepoBounds treeBounds;
				std::unique_ptr<OctreeNode> root;
				int treeSplits;
				float cellLength;
				int noCells;
				std::vector<uint8_t> rootPosition;

				void subsample(
					repo::manipulator::modelconvertor::AbstractPointCloudExport* exporter,
					OctreeNode* node);

			};

			class PointCloudOptimizer {

			public:
				PointCloudOptimizer(
					repo::core::handler::AbstractDatabaseHandler* handler,
					repo::manipulator::modelconvertor::AbstractPointCloudExport* exporter
				);

				void processScene(
					std::string database,
					std::string collection,
					repo::lib::RepoUUID revId,
					repo::lib::RepoBounds cloudBounds
				);

			private:
				repo::core::handler::AbstractDatabaseHandler* handler;
				repo::manipulator::modelconvertor::AbstractPointCloudExport* exporter;

				std::unique_ptr<OctreeNode> processChunk(
					std::string database,
					std::string collection,
					repo::core::model::StreamingPointNode* chunk);
			};


		} // modeloptimizer
	} // manipulator
} // repo