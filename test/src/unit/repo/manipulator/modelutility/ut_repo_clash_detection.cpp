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

// The ODA NWD importer also uses rapidjson, so make sure to import our
// copy into a different namespace to avoid interference between two
// possibly different versions.

#define RAPIDJSON_NAMESPACE repo::rapidjson
#define RAPIDJSON_NAMESPACE_BEGIN namespace repo { namespace rapidjson {
#define RAPIDJSON_NAMESPACE_END } }

#include <repo/manipulator/modelutility/rapidjson/rapidjson.h>
#include <repo/manipulator/modelutility/rapidjson/document.h>

#include <repo/manipulator/modelutility/repo_clash_detection_engine.h>
#include <repo/manipulator/modelutility/clashdetection/clash_pipelines.h>
#include <repo/manipulator/modelutility/clashdetection/clash_clearance.h>
#include <repo/manipulator/modelutility/clashdetection/clash_hard.h>
#include <repo/manipulator/modelutility/clashdetection/sparse_scene_graph.h>

#include "../../../repo_test_utils.h"
#include "../../../repo_test_mesh_utils.h"
#include "../../../repo_test_database_info.h"
#include "../../../repo_test_fileservice_info.h"
#include "../../../repo_test_scene_utils.h"
#include "../../../repo_test_matchers.h"

using namespace testing;
using namespace repo;
using namespace repo::core::model;
using namespace repo::core::handler::database;
using namespace repo::manipulator::modelconvertor;
using namespace repo::manipulator::modelutility;

// The ClashDetection unit tests all use the ClashDetection database. This is
// a dump from a fully functional teamspce. Once restored, the team_member role
// can be added to a user allowing the database to be inspected or maintained
// using the frontend.
// If uploading a new file, remember to copy the links into the tests fileShare
// folder as well.

#define TESTDB "ClashDetection"

#pragma optimize ("", off)

using DatabasePtr = std::shared_ptr<repo::core::handler::MongoDatabaseHandler>;

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

struct ClashDetectionTestHelper
{
	ClashDetectionTestHelper(DatabasePtr handler)
		:handler(handler)
	{
	}

	DatabasePtr handler;

	// Searches for mesh nodes only
	repo::lib::RepoUUID getUniqueIdByName(
		lib::Container* container,
		std::string name)
	{
		repo::core::handler::database::query::RepoQueryBuilder query;
		query.append(repo::core::handler::database::query::Eq(REPO_NODE_LABEL_TYPE, std::string(REPO_NODE_TYPE_MESH)));
		query.append(repo::core::handler::database::query::Eq(REPO_NODE_LABEL_NAME, name));

		// We only ever expect one result, but use findAllByCriteria because it allows
		// skipping the binaries & other fields we are not interested in here.

		repo::core::handler::database::query::RepoProjectionBuilder projection;
		projection.includeField(REPO_NODE_LABEL_ID);

		auto bson = handler->findAllByCriteria(
			container->teamspace,
			container->container + "." + REPO_COLLECTION_SCENE,
			query,
			projection,
			true
		);

		MeshNode m(bson[0]);

		return bson[0].getUUIDField(REPO_NODE_LABEL_ID);
	}

	CompositeObject createCompositeObject(
		repo::lib::Container* container,
		std::string name
	)
	{
		CompositeObject composite;
		composite.id = repo::lib::RepoUUID::createUUID();
		composite.meshes.push_back(MeshReference(container, getUniqueIdByName(container, name)));
		return composite;
	}

	// Will replace anything in the existing sets
	void setCompositeObjectSetsByName(
		ClashDetectionConfig& config,
		std::initializer_list<std::string> a,
		std::initializer_list<std::string> b)
	{
		config.setA.clear();
		for (const auto& name : a) {
			config.setA.push_back(createCompositeObject(config.containers[0].get(), name));
		}
		config.setB.clear();
		for (const auto& name : b) {
			config.setB.push_back(createCompositeObject(config.containers[0].get(), name));
		}
	}

	std::unique_ptr<repo::lib::Container> getContainerByName(
		std::string name)
	{
		auto container = std::make_unique<repo::lib::Container>();
		container->teamspace = TESTDB;

		auto settings = handler->findOneByCriteria(
			TESTDB,
			"settings",
			query::Eq(REPO_NODE_LABEL_NAME, name)
		);
		container->container = settings.getStringField(REPO_NODE_LABEL_ID);

		auto revisions = handler->getAllFromCollectionTailable(
			TESTDB,
			container->container + "." + REPO_COLLECTION_HISTORY
		);
		container->revision = revisions[0].getUUIDField(REPO_NODE_LABEL_ID);

		return container;
	}
};

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

TEST(Clash, Clearance1)
{
	auto handler = getHandler();
	ClashDetectionConfig config;

	ClashDetectionTestHelper helper(handler);

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
	//todo: make sure clearance tests are deterministic
// append in a different order explicitiy to test this...
}