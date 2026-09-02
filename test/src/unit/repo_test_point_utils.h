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

/**
* Contains a set of helper facilities for creating and comparing point clouds
* for unit test implementations.
*/

#include <repo/core/model/bson/repo_node_point.h>
#include <repo/manipulator/modelconvertor/import/repo_model_import_point_cloud_abstract.h>
#include <repo/core/handler/database/repo_query.h>
#include <repo/core/handler/fileservice/repo_data_ref.h>
#include <repo/core/handler/fileservice/repo_blob_files_handler.h>

namespace repo {
	namespace test {
		namespace utils {
			namespace point {

				// A set of point data that might be store by a PointNode.
				struct point_data
				{
					point_data(
						bool name,
						bool sharedId,
						int numParents,
						int numPoints,
						int treeLevels
					);

					std::string name;
					repo::lib::RepoUUID uniqueId;
					repo::lib::RepoUUID sharedId;
					std::vector<repo::lib::RepoUUID> parents;
					std::vector<std::vector<float>> boundingBox;
					std::vector<uint8_t> treePosition;
					std::vector<repo::lib::RepoVector3D> points;
					std::vector<repo::lib::repo_color4d_t> colourAttributes;
				};

				std::unique_ptr<repo::core::model::PointNode> createRandomPoints(
					const int nPoints,
					const std::vector<repo::lib::RepoUUID>& parent
				);

				// Creates a RepoBSON for a PointNode based on the point_data
				repo::core::model::RepoBSON pointNodeTestBSONFactory(point_data data);

				/*
				* Compares mesh nodes for absolute equality of all members. This is beyond
				* the hulls being the same - this method expects that the points are
				* effectively memory-equivalent.
				*/
				void comparePointNode(point_data expected, repo::core::model::PointNode actual);

				// Creates randomised points
				std::vector<repo::lib::RepoVector3D> makePoints(int num);


				// Creates randomised colours
				std::vector<repo::lib::repo_color4d_t> makeColourAttributes(int num);

				// Create randomised tree position
				std::vector<uint8_t> makeTreePosition(int levels);

				// Creates a random point node with the given format using a repeated seed.
				// The RepoNode properties (uniqueId, sharedId, etc) are not initialised.
				repo::core::model::PointNode makeDeterministicPointNode(int treeLevels);

				// Mock importer
				class TestPCImport : public repo::manipulator::modelconvertor::AbstractPointCloudImport
				{
					public:
						TestPCImport(const repo::manipulator::modelconvertor::ModelImportConfig& settings);

						void createTestData(
							int numPoints,
							repo::lib::RepoBounds bounds
						);

						const std::vector<repo::core::model::PointData>& getTestData()
						{
							return points;
						}

						const repo::lib::RepoVector3D64 getOffset()
						{
							return offset;
						}

						virtual ~TestPCImport();

						/**
						* Import model from a given file
						* @param path to the file
						* @param database handler
						* @param error message if failed
						* @return returns a populated RepoScene upon success
						*/
						repo::core::model::RepoScene* importModel(
							std::string filePath,
							std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler,
							uint8_t& errMsg
						);

				protected:
					bool getNextPoint(repo::core::model::PointData& point);
					void resetReader();
					uint8_t loadFile(std::string filePath);

				private:
					bool testDataCreated = false;
					int readPos = 0;
					std::vector<repo::core::model::PointData> points;
					repo::lib::RepoVector3D64 offset;
				};

				struct GenericPoint
				{
					std::vector<float> data;
					int hit;

					void push(repo::lib::RepoVector3D p, repo::lib::repo_color4d_t c)
					{
						data.push_back(p.x);
						data.push_back(p.y);
						data.push_back(p.z);
						data.push_back(c.r);
						data.push_back(c.g);
						data.push_back(c.b);
						data.push_back(c.a);
					}

					void push(repo::core::model::PointData pd)
					{
						data.push_back(pd.position.x);
						data.push_back(pd.position.y);
						data.push_back(pd.position.z);
						data.push_back(pd.colour.r);
						data.push_back(pd.colour.g);
						data.push_back(pd.colour.b);
						data.push_back(pd.colour.a);
					}

					const long hash() const
					{
						size_t hash = 0;
						for (const auto value : data)
						{
							boost::hash_combine(hash, value);
						}
						return hash;
					}

					const bool equals(const GenericPoint& other) const
					{
						return data == other.data;
					}
				};

				std::vector<GenericPoint> getPointsFromDatabase(
					std::string database,
					std::string projectName,
					repo::lib::RepoUUID revId);

				std::vector<GenericPoint> getPointsFromMockImporter(
					TestPCImport* mockImporter);

				bool comparePointClouds(
					std::string database,
					std::string projectName,
					repo::lib::RepoUUID revId,
					TestPCImport* mockImporter);

				void checkChunkCorrectness(
					std::string database,
					std::string projectName,
					repo::lib::RepoUUID revId,
					repo::lib::RepoBounds expectedBounds);
			} // namespace point
		} // namespace utils
	} // namespace test
} // namespace repo