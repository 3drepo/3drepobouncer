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
#include <numbers>
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
#include <repo/manipulator/modelutility/clashdetection/geometry_tests_closed.h>
#include <repo/manipulator/modelutility/clashdetection/geometry_utils.h>
#include <repo/manipulator/modelutility/clashdetection/clash_scheduler.h>
#include <repo/manipulator/modelutility/clashdetection/clash_exceptions.h>
#include <repo/manipulator/modelutility/clashdetection/repo_polydepth.h>
#include <repo/manipulator/modelutility/clashdetection/repo_deformdepth.h>
#include <repo/manipulator/modelutility/clashdetection/clash_node_cache.h>

#include <repo/manipulator/modeloptimizer/bvh/bvh.hpp>
#include <repo/manipulator/modeloptimizer/bvh/sweep_sah_builder.hpp>
#include <repo/manipulator/modelutility/clashdetection/bvh_operators.h>
#include <repo/manipulator/modelutility/clashdetection/predicates.h>
#include "repo/manipulator/modelconvertor/import/odaHelper/file_processor_nwd.h"

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
	
	// If self-intersection options have not been specified, they default to false.

	EXPECT_THAT(config.selfIntersectsA, IsFalse());
	EXPECT_THAT(config.selfIntersectsB, IsFalse());

	EXPECT_THAT(config.setA.size(), Eq(2));

	// ParseJsonFile does not guarantee to maintain the same order as in the file
	// as it uses an unordered map internally. Here we sort for the purposes of
	// checking the results only - in practice the order doesn't matter so no
	// sorting is applied.

	std::sort(config.setA.begin(), config.setA.end(),
		[](const CompositeObject& a, const CompositeObject& b) {
			return a.id < b.id;
		}
	);
	std::sort(config.setB.begin(), config.setB.end(),
		[](const CompositeObject& a, const CompositeObject& b) {
			return a.id < b.id;
		}
	);

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

	config = {};
	ClashDetectionConfig::ParseJsonFile(getDataPath("/clash/config2.json"), config);

	EXPECT_THAT(config.selfIntersectsA, IsTrue());
	EXPECT_THAT(config.selfIntersectsB, IsFalse());

	config = {};
	ClashDetectionConfig::ParseJsonFile(getDataPath("/clash/config3.json"), config);

	EXPECT_THAT(config.selfIntersectsA, IsFalse());
	EXPECT_THAT(config.selfIntersectsB, IsTrue());
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

	// The scheduling problem is NP-Hard, so this test doesn't check if the order
	// is optimal, just that all the original broadphase tests still exist, with
	// no duplicates.

	// The scheduler should work with anything that fits in a void* - here we use
	// indices to test its correctness, and also that it doesn't dereference (it
	// shouldn't), because attempting to will result in an access violation.

	std::vector<std::pair<size_t, size_t>> broadphaseResults;

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

	// Make sure regardless of what the RNG does, that we include some difficult
	// conditions: edges from both directions, duplicates and zero-length edges.

	broadphaseResults.push_back({ 3, 0 });
	broadphaseResults.push_back({ 0, 3 });
	broadphaseResults.push_back({ 3, 0 });
	broadphaseResults.push_back({ 0, 3 });
	
	// The clash detection should never actually compare an entry with itself,
	// but the scheduler should be as robust as possible.

	broadphaseResults.push_back({ 0, 0 });
	broadphaseResults.push_back({ 0, 0 });
	broadphaseResults.push_back({ 0, 0 });


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

TEST(Clash, ClearanceE2E) 
{
	// Test the accuracy of the end-to-end pipeline in Clearance mode, for
	// multiple problem configurations.

	ClashGenerator clashGenerator;

	auto db = std::make_shared<MockDatabase>();

	const int numIterations = 10;
	const int samplesPerDistance = 1000;
	const int selfSamplesA = 100;
	const int selfSamplesB = 100;
	std::vector<double> distances = { -0.001, 2, 5 };

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
			for (int i = 0; i < selfSamplesA; ++i) {
				auto p = clashGenerator.createTrianglesTransformed(space.sample());
				auto [a, b] = scene.add(p);
				config.addCompositeObjects({ a, b }, {});
			}
			for (int i = 0; i < selfSamplesB; ++i) {
				auto p = clashGenerator.createTrianglesTransformed(space.sample());
				auto [a, b] = scene.add(p);
				config.addCompositeObjects({}, { a, b });
			}
		}

		db->setDocuments(scene.bsons);

		auto expected = samplesPerDistance;

		{
			config.tolerance = 1.0;
			clash::Clearance pipeline(db, config);
			auto results = pipeline.runPipeline();
			EXPECT_THAT(results.clashes.size(), Eq(expected));
		}

		{
			config.selfIntersectsA = true;
			expected += selfSamplesA;

			clash::Clearance pipeline(db, config);
			auto results = pipeline.runPipeline();
			EXPECT_THAT(results.clashes.size(), Eq(expected));
		}

		{
			config.tolerance = 3.0;
			clash::Clearance pipeline(db, config);
			auto results = pipeline.runPipeline();
			EXPECT_THAT(results.clashes.size(), Eq(expected * 2));
		}

		{
			config.tolerance = 6.0;
			clash::Clearance pipeline(db, config);
			auto results = pipeline.runPipeline();
			EXPECT_THAT(results.clashes.size(), Eq(expected * 3));
		}
	}
}

TEST(Clash, AccuracyReport)
{
	// Test the accuracy of the end-to-end pipeline in Clearance mode, for multiple
	// problem configurations.

	// To output a report, provide a path and run this test.

	std::string path = {};

	if (!path.size()) {
		GTEST_SKIP(); // Disable this test unless we need to gather data.
	}

	ClashGenerator clashGenerator;

	auto db = std::make_shared<MockDatabase>();

	const int samplesPerDistance = 10000;
	std::vector<double> distances = { 0, 1, 2, 3 };

	ClearanceAccuracyReport report(path);
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

// TODO FT: Remove once the other NWD tests are in palce
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

void getMissingIndexes(
	ClashDetectionConfig& config,
	ClashDetectionReport& report,
	std::vector<int>& missingA,
	std::vector<int>& missingB)
{
	for (int i = 0; i < config.setA.size(); i++) {
		auto compIdA = config.setA[i].id;
		auto compIdB = config.setB[i].id;

		bool foundA = false;
		bool foundB = false;
		for (int j = 0; j < report.clashes.size(); j++) {
			auto clashCompIdA = report.clashes[j].idA;
			auto clashCompIdB = report.clashes[j].idB;

			if (compIdA == clashCompIdA || compIdA == clashCompIdB) {
				foundA = true;
			}

			if (compIdB == clashCompIdA || compIdB == clashCompIdB) {
				foundB = true;
			}
		}

		if (!foundA) {
			missingA.push_back(i);
		}
		if (!foundB) {
			missingB.push_back(i);
		}
	}
}

void getMissingCompIds(
	ClashDetectionConfig& config,
	ClashDetectionReport& report,
	std::vector<repo::lib::RepoUUID>& missingCompIds)
{
	// Look for missing indexes in the clashes
	std::vector<int> missingA;
	std::vector<int> missingB;
	getMissingIndexes(config, report, missingA, missingB);

	// Lookup the missing compound ids using the indexes
	for (auto index : missingA)
	{
		missingCompIds.push_back(config.setA[index].id);
	}
	for (auto index : missingB)
	{
		missingCompIds.push_back(config.setB[index].id);
	}
}

// TODO FT: Remove once the other NWD tests are in palce
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
		EXPECT_THAT(
			std::vector<repo::lib::RepoUUID>({results.clashes[0].idA, results.clashes[0].idB}),
			UnorderedElementsAre(config.setA[0].id, config.setB[0].id)
		);
	}

	// Check objects that are touching via coincident edges

	{
		helper.setCompositeObjectSetsByName(config, container, { "TouchingEdgeA" }, { "TouchingEdgeB" });

		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(1));
		EXPECT_THAT(
			std::vector<repo::lib::RepoUUID>({ results.clashes[0].idA, results.clashes[0].idB }),
			UnorderedElementsAre(config.setA[0].id, config.setB[0].id)
		);
	}


	// Check objects that are touching via coincident faces

	{
		helper.setCompositeObjectSetsByName(config, container, { "TouchingFaceA" }, { "TouchingFaceB" });

		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(1));
		EXPECT_THAT(
			std::vector<repo::lib::RepoUUID>({ results.clashes[0].idA, results.clashes[0].idB }),
			UnorderedElementsAre(config.setA[0].id, config.setB[0].id)
		);
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
		EXPECT_THAT(
			std::vector<repo::lib::RepoUUID>({ results.clashes[0].idA, results.clashes[0].idB }),
			UnorderedElementsAre(config.setA[0].id, config.setB[0].id)
		);

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
		EXPECT_THAT(
			std::vector<repo::lib::RepoUUID>({ results.clashes[0].idA, results.clashes[0].idB }),
			UnorderedElementsAre(config.setA[0].id, config.setB[0].id)
		);

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
		EXPECT_THAT(
			std::vector<repo::lib::RepoUUID>({ results.clashes[0].idA, results.clashes[0].idB }),
			UnorderedElementsAre(config.setA[0].id, config.setB[0].id)
		);

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
		EXPECT_THAT(
			std::vector<repo::lib::RepoUUID>({ results.clashes[0].idA, results.clashes[0].idB }),
			UnorderedElementsAre(config.setA[0].id, config.setB[0].id)
		);

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

TEST(Clash, MeshIdsAllowedOnlyOnce)
{
	// A given mesh id should only be a part of one Composite Object per set.

	auto db = std::make_shared<MockDatabase>();

	ClashGenerator clashGenerator;
	clashGenerator.distance = 1;

	auto clash = clashGenerator.createTrianglesTransformed({});

	ClashDetectionConfigHelper config;
	config.type = ClashDetectionType::Clearance;
	MockClashScene scene(config.getRevision());

	scene.add(clash, config);

	db->setDocuments(scene.bsons);

	CompositeObject objA1;
	objA1.id = repo::lib::RepoUUID::createUUID();
	objA1.meshes.push_back(config.setA[0].meshes[0]);

	config.setA.push_back(objA1); // Duplicate mesh in set A

	clash::Clearance pipeline(db, config);
	try {
		auto results = pipeline.runPipeline();
		FAIL() << "Expected DuplicateMeshIdsException.";
	}
	catch (const clash::ClashDetectionException& ex) {
		auto exception = dynamic_cast<const clash::DuplicateMeshIdsException*>(&ex);
		EXPECT_THAT(exception != nullptr, IsTrue());
		EXPECT_THAT(exception && exception->uniqueId, Eq(config.setA[0].meshes[0].uniqueId));
	}
	catch (std::exception& e) {
		FAIL() << "Expected DuplicateMeshIdsException: " << e.what();
	}
}

struct PipelineRunner {
	ClashDetectionConfigHelper& config;
	std::shared_ptr<MockDatabase> db;

	PipelineRunner(ClashDetectionConfigHelper& config):
		config(config), db(std::make_shared<MockDatabase>())
	{
	}

	template<typename T>
	ClashDetectionReport run(std::initializer_list<T> problems)
	{ 
		config.clearObjectSets();

		MockClashScene scene(config.getRevision());

		for (auto& problem : problems) {
			scene.add(problem, config);
		}

		db->setDocuments(scene.bsons);

		if(config.type == ClashDetectionType::Hard)
		{
			clash::Hard pipeline(db, config);
			return pipeline.runPipeline();
		} else if (config.type == ClashDetectionType::Clearance)
		{
			clash::Clearance pipeline(db, config);
			return pipeline.runPipeline();
		}

		throw std::runtime_error("Unsupported clash detection type");
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

	for (size_t i = 0; i < 100; i++)
	{	
		ClashDetectionConfigHelper config;
		config.type = ClashDetectionType::Clearance;
		PipelineRunner pipeline(config);

		config.tolerance = 100;

		auto p = clashGenerator.createTrianglesVF(repo::lib::RepoBounds({ repo::lib::RepoVector3D64(0, 0, 0) }));

		auto run1 = pipeline.run({ p });
		auto run2 = pipeline.run({ p });

		EXPECT_THAT(run1.clashes.size(), Eq(1));

		// Fingerprints should not be empty for real clashes

		EXPECT_THAT(run1.clashes[0].fingerprint, Not(Eq(0)));

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

	CellDistribution space;

	for (size_t i = 0; i < 100; i++)
	{
		ClashDetectionConfigHelper config;
		config.type = ClashDetectionType::Hard;
		PipelineRunner pipeline(config);

		config.tolerance = 0;

		auto p = clashGenerator.createHardSoup(space.sample());

		auto run1 = pipeline.run({ p });
		auto run2 = pipeline.run({ p });

		EXPECT_THAT(run1.clashes.size(), Eq(1));

		EXPECT_THAT(run1.clashes[0].fingerprint, Not(Eq(0)));
		EXPECT_THAT(run1.clashes[0].fingerprint, Eq(run2.clashes[0].fingerprint));

		p.a.m = repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(1, 0, 0)) * p.a.m;

		auto run3 = pipeline.run({ p });

		EXPECT_THAT(run1.clashes[0].fingerprint, Not(Eq(run3.clashes[0].fingerprint)));
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

	helper.createCompositeObjectsFromContainer(config.setA, m.get());

	helper.createCompositeObjectsFromContainer(config.setB, mm.get());
	helper.createCompositeObjectsFromContainer(config.setB, ft.get());

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
	auto handler = getHandler();
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
	
	// Should support self-clash tests when the flag is set. If the flag is present
	// for one set, the other set may be empty.

	ClashDetectionConfig config;
	config.setA.insert(config.setA.end(), set.begin(), set.end());
	config.selfIntersectsA = true;
	config.tolerance = 1;
	config.type = ClashDetectionType::Clearance;

	auto pipeline = new clash::Clearance(handler, config);
	auto results = pipeline->runPipeline();

	EXPECT_THAT(results.clashes.size(), Eq(3));
}

TEST(Clash, OverlappingSets)
{
	// If a CompositeId is present in both sets, then those Ids should be tested
	// against both sets, and within themselves as well.

	// Creates four boxes all within the tolerance of eachother.

	auto db = std::make_shared<MockDatabase>();
	ClashDetectionConfigHelper config;

	auto box = test::utils::mesh::makeUnitCube();

	{
		config.tolerance = 0.05;
		auto d = 0.4;

		MockClashScene scene(config.getRevision());

		auto b1 = scene.add(TransformedEntity{ box, repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(-d,  d, 0)) });
		auto b2 = scene.add(TransformedEntity{ box, repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(d,  d, 0)) });
		auto b3 = scene.add(TransformedEntity{ box, repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(-d, -d, 0)) });
		auto b4 = scene.add(TransformedEntity{ box, repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(d, -d, 0)) });

		db->setDocuments(scene.bsons);

		{
			// If we split the boxes down the middle, we should get four clashes - 
			// the outer product between the two sets.

			config.setCompositeObjects({ b1, b2 }, { b3, b4 });

			{
				clash::Clearance pipeline(db, config);
				auto results = pipeline.runPipeline();
				EXPECT_THAT(results.clashes.size(), Eq(4));
			}
			{
				clash::Hard pipeline(db, config);
				auto results = pipeline.runPipeline();
				EXPECT_THAT(results.clashes.size(), Eq(4));
			}
		}

		{
			// One box now sits in both sets, so we expect five clashes, because the
			// shared box will clash with the box on its side of the split as well.

			config.setCompositeObjects({ b1, b2, b3 }, { b3, b4 });
			{
				clash::Clearance pipeline(db, config);
				auto results = pipeline.runPipeline();
				EXPECT_THAT(results.clashes.size(), Eq(5));
			}
			{
				clash::Hard pipeline(db, config);
				auto results = pipeline.runPipeline();
				EXPECT_THAT(results.clashes.size(), Eq(5));
			}
		}

		{
			// With an entire side of the split shared between both sets, we expect
			// six clashes - each box on one side clashing with both boxes on the
			// other, the self-clash between the shared ones, and the clash beween
			// the two unique ones on the sides of the first split.

			config.setCompositeObjects({ b1, b3, b4 }, { b2, b3, b4 });
			{
				clash::Clearance pipeline(db, config);
				auto results = pipeline.runPipeline();
				EXPECT_THAT(results.clashes.size(), Eq(6));
			}
			{
				clash::Hard pipeline(db, config);
				auto results = pipeline.runPipeline();
				EXPECT_THAT(results.clashes.size(), Eq(6));
			}
		}
	}
}

TEST(Clash, Contains)
{
	// For the absolute contains case, both clearance and hard mode should detect
	// the clashes in the same way.

	auto handler = getHandler();
	ClashDetectionDatabaseHelper helper(handler);

	auto c = helper.getContainerByName("contains_2");

	auto hull1 = helper.createCompositeObject(c.get(), "Hull1");
	auto hull2 = helper.createCompositeObject(c.get(), "Hull2");
	auto contained1 = helper.createCompositeObject(c.get(), "Contained1");
	auto contained2 = helper.createCompositeObject(c.get(), "Contained2");
	auto contained3 = helper.createCompositeObject(c.get(), "Contained3");
	auto contained4 = helper.createCompositeObject(c.get(), "Contained4");
	auto contained5 = helper.createCompositeObject(c.get(), "Contained5");
	auto contained6 = helper.createCompositeObject(c.get(), "Contained6");

	{
		// The two hulls form separate composite objects

		ClashDetectionConfig config;

		config.setA.push_back(hull1);
		config.setA.push_back(hull2);

		config.setB.push_back(contained1);
		config.setB.push_back(contained2);
		config.setB.push_back(contained3);
		config.setB.push_back(contained4);
		config.setB.push_back(contained5);
		config.setB.push_back(contained6);

		config.tolerance = 0.01;

		{
			config.type = ClashDetectionType::Clearance;
			auto pipeline = new clash::Clearance(handler, config);
			auto results = pipeline->runPipeline();

			EXPECT_THAT(results.clashes.size(), Eq(6));
		}

		{
			config.type = ClashDetectionType::Hard;
			auto pipeline = new clash::Hard(handler, config);
			auto results = pipeline->runPipeline();

			EXPECT_THAT(results.clashes.size(), Eq(6));
		}
	}

	{
		// Hull 1 and 2 form a single composite object, but all the contained
		// components are still separate

		ClashDetectionConfig config;

		config.setA.push_back(combineCompositeObjects({
			hull1,
			hull2
		}));

		config.setB.push_back(contained1);
		config.setB.push_back(contained2);
		config.setB.push_back(contained3);
		config.setB.push_back(contained4);
		config.setB.push_back(contained5);
		config.setB.push_back(contained6);

		config.tolerance = 0.01;

		{
			config.type = ClashDetectionType::Clearance;
			auto pipeline = new clash::Clearance(handler, config);
			auto results = pipeline->runPipeline();

			EXPECT_THAT(results.clashes.size(), Eq(6));
		}

		{
			config.type = ClashDetectionType::Hard;
			auto pipeline = new clash::Hard(handler, config);
			auto results = pipeline->runPipeline();

			EXPECT_THAT(results.clashes.size(), Eq(6));
		}
	}

	{
		// Contained items form single composite objects where all components
		// are inside one of the two hulls, which should detect the same clashes
		// (though only one per composite this time).

		ClashDetectionConfig config;

		config.setA.push_back(hull1);
		config.setA.push_back(hull2);

		config.setB.push_back(combineCompositeObjects({
			contained1,
			contained2,
			contained3,
			contained6
			})
		);
		config.setB.push_back(combineCompositeObjects({
			contained4,
			contained5
			})
		);

		config.tolerance = 0.01;

		{
			config.type = ClashDetectionType::Clearance;
			auto pipeline = new clash::Clearance(handler, config);
			auto results = pipeline->runPipeline();

			EXPECT_THAT(results.clashes.size(), Eq(2));
		}

		{
			config.type = ClashDetectionType::Hard;
			auto pipeline = new clash::Hard(handler, config);
			auto results = pipeline->runPipeline();

			EXPECT_THAT(results.clashes.size(), Eq(2));
		}
	}

	{
		// Even though all combined items are contained in the hulls, because the
		// meshes are not continous this will not necessarily be detected as a clash
		// in Hard Mode.

		ClashDetectionConfig config;

		config.setA.push_back(combineCompositeObjects({
			hull1,
			hull2
			})
		);

		config.setB.push_back(combineCompositeObjects({
			contained1,
			contained2,
			contained3,
			contained6,
			contained4,
			contained5
			})
		);

		config.tolerance = 0.01;

		{
			config.type = ClashDetectionType::Clearance;
			auto pipeline = new clash::Clearance(handler, config);
			auto results = pipeline->runPipeline();

			EXPECT_THAT(results.clashes.size(), Eq(1));
		}

		{
			config.type = ClashDetectionType::Hard;
			auto pipeline = new clash::Hard(handler, config);
			auto results = pipeline->runPipeline();

			EXPECT_THAT(results.clashes.size(), Eq(0));
		}
	}
}

TEST(Clash, BvhTraversal)
{
	// Tests the intra-bvh traversal operator

	// Creates a set of bvh clusters that should be identified to intersect with
	// eachother. The BVH traversal doesn't operate directly on the coordinates
	// so its OK here to just spread these out on the x-axis. The underlying bounds
	// operations unit tests will cover those in more detail.

	auto unit = repo::lib::RepoBounds(
		repo::lib::RepoVector3D64(-0.5, -0.5, -0.5),
		repo::lib::RepoVector3D64(0.5, 0.5, 0.5)
	);

	auto boundingBoxes = std::vector<repo::lib::RepoBounds>();

	// Three boxes overlapping perfectly

	boundingBoxes.push_back(repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(0, 0, 0)) * unit);
	boundingBoxes.push_back(repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(0, 0, 0)) * unit);
	boundingBoxes.push_back(repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(0, 0, 0)) * unit);

	// Three touching eachother

	boundingBoxes.push_back(repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(2, 0, 0)) * unit);
	boundingBoxes.push_back(repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(3, 0, 0)) * unit);
	boundingBoxes.push_back(repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(4, 0, 0)) * unit);

	// Three with a distance of 0.5

	boundingBoxes.push_back(repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(10, 0, 0)) * unit);
	boundingBoxes.push_back(repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(11.5, 0, 0)) * unit);
	boundingBoxes.push_back(repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(13, 0, 0)) * unit);

	// Three with a distance of 1

	boundingBoxes.push_back(repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(20, 0, 0)) * unit);
	boundingBoxes.push_back(repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(22, 0, 0)) * unit);
	boundingBoxes.push_back(repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(24, 0, 0)) * unit);

	// Convert to bvh::bounds

	bvh::Bvh<double> bvh;

	{
		auto bounds = std::vector<bvh::BoundingBox<double>>();
		auto centers = std::vector<bvh::Vector3<double>>();

		for(auto& b : boundingBoxes) {
			bounds.push_back(bvh::BoundingBox<double>(
				bvh::Vector3<double>(b.min().x, b.min().y, b.min().z),
				bvh::Vector3<double>(b.max().x, b.max().y, b.max().z)
			));
		}
		centers.push_back(bounds.back().center());

		auto globalBounds = bvh::compute_bounding_boxes_union(bounds.data(), bounds.size());

		bvh::SweepSahBuilder<bvh::Bvh<double>> builder(bvh);
		builder.max_leaf_size = 1;
		builder.build(globalBounds, bounds.data(), centers.data(), bounds.size());
	}

	struct DistanceQuery : public bvh::DistanceQuery {

		std::vector<std::pair<size_t, size_t>> intersections;

		void intersect(size_t a, size_t b) override {
			// This re-ordering makes it easier to define the expected results. It is not
			// necessary for the actual collision detection - and in fact is better not
			// not to have it because it breaks ordering when testing between two BVHs.
			if (b < a) {
				std::swap(a, b); 
			}
			intersections.push_back({ a, b });
		}
	};

	// With no distance, only the first two groups should intersect, where the first
	// group has three overlapping boxes and the second only two.
	DistanceQuery q;
	q.d = 0;
	q(bvh);
	
	EXPECT_THAT(q.intersections, UnorderedElementsAre(
		std::make_pair(0, 1),
		std::make_pair(0, 2),
		std::make_pair(1, 2),
		std::make_pair(3, 4),
		std::make_pair(4, 5)
	));

	q.intersections.clear();

	// Setting the tolerance to 0.6 will include an additional two intersections
	q.d = 0.6;
	q.intersections.clear();
	q(bvh);

	EXPECT_THAT(q.intersections, UnorderedElementsAre(
		std::make_pair(0, 1),
		std::make_pair(0, 2),
		std::make_pair(1, 2),
		std::make_pair(3, 4),
		std::make_pair(4, 5),
		std::make_pair(6, 7),
		std::make_pair(7, 8)
	));

	// And setting it to 1.1 will include another two intersections from the last group,
	// as well as bringing some boxes in the first two groups close enough that they will also
	// match
	q.d = 1.1;
	q.intersections.clear();
	q(bvh);

	EXPECT_THAT(q.intersections, UnorderedElementsAre(
		std::make_pair(0, 1),
		std::make_pair(0, 2),
		std::make_pair(1, 2),
		std::make_pair(3, 4),
		std::make_pair(4, 5),
		std::make_pair(6, 7),
		std::make_pair(7, 8),
		std::make_pair(9, 10),
		std::make_pair(10, 11),
		std::make_pair(0, 3),
		std::make_pair(1, 3),
		std::make_pair(2, 3),
		std::make_pair(3, 5)
	));
}

TEST(Clash, RepoDeformDepthDb)
{
	// Tests the penetration depth estimation (DeformDepth) of the geometry utils
	// on some common problems that we have expected results for.

	ClashDetectionDatabaseHelper helper(getHandler());

	auto c = helper.getContainerByName("hard_1");

	// The Hard pipeline will swap the operands so the larger object is on the
	// right. For the tests though, we must generate the test data such that this
	// is the case.

	struct ContainsFunctor : geometry::RepoDeformDepth::ContainsFunctor, geometry::MeshView {
		ContainsFunctor(geometry::RepoIndexedMesh& mesh)
			:mesh(mesh)
		{
			bvh::builders::build(bvhB, mesh.vertices, mesh.faces);
		}

		const bvh::Bvh<double>& getBvh() const {
			return bvhB;
		}

		repo::lib::RepoTriangle getTriangle(size_t primitive) const {
			return mesh.getTriangle(primitive);
		}

		const geometry::RepoIndexedMesh& mesh;
		bvh::Bvh<double> bvhB;
		
		bool operator()(const std::vector<repo::lib::RepoVector3D64>& points,
			const repo::lib::RepoVector3D64& m) const override {
			return geometry::contains(
				points,
				repo::lib::RepoBounds(points.data(), points.size()),
				*this,
				m
			);
		}
	};

	auto run = [&](
		const std::string& nameA, 
		const std::string& nameB,
		double tolerance) {
			auto a = helper.getChildMeshNodes(c.get(), nameA);
			auto b = helper.getChildMeshNodes(c.get(), nameB);

			auto contains = std::unique_ptr<ContainsFunctor>(nullptr);
			if (geometry::isClosedAndManifold(b.faces)) {
				contains = std::make_unique<ContainsFunctor>(b);
			}

			geometry::RepoDeformDepth pd(a, b, contains.get(), tolerance);
			pd.iterate(10);

			return pd.getPenetrationDepth() > tolerance;
	};

	EXPECT_THAT(run("set15_a", "set15_b", 0), IsTrue());
	EXPECT_THAT(run("set15_a", "set15_b", 2), IsFalse());

	EXPECT_THAT(run("set1_a", "set1_b", 1), IsTrue());
	EXPECT_THAT(run("set1_a", "set1_b", 600), IsFalse());

	EXPECT_THAT(run("set2_a", "set2_b", 1), IsTrue());

	EXPECT_THAT(run("set3_a", "set3_b", 1), IsTrue());
	EXPECT_THAT(run("set3_a", "set3_b", 60), IsTrue());

	EXPECT_THAT(run("set4_a", "set4_b", 1), IsTrue());
	EXPECT_THAT(run("set4_a", "set4_b", 500), IsFalse());

	EXPECT_THAT(run("set5_a1", "set5_b", 1), IsTrue());
	EXPECT_THAT(run("set5_a2", "set5_b", 1), IsTrue());
	EXPECT_THAT(run("set5_a3", "set5_b", 1), IsTrue());
	EXPECT_THAT(run("set5_a4", "set5_b", 1), IsTrue());
	EXPECT_THAT(run("set5_a1", "set5_b", 250), IsFalse());
	EXPECT_THAT(run("set5_a2", "set5_b", 250), IsFalse());
	EXPECT_THAT(run("set5_a3", "set5_b", 300), IsFalse());
	EXPECT_THAT(run("set5_a4", "set5_b", 1100), IsFalse());

	EXPECT_THAT(run("set6_a", "set6_b", 1), IsTrue());
	EXPECT_THAT(run("set6_a", "set6_b", 10), IsFalse());

	// These sets correspond to in-contact configurations that are restrained on
	// one or more sides and so will not be resolved via a local search.

	EXPECT_THAT(run("set7_a", "set7_b", 0), IsTrue());
	EXPECT_THAT(run("set7_a", "set7_b", 1), IsFalse());

	EXPECT_THAT(run("set8_a1", "set8_b", 0), IsTrue());
	EXPECT_THAT(run("set8_a2", "set8_b", 0), IsTrue());
	EXPECT_THAT(run("set8_a3", "set8_b", 0), IsTrue());
	EXPECT_THAT(run("set8_a1", "set8_b", 1), IsFalse());
	EXPECT_THAT(run("set8_a2", "set8_b", 1), IsFalse());
	EXPECT_THAT(run("set8_a3", "set8_b", 1), IsFalse());

	EXPECT_THAT(run("set9_a", "set9_b", 0), IsTrue());
	EXPECT_THAT(run("set9_a", "set9_b", 1), IsFalse());

	// This set is a box contained in another with co-incident faces. None of
	// the faces will actually intersect properly, but due to the contains
	// test, this should be detected as a clash.

	EXPECT_THAT(run("set10_a", "set10_b", 100), IsTrue());

	// These following tests correspond to overlaps. Overlaps are only detected
	// when the tolerance is zero if the ends are open. If the ends are closed,
	// they should be detected with non-trivial tolerances.

	EXPECT_THAT(run("set11_a", "set11_b", 0), IsTrue());
	EXPECT_THAT(run("set12_a", "set12_b", 0), IsTrue());
	EXPECT_THAT(run("set13_a", "set13_b", 0), IsTrue());

	EXPECT_THAT(run("set14_a", "set14_b", 100), IsTrue());
}

TEST(Clash, RepoDeformDepthDegenerateGeometry)
{
	auto buildIndexedMesh = [](const repo::core::model::MeshNode& m) {
		geometry::RepoIndexedMesh mesh;
		std::vector<repo::lib::RepoVector3D64> vertices;
		for (const auto& v : m.getVertices()) {
			vertices.push_back(v);
		}
		mesh.vertices = std::move(vertices);
		mesh.faces = m.getFaces();
		return mesh;
	};

	// RepoDeformDepth should be robust to degenerate triangles (i.e. those that
	// collapse to a line or a point). Ideally such primitives would not exist,
	// but they do occur in real models, so DeformDepth should not only not crash,
	// but should not give the wrong result either.

	// To test this we create a two boxes that are intersecting, and along the
	// diagonal of one box we insert degenerate triangles.

	auto box1 = repo::test::utils::mesh::makeUnitCube();
	auto box2 = repo::test::utils::mesh::makeUnitCube();

	box2.applyTransformation(
		repo::lib::RepoMatrix::scale(repo::lib::RepoVector3D64(4, 4, 4))
	);

	box1.applyTransformation(
		repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(-2, 0, 0))
	);

	geometry::RepoIndexedMesh a = buildIndexedMesh(box1);
	geometry::RepoIndexedMesh b = buildIndexedMesh(box2);

	// Add a degenerate triangle to box2 - box1 will hit this.

	auto vi = b.vertices.size();
	b.vertices.push_back(repo::lib::RepoVector3D64(-2,  2,  2));
	b.vertices.push_back(repo::lib::RepoVector3D64(-2, -2, -2));

	b.faces.push_back(repo::lib::repo_face_t({
		vi,
		vi + 1,
		vi + 1
	}));

	{
		geometry::RepoDeformDepth pd(a, b, nullptr, 0);
		pd.iterate(10);
		EXPECT_THAT(pd.getPenetrationDepth(), Gt(0));
	}

	{
		geometry::RepoDeformDepth pd(a, b, nullptr, 0.55);
		pd.iterate(10);
		EXPECT_THAT(pd.getPenetrationDepth(), Lt(0.55));
	}
}

TEST(Clash, HardE2E)
{
	ClashGenerator clashGenerator;

	auto db = std::make_shared<MockDatabase>();

	// For this test, the distance defines the spacing amongst the non-intersecting
	// pairs.
	clashGenerator.distance = { 0.1, 4 };

	const int clashesPerSample = 20;
	const int selfSamplesA = 5;
	const int selfSamplesB = 5;
	CellDistribution space;

	for (int itr = 0; itr < 500; ++itr)
	{
		ClashDetectionConfigHelper config;
		config.type = ClashDetectionType::Hard;
		config.tolerance = 0.0;
		MockClashScene scene(config.getRevision());

		for (int j = 0; j < clashesPerSample; j++) {
			auto clash = clashGenerator.createHardSoup(space.sample());
			scene.add(clash, config);
		}

		for(int j = 0; j < selfSamplesA; j++) {
			auto clash = clashGenerator.createHardSoup(space.sample());
			auto [a, b] = scene.add(clash);
			config.addCompositeObjects({ a, b }, {});
		}

		for (int j = 0; j < selfSamplesB; j++) {
			auto clash = clashGenerator.createHardSoup(space.sample());
			auto [a, b] = scene.add(clash);
			config.addCompositeObjects({}, { a, b });
		}

		db->setDocuments(scene.bsons);

		{
			clash::Hard pipeline(db, config);
			auto results = pipeline.runPipeline();
			EXPECT_THAT(results.clashes.size(), Eq(clashesPerSample));
		}

		{
			config.selfIntersectsA = true;
			clash::Hard pipeline(db, config);
			auto results = pipeline.runPipeline();
			EXPECT_THAT(results.clashes.size(), Eq(clashesPerSample + selfSamplesA));
		}

		{
			config.selfIntersectsB = true;
			config.selfIntersectsA = false;
			clash::Hard pipeline(db, config);
			auto results = pipeline.runPipeline();
			EXPECT_THAT(results.clashes.size(), Eq(clashesPerSample + selfSamplesB));
		}

		{
			config.selfIntersectsA = true;
			clash::Hard pipeline(db, config);
			auto results = pipeline.runPipeline();
			EXPECT_THAT(results.clashes.size(), Eq(clashesPerSample + selfSamplesA + selfSamplesB));
		}
	}
}

TEST(Clash, HardTolerance) 
{
	// Tolerance in hard mode means to accept clashes that can be resolved by
	// a translation less than the tolerance...

	auto cube = repo::test::utils::mesh::makeUnitCube();
	auto cone = repo::test::utils::mesh::makeUnitCone();

	// This line moves the cone so that it overlaps the box by 0.1 units.

	auto t = repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(0, 0, 0.9));

	// Validate the test data...
	{
		auto a = ClashGenerator::triangles(cube);
		auto b = ClashGenerator::triangles(cone);
		ClashGenerator::applyTransforms(b, t);

		EXPECT_THAT(intersects(a, b), IsTrue());

		geometry::RepoPolyDepth pd(a, b);
		pd.iterate(10);
		auto v0 = pd.getPenetrationVector();

		EXPECT_THAT(v0.norm(), Lt(0.2));
	}

	ClashDetectionConfigHelper config;
	config.type = ClashDetectionType::Hard;
	MockClashScene scene(config.getRevision());

	TransformMeshes problem = { { cube, {} }, { cone, t } };
	scene.add(problem, config);

	auto db = std::make_shared<MockDatabase>();
	db->setDocuments(scene.bsons);

	{
		config.tolerance = 0.0;
		clash::Hard pipeline(db, config);
		auto results = pipeline.runPipeline();
		EXPECT_THAT(results.clashes.size(), Eq(1));
	}

	{
		config.tolerance = 0.2;
		clash::Hard pipeline(db, config);
		auto results = pipeline.runPipeline();
		EXPECT_THAT(results.clashes.size(), Eq(0));
	}
}

TEST(Clash, NodeCache)
{
	static size_t nodeCount = 0;

	struct Node {

		size_t i;

		Node() {
			nodeCount++;
		}

		~Node() {
			nodeCount--;
		}
	};

	// Cache expects to be keyed by objects (specifically, their addresses)

	struct K {
		size_t i;
	};

	std::vector<K> keys;

	for (size_t i = 0; i < 10; i++) {
		keys.push_back(K{i});
	}

	struct Cache : public repo::manipulator::modelutility::clash::ResourceCache<K, Node>
	{
		void initialise(const K& key, Node& node) const override {
			node.i = key.i;
		}
	};

	EXPECT_THROW({
		{
			Cache cache;
			std::vector<Cache::Record*> records;
			for(auto& key : keys) {
				records.push_back(cache.get(key));
			}

			EXPECT_THAT(nodeCount, Eq(keys.size()));
		} // The expected exception will be thrown on exiting this inner scope, when cache is destroyed
	}, std::runtime_error);

	nodeCount = 0;

	{
		// Cache is cleaned up properly, but without using auto collection

		Cache cache;
		std::vector<Cache::Record*> records;
		for (auto& key : keys) {
			records.push_back(cache.get(key));
		}

		EXPECT_THAT(nodeCount, Eq(keys.size()));

		for (auto& key : keys) {
			cache.release(key);
		}

		EXPECT_THAT(nodeCount, Eq(0));
	}

	{
		// Getting a record a second time returns the same pointer

		Cache cache;
		std::vector<Cache::Record*> records;
		for (auto& key : keys) {
			records.push_back(cache.get(key));
		}

		for (size_t itr = 0; itr < 3; itr++) {
			for (size_t i = 0; i < keys.size(); i++) {
				auto record = cache.get(keys[i]);
				EXPECT_THAT(record, Eq(records[i]));
			}
		}

		// Similarly, we only have to delete them once...
		for (auto& key : keys) {
			cache.release(key);
		}
	}

	{
		// Getting a reference from a record will clean that record up when it goes
		// out of scope

		Cache cache;
		std::vector<Cache::Record*> records;
		for (int i = 0; i < 3; i++) {
			records.push_back(cache.get(keys[i]));
		}

		EXPECT_THAT(nodeCount, Eq(3));

		records[0]->getReference();
		EXPECT_THAT(nodeCount, Eq(2));

		records[1]->getReference();
		EXPECT_THAT(nodeCount, Eq(1));

		records[2]->getReference();
		EXPECT_THAT(nodeCount, Eq(0));
	}

	{
		// Holding a reference will keep the record alive

		Cache cache;
		std::vector<Cache::Record*> records;
		for (int i = 0; i < 3; i++) {
			records.push_back(cache.get(keys[i]));
		}

		EXPECT_THAT(nodeCount, Eq(3));

		auto ref0 = records[0]->getReference();
		EXPECT_THAT(nodeCount, Eq(3));

		auto ref1 = records[1]->getReference();
		EXPECT_THAT(nodeCount, Eq(3));

		records[2]->getReference();
		EXPECT_THAT(nodeCount, Eq(2));

		ref0 = nullptr;
		EXPECT_THAT(nodeCount, Eq(1));

		ref1 = nullptr;
		EXPECT_THAT(nodeCount, Eq(0));
	}

	{
		// Cache entries should work with std::swap

		Cache cache;
		std::vector<Cache::Record*> records;

		auto r0 = cache.get(keys[0])->getReference();
		auto r1 = cache.get(keys[1])->getReference();
		auto r2 = cache.get(keys[1])->getReference();

		EXPECT_THAT(r0->i, Eq(keys[0].i));
		EXPECT_THAT(r1->i, Eq(keys[1].i));
		EXPECT_THAT(r2->i, Eq(keys[1].i));

		std::swap(r0, r1);

		EXPECT_THAT(r0->i, Eq(keys[1].i));
		EXPECT_THAT(r1->i, Eq(keys[0].i));
	}
}

TEST(Clash, ResultsSerialisation)
{
	RepoRandomGenerator random;

	ClashDetectionConfig config;
	config.resultsFile = getDataPath("clash/tmp_results_1.json");

	ClashDetectionReport report;
	
	for(int i = 0; i < 3; i++) {
		ClashDetectionResult result;
		result.idA = repo::lib::RepoUUID::createUUID();
		result.idB = repo::lib::RepoUUID::createUUID();
		result.fingerprint = 12345678;
		result.positions = {
			random.vector({100,1000}),
			random.vector({0.1, 0.99})
		};
		report.clashes.push_back(result);
	}

	ClashDetectionEngineUtils::writeJson(report, config);

	repo::lib::Container container;
	container.container = "TestContainer";
	container.revision = repo::lib::RepoUUID::createUUID();
	container.teamspace = "TestTeamspace";

	using namespace repo::manipulator::modelutility::clash;

	report.errors.push_back(
		std::make_shared<MeshBoundsException>(
			container,
			repo::lib::RepoUUID::createUUID()
		)
	);

	report.errors.push_back(
		std::make_shared<TransformBoundsException>(
			container,
			repo::lib::RepoUUID::createUUID()
		)
	);

	std::set<repo::lib::RepoUUID> overlappingSetIds = {
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID()
	};

	report.errors.push_back(
		std::make_shared<DuplicateMeshIdsException>(
			repo::lib::RepoUUID::createUUID()
		)
	);

	report.errors.push_back(
		std::make_shared<DegenerateTestException>(
			repo::lib::RepoUUID::createUUID(),
			repo::lib::RepoUUID::createUUID(),
			"Degenerate Test Reason"
		)
	);

	config.resultsFile = getDataPath("clash/tmp_results_2.json");
	ClashDetectionEngineUtils::writeJson(report, config);

	auto readFile = [](const std::string& path) -> std::string {
		std::ifstream file(path);
		std::stringstream buffer;
		buffer << file.rdbuf();
		return buffer.str();
		};

	{
		rapidjson::Document doc;
		std::string json = readFile(getDataPath("clash/tmp_results_1.json"));
		doc.Parse(json.c_str());

		EXPECT_TRUE(doc.HasMember("clashes"));
		EXPECT_TRUE(doc["clashes"].IsArray());
		EXPECT_EQ(doc["clashes"].Size(), report.clashes.size());

		for (size_t i = 0; i < report.clashes.size(); ++i) {
			const auto& clash = report.clashes[i];
			const auto& jsonClash = doc["clashes"][i];

			EXPECT_EQ(jsonClash["a"].GetString(), clash.idA.toString());
			EXPECT_EQ(jsonClash["b"].GetString(), clash.idB.toString());

			EXPECT_EQ(jsonClash["fingerprint"].GetString(), std::to_string(clash.fingerprint));

			EXPECT_TRUE(jsonClash["positions"].IsArray());
			EXPECT_EQ(jsonClash["positions"].Size(), clash.positions.size());
			for (size_t j = 0; j < clash.positions.size(); ++j) {
				const auto& pos = clash.positions[j];
				const auto& jsonPos = jsonClash["positions"][j];

				EXPECT_TRUE(jsonPos.IsArray());
				EXPECT_THAT(jsonPos[0].GetDouble(), DoubleNear(pos.x, FLT_EPSILON));
				EXPECT_THAT(jsonPos[1].GetDouble(), DoubleNear(pos.y, FLT_EPSILON));
				EXPECT_THAT(jsonPos[2].GetDouble(), DoubleNear(pos.z, FLT_EPSILON));
			}
		}
	}
	{
		rapidjson::Document doc;
		std::string json = readFile(getDataPath("clash/tmp_results_2.json"));
		doc.Parse(json.c_str());

		EXPECT_TRUE(doc.HasMember("errors"));
		EXPECT_TRUE(doc["errors"].IsArray());
		EXPECT_EQ(doc["errors"].Size(), report.errors.size());

		{
			const auto& error = std::dynamic_pointer_cast<MeshBoundsException>(report.errors[0]);
			const auto& jsonError = doc["errors"][0];
			EXPECT_EQ(jsonError["type"].GetString(), std::string("MeshBoundsException"));
			EXPECT_EQ(jsonError["teamspace"].GetString(), error->container.teamspace);
			EXPECT_EQ(jsonError["container"].GetString(), error->container.container);
			EXPECT_EQ(jsonError["revision"].GetString(), error->container.revision.toString());
			EXPECT_EQ(jsonError["uniqueId"].GetString(), error->uniqueId.toString());
		}

		{
			const auto& error = std::dynamic_pointer_cast<TransformBoundsException>(report.errors[1]);
			const auto& jsonError = doc["errors"][1];
			EXPECT_EQ(jsonError["type"].GetString(), std::string("TransformBoundsException"));
			EXPECT_EQ(jsonError["teamspace"].GetString(), error->container.teamspace);
			EXPECT_EQ(jsonError["container"].GetString(), error->container.container);
			EXPECT_EQ(jsonError["revision"].GetString(), error->container.revision.toString());
			EXPECT_EQ(jsonError["uniqueId"].GetString(), error->uniqueId.toString());
		}

		{
			const auto& error = std::dynamic_pointer_cast<DuplicateMeshIdsException>(report.errors[2]);
			const auto& jsonError = doc["errors"][2];
			EXPECT_EQ(jsonError["type"].GetString(), std::string("DuplicateMeshIdsException"));
			EXPECT_EQ(jsonError["uniqueId"].GetString(), error->uniqueId.toString());
		}

		{
			const auto& error = std::dynamic_pointer_cast<DegenerateTestException>(report.errors[3]);
			const auto& jsonError = doc["errors"][3];
			EXPECT_EQ(jsonError["type"].GetString(), std::string("DegenerateTestException"));
			EXPECT_EQ(jsonError["compositeIdA"].GetString(), error->compositeIdA.toString());
			EXPECT_EQ(jsonError["compositeIdB"].GetString(), error->compositeIdB.toString());
			EXPECT_EQ(jsonError["reason"].GetString(), std::string("Degenerate Test Reason"));
		}
	}
}

// Shared functions used by the file type tests
void RunDisjointTest(
	const std::unique_ptr<repo::lib::Container>& container,
	int noSamples,
	int& halfToleranceClashes)
{
	auto handler = getHandler();

	ClashDetectionConfig config;
	ClashDetectionDatabaseHelper helper(handler);

	std::unordered_map<repo::lib::RepoUUID, std::unordered_map<std::string, repo::lib::RepoVariant>, repo::lib::RepoUUIDHasher> metadataMap;
	helper.setCompositeObjectsByMetadataValue(config, container, "ClashSetA", "ClashSetB", metadataMap);

	EXPECT_THAT(config.setA.size(), Eq(noSamples));
	EXPECT_THAT(config.setB.size(), Eq(noSamples));

	// Test Clearance Mode
	// No tolerance
	{
		config.tolerance = 0.0f;
		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();
		EXPECT_THAT(results.clashes.size(), Eq(0));
	}

	// Test Clearance Mode
	// Tolerance so that half of the set should be included
	{
		config.tolerance = 5000.1f;
		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();
		halfToleranceClashes += results.clashes.size();
	}

	// Test Clearance Mode (Tolerance 20,000)
	// Tolerance so high that all instances should be included
	{
		config.tolerance = 20000;
		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(noSamples));
	}

	// Test Hard Mode
	// No tolerance
	{
		config.tolerance = 0.0f;
		auto pipeline = new clash::Hard(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(0));
	}

	// Test Hard Mode
	// High tolerance
	{
		config.tolerance = FLT_MAX;
		auto pipeline = new clash::Hard(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(0));
	}
}

void RunIntersectClosedTest(
	const std::unique_ptr<repo::lib::Container>& container,
	int noSamples,
	std::string metadataDescriptor)
{
	auto handler = getHandler();

	ClashDetectionConfig config;
	ClashDetectionDatabaseHelper helper(handler);

	std::unordered_map<repo::lib::RepoUUID, std::unordered_map<std::string, repo::lib::RepoVariant>, repo::lib::RepoUUIDHasher> metadataMap;
	helper.setCompositeObjectsByMetadataValue(config, container, "ClashSetA", "ClashSetB", metadataMap);

	EXPECT_THAT(config.setA.size(), Eq(noSamples));
	EXPECT_THAT(config.setB.size(), Eq(noSamples));

	// Test Clearance Mode
	// Low tolerance
	{
		config.tolerance = 1.0f;
		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(noSamples));

		// Check all clash results whether the closest points are indeed identical
		for (auto clash : results.clashes)
		{
			float diff = (clash.positions[0] - clash.positions[1]).norm();
			EXPECT_THAT(diff, Eq(0));
		}
	}

	// Test Clearance Mode
	// High tolerance
	{
		config.tolerance = 20000.0f;
		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(noSamples));

		// Check all clash results whether the closest points are indeed identical
		for (auto clash : results.clashes)
		{
			float diff = (clash.positions[0] - clash.positions[1]).norm();
			EXPECT_THAT(diff, Eq(0));
		}
	}

	// Test Hard Mode
	// No tolerance, should flag all instances
	{
		config.tolerance = 0.0f;
		auto pipeline = new clash::Hard(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(noSamples));
	}

	// Test Hard Mode
	// Medium tolerance, should flag at least half of the instances (across all files).
	// It is possible to get false positives, but we should never get false negatives.
	{
		config.tolerance = 2500.0f;
		auto pipeline = new clash::Hard(handler, config);
		auto results = pipeline->runPipeline();

		// Check metadata to make sure no instances are dropped that should be included
		std::vector<repo::lib::RepoUUID> missingCompIds;
		getMissingCompIds(config, results, missingCompIds);

		if (missingCompIds.size() > 0)
		{
			for (auto missingId : missingCompIds)
			{
				auto metaEntry = metadataMap.find(missingId);
				if (metaEntry != metadataMap.end())
				{
					double minPenDepth = boost::get<double>(metaEntry->second[metadataDescriptor]);
					EXPECT_THAT(minPenDepth, Lt(2500.0f));
				}
				else
				{
					FAIL() << "Metadata entry missing for Compound ID";
				}
			}
		}
	}

	// Test Hard Mode
	// High tolerance, all instances should be excluded
	{
		config.tolerance = FLT_MAX;
		auto pipeline = new clash::Hard(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(0));
	}
}

void RunIntersectOpenTest(
	const std::unique_ptr<repo::lib::Container>& container,
	int noSamples,
	std::string metadataDescriptor)
{
	auto handler = getHandler();

	ClashDetectionConfig config;
	ClashDetectionDatabaseHelper helper(handler);

	std::unordered_map<repo::lib::RepoUUID, std::unordered_map<std::string, repo::lib::RepoVariant>, repo::lib::RepoUUIDHasher> metadataMap;
	helper.setCompositeObjectsByMetadataValue(config, container, "ClashSetA", "ClashSetB", metadataMap);

	EXPECT_THAT(config.setA.size(), Eq(noSamples));
	EXPECT_THAT(config.setB.size(), Eq(noSamples));

	// Test Clearance Mode
	// Low tolerance
	{
		config.tolerance = 1.0f;
		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(noSamples));

		// Check all clash results whether the closest points are indeed identical
		for (auto clash : results.clashes)
		{
			float diff = (clash.positions[0] - clash.positions[1]).norm();
			EXPECT_THAT(diff, Eq(0));
		}
	}

	// Test Clearance Mode
	// High tolerance
	{
		config.tolerance = 20000.0f;
		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(noSamples));

		// Check all clash results whether the closest points are indeed identical
		for (auto clash : results.clashes)
		{
			float diff = (clash.positions[0] - clash.positions[1]).norm();
			EXPECT_THAT(diff, Eq(0));
		}
	}

	// Test Hard Mode
	// No tolerance, should flag all instances
	{
		config.tolerance = 0.0f;
		auto pipeline = new clash::Hard(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(noSamples));
	}

	// Test Hard Mode
	// Medium tolerance, should flag at least half of the instances.
	// It is possible to get false positives, but we should never get false negatives.
	{
		config.tolerance = 2500.0f;
		auto pipeline = new clash::Hard(handler, config);
		auto results = pipeline->runPipeline();

		// Check metadata to make sure no instances are dropped that should be included
		std::vector<repo::lib::RepoUUID> missingCompIds;
		getMissingCompIds(config, results, missingCompIds);

		if (missingCompIds.size() > 0)
		{
			for (auto missingId : missingCompIds)
			{
				auto metaEntry = metadataMap.find(missingId);
				if (metaEntry != metadataMap.end())
				{
					double minPenDepth = boost::get<double>(metaEntry->second[metadataDescriptor]);
					EXPECT_THAT(minPenDepth, Lt(2500.0f));
				}
				else
				{
					FAIL() << "Metadata entry missing for Compound ID";
				}
			}
		}
	}

	// Test Hard Mode
	// High tolerance, all instances should be excluded
	{
		config.tolerance = FLT_MAX;
		auto pipeline = new clash::Hard(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(0));
	}
}

void RunCoversTest(
	const std::unique_ptr<repo::lib::Container>& container,
	int noSamples,
	std::string metadataDescriptor)
{
	auto handler = getHandler();

	ClashDetectionConfig config;
	ClashDetectionDatabaseHelper helper(handler);

	std::unordered_map<repo::lib::RepoUUID, std::unordered_map<std::string, repo::lib::RepoVariant>, repo::lib::RepoUUIDHasher> metadataMap;
	helper.setCompositeObjectsByMetadataValue(config, container, "ClashSetA", "ClashSetB", metadataMap);

	EXPECT_THAT(config.setA.size(), Eq(noSamples));
	EXPECT_THAT(config.setB.size(), Eq(noSamples));

	// Test Clearance Mode
	// No tolerance, no clashes
	{
		config.tolerance = 0.1f;
		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(0));
	}

	// Test Clearance mode
	// Medium tolerance, around half of the instances should be found (across all files)
	// with none exceeding the tolerance
	{
		config.tolerance = 2500.0f;
		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		// Check metadata to make sure no instances are dropped that should be included
		std::vector<repo::lib::RepoUUID> missingCompIds;
		getMissingCompIds(config, results, missingCompIds);

		if (missingCompIds.size() > 0)
		{
			for (auto missingId : missingCompIds)
			{
				auto metaEntry = metadataMap.find(missingId);
				if (metaEntry != metadataMap.end())
				{
					double separationDistance = boost::get<double>(metaEntry->second[metadataDescriptor]);
					EXPECT_THAT(separationDistance, Ge(2500.0f));
				}
				else
				{
					FAIL() << "Metadata entry missing for Compound ID";
				}
			}
		}
	}

	// Test Clearance mode
	// High tolerance, all of the instances should be found
	{
		config.tolerance = 5000.0f;
		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(noSamples));

		for (auto clash : results.clashes) {
			auto metaA = metadataMap.find(clash.idA);
			auto metaB = metadataMap.find(clash.idB);

			EXPECT_THAT(metaA, Ne(metadataMap.end()));
			EXPECT_THAT(metaB, Ne(metadataMap.end()));

			float separationDistanceA = boost::get<double>(metaA->second[metadataDescriptor]);
			float separationDistanceB = boost::get<double>(metaB->second[metadataDescriptor]);

			EXPECT_THAT(separationDistanceA, Eq(separationDistanceB));

			auto p1 = clash.positions[0];
			auto p2 = clash.positions[1];
			float clashDistance = (p2 - p1).norm();

			EXPECT_THAT(separationDistanceA, FloatNear(clashDistance, 0.001f));
		}
	}

	// Test Hard Mode
	// Low tolerance
	{
		config.tolerance = 0.0f;
		auto pipeline = new clash::Hard(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(0));
	}

	// Test Hard Mode
	// High tolerance
	{
		config.tolerance = FLT_MAX;
		auto pipeline = new clash::Hard(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(0));
	}
}

void RunContainsTest(
	const std::unique_ptr<repo::lib::Container>& container,
	int noSamples,
	std::string metadataDescriptor)
{
	auto handler = getHandler();

	ClashDetectionConfig config;
	ClashDetectionDatabaseHelper helper(handler);

	std::unordered_map<repo::lib::RepoUUID, std::unordered_map<std::string, repo::lib::RepoVariant>, repo::lib::RepoUUIDHasher> metadataMap;
	helper.setCompositeObjectsByMetadataValue(config, container, "ClashSetA", "ClashSetB", metadataMap);

	EXPECT_THAT(config.setA.size(), Eq(noSamples));
	EXPECT_THAT(config.setB.size(), Eq(noSamples));

	// Test Clearance Mode
	// Low Tolerance
	{
		config.tolerance = 1.f;
		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(noSamples));

		// Check all clash results whether the closest points are indeed identical
		for (auto clash : results.clashes)
		{
			float diff = (clash.positions[0] - clash.positions[1]).norm();
			EXPECT_THAT(diff, Eq(0));
		}
	}

	// Test Clearance Mode
	// High Tolerance
	{
		config.tolerance = 20000.f;
		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(noSamples));

		// Check all clash results whether the closest points are indeed identical
		for (auto clash : results.clashes)
		{
			float diff = (clash.positions[0] - clash.positions[1]).norm();
			EXPECT_THAT(diff, Eq(0));
		}
	}

	// Test Hard Mode
	// No tolerance, all instances should be flagged
	{
		config.tolerance = 0.0f;
		auto pipeline = new clash::Hard(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(noSamples));
	}

	// Test Hard Mode
	// Medium tolerance, should flag at least half of the instances.
	// It is possible to get false positives, but we should never get false negatives.
	{
		config.tolerance = 2500.0f;
		auto pipeline = new clash::Hard(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Ge(noSamples / 2));

		// Check metadata to make sure no instances are dropped that should be included
		std::vector<repo::lib::RepoUUID> missingCompIds;
		getMissingCompIds(config, results, missingCompIds);

		if (missingCompIds.size() > 0)
		{
			for (auto missingId : missingCompIds)
			{
				auto metaEntry = metadataMap.find(missingId);
				if (metaEntry != metadataMap.end())
				{
					double minPenDepth = boost::get<double>(metaEntry->second[metadataDescriptor]);
					EXPECT_THAT(minPenDepth, Lt(2500.0f));
				}
				else
				{
					FAIL() << "Metadata entry missing for Compound ID";
				}
			}
		}
	}

	// Test Hard Mode
	// High tolerance, no instances should be flagged
	{
		config.tolerance = FLT_MAX;
		auto pipeline = new clash::Hard(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(0));
	}
}

void RunMeetTest(
	const std::unique_ptr<repo::lib::Container>& container,
	int noSamples)
{
	auto handler = getHandler();

	ClashDetectionConfig config;
	ClashDetectionDatabaseHelper helper(handler);

	helper.setCompositeObjectsByMetadataValue(config, container, "ClashSetA", "ClashSetB");

	EXPECT_THAT(config.setA.size(), Eq(noSamples));
	EXPECT_THAT(config.setB.size(), Eq(noSamples));

	// Get map from unique mesh ids to bounds from DB
	std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoBounds, repo::lib::RepoUUIDHasher> uidToBoundsMap;
	helper.getBoundsForContainer(container.get(), uidToBoundsMap);

	// Remap to the composite ids
	std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoBounds, repo::lib::RepoUUIDHasher> compidToBoundsMap;
	for (auto compObj : config.setA)
	{
		repo::lib::RepoBounds bounds;
		for (auto& mesh : compObj.meshes)
		{
			auto uid = mesh.uniqueId;
			auto boundsEntry = uidToBoundsMap.find(uid);
			if (boundsEntry != uidToBoundsMap.end())
				bounds.encapsulate(boundsEntry->second);
		}
		compidToBoundsMap.insert({ compObj.id, bounds });
	}
	for (auto compObj : config.setB)
	{
		repo::lib::RepoBounds bounds;
		for (auto& mesh : compObj.meshes)
		{
			auto uid = mesh.uniqueId;
			auto boundsEntry = uidToBoundsMap.find(uid);
			if (boundsEntry != uidToBoundsMap.end())
				bounds.encapsulate(boundsEntry->second);
		}
		compidToBoundsMap.insert({ compObj.id, bounds });
	}

	// Test Clearance Mode
	// Low Tolerance
	{
		config.tolerance = 1.0f;

		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(noSamples));

		// Check all clash results whether the closest points are indeed under
		// the threshold
		for (auto clash : results.clashes)
		{
			auto boundsEntryA = compidToBoundsMap.find(clash.idA);
			auto boundsEntryB = compidToBoundsMap.find(clash.idB);

			if (boundsEntryA != compidToBoundsMap.end() && boundsEntryB != compidToBoundsMap.end())
			{
				float threshold = geometry::contactThreshold(boundsEntryA->second, boundsEntryB->second);

				float diff = (clash.positions[0] - clash.positions[1]).norm();
				EXPECT_THAT(diff, Le(threshold));
			}
			else
			{
				FAIL() << "Could not retrieve bounds for either or both of the composites";
			}
		}
	}

	// Test Clearance Mode
	// High Tolerance
	{
		config.tolerance = 20000.0f;

		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(noSamples));

		// Check all clash results whether the closest points are indeed under
		// the threshold
		for (auto clash : results.clashes)
		{
			auto boundsEntryA = compidToBoundsMap.find(clash.idA);
			auto boundsEntryB = compidToBoundsMap.find(clash.idB);

			if (boundsEntryA != compidToBoundsMap.end() && boundsEntryB != compidToBoundsMap.end())
			{
				float threshold = geometry::contactThreshold(boundsEntryA->second, boundsEntryB->second);

				float diff = (clash.positions[0] - clash.positions[1]).norm();
				EXPECT_THAT(diff, Le(threshold));
			}
			else
			{
				FAIL() << "Could not retrieve bounds for either or both of the composites";
			}
		}
	}

	// Test Hard Mode
	// High Tolerance
	{
		config.tolerance = FLT_MAX;

		auto pipeline = new clash::Hard(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(0));
	}
}

void RunOverlapTest(
	const std::unique_ptr<repo::lib::Container>& container,
	int noSamples,
	std::string metadataDescriptor)
{

	auto handler = getHandler();

	ClashDetectionConfig config;
	ClashDetectionDatabaseHelper helper(handler);

	std::unordered_map<repo::lib::RepoUUID, std::unordered_map<std::string, repo::lib::RepoVariant>, repo::lib::RepoUUIDHasher> metadataMap;
	helper.setCompositeObjectsByMetadataValue(config, container, "ClashSetA", "ClashSetB", metadataMap);

	EXPECT_THAT(config.setA.size(), Eq(noSamples));
	EXPECT_THAT(config.setB.size(), Eq(noSamples));

	// Test Clearance Mode
	// Low Tolerance
	{
		config.tolerance = 1;
		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(noSamples));

		// Check all clash results whether the closest points are indeed identical
		for (auto clash : results.clashes)
		{
			float diff = (clash.positions[0] - clash.positions[1]).norm();
			EXPECT_THAT(diff, Eq(0));
		}
	}

	// Test Clearance Mode
	// High Tolerance
	{
		config.tolerance = 20000;
		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(noSamples));

		// Check all clash results whether the closest points are indeed identical
		for (auto clash : results.clashes)
		{
			float diff = (clash.positions[0] - clash.positions[1]).norm();
			EXPECT_THAT(diff, Eq(0));
		}
	}

	// Test Hard Mode 
	// Low Tolerance, should flag all instances
	{
		config.tolerance = 0;
		auto pipeline = new clash::Hard(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(noSamples));
	}

	// Test Hard Mode
	// Medium Tolerance, should flag at least half of the instances across all files
	// It is possible to get false positives, but we should never get false negatives.
	{
		config.tolerance = 2500.0f;
		auto pipeline = new clash::Hard(handler, config);
		auto results = pipeline->runPipeline();

		// Check metadata to make sure no instances are dropped that should be included
		std::vector<repo::lib::RepoUUID> missingCompIds;
		getMissingCompIds(config, results, missingCompIds);

		if (missingCompIds.size() > 0)
		{
			for (auto missingId : missingCompIds)
			{
				auto metaEntry = metadataMap.find(missingId);
				if (metaEntry != metadataMap.end())
				{
					double minPenDepth = boost::get<double>(metaEntry->second[metadataDescriptor]);
					EXPECT_THAT(minPenDepth, Lt(2500.0f));
				}
				else
				{
					FAIL() << "Metadata entry missing for Compound ID";
				}
			}
		}
	}

	// Test Hard Mode
	// High Tolerance, no instances should be flagged
	{
		config.tolerance = FLT_MAX;
		auto pipeline = new clash::Hard(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(0));
	}
}

void RunEqualTest(
	const std::unique_ptr<repo::lib::Container>& container,
	int noSamples,
	std::string metadataDescriptor)
{
	auto handler = getHandler();

	ClashDetectionConfig config;
	ClashDetectionDatabaseHelper helper(handler);

	std::unordered_map<repo::lib::RepoUUID, std::unordered_map<std::string, repo::lib::RepoVariant>, repo::lib::RepoUUIDHasher> metadataMap;
	helper.setCompositeObjectsByMetadataValue(config, container, "ClashSetA", "ClashSetB", metadataMap);

	EXPECT_THAT(config.setA.size(), Eq(noSamples));
	EXPECT_THAT(config.setB.size(), Eq(noSamples));

	// Test Clearance Mode
	// Low Tolerance
	{
		config.tolerance = 1;
		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(noSamples));

		// Check all clash results whether the closest points are indeed identical
		for (auto clash : results.clashes)
		{
			float diff = (clash.positions[0] - clash.positions[1]).norm();
			EXPECT_THAT(diff, Eq(0));
		}
	}

	// Test Clearance Mode
	// High Tolerance
	{
		config.tolerance = 20000;
		auto pipeline = new clash::Clearance(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(noSamples));

		// Check all clash results whether the closest points are indeed identical
		for (auto clash : results.clashes)
		{
			float diff = (clash.positions[0] - clash.positions[1]).norm();
			EXPECT_THAT(diff, Eq(0));
		}
	}

	// Test Hard Mode 
	// Low Tolerance, should flag all instances
	{
		config.tolerance = 0;
		auto pipeline = new clash::Hard(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(noSamples));
	}

	// Test Hard Mode
	// Medium Tolerance, should flag at least half of the instances
	// It is possible to get false positives, but we should never get false negatives.
	{
		config.tolerance = 2500.0f;
		auto pipeline = new clash::Hard(handler, config);
		auto results = pipeline->runPipeline();

		// Check metadata to make sure no instances are dropped that should be included
		std::vector<repo::lib::RepoUUID> missingCompIds;
		getMissingCompIds(config, results, missingCompIds);

		if (missingCompIds.size() > 0)
		{
			for (auto missingId : missingCompIds)
			{
				auto metaEntry = metadataMap.find(missingId);
				if (metaEntry != metadataMap.end())
				{
					double minPenDepth = boost::get<double>(metaEntry->second[metadataDescriptor]);
					EXPECT_THAT(minPenDepth, Lt(2500.0f));
				}
				else
				{
					FAIL() << "Metadata entry missing for Compound ID";
				}
			}
		}
	}

	// Test Hard Mode
	// High Tolerance, no instances should be flagged
	{
		config.tolerance = FLT_MAX;
		auto pipeline = new clash::Hard(handler, config);
		auto results = pipeline->runPipeline();

		EXPECT_THAT(results.clashes.size(), Eq(0));
	}
}

// Clash tests for auto-generated intersections by file type: RVT
TEST(ClashRvt, RvtDisjoint)
{
	// GTEST_SKIP(); // Skip until I figured out how to make Travis pass this

	// Tests auto-generated samples of Intersection Case A (Disjoint) from a Revit File

	int noParts = 5;
	int totalSamples = 10000;
	int samplesPerSegment = totalSamples / noParts;
	std::string baseName = "revitDisjoint_Part";
	std::string postFix = ".rvt";

	int halfToleranceClashes = 0;
	for (int i = 0; i < noParts; i++) {
		std::string fileName = baseName + std::to_string(i) + postFix;
		std::string path = "/clash/RVT/" + fileName;

		auto container = makeTemporaryContainer();
		importModel(getDataPath(path), *container);

		RunDisjointTest(container, samplesPerSegment, halfToleranceClashes);
	}

	EXPECT_THAT(halfToleranceClashes, Eq(totalSamples / 2));
}

TEST(ClashRvt, RvtIntersectClosed)
{
	// GTEST_SKIP(); // Skip until I figured out how to make Travis pass this

	// Tests 10k auto-generated samples of Intersection Case B (Intersect Closed) from a Revit File

	int noParts = 5;
	int totalSamples = 10000;
	int samplesPerSegment = totalSamples / noParts;
	std::string baseName = "revitIntersectClosed_Part";
	std::string postFix = ".rvt";

	for (int i = 0; i < noParts; i++) {
		std::string fileName = baseName + std::to_string(i) + postFix;
		std::string path = "/clash/RVT/" + fileName;

		auto container = makeTemporaryContainer();
		importModel(getDataPath(path), *container);

		RunIntersectClosedTest(container, samplesPerSegment, "Dimensions::MinPenDepth");
	}
}

TEST(ClashRvt, RvtIntersectOpen)
{
	// GTEST_SKIP(); // Skip until I figured out how to make Travis pass this

	// Tests 10k auto-generated samples of Intersection Case C (Intersect Closed) from a Revit File

	int noParts = 5;
	int totalSamples = 10000;
	int samplesPerSegment = totalSamples / noParts;
	std::string baseName = "revitIntersectOpen_Part";
	std::string postFix = ".rvt";

	for (int i = 0; i < noParts; i++) {
		std::string fileName = baseName + std::to_string(i) + postFix;
		std::string path = "/clash/RVT/" + fileName;

		auto container = makeTemporaryContainer();
		importModel(getDataPath(path), *container);

		RunIntersectOpenTest(container, samplesPerSegment, "Dimensions::MinPenDepth");
	}
}

TEST(ClashRvt, RvtCovers)
{
	// GTEST_SKIP(); // Skip until I figured out how to make Travis pass this

	// Tests 10k auto-generated samples of Intersection Case D (Covers) from a Revit File

	int noParts = 5;
	int totalSamples = 10000;
	int samplesPerSegment = totalSamples / noParts;
	std::string baseName = "revitCovers_Part";
	std::string postFix = ".rvt";

	for (int i = 0; i < noParts; i++) {
		std::string fileName = baseName + std::to_string(i) + postFix;
		std::string path = "/clash/RVT/" + fileName;

		auto container = makeTemporaryContainer();
		importModel(getDataPath(path), *container);

		RunCoversTest(container, samplesPerSegment, "Dimensions::ShortestDist");
	}
}

TEST(ClashRvt, RvtContains)
{
	// GTEST_SKIP(); // Skip until I figured out how to make Travis pass this

	// Tests 10k auto-generated samples of Intersection Case E (Contains) from a Revit File

	int noParts = 5;
	int totalSamples = 10000;
	int samplesPerSegment = totalSamples / noParts;
	std::string baseName = "revitContains_Part";
	std::string postFix = ".rvt";

	for (int i = 0; i < noParts; i++) {
		std::string fileName = baseName + std::to_string(i) + postFix;
		std::string path = "/clash/RVT/" + fileName;

		auto container = makeTemporaryContainer();
		importModel(getDataPath(path), *container);

		RunContainsTest(container, samplesPerSegment, "Dimensions::ShortestDist");
	}
}

TEST(ClashRvt, RvtMeet)
{
	// GTEST_SKIP(); // Skip until I figured out how to make Travis pass this

	// Tests 10k auto-generated samples of Intersection Case F (Meet) from a Revit File

	int noParts = 5;
	int totalSamples = 10000;
	int samplesPerSegment = totalSamples / noParts;
	std::string baseName = "revitMeet_Part";
	std::string postFix = ".rvt";

	for (int i = 0; i < noParts; i++) {
		std::string fileName = baseName + std::to_string(i) + postFix;
		std::string path = "/clash/RVT/" + fileName;

		auto container = makeTemporaryContainer();
		importModel(getDataPath(path), *container);

		RunMeetTest(container, samplesPerSegment);
	}
}

TEST(ClashRvt, RvtOverlap)
{
	// GTEST_SKIP(); // Skip until I figured out how to make Travis pass this

	// Tests 10k auto-generated samples of Intersection Case G (Overlap) from a Revit File

	int noParts = 5;
	int totalSamples = 10000;
	int samplesPerSegment = totalSamples / noParts;
	std::string baseName = "revitOverlap_Part";
	std::string postFix = ".rvt";

	for (int i = 0; i < noParts; i++) {
		std::string fileName = baseName + std::to_string(i) + postFix;
		std::string path = "/clash/RVT/" + fileName;

		auto container = makeTemporaryContainer();
		importModel(getDataPath(path), *container);

		RunOverlapTest(container, samplesPerSegment, "Dimensions::PenDepth");
	}
}

TEST(ClashRvt, RvtEqual)
{
	// Tests 10k auto-generated samples of Intersection Case H (Equal) from a Revit File

	int noParts = 5;
	int totalSamples = 10000;
	int samplesPerSegment = totalSamples / noParts;
	std::string baseName = "revitEqual_Part";
	std::string postFix = ".rvt";

	for (int i = 0; i < noParts; i++) {
		std::string fileName = baseName + std::to_string(i) + postFix;
		std::string path = "/clash/RVT/" + fileName;

		auto container = makeTemporaryContainer();
		importModel(getDataPath(path), *container);

		RunEqualTest(container, samplesPerSegment, "Dimensions::PenDepth");
	}
}

// Clash tests for auto-generated intersections by file type: NWD
TEST(ClashNwd, NwdDisjoint)
{
	GTEST_SKIP(); // Skip until Files are checked in

	// Tests auto-generated samples of Intersection Case A (Disjoint) from a Navis File

	int noParts = 5;
	int totalSamples = 10000;
	int samplesPerSegment = totalSamples / noParts;
	std::string baseName = "navisDisjoint_Part";
	std::string postFix = ".nwd";

	int halfToleranceClashes = 0;
	for (int i = 0; i < noParts; i++) {
		std::string fileName = baseName + std::to_string(i) + postFix;
		std::string path = "/clash/NWD/" + fileName;

		auto container = makeTemporaryContainer();
		importModel(getDataPath(path), *container);

		RunDisjointTest(container, samplesPerSegment, halfToleranceClashes);
	}

	EXPECT_THAT(halfToleranceClashes, Eq(totalSamples / 2));
}

TEST_F(ClashNwd, NwdIntersectClosed)
{
	GTEST_SKIP(); // Skip until Files are checked in

	// Tests 10k auto-generated samples of Intersection Case B (Intersect Closed) from a Navis File

	int noParts = 5;
	int totalSamples = 10000;
	int samplesPerSegment = totalSamples / noParts;
	std::string baseName = "navisIntersectClosed_Part";
	std::string postFix = ".nwd";

	for (int i = 0; i < noParts; i++) {
		std::string fileName = baseName + std::to_string(i) + postFix;
		std::string path = "/clash/NWD/" + fileName;

		auto container = makeTemporaryContainer();
		importModel(getDataPath(path), *container);

		RunIntersectClosedTest(container, samplesPerSegment, "Custom::MinPenDepth");
	}
}

TEST_F(ClashNwd, NwdIntersectOpen)
{
	GTEST_SKIP(); // Skip until the files are checked in

	// Tests 10k auto-generated samples of Intersection Case C (Intersect Closed) from a Navis File

	int noParts = 5;
	int totalSamples = 10000;
	int samplesPerSegment = totalSamples / noParts;
	std::string baseName = "navisIntersectOpen_Part";
	std::string postFix = ".nwd";

	for (int i = 0; i < noParts; i++) {
		std::string fileName = baseName + std::to_string(i) + postFix;
		std::string path = "/clash/NWD/" + fileName;

		auto container = makeTemporaryContainer();
		importModel(getDataPath(path), *container);

		RunIntersectOpenTest(container, samplesPerSegment, "Custom::MinPenDepth");
	}
}

TEST_F(ClashNwd, NwdCovers)
{
	GTEST_SKIP(); // Skip until the files are checked in

	// Tests 10k auto-generated samples of Intersection Case D (Covers) from a Navis File

	int noParts = 5;
	int totalSamples = 10000;
	int samplesPerSegment = totalSamples / noParts;
	std::string baseName = "navisCovers_Part";
	std::string postFix = ".nwd";

	for (int i = 0; i < noParts; i++) {
		std::string fileName = baseName + std::to_string(i) + postFix;
		std::string path = "/clash/NWD/" + fileName;

		auto container = makeTemporaryContainer();
		importModel(getDataPath(path), *container);

		RunCoversTest(container, samplesPerSegment, "Custom::ShortestDist");
	}
}

TEST_F(ClashNwd, NwdContains)
{
	GTEST_SKIP(); // Skip until files are checked in

	// Tests 10k auto-generated samples of Intersection Case E (Contains) from a Navis File

	int noParts = 5;
	int totalSamples = 10000;
	int samplesPerSegment = totalSamples / noParts;
	std::string baseName = "navisContains_Part";
	std::string postFix = ".nwd";

	for (int i = 0; i < noParts; i++) {
		std::string fileName = baseName + std::to_string(i) + postFix;
		std::string path = "/clash/NWD/" + fileName;

		auto container = makeTemporaryContainer();
		importModel(getDataPath(path), *container);

		RunContainsTest(container, samplesPerSegment, "Custom::ShortestDist");
	}
}

TEST_F(ClashNwd, NwdMeet)
{
	GTEST_SKIP(); // Skip until files are checked in

	// Tests 10k auto-generated samples of Intersection Case F (Meet) from a Navis File

	int noParts = 5;
	int totalSamples = 10000;
	int samplesPerSegment = totalSamples / noParts;
	std::string baseName = "navisMeet_Part";
	std::string postFix = ".nwd";

	for (int i = 0; i < noParts; i++) {
		std::string fileName = baseName + std::to_string(i) + postFix;
		std::string path = "/clash/NWD/" + fileName;

		auto container = makeTemporaryContainer();
		importModel(getDataPath(path), *container);

		RunMeetTest(container, samplesPerSegment);
	}
}

TEST_F(ClashNwd, NwdOverlap)
{
	GTEST_SKIP(); // Skip until files are checked in

	// Tests 10k auto-generated samples of Intersection Case G (Overlap) from a Navis File

	int noParts = 5;
	int totalSamples = 10000;
	int samplesPerSegment = totalSamples / noParts;
	std::string baseName = "navisOverlap_Part";
	std::string postFix = ".nwd";

	for (int i = 0; i < noParts; i++) {
		std::string fileName = baseName + std::to_string(i) + postFix;
		std::string path = "/clash/NWD/" + fileName;

		auto container = makeTemporaryContainer();
		importModel(getDataPath(path), *container);

		RunOverlapTest(container, samplesPerSegment, "Custom::PenDepth");
	}
}

TEST_F(ClashNwd, NwdEqual)
{
	GTEST_SKIP(); // Skip until files are checked in

	// Tests 10k auto-generated samples of Intersection Case H (Equal) from a Navis File

	int noParts = 5;
	int totalSamples = 10000;
	int samplesPerSegment = totalSamples / noParts;
	std::string baseName = "navisEqual_Part";
	std::string postFix = ".nwd";

	for (int i = 0; i < noParts; i++) {
		std::string fileName = baseName + std::to_string(i) + postFix;
		std::string path = "/clash/NWD/" + fileName;

		auto container = makeTemporaryContainer();
		importModel(getDataPath(path), *container);

		RunEqualTest(container, samplesPerSegment, "Custom::PenDepth");
	}
}