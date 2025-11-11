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
#include <repo/manipulator/modelutility/clashdetection/clash_exceptions.h>
#include <repo/manipulator/modelutility/clashdetection/repo_polydepth.h>

#include <repo/manipulator/modelutility/clashdetection/predicates.h>

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
	// Tests the parsing of a clash detection config file. This test does not
	// validate the config for internal consistency, which is the responsibility
	// of whatever creates it.

	ClashDetectionConfig config;
	ClashDetectionConfig::ParseJsonFile(getDataPath("/clash/config1.json"), config);

	EXPECT_THAT(config.type, Eq(ClashDetectionType::Clearance));
	EXPECT_THAT(config.tolerance, Eq(0.0001));
	EXPECT_THAT(config.resultsFile, StrEq("results/clash_results.json"));
	
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

	EXPECT_THAT(config.setB.size(), Eq(3));

	EXPECT_THAT(config.setB[0].id, Eq(repo::lib::RepoUUID("06208325-6ea7-45a8-b5a1-ab8be8c4da79")));
	EXPECT_THAT(config.setB[0].meshes.size(), Eq(1));
	EXPECT_THAT(config.setB[0].meshes[0].uniqueId, Eq(repo::lib::RepoUUID("8e8a7c81-a7df-4587-9976-190ff5680a97")));
	EXPECT_THAT(config.setB[0].meshes[0].container->teamspace, StrEq("clash"));
	EXPECT_THAT(config.setB[0].meshes[0].container->container, StrEq("ea72bf4b-ab53-4f59-bef3-694b66484192"));
	EXPECT_THAT(config.setB[0].meshes[0].container->revision == repo::lib::RepoUUID("95988a2e-f71a-462d-9d00-de724fc7cd05"), IsTrue());

	EXPECT_THAT(config.setB[1].id, Eq(repo::lib::RepoUUID("22ea928f-920d-493e-b6b8-7d510842a43f")));
	EXPECT_THAT(config.setB[1].meshes.size(), Eq(2));
	EXPECT_THAT(config.setB[1].meshes[0].uniqueId, Eq(repo::lib::RepoUUID("655b8b50-70d3-4b8a-b4bf-02eed23202e3")));
	EXPECT_THAT(config.setB[1].meshes[0].container->teamspace, StrEq("clash"));
	EXPECT_THAT(config.setB[1].meshes[0].container->container, StrEq("ea72bf4b-ab53-4f59-bef3-694b66484192"));
	EXPECT_THAT(config.setB[1].meshes[0].container->revision == repo::lib::RepoUUID("95988a2e-f71a-462d-9d00-de724fc7cd05"), IsTrue());
	EXPECT_THAT(config.setB[1].meshes[1].uniqueId, Eq(repo::lib::RepoUUID("8ff6d436-91eb-45ed-b110-d8a21354a16d")));
	EXPECT_THAT(config.setB[1].meshes[1].container->teamspace, StrEq("clash"));
	EXPECT_THAT(config.setB[1].meshes[1].container->container, StrEq("d6741305-7014-4d4c-af41-a1b743a35412"));
	EXPECT_THAT(config.setB[1].meshes[1].container->revision == repo::lib::RepoUUID("f7caff9d-d2c2-440e-8b32-7cee46c23f32"), IsTrue());

	EXPECT_THAT(config.setB[2].meshes.size(), Eq(1));
	EXPECT_THAT(config.setB[2].id, Eq(repo::lib::RepoUUID("44fd3346-c9cb-4858-b13e-3683c8fe75bb")));
	EXPECT_THAT(config.setB[2].meshes[0].uniqueId, Eq(repo::lib::RepoUUID("2fc9c8e0-57a4-4b0b-a6f9-1d1a68db3cc4")));
	EXPECT_THAT(config.setB[2].meshes[0].container->teamspace, StrEq("clash"));
	EXPECT_THAT(config.setB[2].meshes[0].container->container, StrEq("d6741305-7014-4d4c-af41-a1b743a35412"));
	EXPECT_THAT(config.setB[2].meshes[0].container->revision == repo::lib::RepoUUID("f7caff9d-d2c2-440e-8b32-7cee46c23f32"), IsTrue());
}

TEST(Clash, EmptySets) 
{
	// If the sets are empty, the engine should do nothing, and it should not crash
	// or leak.

	auto handler = getHandler();
	ClashDetectionEngine engine(handler);

	ClashDetectionConfig config;
	config.type = ClashDetectionType::Clearance;
	config.resultsFile = "clash_results.json";
	config.tolerance = 1;

	engine.runClashDetection(config);
}

TEST(Clash, Scheduler)
{
	// Tests if the ClashScheduler returns a sensible narrowphase set order.

	// The scheduling problem is NP-Hard, so this test doesn't check if the
	// order is optimal, just that all the original broadphase tests still
	// exist, with no duplicates.

	clash::BroadphaseResults broadphaseResults;

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
* probabilisitcally. Primitives are generated in different known configurations
* as a ground truth, to which the measured distances are compared.
* 
* Individual tests for specific intersection configurations are part of the
* geometry unit tests. These serve to test these methods, but also to validate
* the clash generator algorithms as well to an extent, before they are used in 
* the end-to-end tests.
* 
* The current guaranteed accuracy is 1 in 8e6 for vertices, with transforms up
* to 1e11 from the origin. For models imported in mm, this represents an error
* of ±1 mm.
* 
* This domain is determined by the source data the engine has to work with: 
* single precision vertices from the binary buffers, and double precision
* transformations stored in Mongo.
*/

TEST(Clash, TriangleDistanceE2E) 
{
	// Test the accuracy of the end-to-end pipeline in Clearance mode, for
	// multiple problem configurations.

	ClashGenerator clashGenerator;

	auto db = std::make_shared<MockDatabase>();

	const int numIterations = 1;
	const int samplesPerDistance = 10000;
	std::vector<double> distances = { 0, 2, 5 };

	// With an expected accuracy of ±1.0, the expected measured distance ranges
	// are: { 0..1, 1..3 4..6 }.

	for (int i = 0; i < numIterations; ++i)
	{
		CellDistribution space;

		ClashDetectionConfigHelper config;
		config.type = ClashDetectionType::Clearance;

		MockClashScene scene(config.getRevision());

		for (auto d : distances) {
			clashGenerator.distance = d;
			for (int i = 0; i < samplesPerDistance; ++i) {
				auto p = clashGenerator.createTrianglesTransformed(space.sample());
				scene.add(p, config);
			}
		}

		db->setDocuments(scene.bsons);

		{
			config.tolerance = 1.0;
			clash::Clearance pipeline(db, config);
			auto results = pipeline.runPipeline();
			EXPECT_THAT(results.clashes.size(), Eq(samplesPerDistance));
		}

		{
			config.tolerance = 3.0;
			clash::Clearance pipeline(db, config);
			auto results = pipeline.runPipeline();
			EXPECT_THAT(results.clashes.size(), Eq(samplesPerDistance * 2));
		}

		{
			config.tolerance = 6.0;
			clash::Clearance pipeline(db, config);
			auto results = pipeline.runPipeline();
			EXPECT_THAT(results.clashes.size(), Eq(samplesPerDistance * 3));
		}
	}
}

TEST(Clash, AccuracyReport)
{
	// Test the accuracy of the end-to-end pipeline in Clearance mode, for multiple
	// problem configurations.

	GTEST_SKIP(); // Disable this test unless we need to gather more data.

	ClashGenerator clashGenerator;

	auto db = std::make_shared<MockDatabase>();

	const int samplesPerDistance = 10000;
	std::vector<double> distances = { 0, 1, 2, 3 };

	ClearanceAccuracyReport report("C://3drepo//3drepobouncer_ISSUE797//clearanceAccuracyReport.errors.bin");
	CellDistribution space;
	
	// Due to the BVH construction and traversal, it is more efficient to perform
	// multiple iterations of small scenes, rather than one large scene.

	for (int i = 0; i < 100; ++i) {
		for (auto d : distances) {
			ClashDetectionConfigHelper config;
			config.type = ClashDetectionType::Clearance;
			MockClashScene scene(config.getRevision());

			clashGenerator.distance = d;
			for (int i = 0; i < samplesPerDistance; ++i) {
				auto p = clashGenerator.createTrianglesTransformed(space.sample());
				auto ids = scene.add(p, config);
			}

			db->setDocuments(scene.bsons);

			config.tolerance = d + 1.0;
			clash::Clearance pipeline(db, config);
			auto results = pipeline.runPipeline();

			report.add(results, d);
		}
	}
}

/*
* These next tests check the accuracy of the clash detection when running on
* real imports. The purpose of these tests is to ensure the import pipeline
* does not degrade the accuracy of the geometry beyond acceptable limits. A
* test should exist for all supported file formats.
*/

TEST(Clash, Rvt)
{
	// Tests that geometry of a known distance is correctly measured after
	// going through the bouncer import pipeline.

	auto handler = getHandler();
	auto container = makeTemporaryContainer();

	importModel(getDataPath("/clash/clearance.rvt"), *container);

	ClashDetectionConfig config;
	ClashDetectionDatabaseHelper helper(handler);

	helper.setCompositeObjectsByMetadataValue(config, container, "ClashSetA", "ClashSetB");

	config.tolerance = 2;

	auto pipeline = new clash::Clearance(handler, config);
	auto results = pipeline->runPipeline();

	EXPECT_THAT(results.clashes.size(), Eq(10000));
}

TEST(Clash, Nwd)
{
	// Tests that geometry of a known distance is correctly measured after
	// going through the bouncer import pipeline.

	auto handler = getHandler();
	auto container = makeTemporaryContainer();

	importModel(getDataPath("/clash/clearance.nwd"), *container);

	ClashDetectionConfig config;
	ClashDetectionDatabaseHelper helper(handler);

	helper.setCompositeObjectsByMetadataValue(config, container, "ClashSetA", "ClashSetB");

	config.tolerance = 2;

	auto pipeline = new clash::Clearance(handler, config);
	auto results = pipeline->runPipeline();

	EXPECT_THAT(results.clashes.size(), Eq(10000));
}

TEST(Clash, Clearance1)
{
	/*
	* Performs Clearance tests against a number of manually configured imports.
	* This test is used to verify end-to-end functionality against a real
	* collection with pre-defined ground truth. It covers the same functionality
	* as the accuracy tests above but with a different approach to the tests
	* to maximise robustness.
	*/

	auto handler = getHandler();
	ClashDetectionConfig config;

	ClashDetectionDatabaseHelper helper(handler);

	auto container = helper.getContainerByName("clearance_1");

	// Check objects that are touching via coincident vertices

	// The tolerance for "zero" should be epsilon, as quantisation error will be
	// expected in any measured distances.

	config.tolerance = 0.000001;

	{
		helper.setCompositeObjectSetsByName(config, container, { "TouchingPointsA" }, { "TouchingPointsB" });

		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(1));
		EXPECT_THAT(results.clashes[0].idA, Eq(config.setA[0].id));
		EXPECT_THAT(results.clashes[0].idB, Eq(config.setB[0].id));
	}

	// Check objects that are touching via conincident edges

	{
		helper.setCompositeObjectSetsByName(config, container, { "TouchingEdgeA" }, { "TouchingEdgeB" });

		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(1));
		EXPECT_THAT(results.clashes[0].idA, Eq(config.setA[0].id));
		EXPECT_THAT(results.clashes[0].idB, Eq(config.setB[0].id));
	}


	// Check objects that are touching via coincident faces

	{
		helper.setCompositeObjectSetsByName(config, container, { "TouchingFaceA" }, { "TouchingFaceB" });

		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(1));
		EXPECT_THAT(results.clashes[0].idA, Eq(config.setA[0].id));
		EXPECT_THAT(results.clashes[0].idB, Eq(config.setB[0].id));
	}


	// Check objects for which the closest distance is a point to another point

	{
		helper.setCompositeObjectSetsByName(config, container, { "ClosestPointPoint1A" }, { "ClosestPointPoint1B" });

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
		helper.setCompositeObjectSetsByName(config, container, { "ClosestPointFace1A" }, { "ClosestPointFace1B" });

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
		helper.setCompositeObjectSetsByName(config, container, { "ClosestPointEdge1A" }, { "ClosestPointEdge1B" });

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
		helper.setCompositeObjectSetsByName(config, container, { "ClosestEdgeEdge1A" }, { "ClosestEdgeEdge1B" });

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

/*
* This next set of tests checks specific features of the engine, such as its
* error handling.
*/

TEST(Clash, SupportedRanges)
{
	// If a mesh is greater than the mesh limits in any dimension after being subject
	// to scale, or two transformations have a difference in offset greater than the
	// translation limit, then we cannot guarantee accuracy of the algorithm as the
	// precision in the original vertices will have been lost. In this case we should
	// issue a warning and refuse to return any results.

	auto db = std::make_shared<MockDatabase>();

	ClashGenerator clashGenerator;
	clashGenerator.distance = 1;
	
	{
		// These settings will generate primitives that are larger than the mesh limit
		// in at least one dimension.

		clashGenerator.size1 = MESH_LIMIT * 2;
		clashGenerator.size2 = MESH_LIMIT * 2;

		ClashDetectionConfigHelper config;
		config.type = ClashDetectionType::Clearance;
		MockClashScene scene(config.getRevision());
		scene.add(clashGenerator.createTrianglesVF(
			repo::lib::RepoBounds({ repo::lib::RepoVector3D64(0, 0, 0) })),
			config
		);

		db->setDocuments(scene.bsons);

		clash::Clearance pipeline(db, config);
		try {
			auto results = pipeline.runPipeline();
			FAIL() << "Expected MeshBoundsException due to unsupported mesh size.";
		}
		catch (const clash::ClashDetectionException& ex) {
			auto meshException = dynamic_cast<const clash::MeshBoundsException*>(&ex);
			EXPECT_THAT(meshException != nullptr, IsTrue());
		}
		catch (std::exception& e) {
			FAIL() << "Expected MeshBoundsException due to unsupported mesh size: " << e.what();
		}
	}

	{
		clashGenerator.size1 = 1e6;
		clashGenerator.size2 = 1e6;

		ClashDetectionConfigHelper config;
		config.type = ClashDetectionType::Clearance;
		MockClashScene scene(config.getRevision());
		scene.add(clashGenerator.createTrianglesVF(
			repo::lib::RepoBounds({ repo::lib::RepoVector3D64(1e12, 1e12, 1e12), repo::lib::RepoVector3D64(2e12, 2e12, 2e12) })),
			config
		);

		db->setDocuments(scene.bsons);

		clash::Clearance pipeline(db, config);
		try {
			auto results = pipeline.runPipeline();
			FAIL() << "Expected TransformBoundsException due to unsupported transform size.";
		}
		catch (const clash::ClashDetectionException& ex) {
			auto transformException = dynamic_cast<const clash::TransformBoundsException*>(&ex);
			EXPECT_THAT(transformException != nullptr, IsTrue());
		}
		catch (std::exception& e) {
			FAIL() << "Expected TransformBoundsException due to offset: " << e.what();
		}
	}

	{
		clashGenerator.size1 = 1e6;
		clashGenerator.size2 = 1e6;

		ClashDetectionConfigHelper config;
		config.type = ClashDetectionType::Clearance;
		MockClashScene scene(config.getRevision());

		auto problem = clashGenerator.createTrianglesVF(repo::lib::RepoBounds({ repo::lib::RepoVector3D64(0, 0, 0) }));

		problem.a.m = repo::lib::RepoMatrix::scale(repo::lib::RepoVector3D64(1e6, 1e6, 1e6)) * problem.a.m;

		scene.add(problem, config);

		db->setDocuments(scene.bsons);

		clash::Clearance pipeline(db, config);
		try {
			auto results = pipeline.runPipeline();
			FAIL() << "Expected TransformBoundsException due to unsupported transform size.";
		}
		catch (const clash::ClashDetectionException& ex) {
			auto transformException = dynamic_cast<const clash::TransformBoundsException*>(&ex);
			EXPECT_THAT(transformException != nullptr, IsTrue());
		}
		catch (std::exception& e) {
			FAIL() << "Expected TransformBoundsException due to scale: " << e.what();
		}
	}
}

struct PipelineRunner {
	ClashDetectionConfigHelper& config;
	std::shared_ptr<MockDatabase> db;

	PipelineRunner(ClashDetectionConfigHelper& config):
		config(config), db(std::make_shared<MockDatabase>())
	{
	}

	ClashDetectionReport run(std::initializer_list<testing::TransformTriangles> problems)
	{ 
		config.clearObjectSets();

		MockClashScene scene(config.getRevision());

		for (auto& problem : problems) {
			scene.add(problem, config);
		}

		db->setDocuments(scene.bsons);
		clash::Clearance pipeline(db, config);
		return pipeline.runPipeline();
	}
};

TEST(Clash, Fingerprinting)
{
	// Clash fingerprints are used to determine if a clash is the same across
	// multiple runs. If multiple primitives are involved, the fingerprint should
	// remain the same, regardless of the order in which they are processed.
	// Similarly, if an algorithm takes advantage of early termination, the
	// fingerprinting method should be able to handle this as well.

	ClashGenerator clashGenerator;
	clashGenerator.distance = 0;

	ClashDetectionConfigHelper config;
	config.type = ClashDetectionType::Clearance;
	PipelineRunner pipeline(config);

	{	
		config.tolerance = 100;

		auto p = clashGenerator.createTrianglesVF(repo::lib::RepoBounds({ repo::lib::RepoVector3D64(0, 0, 0) }));

		auto run1 = pipeline.run({ p });
		auto run2 = pipeline.run({ p });

		// Running the clash with the same primitives should result in the same
		// fingerprints, even though by calling the runClearancePipeline helper
		// method, the UUIDs will have changed.

		EXPECT_THAT(run1.clashes[0].fingerprint, Eq(run2.clashes[0].fingerprint));

		// The tolerance for this test is set very high so we can make minor adjustments
		// to the same problem to test the fingerprinting sensitivity.
		// Small changes to the primitives (and so intersection location) should result
		// in different fingerprints.

		p.a.m = repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(1, 0, 0)) * p.a.m;

		auto run3 = pipeline.run({ p });

		EXPECT_THAT(run1.clashes[0].fingerprint, Not(Eq(run3.clashes[0].fingerprint)));

		// We don't care too much about fingerprints being unique - as they
		// should be evaluated in the context of the ids as well.
	}
}

TEST(Clash, Units)
{
	// The clash engine should respect the units in the model settings, with
	// clashes only being detected when units are properly applied, including
	// where clash detection sets mix containers of different units.

	auto handler = getHandler();
	ClashDetectionConfig config;
	ClashDetectionDatabaseHelper helper(handler);

	auto m = helper.getContainerByName("cone_m");
	auto mm = helper.getContainerByName("cone_mm");
	auto ft = helper.getContainerByName("cone_ft");

	helper.createCompositeObjectsByMetadataValue(config.setA, m.get(), "Cone");

	helper.createCompositeObjectsByMetadataValue(config.setB, mm.get(), "Cone");
	helper.createCompositeObjectsByMetadataValue(config.setB, ft.get(), "Cone");

	config.tolerance = 10; // 10 mm
	auto pipeline = new clash::Clearance(handler, config);
	auto results = pipeline->runPipeline();

	EXPECT_THAT(results.clashes.size(), Eq(2));

	// Clash results though are always given in mm, in Project Coordinates.

	for (const auto& clash : results.clashes) {
		for (const auto& position : clash.positions) {
			EXPECT_THAT(position, VectorNear(repo::lib::RepoVector3D64(-2540.357666, 0.000075, -1926.315186), 1));
		}
	}
}

TEST(Clash, SelfClearance)
{
	// Self-Clearance is not currently supported - sets must be disjoint and
	// an exception will be thrown if they are not. This is a future feature.

	auto handler = getHandler();
	ClashDetectionConfig config;
	ClashDetectionDatabaseHelper helper(handler);

	auto c = helper.getContainerByName("cubes_self");

	auto set = {
		helper.createCompositeObject(c.get(), "Cube1"),
		helper.createCompositeObject(c.get(), "Cube2"),
		helper.createCompositeObject(c.get(), "Cube3"),
		helper.createCompositeObject(c.get(), "Cube4"),
		helper.createCompositeObject(c.get(), "Cube5"),
		helper.createCompositeObject(c.get(), "Cube6"),
		helper.createCompositeObject(c.get(), "Cube7")
	};
	
	config.setA.insert(config.setA.end(), set.begin(), set.end());
	config.setB.insert(config.setB.end(), set.begin(), set.end());

	config.tolerance = 1;
	config.type = ClashDetectionType::Clearance;

	auto pipeline = new clash::Clearance(handler, config);
	try {
		auto results = pipeline->runPipeline();
		FAIL() << "Expected OverlappingSetsException due to due to self-tests.";
	}
	catch (const clash::ClashDetectionException& ex) {
		auto exception = dynamic_cast<const clash::OverlappingSetsException*>(&ex);
		EXPECT_THAT(exception != nullptr, IsTrue());
	}
	catch (std::exception& e) {
		FAIL() << "Expected OverlappingSetsException due to self-tests: " << e.what();
	}
}

TEST(Clash, RepoPolyDepth1)
{
	// Tests the penetration depth estimation (PolyDepth) of the geometry utils
	// with procedural geometry. Penetration depth is approximate, with a
	// guaranteed upper bound only, so the acceptance criteria is that the
	// estimated depth is greater than or equal to the generated depth, and that
	// applying the correction always resolves the collision. 

	CellDistribution space;
	ClashGenerator clashGenerator;
	
	// For this test, the distance defines the spacing amongst the non-intersecting
	// pairs.
	clashGenerator.distance = {0.1, 4};

	for (int itr = 0; itr < 50000; ++itr)
	{
		auto clash = clashGenerator.createHardSoup(space.sample());

		auto a = ClashGenerator::triangles(clash.a);
		auto b = ClashGenerator::triangles(clash.b);

		EXPECT_THAT(intersects(a, b), IsTrue());

		geometry::RepoPolyDepth pd(a, b);

		// Before any iterations, PolyDepth should return a valid upper bounds.

		auto v = pd.getPenetrationVector();

		// The penetration vector should be non-zero, and non-infinity, and it
		// should work to resolve the collision when applied to set a.

		EXPECT_THAT(v.norm(), Gt(0));
		EXPECT_THAT(std::isfinite(v.norm()), IsTrue());

		ClashGenerator::applyTransforms(a, repo::lib::RepoMatrix::translate(v));

		EXPECT_THAT(intersects(a, b), IsFalse());
	}
}

TEST(Clash, RepoPolyDepth2)
{
	// Tests the penetration depth estimation (PolyDepth) of the geometry utils
	// on some difficult problems that we have an expected result for.
	// Contractually PolyDepth only guarantees an upper bound, so if the
	// implementation changes, the acceptance criteria may need to change as well.


}

TEST(Clash, Hard1)
{
	// A simple test case of two interpenetrating polyhedra close to the origin.

	auto db = std::make_shared<MockDatabase>();

	ClashDetectionConfigHelper config;
	config.type = ClashDetectionType::Hard;
	config.tolerance = 0;

	CellDistribution space(1, 1);
	ClashGenerator clashGenerator;

	clashGenerator.size1 = 1;
	clashGenerator.size2 = 1;

	auto clash = clashGenerator.createHard1(space.sample());

	MockClashScene scene(config.getRevision());
	scene.add(clash, config);

	db->setDocuments(scene.bsons);

	auto pipeline = new clash::Hard(db, config);
	auto results = pipeline->runPipeline();

	EXPECT_THAT(results.clashes.size(), Eq(0));
}

TEST(Clash, ResultsSerialisation)
{

}

TEST(Clash, NodeCache)
{
	// need to make sure cache only returns one object even if called multiple times

	// need to make sure cache doesn't go away until all references have gone.
}