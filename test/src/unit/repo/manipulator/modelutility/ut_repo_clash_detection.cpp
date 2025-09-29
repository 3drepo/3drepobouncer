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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>
#include <numeric>
#include <random>
#include <exception>
#include <repo_log.h>

// The ODA NWD importer also uses rapidjson, so make sure to import our
// copy into a different namespace to avoid interference between two
// possibly different versions.

#define RAPIDJSON_NAMESPACE repo::rapidjson
#define RAPIDJSON_NAMESPACE_BEGIN namespace repo { namespace rapidjson {
#define RAPIDJSON_NAMESPACE_END } }

#include <repo/manipulator/modelutility/rapidjson/rapidjson.h>
#include <repo/manipulator/modelutility/rapidjson/document.h>

#include <repo/manipulator/modelutility/repo_scene_builder.h>
#include <repo/manipulator/modelutility/repo_scene_manager.h>

#include <repo/core/model/bson/repo_bson.h>

#include <repo/manipulator/modelutility/repo_clash_detection_engine.h>
#include <repo/manipulator/modelutility/repo_clash_detection_config.h>
#include <repo/manipulator/modelutility/clashdetection/clash_pipelines.h>
#include <repo/manipulator/modelutility/clashdetection/clash_clearance.h>
#include <repo/manipulator/modelutility/clashdetection/clash_hard.h>
#include <repo/manipulator/modelutility/clashdetection/sparse_scene_graph.h>
#include <repo/manipulator/modelutility/clashdetection/geometry_tests.h>
#include <repo/manipulator/modelutility/clashdetection/clash_scheduler.h>

#include "../../../repo_test_utils.h"
#include "../../../repo_test_clash_utils.h"
#include "../../../repo_test_mesh_utils.h"
#include "../../../repo_test_database_info.h"
#include "../../../repo_test_fileservice_info.h"
#include "../../../repo_test_scene_utils.h"
#include "../../../repo_test_matchers.h"
#include "../../../repo_test_mock_database.h"
#include "../../../repo_test_mock_clash_scene.h"
#include "../../../repo_test_random_generator.h"

using namespace testing;
using namespace repo;
using namespace repo::core::model;
using namespace repo::core::handler::database;
using namespace repo::manipulator::modelconvertor;
using namespace repo::manipulator::modelutility;

#define TESTDB "ClashDetection"

#pragma optimize ("", off)

repo::core::model::MeshNode createPointMesh(std::initializer_list<repo::lib::RepoVector3D> points)
{
	std::vector<repo::lib::repo_face_t> faces;
	for(size_t i = 0; i < points.size(); ++i)
	{
		faces.push_back({i});
	}
	return RepoBSONFactory::makeMeshNode(
		std::vector<repo::lib::RepoVector3D>(points),
		faces,
		{},
		{}
	);
}

struct BasisVectors
{
	repo::lib::RepoVector3D64 x = {1, 0, 0};
	repo::lib::RepoVector3D64 y = {0, 1, 0};
	repo::lib::RepoVector3D64 z = {0, 0, 1};
	BasisVectors() = default;
	BasisVectors(repo::lib::RepoVector3D64 x, repo::lib::RepoVector3D64 y, repo::lib::RepoVector3D64 z)
		:x(x), y(y), z(z) {
	}
};

struct ProceduralBox
{
	std::vector<repo::lib::RepoVector3D64> vertices;
	
	ProceduralBox(repo::lib::RepoVector3D64 size)
		:vertices({
				{-size.x, -size.y, -size.z}, // Bottom-left-front
				{ size.x, -size.y, -size.z}, // Bottom-right-front
				{ size.x,  size.y, -size.z}, // Top-right-front
				{-size.x,  size.y, -size.z}, // Top-left-front
				{-size.x, -size.y,  size.z}, // Bottom-left-back
				{ size.x, -size.y,  size.z}, // Bottom-right-back
				{ size.x,  size.y,  size.z}, // Top-right-back
				{-size.x,  size.y,  size.z}  // Top-left-back
			})
	{
	}

	repo::core::model::MeshNode getMeshNode() const {
		std::vector<repo::lib::repo_face_t> faces = {
			{0, 1, 2}, {0, 2, 3}, // Front face
			{4, 5, 6}, {4, 6, 7}, // Back face
			{0, 1, 5}, {0, 5, 4}, // Bottom face
			{2, 3, 7}, {2, 7, 6}, // Top face
			{0, 3, 7}, {0, 7, 4}, // Left face
			{1, 2, 6}, {1, 6, 5}  // Right face
		};

		std::vector<repo::lib::RepoVector3D> vv;
		for (auto& v : vertices) {
			vv.push_back(repo::lib::RepoVector3D(v.x, v.y, v.z));
		}

		auto n = RepoBSONFactory::makeMeshNode(
			vv,
			faces,
			{},
			{}
		);

		n.updateBoundingBox();

		return n;
	}

	operator repo::core::model::MeshNode() const
	{
		return getMeshNode();
	}
};

repo::core::model::MeshNode createBoxMesh(
	repo::lib::RepoVector3D64 size,
	repo::lib::RepoVector3D64 origin,
	repo::lib::RepoUUID parentId)
{
	auto n = ProceduralBox(size).getMeshNode();
	n.addParents({ parentId });
	n.setMaterial(repo::lib::repo_material_t::DefaultMaterial());
	n.changeName("Box");
	return n;
}

TEST(Clash, SparseSceneGraph)
{
	// Tests a composite scene graph from across two Containers, including multiple
	// inherited transformations.

	auto handler = getHandler();

	lib::Container container1;
	container1.teamspace = TESTDB;
	container1.container = "62e73fcf-f97c-4562-9160-b4958adffb48";
	container1.revision = repo::lib::RepoUUID("67a09fc8-c1ae-4052-b052-444010821173");

	// Cube was imported units of m from an OBJ

	sparse::SceneGraph scene;
	scene.populate(
		handler,
		&container1,
		{
			repo::lib::RepoUUID("52df8d1c-18f8-4e7f-9fb3-34490f429817"),
			repo::lib::RepoUUID("8f553202-4f66-48eb-ac25-5a24e337f51e"),
			repo::lib::RepoUUID("1b43b57e-31ce-42fb-a9b8-a673dbc499f7"),
			repo::lib::RepoUUID("dbea06fe-0e4b-4a88-b660-80e82daf837b"),
		}
	);

	lib::Container container2;
	container2.teamspace = TESTDB;
	container2.container = "74b9ae3f-9198-40ff-96f8-5844957d8598";
	container2.revision = repo::lib::RepoUUID("2a37d90e-c973-4eac-b5c8-5c7095e659c0");

	scene.populate(
		handler,
		&container2,
		{
			repo::lib::RepoUUID("d1e9a204-f709-48a7-b464-5cc44eaf87d8"), // Cube.001
			repo::lib::RepoUUID("2e891c27-452b-4e89-8700-250646835fa9"), // Cube.002
		}
	);

	std::vector<sparse::Node> nodes;
	scene.getNodes(nodes);

	EXPECT_THAT(nodes.size(), Eq(6)); // There are 8 meshes in the above combined scenes; we should only retrieve the ones that were requested.

	std::unordered_map<repo::lib::RepoUUID, sparse::Node, repo::lib::RepoUUIDHasher> map;

	for (auto& node : nodes) {
		handler->loadBinaryBuffers(node.container->teamspace, node.container->container + "." + REPO_COLLECTION_SCENE, node.mesh);
		map[node.uniqueId] = node;
	}

	// The reference data for the following tests was collected by loading the model
	// and using the Measure Point tool, extracting the Project Space coordinates
	// from the DropBookmarkPin message (which is Project Coordinates).

	{
		auto node = map[repo::lib::RepoUUID("52df8d1c-18f8-4e7f-9fb3-34490f429817")];
		auto mesh = MeshNode(node.mesh);
		mesh.applyTransformation(node.matrix);

		EXPECT_THAT(mesh.getName(), StrEq("Cube"));

		auto expected = createPointMesh({
				{-1, -1, -3},
				{-1, 1, -3},
				{1, 1, -3},
				{1, -1, -3},
				{1, -1, -5},
				{1, 1, -5},
				{-1 ,1, -5},
				{-1 ,-1, -5},
			}
		);

		EXPECT_THAT(repo::test::utils::mesh::hausdorffDistance(mesh, expected), Lt(0.00001));
	}

	{
		auto node = map[repo::lib::RepoUUID("8f553202-4f66-48eb-ac25-5a24e337f51e")];
		auto mesh = MeshNode(node.mesh);
		mesh.applyTransformation(node.matrix);

		EXPECT_THAT(mesh.getName(), StrEq("Cube.001"));

		auto expected = createPointMesh({
				{6.652196884155273, 1.000000238418579, 2.623666763305664},
				{7.17236852645874, 1.000000238418579, 4.554838180541992},
				{5.24119758605957, 1.000000238418579, 5.075010299682617},
				{4.721025466918945, 1.000000238418579, 3.143838882446289},
				{4.721025705337524, -0.9999997615814209, 3.143838882446289},
				{5.24119758605957, -0.9999997615814209, 5.075010299682617},
				{7.17236852645874, -0.9999997615814209, 4.554838180541992},
				{6.652196884155273, -0.9999997615814209, 2.623666763305664}
			}
		);

		EXPECT_THAT(repo::test::utils::mesh::hausdorffDistance(mesh, expected), Lt(0.00001));
	}

	{
		auto node = map[repo::lib::RepoUUID("1b43b57e-31ce-42fb-a9b8-a673dbc499f7")];
		auto mesh = MeshNode(node.mesh);
		mesh.applyTransformation(node.matrix);

		EXPECT_THAT(mesh.getName(), StrEq("Cube.002"));

		auto expected = createPointMesh({
			{5.9490275382995605, -2.800995111465454, -4.697843074798584},
			{6.049928665161133, -1.3614065647125244, -6.082547187805176},
			{4.056952714920044, -1.3816282749176025, -6.2487945556640625},
			{3.9560515880584717, -2.8212168216705322, -4.864090919494629},
			{6.183592796325684, -2.7496349811553955, -7.516059875488281},
			{4.190617084503174, -2.7698566913604736, -7.682306289672852},
			{6.0826921463012695, -4.189223051071167, -6.131355285644531},
			{4.089716196060181, -4.209445238113403, -6.297603607177734}
			}
		);

		EXPECT_THAT(repo::test::utils::mesh::hausdorffDistance(mesh, expected), Lt(0.00001));
	}

	{
		auto node = map[repo::lib::RepoUUID("dbea06fe-0e4b-4a88-b660-80e82daf837b")];
		auto mesh = MeshNode(node.mesh);
		mesh.applyTransformation(node.matrix);

		EXPECT_THAT(mesh.getName(), StrEq("Cube.003"));

		auto expected = createPointMesh({
			{9.444141387939453, 1.5074551105499268, -1.0000019073486328},
			{10.990434646606445, 2.775909662246704, -1.000002384185791},
			{9.444141387939453, 1.5074551105499268, 0.9999980926513672},
			{10.990434646606445, 2.775909662246704, 0.9999980926513672},
			{12.258890151977539, 1.2296171188354492, 0.9999980926513672},
			{12.258889198303223, 1.2296171188354492, -1.0000019073486328},
			{10.712596893310547, -0.038837432861328125, 0.9999980926513672},
			{10.71259593963623, -0.03883790969848633, -1.0000019073486328},
			}
		);

		EXPECT_THAT(repo::test::utils::mesh::hausdorffDistance(mesh, expected), Lt(0.00001));
	}

	{
		auto node = map[repo::lib::RepoUUID("d1e9a204-f709-48a7-b464-5cc44eaf87d8")];
		auto mesh = MeshNode(node.mesh);
		mesh.applyTransformation(node.matrix);

		EXPECT_THAT(mesh.getName(), StrEq("Cube.001"));

		auto expected = createPointMesh({
			{-4.288780450820923, 7.694965183734894, -3.534550189971924},
			{-3.9267463684082036, 8.825610935688019, -5.144075512886047},
			{-2.530199527740479, 7.5253923535346985, -5.743314623832703},
			{-2.8922336101531987, 6.394746124744415, -4.133789420127869},
			{-3.9153311252594, 6.5099762082099915, -6.768176198005676},
			{-4.27736508846283, 5.3793304562568665, -5.158650994300842},
			{-5.311877965927124, 7.810195744037628, -6.168937087059021},
			{-5.673912048339844, 6.679549992084503, -4.559411883354187},
			}
		);

		EXPECT_THAT(repo::test::utils::mesh::hausdorffDistance(mesh, expected), Lt(0.00001));
	}

	{
		auto node = map[repo::lib::RepoUUID("2e891c27-452b-4e89-8700-250646835fa9")];
		auto mesh = MeshNode(node.mesh);
		mesh.applyTransformation(node.matrix);

		EXPECT_THAT(mesh.getName(), StrEq("Cube.002"));

		auto expected = createPointMesh({
			{-4.319791078567505, 5.375168144702911, -2.266958713531494},
			{-5.6826207637786865, 3.9113729596138, -2.266958713531494},
			{-5.6826207637786865, 3.9113739132881165, -0.26695871353149414},
			{-2.855996370315552, 4.012338936328888, -2.266958713531494},
			{-2.855996131896973, 4.012339413166046, -0.26695871353149414},
			{-4.218825817108154, 2.5485443472862244, -0.26695823669433594},
			{-4.218825817108154, 2.5485441088676453, -2.266958713531494},
			{-4.319791078567505, 5.37516862154007, -0.26695919036865234},
			}
		);

		EXPECT_THAT(repo::test::utils::mesh::hausdorffDistance(mesh, expected), Lt(0.00001));
	}
}

TEST(Clash, Config)
{
	ClashDetectionConfig config;
	ClashDetectionConfig::ParseJsonFile(getDataPath("/clash/config1.json"), config);

	EXPECT_THAT(config.type, Eq(ClashDetectionType::Clearance));
	EXPECT_THAT(config.tolerance, Eq(0.0001));
	
	EXPECT_THAT(config.setA.size(), Eq(2));

	EXPECT_THAT(config.setA[0].id, Eq(repo::lib::RepoUUID("898f194c-3852-43ac-9be5-85d315838768")));
	EXPECT_THAT(config.setA[0].meshes[0].uniqueId, Eq(repo::lib::RepoUUID("266f6406-105f-4c43-b958-5a758eb15982")));
	EXPECT_THAT(config.setA[0].meshes[0].container->teamspace, StrEq("clash"));
	EXPECT_THAT(config.setA[0].meshes[0].container->container, StrEq("7cfef3ac-c133-417d-8bbe-c299a4725a95"));
	EXPECT_THAT(config.setA[0].meshes[0].container->revision == repo::lib::RepoUUID("f70777ea-1f05-4f2a-b71b-1578d0710deb"), IsTrue());

	EXPECT_THAT(config.setA[1].id, Eq(repo::lib::RepoUUID("a8f4761c-1b8a-4191-8e55-6096f602ca6e")));
	EXPECT_THAT(config.setA[1].meshes[0].uniqueId, Eq(repo::lib::RepoUUID("b5ac2ae0-13de-4e68-8188-01bf04233e39")));
	EXPECT_THAT(config.setA[1].meshes[0].container->teamspace, StrEq("clash"));
	EXPECT_THAT(config.setA[1].meshes[0].container->container, StrEq("7cfef3ac-c133-417d-8bbe-c299a4725a95"));
	EXPECT_THAT(config.setA[1].meshes[0].container->revision == repo::lib::RepoUUID("f70777ea-1f05-4f2a-b71b-1578d0710deb"), IsTrue());

	EXPECT_THAT(config.setB[0].id, Eq(repo::lib::RepoUUID("06208325-6ea7-45a8-b5a1-ab8be8c4da79")));
	EXPECT_THAT(config.setB[0].meshes[0].uniqueId, Eq(repo::lib::RepoUUID("8e8a7c81-a7df-4587-9976-190ff5680a97")));
	EXPECT_THAT(config.setB[0].meshes[0].container->teamspace, StrEq("clash"));
	EXPECT_THAT(config.setB[0].meshes[0].container->container, StrEq("ea72bf4b-ab53-4f59-bef3-694b66484192"));
	EXPECT_THAT(config.setB[0].meshes[0].container->revision == repo::lib::RepoUUID("95988a2e-f71a-462d-9d00-de724fc7cd05"), IsTrue());

	EXPECT_THAT(config.setB[1].id, Eq(repo::lib::RepoUUID("22ea928f-920d-493e-b6b8-7d510842a43f")));
	EXPECT_THAT(config.setB[1].meshes[0].uniqueId, Eq(repo::lib::RepoUUID("655b8b50-70d3-4b8a-b4bf-02eed23202e3")));
	EXPECT_THAT(config.setB[1].meshes[0].container->teamspace, StrEq("clash"));
	EXPECT_THAT(config.setB[1].meshes[0].container->container, StrEq("ea72bf4b-ab53-4f59-bef3-694b66484192"));
	EXPECT_THAT(config.setB[1].meshes[0].container->revision == repo::lib::RepoUUID("95988a2e-f71a-462d-9d00-de724fc7cd05"), IsTrue());

	EXPECT_THAT(config.containers.size(), Eq(2));

	// todo: check support for multiple containers


}

TEST(Clash, Scheduler)
{
	// Tests if the ClashScheduler returns a sensible narrowphase set.
	// The scheduling problem is NP-Hard, so this test doesn't check if the
	// order is optimal, just that all the original broadphase tests still
	// exist, with no duplicates.

	std::vector<std::pair<int, int>> broadphaseResults;

	RepoRandomGenerator random;

	for (auto i = 0; i < 1000; i++) {
		if (random.boolean()) {
			int refs = random.range(1, 10);
			for (auto r = 0; r < refs; r++) {
				broadphaseResults.push_back({ i, random.range(0, 1000) });
			}
		}
		if (random.boolean()) {
			int refs = random.range(1, 10);
			for (auto r = 0; r < refs; r++) {
				broadphaseResults.push_back({ random.range(0, 1000), i });
			}
		}
	}

	auto copy = broadphaseResults;

	clash::ClashScheduler::schedule(broadphaseResults);

	EXPECT_THAT(broadphaseResults, UnorderedElementsAreArray(copy));
}

/*
* This next set of tests checks the accuracy of the engine. Accuracy is tested
* numerically. Primitives are generated in different known configurations as a
* ground truth, to which the measured distances are compared.
* 
* These tests use mixed precision libraries to build the ground truth, after
* which they are downcast to the expected precision the engine will have to work
* with: single precision for vertices, and double for transformations. These are
* what are used to store scenes in the database.
* 
* Accuracy is tested at both the unit and end-to-end level. The current guaranteed
* accuracy is 1 in 8e6 - that is, an error of at most 0.5 is allowed on 'either
* side' of the expected distance.
*/

ClashGenerator clashGenerator;

TEST(Clash, LineLineDistanceUnit)
{
	// Tests the accuracy of the line-line distance test across different scales.
	// The test must be accurate for any pair of lines to within 1 in 8e6.

	std::vector<int> meshPowers = { 2, 3, 4, 5, 6 };
	std::vector<double> distances = { 0, 0.1, 1, 2, 10 };

	for (auto d : distances) {
		for (auto pM : meshPowers) {

			double minError = DBL_MAX;
			double maxError = -DBL_MAX;

			double scale = 8 * std::pow(10, pM);
			for (int i = 0; i < 1000000; ++i) {

				auto p = clashGenerator.createLines(scale, d, 0);

				bool downcast = true;
				if (downcast) {
					p.first.start = repo::lib::RepoVector3D(p.first.start);
					p.first.end = repo::lib::RepoVector3D(p.first.end);
					p.second.start = repo::lib::RepoVector3D(p.second.start);
					p.second.end = repo::lib::RepoVector3D(p.second.end);
				}

				auto line = geometry::closestPointLineLine(p.first, p.second);
				auto m = line.magnitude();
				auto e = abs(m - d);

				minError = std::min(minError, e);
				maxError = std::max(maxError, e);

				// 0.5 on either side of d, for a total permissible error of 1
				EXPECT_THAT(e, Lt(0.5));
			}

			std::cout << "Scale: " << scale << " Expected Distance: " << d << ", Max Error: " << maxError << std::endl;
		}
	}
}

TEST(Clash, LineLineDistanceE2E)
{
	// Test the accuracy of the end-to-end pipeline in Clearance mode, for line-line
	// distances.

	auto db = std::make_shared<MockDatabase>();

	std::vector<double> meshPowers = { 0.01, 1, 4, 6 };
	std::vector<double> transformPowers = { 0, 2, 5, 8, 11 };
	std::vector<double> distances = { 0, 1, 2 };

	for (auto pT : transformPowers) {
		for (auto pM : meshPowers) {
			for (auto d : distances) {

				double meshScale = 8 * std::pow(10, pM);
				double transformScale = std::pow(10, pT);

				for (int i = 0; i < 1000; ++i) {

					ClashDetectionConfigHelper config;
					config.type = ClashDetectionType::Clearance;
					config.containers[0]->revision = repo::lib::RepoUUID::createUUID();

					MockClashScene scene(config.containers[0]->revision);

					auto p = clashGenerator.createLinesTransformed(meshScale, d, transformScale);
					auto ids = scene.add(p, config);

					db->setDocuments(scene.bsons);
					
					{
						config.tolerance = d + 0.5;
						clash::Clearance pipeline(db, config);
						auto results = pipeline.runPipeline();
						EXPECT_THAT(results.clashes.size(), Eq(1));
					}

					{
						config.tolerance = d - 0.5;
						clash::Clearance pipeline(db, config);
						auto results = pipeline.runPipeline();
						EXPECT_THAT(results.clashes.size(), Eq(0));
					}
				}
			}
		}
	}
}

TEST(Clash, SupportedRanges)
{
	// If a mesh is greater than 8e6 in any dimension after being subject to scale
	// or two transformations have a difference in offset greater than 10e10,
	// then we cannot guarantee accuracy of the algorithm as the precision in the
	// original vertices will have been lost.
	// In this case we should issue a warning and refuse to return any results.





}

TEST(Clash, SelfClearance)
{
	// Test a single set (duplicated between A and B) for self-clearance.
	// Identical Composite ids should not be tested against eachother.


}

TEST(Clash, Rvt)
{
	// Clash tests should be robust to quantisation from large offsets and
	// rotations.

	auto handler = getHandler();
	ClashDetectionConfig config;
	ClashDetectionDatabaseHelper helper(handler);

	auto container = helper.getContainerByName("clearance_1_rvt");
	config.containers.push_back(std::move(container));

	helper.setCompositeObjectSetsByName(config,
		{
			"Casework 1_323641",
		},
		{
			"Casework 1_323623",
			"Casework 1_323729",
			"Casework 1_323902",
		}
		);

	config.tolerance = 0.001;

	auto pipeline = new clash::Clearance(handler, config);
	auto results = pipeline->runPipeline();

	EXPECT_THAT(results.clashes.size(), Eq(3));
}

TEST(Clash, Clearance1)
{
	/*
	* Performs Clearance tests against a number of manually configured imports.
	* This test is used to verify end-to-end functionality against a real
	* collection.
	*/

	auto handler = getHandler();
	ClashDetectionConfig config;

	ClashDetectionDatabaseHelper helper(handler);

	auto container = helper.getContainerByName("clearance_1");
	config.containers.push_back(std::move(container));


	// Check objects that are touching via coincident vertices

	// The tolerance for "zero" should be epsilon, as quantisation error will be
	// expected in any measured distances.

	config.tolerance = 0.000001;

	{
		helper.setCompositeObjectSetsByName(config, { "TouchingPointsA" }, { "TouchingPointsB" });

		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(1));
		EXPECT_THAT(results.clashes[0].idA, Eq(config.setA[0].id));
		EXPECT_THAT(results.clashes[0].idB, Eq(config.setB[0].id));
	}

	// Check objects that are touching via conincident edges

	{
		helper.setCompositeObjectSetsByName(config, { "TouchingEdgeA" }, { "TouchingEdgeB" });

		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(1));
		EXPECT_THAT(results.clashes[0].idA, Eq(config.setA[0].id));
		EXPECT_THAT(results.clashes[0].idB, Eq(config.setB[0].id));
	}


	// Check objects that are touching via coincident faces

	{
		helper.setCompositeObjectSetsByName(config, { "TouchingFaceA" }, { "TouchingFaceB" });

		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(1));
		EXPECT_THAT(results.clashes[0].idA, Eq(config.setA[0].id));
		EXPECT_THAT(results.clashes[0].idB, Eq(config.setB[0].id));
	}


	// Check objects for which the closest distance is a point to another point

	{
		helper.setCompositeObjectSetsByName(config, { "ClosestPointPoint1A" }, { "ClosestPointPoint1B" });

		config.tolerance = 1.0;

		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(0));
	}
	{
		config.tolerance = 1.2;

		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(1));
		EXPECT_THAT(results.clashes[0].idA, Eq(config.setA[0].id));
		EXPECT_THAT(results.clashes[0].idB, Eq(config.setB[0].id));

		// The positions reported for a clearance clash are the two points closest
		// to each other out of all pairs that violate the tolerance.

		EXPECT_THAT(
			results.clashes[0].positions,
			UnorderedVectorsAre(std::initializer_list<repo::lib::RepoVector3D64>({
				repo::lib::RepoVector3D64(25.540725708007812, 0.1723330020904541, -2.916276693344116),
				repo::lib::RepoVector3D64(25.541114807128906, -0.011896967887878418, -1.731921672821045)
				}),
				0.00001
			)
		);
	}

	// Check objects for which the closest distance is a point to a face

	{
		helper.setCompositeObjectSetsByName(config, { "ClosestPointFace1A" }, { "ClosestPointFace1B" });

		config.tolerance = 0.1;

		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(0));
	}
	{
		config.tolerance = 0.3;

		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(1));
		EXPECT_THAT(results.clashes[0].idA, Eq(config.setA[0].id));
		EXPECT_THAT(results.clashes[0].idB, Eq(config.setB[0].id));

		// The positions reported for a clearance clash are the two points closest
		// to each other out of all pairs that violate the tolerance.

		EXPECT_THAT(
			results.clashes[0].positions,
			UnorderedVectorsAre(std::initializer_list<repo::lib::RepoVector3D64>({
				repo::lib::RepoVector3D64(34.74543380737305,-0.01658613234758377,-1.2869782447814941),
				repo::lib::RepoVector3D64(34.74543380737305,-0.0165863037109375,-1)
				}),
				0.00001
			)
		);
	}

	// Check objects for which the closest distance is a point to an edge

	{
		helper.setCompositeObjectSetsByName(config, { "ClosestPointEdge1A" }, { "ClosestPointEdge1B" });

		config.tolerance = 0.1;

		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(0));
	}
	{
		config.tolerance = 0.2;

		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(1));
		EXPECT_THAT(results.clashes[0].idA, Eq(config.setA[0].id));
		EXPECT_THAT(results.clashes[0].idB, Eq(config.setB[0].id));

		// The positions reported for a clearance clash are the two points closest
		// to each other out of all pairs that violate the tolerance.

		EXPECT_THAT(
			results.clashes[0].positions,
			UnorderedVectorsAre(std::initializer_list<repo::lib::RepoVector3D64>({
				repo::lib::RepoVector3D64(43.709842681884766,-0.005729407072067261,-1.5374937057495117),
				repo::lib::RepoVector3D64(43.709842681884766,-0.005729317665100098,-1.4142134189605713)
				}),
				0.00001
			)
		);
	}

	// Check objects for which the closest distance is an edge to an edge

	{
		helper.setCompositeObjectSetsByName(config, { "ClosestEdgeEdge1A" }, { "ClosestEdgeEdge1B" });

		config.tolerance = 0.5;

		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(0));
	}
	{
		config.tolerance = 1.0;

		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(1));
		EXPECT_THAT(results.clashes[0].idA, Eq(config.setA[0].id));
		EXPECT_THAT(results.clashes[0].idB, Eq(config.setB[0].id));

		// The positions reported for a clearance clash are the two points closest
		// to each other out of all pairs that violate the tolerance.

		EXPECT_THAT(
			results.clashes[0].positions,
			UnorderedVectorsAre(std::initializer_list<repo::lib::RepoVector3D64>({
				repo::lib::RepoVector3D64(55.39568328857422,0.019692540168762207,-2.0002315044403076),
				repo::lib::RepoVector3D64(55.39568328857422,0.019691824913024902,-1.4142136573791504)
				}),
				0.00001
			)
		);
	}
}

TEST(Clash, Clearance2)
{
	// Clash fingerprints are used to determine if a clash is the same across
	// multiple runs. If multiple primitives are involved, the fingerprint should
	// remain the same, regardless of the order in which they are processed.
	// Similarly, if an algorithm takes advantage of early termination, the
	// fingerprinting method should be able to handle this as well.

	ClashDetectionConfig config;
	auto handler = getHandler();

	auto pipeline = new clash::Clearance(handler, config);

	auto composite = pipeline->createCompositeClash();

	struct NarrowphaseResult
	{

	};



}