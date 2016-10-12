/**
*  Copyright (C) 2015 3D Repo Ltd
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

#include <cstdlib>

#include <gtest/gtest.h>

#include <repo/core/model/bson/repo_node_mesh.h>
#include <repo/core/model/bson/repo_node_texture.h>
#include <repo/core/model/bson/repo_node_transformation.h>
#include <repo/core/model/bson/repo_bson_factory.h>
#include <repo/core/model/collection/repo_scene.h>

#include "../../../../repo_test_utils.h"
#include "../../../../repo_test_database_info.h"

using namespace repo::core::model;

const static RepoScene::GraphType defaultG = RepoScene::GraphType::DEFAULT;

static RepoBSON makeRandomNode(
	const std::string &name = "")
{
	return RepoBSONFactory::appendDefaults("", 0U, generateUUID(), name);
}

static RepoBSON makeRandomNode(
	const repoUUID &parent,
	const std::string &name = "")
{
	return RepoBSONFactory::appendDefaults("", 0U, generateUUID(), name, { parent });
}

TEST(RepoSceneTest, Constructor)
{
	//Just to make sure it doesn't throw exceptiosn with empty
	RepoScene scene;
	RepoNodeSet empty;
	std::vector<std::string> files;
	RepoScene scene2(files, empty, empty, empty, empty, empty, empty);
}

TEST(RepoSceneTest, FilterNodesByType)
{
	std::vector<RepoNode*> nodes;
	TextureNode texNode;
	MeshNode meshNode;
	TransformationNode transNode;
	//Check empty vector doesn't crash
	EXPECT_EQ(0, RepoScene::filterNodesByType(nodes, NodeType::MAP).size());

	int nMeshes = rand() % 10 + 1;
	int nTrans = rand() % 10 + 1;
	int nTexts = rand() % 10 + 1;
	for (int i = 0; i < nMeshes; ++i)
		nodes.push_back(&meshNode);

	for (int i = 0; i < nTrans; ++i)
		nodes.push_back(&transNode);

	for (int i = 0; i < nTexts; ++i)
		nodes.push_back(&texNode);

	auto meshNodes = RepoScene::filterNodesByType(nodes, NodeType::MESH);
	EXPECT_EQ(nMeshes, meshNodes.size());
	for (const auto &m : meshNodes)
		EXPECT_EQ(m, &meshNode);

	auto transNodes = RepoScene::filterNodesByType(nodes, NodeType::TRANSFORMATION);
	EXPECT_EQ(nTrans, transNodes.size());
	for (const auto &m : transNodes)
		EXPECT_EQ(m, &transNode);

	auto textNodes = RepoScene::filterNodesByType(nodes, NodeType::TEXTURE);
	EXPECT_EQ(nTexts, textNodes.size());
	for (const auto &m : textNodes)
		EXPECT_EQ(m, &texNode);
}

TEST(RepoSceneTest, StatusCheck)
{
	RepoScene emptyScene;
	EXPECT_TRUE(emptyScene.isOK());
	EXPECT_FALSE(emptyScene.isMissingTexture());

	RepoNodeSet empty;
	std::vector<std::string> files;
	RepoScene scene2(files, empty, empty, empty, empty, empty, empty);

	EXPECT_TRUE(scene2.isOK());
	EXPECT_FALSE(scene2.isMissingTexture());

	emptyScene.setMissingTexture();
	EXPECT_FALSE(emptyScene.isOK());
	EXPECT_TRUE(emptyScene.isMissingTexture());
}

TEST(RepoSceneTest, AddMetadata)
{
	RepoNodeSet transNodes;
	RepoNodeSet meshNodes;
	RepoNodeSet metaNodes;
	RepoNodeSet empty;

	auto root = TransformationNode(makeRandomNode(getRandomString(rand() % 100 + 1)));

	auto t1 = TransformationNode(makeRandomNode(root.getSharedID(), getRandomString(rand() % 100 + 1)));
	auto t2 = TransformationNode(makeRandomNode(root.getSharedID(), getRandomString(rand() % 100 + 1)));
	auto t3 = TransformationNode(makeRandomNode(root.getSharedID(), getRandomString(rand() % 100 + 1)));

	auto m1 = MeshNode(makeRandomNode(t1.getSharedID()));
	auto m2 = MeshNode(makeRandomNode(t1.getSharedID()));
	auto m3 = MeshNode(makeRandomNode(t2.getSharedID()));

	auto mm1 = MetadataNode(makeRandomNode(t1.getName()));
	auto mm2 = MetadataNode(makeRandomNode(t2.getName()));
	auto mm3 = MetadataNode(makeRandomNode(t3.getName()));

	repoTrace << "t1 name: " << t1.getName();
	repoTrace << "t2 name: " << t2.getName();
	repoTrace << "t3 name: " << t3.getName();

	transNodes.insert(new TransformationNode(t1));
	transNodes.insert(new TransformationNode(t2));
	transNodes.insert(new TransformationNode(t3));

	meshNodes.insert(new MeshNode(m1));
	meshNodes.insert(new MeshNode(m2));
	meshNodes.insert(new MeshNode(m3));

	metaNodes.insert(new MetadataNode(mm1));
	metaNodes.insert(new MetadataNode(mm2));
	metaNodes.insert(new MetadataNode(mm3));

	RepoScene scene,
		scene2(std::vector<std::string>(), empty, meshNodes, empty, empty, empty, transNodes);
	scene.addMetadata(metaNodes, true);
	EXPECT_EQ(0, scene.getAllMetadata(defaultG).size());
	scene2.addMetadata(metaNodes, true, false); //no propagation check
	auto scene2Meta = scene2.getAllMetadata(defaultG);
	EXPECT_EQ(metaNodes.size(), scene2Meta.size());
	for (const auto &s2meta : scene2Meta)
	{
		auto parents = s2meta->getParentIDs();
		EXPECT_EQ(1, parents.size());
		for (const auto &parent : parents)
		{
			repoTrace << "parent: " << UUIDtoString(parent);
			auto node = scene2.getNodeBySharedID(RepoScene::GraphType::DEFAULT, parent);
			EXPECT_EQ(NodeType::TRANSFORMATION, node->getTypeAsEnum());
			EXPECT_EQ(s2meta->getName(), node->getName());
		}
	}

	transNodes.clear();
	transNodes.insert(new TransformationNode(t1));
	transNodes.insert(new TransformationNode(t2));
	transNodes.insert(new TransformationNode(t3));

	meshNodes.clear();
	meshNodes.insert(new MeshNode(m1));
	meshNodes.insert(new MeshNode(m2));
	meshNodes.insert(new MeshNode(m3));

	metaNodes.clear();
	metaNodes.insert(new MetadataNode(mm1));
	metaNodes.insert(new MetadataNode(mm2));
	metaNodes.insert(new MetadataNode(mm3));

	RepoScene scene3(std::vector<std::string>(), empty, meshNodes, empty, empty, empty, transNodes);

	scene3.addMetadata(metaNodes, true, true); //propagation check

	auto scene3Meta = scene3.getAllMetadata(defaultG);
	EXPECT_EQ(metaNodes.size(), scene2Meta.size());
	for (const auto &s3meta : scene3Meta)
	{
		auto parents = s3meta->getParentIDs();
		EXPECT_TRUE(parents.size() > 0);
		for (const auto &parent : parents)
		{
			auto node = scene3.getNodeBySharedID(RepoScene::GraphType::DEFAULT, parent);

			if (NodeType::TRANSFORMATION == node->getTypeAsEnum())
				EXPECT_EQ(s3meta->getName(), node->getName());
			else
			{
				EXPECT_EQ(NodeType::MESH, node->getTypeAsEnum());
			}
		}
	}
}

TEST(RepoSceneTest, AddAndClearStashGraph)
{
	RepoScene scene;
	RepoNodeSet empty;
	auto stashGraph = RepoScene::GraphType::OPTIMIZED;

	EXPECT_FALSE(scene.hasRoot(stashGraph));
	scene.addStashGraph(empty, empty, empty, empty, empty);
	EXPECT_FALSE(scene.hasRoot(stashGraph));

	RepoNodeSet transNodes;
	RepoNodeSet meshNodes;
	RepoNodeSet metaNodes;

	auto root = TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1)));

	auto t1 = TransformationNode(makeRandomNode(root.getSharedID(), getRandomString(rand() % 10 + 1)));
	auto t2 = TransformationNode(makeRandomNode(root.getSharedID(), getRandomString(rand() % 10 + 1)));
	auto t3 = TransformationNode(makeRandomNode(root.getSharedID(), getRandomString(rand() % 10 + 1)));

	auto m1 = MeshNode(makeRandomNode(t1.getSharedID()));
	auto m2 = MeshNode(makeRandomNode(t1.getSharedID()));
	auto m3 = MeshNode(makeRandomNode(t2.getSharedID()));

	auto rootNode = new TransformationNode(root);
	transNodes.insert(rootNode);
	transNodes.insert(new TransformationNode(t1));
	transNodes.insert(new TransformationNode(t2));
	transNodes.insert(new TransformationNode(t3));

	meshNodes.insert(new MeshNode(m1));
	meshNodes.insert(new MeshNode(m2));
	meshNodes.insert(new MeshNode(m3));

	scene.addStashGraph(empty, meshNodes, empty, empty, transNodes);
	EXPECT_EQ(0, scene.getAllCameras(stashGraph).size());
	EXPECT_EQ(0, scene.getAllTextures(stashGraph).size());
	EXPECT_EQ(0, scene.getAllMaterials(stashGraph).size());
	EXPECT_EQ(meshNodes.size(), scene.getAllMeshes(stashGraph).size());
	EXPECT_EQ(transNodes.size(), scene.getAllTransformations(stashGraph).size());
	EXPECT_TRUE(scene.hasRoot(stashGraph));
	EXPECT_FALSE(scene.hasRoot(RepoScene::GraphType::DEFAULT));
	EXPECT_EQ(rootNode, scene.getRoot(stashGraph));

	scene.clearStash();

	EXPECT_EQ(0, scene.getAllCameras(stashGraph).size());
	EXPECT_EQ(0, scene.getAllTextures(stashGraph).size());
	EXPECT_EQ(0, scene.getAllMaterials(stashGraph).size());
	EXPECT_EQ(0, scene.getAllMeshes(stashGraph).size());
	EXPECT_EQ(0, scene.getAllTransformations(stashGraph).size());
	EXPECT_FALSE(scene.hasRoot(stashGraph));
	EXPECT_FALSE(scene.hasRoot(RepoScene::GraphType::DEFAULT));
	EXPECT_EQ(nullptr, scene.getRoot(stashGraph));
}

TEST(RepoSceneTest, CommitScene)
{
	RepoScene scene;
	std::string errMsg;
	std::string commitUser = "me";

	//Commiting an empty scene should fail (fails on empty project/database name)
	EXPECT_FALSE(scene.commit(getHandler(), errMsg, commitUser));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();

	scene.setDatabaseAndProjectName("sceneCommit", "test1");
	EXPECT_FALSE(scene.commit(getHandler(), errMsg, commitUser));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();

	RepoNodeSet transNodes;
	RepoNodeSet meshNodes, empty;

	auto root = new TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1)));

	auto t1 = new TransformationNode(makeRandomNode(root->getSharedID(), getRandomString(rand() % 10 + 1)));

	auto m1 = new MeshNode(makeRandomNode(t1->getSharedID()));
	auto m2 = new MeshNode(makeRandomNode(t1->getSharedID()));

	transNodes.insert(root);
	transNodes.insert(t1);

	meshNodes.insert(m1);
	meshNodes.insert(m2);

	RepoScene scene2(std::vector<std::string>(), empty, meshNodes, empty, empty, empty, transNodes);
	scene2.setDatabaseAndProjectName("sceneCommit", "test2");
	EXPECT_FALSE(scene2.commit(nullptr, errMsg, commitUser));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();

	std::string commitMsg = "this is a commit message for this commit.";
	std::string commitTag = "test";

	EXPECT_TRUE(scene2.commit(getHandler(), errMsg, commitUser, commitMsg, commitTag));
	EXPECT_TRUE(errMsg.empty());

	EXPECT_TRUE(scene2.isRevisioned());
	EXPECT_EQ(scene2.getOwner(), commitUser);
	EXPECT_EQ(scene2.getTag(), commitTag);
	EXPECT_EQ(scene2.getMessage(), commitMsg);
}

TEST(RepoSceneTest, CommitStash)
{
	RepoScene scene;
	std::string errMsg;
	EXPECT_FALSE(scene.commitStash(getHandler(), errMsg));
	errMsg.clear();

	RepoNodeSet transNodes, transNodes2;
	RepoNodeSet meshNodes, meshNodes2, empty;

	auto root = new TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1)));

	auto t1 = new TransformationNode(makeRandomNode(root->getSharedID(), getRandomString(rand() % 10 + 1)));

	auto m1 = new MeshNode(makeRandomNode(t1->getSharedID()));
	auto m2 = new MeshNode(makeRandomNode(t1->getSharedID()));

	transNodes.insert(root);
	transNodes.insert(t1);

	meshNodes.insert(m1);
	meshNodes.insert(m2);

	transNodes2.insert(new TransformationNode(*root));
	transNodes2.insert(new TransformationNode(*t1));

	meshNodes2.insert(new MeshNode(*m1));
	meshNodes2.insert(new MeshNode(*m2));

	RepoScene scene2(std::vector<std::string>(), empty, meshNodes, empty, empty, empty, transNodes);
	scene2.setDatabaseAndProjectName("stashCommit", "test");
	ASSERT_TRUE(scene2.commit(getHandler(), errMsg, "blah"));
	errMsg.clear();
	//Empty stash shouldn't have commit but should return true(apparently)
	EXPECT_TRUE(scene2.commitStash(getHandler(), errMsg));
	errMsg.clear();
	scene2.addStashGraph(empty, meshNodes2, empty, empty, transNodes2);
	EXPECT_FALSE(scene2.commitStash(nullptr, errMsg));
	errMsg.clear();
	EXPECT_TRUE(scene2.commitStash(getHandler(), errMsg));
}

TEST(RepoSceneTest, GetSetDatabaseProjectName)
{
	RepoScene scene;
	EXPECT_TRUE(scene.getDatabaseName().empty());
	EXPECT_TRUE(scene.getProjectName().empty());
	std::string dbName = "dbName1245";
	std::string projName = "dbprobabba";
	scene.setDatabaseAndProjectName(dbName, projName);
	EXPECT_EQ(dbName, scene.getDatabaseName());
	EXPECT_EQ(projName, scene.getProjectName());

	RepoScene scene2(dbName, projName);
	EXPECT_EQ(dbName, scene2.getDatabaseName());
	EXPECT_EQ(projName, scene2.getProjectName());

	std::string badDbName = "system.h$<>:|/\.a?";
	std::string badProjName = "p r o j e c t$...";
	scene2.setDatabaseAndProjectName(badDbName, badProjName);
	EXPECT_EQ("system_h_______a_", scene2.getDatabaseName());
	EXPECT_EQ("p_r_o_j_e_c_t____", scene2.getProjectName());

	RepoScene scene3(badDbName, badProjName);
	EXPECT_EQ("system_h_______a_", scene3.getDatabaseName());
	EXPECT_EQ("p_r_o_j_e_c_t____", scene3.getProjectName());
}

TEST(RepoSceneTest, getExtensions)
{
	RepoScene scene;
	EXPECT_EQ(scene.getStashExtension(), REPO_COLLECTION_STASH_REPO);
	EXPECT_EQ(scene.getRawExtension(), REPO_COLLECTION_RAW);
	EXPECT_EQ(scene.getSRCExtension(), REPO_COLLECTION_STASH_SRC);
	EXPECT_EQ(scene.getGLTFExtension(), REPO_COLLECTION_STASH_GLTF);
	EXPECT_EQ(scene.getJSONExtension(), REPO_COLLECTION_STASH_JSON);

	RepoScene scene2("db", "proj", "scene", "rev", "stash", "raw", "iss", "src", "gltf", "json");

	EXPECT_EQ(scene2.getStashExtension(), "stash");
	EXPECT_EQ(scene2.getRawExtension(), "raw");
	EXPECT_EQ(scene2.getSRCExtension(), "src");
	EXPECT_EQ(scene2.getGLTFExtension(), "gltf");
	EXPECT_EQ(scene2.getJSONExtension(), "json");
}

TEST(RepoSceneTest, getSetRevisionID)
{
	RepoScene scene;
	EXPECT_TRUE(scene.isHeadRevision());
	repoUUID id = generateUUID();
	scene.setRevision(id);
	EXPECT_EQ(id, scene.getRevisionID());
	EXPECT_FALSE(scene.isHeadRevision());
}

TEST(RepoSceneTest, getRevisionProperties)
{
	RepoScene scene;

	EXPECT_TRUE(scene.getOwner().empty());
	EXPECT_TRUE(scene.getTag().empty());
	EXPECT_TRUE(scene.getMessage().empty());
	EXPECT_TRUE(compareStdVectors(scene.getWorldOffset(), std::vector<double>({ 0, 0, 0 })));

	//commit a scene and try again
	std::string errMsg;
	std::string commitUser = "me";
	std::string commitMessage = "message for my commit";
	std::string commitTag = "tagggg";

	RepoNodeSet transNodes;
	RepoNodeSet meshNodes, empty;

	auto root = new TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1)));

	auto t1 = new TransformationNode(makeRandomNode(root->getSharedID(), getRandomString(rand() % 10 + 1)));

	auto m1 = new MeshNode(makeRandomNode(t1->getSharedID()));
	auto m2 = new MeshNode(makeRandomNode(t1->getSharedID()));

	transNodes.insert(root);
	transNodes.insert(t1);

	meshNodes.insert(m1);
	meshNodes.insert(m2);
	std::vector<double> offset({ rand() / 1000., rand() / 1000., rand() / 1000. });

	RepoScene scene2(std::vector<std::string>(), empty, meshNodes, empty, empty, empty, transNodes);
	scene2.setDatabaseAndProjectName("sceneCommit", "test2");
	ASSERT_TRUE(scene2.commit(getHandler(), errMsg, commitUser, commitMessage, commitTag));

	scene2.setWorldOffset(offset);
	EXPECT_EQ(scene2.getOwner(), commitUser);
	EXPECT_EQ(scene2.getTag(), commitTag);
	EXPECT_EQ(scene2.getMessage(), commitMessage);
	EXPECT_TRUE(compareStdVectors(scene2.getWorldOffset(), offset));
}

TEST(RepoSceneTest, getSetBranchID)
{
	RepoScene scene;
	EXPECT_EQ(UUIDtoString(scene.getBranchID()), REPO_HISTORY_MASTER_BRANCH);

	repoUUID newBranch = generateUUID();
	scene.setBranch(newBranch);

	EXPECT_EQ(scene.getBranchID(), newBranch);
}

TEST(RepoSceneTest, setCommitMessage)
{
	RepoScene scene;
	EXPECT_TRUE(scene.getMessage().empty());

	auto message = getRandomString(rand() % 10 + 1);
	scene.setCommitMessage(message);
	EXPECT_EQ(scene.getMessage(), message);
}

TEST(RepoSceneTest, getBranchName)
{
	RepoScene scene;
	EXPECT_EQ(scene.getBranchName(), "master");
}

TEST(RepoSceneTest, getViewGraph)
{
	RepoScene scene;

	EXPECT_EQ(scene.getViewGraph(), RepoScene::GraphType::DEFAULT);

	RepoNodeSet transNodes, transNodes2;
	RepoNodeSet meshNodes, meshNodes2, empty;

	auto root = new TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1)));

	auto t1 = new TransformationNode(makeRandomNode(root->getSharedID(), getRandomString(rand() % 10 + 1)));

	auto m1 = new MeshNode(makeRandomNode(t1->getSharedID()));
	auto m2 = new MeshNode(makeRandomNode(t1->getSharedID()));

	transNodes.insert(root);
	transNodes.insert(t1);

	meshNodes.insert(m1);
	meshNodes.insert(m2);

	transNodes2.insert(new TransformationNode(*root));
	transNodes2.insert(new TransformationNode(*t1));

	meshNodes2.insert(new MeshNode(*m1));
	meshNodes2.insert(new MeshNode(*m2));

	RepoScene scene2(std::vector<std::string>(), empty, meshNodes, empty, empty, empty, transNodes);
	scene2.addStashGraph(empty, meshNodes2, empty, empty, transNodes2);

	EXPECT_EQ(scene2.getViewGraph(), RepoScene::GraphType::OPTIMIZED);
}

TEST(RepoSceneTest, loadRevision)
{
	std::string errMsg;
	EXPECT_FALSE(RepoScene().loadRevision(getHandler(), errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
	EXPECT_TRUE(RepoScene(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ).loadRevision(getHandler(), errMsg));
	EXPECT_TRUE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(RepoScene("nonexistantDatabase", "NonExistantProject").loadRevision(getHandler(), errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(RepoScene(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ).loadRevision(nullptr, errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
}

TEST(RepoSceneTest, loadScene)
{
	std::string errMsg;
	EXPECT_FALSE(RepoScene().loadScene(getHandler(), errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
	EXPECT_TRUE(RepoScene(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ).loadScene(getHandler(), errMsg));
	EXPECT_TRUE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(RepoScene("nonexistantDatabase", "NonExistantProject").loadScene(getHandler(), errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(RepoScene(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ).loadScene(nullptr, errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
}

TEST(RepoSceneTest, loadStash)
{
	std::string errMsg;
	EXPECT_FALSE(RepoScene().loadStash(getHandler(), errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
	EXPECT_TRUE(RepoScene(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ).loadStash(getHandler(), errMsg));
	EXPECT_TRUE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(RepoScene("nonexistantDatabase", "NonExistantProject").loadStash(getHandler(), errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(RepoScene(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ).loadStash(nullptr, errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
}