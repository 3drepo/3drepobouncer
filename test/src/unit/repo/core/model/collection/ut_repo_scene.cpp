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

	auto t1 = TransformationNode(makeRandomNode(root.getSharedID(), getRandomString(rand() % 100 + 1) + root.getName()));
	auto t2 = TransformationNode(makeRandomNode(root.getSharedID(), getRandomString(rand() % 100 + 1) + t1.getName()));
	auto t3 = TransformationNode(makeRandomNode(root.getSharedID(), getRandomString(rand() % 100 + 1) + t2.getName()));

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

	std::string badDbName = "system.h$<>:|/\\.a?";
	std::string badProjName = "p r o j e c t$...";
	scene2.setDatabaseAndProjectName(badDbName, badProjName);
	EXPECT_EQ("system_h________a_", scene2.getDatabaseName());
	EXPECT_EQ("p_r_o_j_e_c_t____", scene2.getProjectName());

	RepoScene scene3(badDbName, badProjName);
	EXPECT_EQ("system_h________a_", scene3.getDatabaseName());
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

TEST(RepoSceneTest, updateRevisionStatus)
{
	RepoScene scene;

	EXPECT_FALSE(scene.updateRevisionStatus(getHandler(), RevisionNode::UploadStatus::GEN_DEFAULT));
	EXPECT_FALSE(scene.updateRevisionStatus(nullptr, RevisionNode::UploadStatus::GEN_DEFAULT));

	scene = RepoScene(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ);
	std::string errMsg;
	scene.loadRevision(getHandler(), errMsg);
	EXPECT_TRUE(scene.updateRevisionStatus(getHandler(), RevisionNode::UploadStatus::GEN_DEFAULT));
	EXPECT_TRUE(scene.updateRevisionStatus(getHandler(), RevisionNode::UploadStatus::COMPLETE));
}

TEST(RepoSceneTest, printStats)
{
	RepoScene scene;
	//Not much to test, just make sure it doesn't crash.
	std::stringstream ss;
	scene.printStatistics(ss);

	scene = RepoScene(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ);
	scene.printStatistics(ss);
}

TEST(RepoSceneTest, abandonChild)
{
	RepoNodeSet transNodes, meshNodes, empty;

	auto root = new TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1)));

	auto m1 = new MeshNode(makeRandomNode(root->getSharedID()));

	transNodes.insert(root);

	meshNodes.insert(m1);

	RepoScene scene(std::vector<std::string>(), empty, meshNodes, empty, empty, empty, transNodes);
	auto parentIDs = m1->getParentIDs();
	ASSERT_TRUE(std::find(parentIDs.begin(), parentIDs.end(), root->getSharedID()) != parentIDs.end());
	auto children = scene.getChildrenAsNodes(RepoScene::GraphType::DEFAULT, root->getSharedID());
	ASSERT_TRUE(children.size() == 1 && children[0] == m1);

	scene.abandonChild(RepoScene::GraphType::DEFAULT, root->getSharedID(), m1, false, false);

	parentIDs = m1->getParentIDs();
	EXPECT_TRUE(std::find(parentIDs.begin(), parentIDs.end(), root->getSharedID()) != parentIDs.end());
	children = scene.getChildrenAsNodes(RepoScene::GraphType::DEFAULT, root->getSharedID());
	EXPECT_TRUE(children.size() == 1 && children[0] == m1);

	scene.abandonChild(RepoScene::GraphType::DEFAULT, root->getSharedID(), m1, true, false);

	parentIDs = m1->getParentIDs();
	EXPECT_TRUE(std::find(parentIDs.begin(), parentIDs.end(), root->getSharedID()) != parentIDs.end());
	children = scene.getChildrenAsNodes(RepoScene::GraphType::DEFAULT, root->getSharedID());
	EXPECT_FALSE(children.size());

	scene.abandonChild(RepoScene::GraphType::DEFAULT, root->getSharedID(), m1, false, true);

	parentIDs = m1->getParentIDs();
	EXPECT_FALSE(std::find(parentIDs.begin(), parentIDs.end(), root->getSharedID()) != parentIDs.end());

	scene.abandonChild(RepoScene::GraphType::DEFAULT, generateUUID(), m1, false, false); //shoudln't work with unrecognised parent
	scene.abandonChild(RepoScene::GraphType::DEFAULT, root->getSharedID(), nullptr, false, false); //shoudln't work with unrecognised parent
}

TEST(RepoSceneTest, addInheritance)
{
	RepoNodeSet transNodes, meshNodes, empty;

	auto root = new TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1)));

	auto m1 = new MeshNode(makeRandomNode(root->getSharedID()));
	auto m2 = new MeshNode(makeRandomNode(root->getSharedID()));

	transNodes.insert(root);
	meshNodes.insert(m1);
	meshNodes.insert(m2);

	RepoScene scene(std::vector<std::string>(), empty, meshNodes, empty, empty, empty, transNodes);

	scene.addInheritance(RepoScene::GraphType::DEFAULT, nullptr, nullptr); //nothing should happen and should not crash
	scene.addInheritance(RepoScene::GraphType::DEFAULT, generateUUID(), generateUUID()); //nothing should happen and should not crash

	scene.addInheritance(RepoScene::GraphType::DEFAULT, root->getUniqueID(), m1->getUniqueID()); //this already exist
	EXPECT_EQ(1, m1->getParentIDs().size());
	EXPECT_EQ(2, scene.getChildrenAsNodes(RepoScene::GraphType::DEFAULT, root->getSharedID()).size());

	scene.addInheritance(RepoScene::GraphType::DEFAULT, m1->getUniqueID(), m2->getUniqueID());
	auto parentIDs = m2->getParentIDs();
	EXPECT_EQ(2, parentIDs.size());
	EXPECT_TRUE(std::find(parentIDs.begin(), parentIDs.end(), m1->getSharedID()) != parentIDs.end());

	auto childrenOfM1 = scene.getChildrenAsNodes(RepoScene::GraphType::DEFAULT, m1->getSharedID());
	EXPECT_EQ(1, childrenOfM1.size());
	EXPECT_EQ(m2, childrenOfM1[0]);
}

TEST(RepoSceneTest, getChildrenAsNodes)
{
	RepoNodeSet transNodes, meshNodes, empty;

	auto root = new TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1)));

	auto m1 = new MeshNode(makeRandomNode(root->getSharedID()));
	auto m2 = new MeshNode(makeRandomNode(root->getSharedID()));

	transNodes.insert(root);
	meshNodes.insert(m1);
	meshNodes.insert(m2);

	RepoScene scene(std::vector<std::string>(), empty, meshNodes, empty, empty, empty, transNodes);

	auto children = scene.getChildrenAsNodes(RepoScene::GraphType::DEFAULT, root->getSharedID());
	EXPECT_EQ(2, children.size());
	EXPECT_TRUE(std::find(children.begin(), children.end(), m1) != children.end());
	EXPECT_TRUE(std::find(children.begin(), children.end(), m2) != children.end());

	EXPECT_EQ(0, scene.getChildrenAsNodes(RepoScene::GraphType::DEFAULT, m1->getSharedID()).size());
	EXPECT_EQ(0, scene.getChildrenAsNodes(RepoScene::GraphType::DEFAULT, m2->getSharedID()).size());
	EXPECT_EQ(0, scene.getChildrenAsNodes(RepoScene::GraphType::DEFAULT, generateUUID()).size());

	children = scene.getChildrenNodesFiltered(RepoScene::GraphType::DEFAULT, root->getSharedID(), NodeType::MESH);
	EXPECT_EQ(2, children.size());
	EXPECT_TRUE(std::find(children.begin(), children.end(), m1) != children.end());
	EXPECT_TRUE(std::find(children.begin(), children.end(), m2) != children.end());

	EXPECT_EQ(0, scene.getChildrenNodesFiltered(RepoScene::GraphType::DEFAULT, root->getSharedID(), NodeType::UNKNOWN).size());
	EXPECT_EQ(0, scene.getChildrenNodesFiltered(RepoScene::GraphType::DEFAULT, root->getSharedID(), NodeType::TRANSFORMATION).size());
}

TEST(RepoSceneTest, getParentAsNodesFiltered)
{
	RepoNodeSet transNodes, meshNodes, empty;

	auto root = new TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1)));

	auto m1 = new MeshNode(makeRandomNode(root->getSharedID()));
	auto m2 = new MeshNode(makeRandomNode(root->getSharedID()));

	transNodes.insert(root);
	meshNodes.insert(m1);
	meshNodes.insert(m2);

	RepoScene scene(std::vector<std::string>(), empty, meshNodes, empty, empty, empty, transNodes);

	auto parents = scene.getParentNodesFiltered(RepoScene::GraphType::DEFAULT, nullptr, NodeType::MESH);
	EXPECT_EQ(0, parents.size());

	parents = scene.getParentNodesFiltered(RepoScene::GraphType::DEFAULT, m1, NodeType::TRANSFORMATION);
	EXPECT_EQ(1, parents.size());
	EXPECT_EQ(root, parents[0]);

	EXPECT_EQ(0, scene.getParentNodesFiltered(RepoScene::GraphType::DEFAULT, m1, NodeType::MESH).size());
}

TEST(RepoSceneTest, getSceneFromReference)
{
	RepoScene scene;
	EXPECT_FALSE(scene.getSceneFromReference(defaultG, generateUUID()));
	scene = RepoScene(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_FED);
	std::string errMsg;
	scene.loadScene(getHandler(), errMsg);
	ASSERT_TRUE(errMsg.empty());

	auto references = scene.getAllReferences(defaultG);
	ASSERT_TRUE(references.size());
	for (const auto &ref : references)
		EXPECT_TRUE(scene.getSceneFromReference(defaultG, ref->getSharedID()));
	EXPECT_FALSE(scene.getSceneFromReference(defaultG, generateUUID()));
}

TEST(RepoSceneTest, getTextureIDForMesh)
{
	RepoScene scene;
	EXPECT_TRUE(scene.getTextureIDForMesh(defaultG, generateUUID()).empty());


	RepoNodeSet transNodes, meshNodes, empty, matNodes, texNodes;

	auto root = new TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1)));
	auto m1 = new MeshNode(makeRandomNode(root->getSharedID()));
	auto m2 = new MeshNode(makeRandomNode(root->getSharedID()));
	auto mat1 = new MaterialNode(makeRandomNode(m1->getSharedID()));
	auto mat2 = new MaterialNode(makeRandomNode(m2->getSharedID()));
	auto tex1 = new TextureNode(makeRandomNode(mat1->getSharedID()));

	transNodes.insert(root);
	meshNodes.insert(m1);
	meshNodes.insert(m2);

	matNodes.insert(mat1);
	matNodes.insert(mat2);
	texNodes.insert(tex1);

	auto scene2 = RepoScene(std::vector<std::string>(), empty, meshNodes, matNodes, empty, texNodes, transNodes);

	EXPECT_EQ(UUIDtoString(tex1->getUniqueID()), scene2.getTextureIDForMesh(defaultG, m1->getSharedID()));
	EXPECT_TRUE(scene2.getTextureIDForMesh(defaultG, m2->getSharedID()).empty());
}

TEST(RepoSceneTest, getAllNodes)
{
	RepoScene scene;
	EXPECT_EQ(0, scene.getAllCameras(defaultG).size());
	EXPECT_EQ(0, scene.getAllCameras(RepoScene::GraphType::OPTIMIZED).size());
	EXPECT_EQ(0, scene.getAllMeshes(defaultG).size());
	EXPECT_EQ(0, scene.getAllMeshes(RepoScene::GraphType::OPTIMIZED).size());
	EXPECT_EQ(0, scene.getAllMaterials(defaultG).size());
	EXPECT_EQ(0, scene.getAllMaterials(RepoScene::GraphType::OPTIMIZED).size());
	EXPECT_EQ(0, scene.getAllTextures(defaultG).size());
	EXPECT_EQ(0, scene.getAllTextures(RepoScene::GraphType::OPTIMIZED).size());
	EXPECT_EQ(0, scene.getAllTransformations(defaultG).size());
	EXPECT_EQ(0, scene.getAllTransformations(RepoScene::GraphType::OPTIMIZED).size());
	EXPECT_EQ(0, scene.getAllMetadata(defaultG).size());
	EXPECT_EQ(0, scene.getAllMetadata(RepoScene::GraphType::OPTIMIZED).size());
	EXPECT_EQ(0, scene.getAllReferences(defaultG).size());
	EXPECT_EQ(0, scene.getAllReferences(RepoScene::GraphType::OPTIMIZED).size());
	EXPECT_EQ(0, scene.getAllMaps(defaultG).size());
	EXPECT_EQ(0, scene.getAllMaps(RepoScene::GraphType::OPTIMIZED).size());

	RepoNodeSet transNodes, meshNodes, metaNodes, matNodes, texNodes, camNodes, refNodes, mapsNodes;

	auto root = new TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1)));
	meshNodes.insert(new MeshNode(makeRandomNode(root->getSharedID())));
	meshNodes.insert(new MeshNode(makeRandomNode(root->getSharedID())));
	matNodes.insert(new MaterialNode(makeRandomNode(root->getSharedID())));
	matNodes.insert(new MaterialNode(makeRandomNode(root->getSharedID())));
	texNodes.insert(new TextureNode(makeRandomNode(root->getSharedID())));
	camNodes.insert(new CameraNode(makeRandomNode(root->getSharedID())));
	camNodes.insert(new CameraNode(makeRandomNode(root->getSharedID())));
	metaNodes.insert(new MetadataNode(makeRandomNode(root->getSharedID())));
	metaNodes.insert(new MetadataNode(makeRandomNode(root->getSharedID())));
	metaNodes.insert(new MetadataNode(makeRandomNode(root->getSharedID())));
	refNodes.insert(new ReferenceNode(makeRandomNode(root->getSharedID())));
	mapsNodes.insert(new MapNode(makeRandomNode(root->getSharedID())));
	mapsNodes.insert(new MapNode(makeRandomNode(root->getSharedID())));
	mapsNodes.insert(new MapNode(makeRandomNode(root->getSharedID())));
	
	transNodes.insert(root);

	auto scene2 = RepoScene(std::vector<std::string>(), camNodes, meshNodes, matNodes, metaNodes, texNodes, transNodes, refNodes, mapsNodes);

	auto allSharedIDs = scene2.getAllSharedIDs(defaultG);
	EXPECT_EQ(0, scene2.getAllSharedIDs(RepoScene::GraphType::OPTIMIZED).size());

	auto cams = scene2.getAllCameras(defaultG);
	EXPECT_EQ(camNodes.size(), cams.size());
	EXPECT_EQ(0, scene2.getAllCameras(RepoScene::GraphType::OPTIMIZED).size());
	for (const auto cam : cams)
	{
		EXPECT_NE(camNodes.end(), camNodes.find(cam));
		EXPECT_NE(allSharedIDs.end(), allSharedIDs.find(cam->getSharedID()));
	}

	auto meshes = scene2.getAllMeshes(defaultG);
	EXPECT_EQ(meshNodes.size(), meshes.size());
	EXPECT_EQ(0, scene2.getAllMeshes(RepoScene::GraphType::OPTIMIZED).size());
	for (const auto mesh : meshes)
	{
		EXPECT_NE(meshes.end(), meshes.find(mesh));
		EXPECT_NE(allSharedIDs.end(), allSharedIDs.find(mesh->getSharedID()));
	}


	auto mats = scene2.getAllMaterials(defaultG);
	EXPECT_EQ(matNodes.size(), mats.size());
	EXPECT_EQ(0, scene2.getAllMaterials(RepoScene::GraphType::OPTIMIZED).size());
	for (const auto mat : mats)
	{
		EXPECT_NE(mats.end(), mats.find(mat));
		EXPECT_NE(allSharedIDs.end(), allSharedIDs.find(mat->getSharedID()));
	}

	auto texts = scene2.getAllTextures(defaultG);
	EXPECT_EQ(texNodes.size(), texts.size());
	EXPECT_EQ(0, scene2.getAllTextures(RepoScene::GraphType::OPTIMIZED).size());
	for (const auto tex : texts)
	{
		EXPECT_NE(texts.end(), texts.find(tex));
		EXPECT_NE(allSharedIDs.end(), allSharedIDs.find(tex->getSharedID()));
	}

	auto trans = scene2.getAllTransformations(defaultG);
	EXPECT_EQ(transNodes.size(), trans.size());
	EXPECT_EQ(0, scene2.getAllTransformations(RepoScene::GraphType::OPTIMIZED).size());
	for (const auto tran : trans)
	{
		EXPECT_NE(trans.end(), trans.find(tran));
		EXPECT_NE(allSharedIDs.end(), allSharedIDs.find(tran->getSharedID()));
	}


	auto metas = scene2.getAllMetadata(defaultG);
	EXPECT_EQ(metaNodes.size(), metas.size());
	EXPECT_EQ(0, scene2.getAllMetadata(RepoScene::GraphType::OPTIMIZED).size());
	for (const auto meta : metas)
	{
		EXPECT_NE(metas.end(), metas.find(meta));
		EXPECT_NE(allSharedIDs.end(), allSharedIDs.find(meta->getSharedID()));
	}

	auto refs = scene2.getAllReferences(defaultG);
	EXPECT_EQ(refNodes.size(), refs.size());
	EXPECT_EQ(0, scene2.getAllReferences(RepoScene::GraphType::OPTIMIZED).size());
	for (const auto ref : refs)
	{
		EXPECT_NE(refs.end(), refs.find(ref));
		EXPECT_NE(allSharedIDs.end(), allSharedIDs.find(ref->getSharedID()));
	}

	auto maps = scene2.getAllMaps(defaultG);
	EXPECT_EQ(mapsNodes.size(), maps.size());
	EXPECT_EQ(0, scene2.getAllMaps(RepoScene::GraphType::OPTIMIZED).size());
	for (const auto map : maps)
	{
		EXPECT_NE(maps.end(), maps.find(map));
		EXPECT_NE(allSharedIDs.end(), allSharedIDs.find(map->getSharedID()));
	}	
}

TEST(RepoSceneTest, getAllDescendantsByType)
{
	RepoScene scene;
	EXPECT_EQ(0, scene.getAllDescendantsByType(defaultG, generateUUID(), NodeType::CAMERA).size());
	EXPECT_EQ(0, scene.getAllDescendantsByType(RepoScene::GraphType::OPTIMIZED, generateUUID(), NodeType::CAMERA).size());

	RepoNodeSet transNodes, meshNodes, empty, matNodes, texNodes;

	auto root = new TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1)));
	auto trans2 = new TransformationNode(makeRandomNode(root->getSharedID()));
	auto m1 = new MeshNode(makeRandomNode(root->getSharedID()));
	auto m2 = new MeshNode(makeRandomNode(trans2->getSharedID()));
	auto mat1 = new MaterialNode(makeRandomNode(m1->getSharedID()));
	auto mat2 = new MaterialNode(makeRandomNode(m2->getSharedID()));
	auto tex1 = new TextureNode(makeRandomNode(mat1->getSharedID()));

	transNodes.insert(root);
	transNodes.insert(trans2);
	meshNodes.insert(m1);
	meshNodes.insert(m2);

	matNodes.insert(mat1);
	matNodes.insert(mat2);
	texNodes.insert(tex1);

	auto scene2 = RepoScene(std::vector<std::string>(), empty, meshNodes, matNodes, empty, texNodes, transNodes);
	auto trans = scene2.getAllDescendantsByType(defaultG, root->getSharedID(), NodeType::TRANSFORMATION);
	ASSERT_EQ(1, trans.size());
	EXPECT_EQ(trans2, trans[0]);
	EXPECT_EQ(0, scene2.getAllDescendantsByType(defaultG, trans2->getSharedID(), NodeType::TRANSFORMATION).size());

	auto meshes = scene2.getAllDescendantsByType(defaultG, root->getSharedID(), NodeType::MESH);
	ASSERT_EQ(2, meshes.size());
	EXPECT_FALSE( std::find(meshes.begin(), meshes.end(), m1) == meshes.end());
	EXPECT_FALSE(std::find(meshes.begin(), meshes.end(), m2) == meshes.end());

	meshes = scene2.getAllDescendantsByType(defaultG, trans2->getSharedID(), NodeType::MESH);
	ASSERT_EQ(1, meshes.size());
	EXPECT_TRUE(std::find(meshes.begin(), meshes.end(), m1) == meshes.end());
	EXPECT_FALSE(std::find(meshes.begin(), meshes.end(), m2) == meshes.end());

	auto texes = scene2.getAllDescendantsByType(defaultG, root->getSharedID(), NodeType::TEXTURE);
	ASSERT_EQ(1, texes.size());
	EXPECT_EQ(texes[0], tex1);
}

TEST(RepoSceneTest, getSceneBoundingBox)
{
	RepoScene scene;
	EXPECT_EQ(0, scene.getSceneBoundingBox().size());

	std::string errMsg;
	RepoScene scene2(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ);
	scene2.loadScene(getHandler(), errMsg);
	auto bb = scene2.getSceneBoundingBox();
	EXPECT_TRUE(compareVectors(bb, getGoldenDataForBBoxTest()));
}

TEST(RepoSceneTest, getNodeBySharedID)
{
	RepoNodeSet transNodes, meshNodes, empty, matNodes, texNodes;

	auto root = new TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1)));
	auto trans2 = new TransformationNode(makeRandomNode(root->getSharedID()));
	auto m1 = new MeshNode(makeRandomNode(root->getSharedID()));
	auto m2 = new MeshNode(makeRandomNode(trans2->getSharedID()));
	auto mat1 = new MaterialNode(makeRandomNode(m1->getSharedID()));
	auto mat2 = new MaterialNode(makeRandomNode(m2->getSharedID()));
	auto tex1 = new TextureNode(makeRandomNode(mat1->getSharedID()));

	transNodes.insert(root);
	transNodes.insert(trans2);
	meshNodes.insert(m1);
	meshNodes.insert(m2);

	matNodes.insert(mat1);
	matNodes.insert(mat2);
	texNodes.insert(tex1);

	auto rootst = new TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1)));
	auto trans2st = new TransformationNode(makeRandomNode(rootst->getSharedID()));
	auto m1st = new MeshNode(makeRandomNode(rootst->getSharedID()));
	auto m2st = new MeshNode(makeRandomNode(trans2st->getSharedID()));
	auto mat1st = new MaterialNode(makeRandomNode(m1st->getSharedID()));
	auto mat2st = new MaterialNode(makeRandomNode(m2st->getSharedID()));
	auto tex1st = new TextureNode(makeRandomNode(mat1st->getSharedID()));

	transNodes.insert(rootst);
	transNodes.insert(trans2st);
	meshNodes.insert(m1st);
	meshNodes.insert(m2st);

	matNodes.insert(mat1st);
	matNodes.insert(mat2st);
	texNodes.insert(tex1st);

	auto scene2 = RepoScene(std::vector<std::string>(), empty, meshNodes, matNodes, empty, texNodes, transNodes);

	EXPECT_EQ(root, scene2.getNodeBySharedID(defaultG, root->getSharedID()));
	EXPECT_EQ(nullptr, scene2.getNodeBySharedID(defaultG, generateUUID()));
	EXPECT_EQ(nullptr, scene2.getNodeBySharedID(RepoScene::GraphType::OPTIMIZED, m2st->getSharedID()));

}

TEST(RepoSceneTest, getNodeByUniqueID)
{
	RepoNodeSet transNodes, meshNodes, empty, matNodes, texNodes;

	auto root = new TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1)));
	auto trans2 = new TransformationNode(makeRandomNode(root->getSharedID()));
	auto m1 = new MeshNode(makeRandomNode(root->getSharedID()));
	auto m2 = new MeshNode(makeRandomNode(trans2->getSharedID()));
	auto mat1 = new MaterialNode(makeRandomNode(m1->getSharedID()));
	auto mat2 = new MaterialNode(makeRandomNode(m2->getSharedID()));
	auto tex1 = new TextureNode(makeRandomNode(mat1->getSharedID()));

	transNodes.insert(root);
	transNodes.insert(trans2);
	meshNodes.insert(m1);
	meshNodes.insert(m2);

	matNodes.insert(mat1);
	matNodes.insert(mat2);
	texNodes.insert(tex1);

	auto rootst = new TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1)));
	auto trans2st = new TransformationNode(makeRandomNode(rootst->getSharedID()));
	auto m1st = new MeshNode(makeRandomNode(rootst->getSharedID()));
	auto m2st = new MeshNode(makeRandomNode(trans2st->getSharedID()));
	auto mat1st = new MaterialNode(makeRandomNode(m1st->getSharedID()));
	auto mat2st = new MaterialNode(makeRandomNode(m2st->getSharedID()));
	auto tex1st = new TextureNode(makeRandomNode(mat1st->getSharedID()));

	transNodes.insert(rootst);
	transNodes.insert(trans2st);
	meshNodes.insert(m1st);
	meshNodes.insert(m2st);

	matNodes.insert(mat1st);
	matNodes.insert(mat2st);
	texNodes.insert(tex1st);

	auto scene2 = RepoScene(std::vector<std::string>(), empty, meshNodes, matNodes, empty, texNodes, transNodes);

	EXPECT_EQ(root, scene2.getNodeByUniqueID(defaultG, root->getUniqueID()));
	EXPECT_EQ(nullptr, scene2.getNodeBySharedID(defaultG, generateUUID()));
	EXPECT_EQ(nullptr, scene2.getNodeBySharedID(RepoScene::GraphType::OPTIMIZED, m2st->getUniqueID()));

}

TEST(RepoSceneTest, hasRoot)
{
	RepoScene scene;
	EXPECT_FALSE(scene.hasRoot(defaultG));
	EXPECT_FALSE(scene.hasRoot(RepoScene::GraphType::OPTIMIZED));

	RepoNodeSet transNodes, transNodesStash, empty;
	auto root = new TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1)));
	auto rootStash = new TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1)));
	transNodes.insert(root);
	transNodesStash.insert(rootStash);

	auto scene2 = RepoScene(std::vector<std::string>(), empty, empty, empty, empty, empty, transNodes);
	EXPECT_TRUE(scene2.hasRoot(defaultG));
	EXPECT_FALSE(scene2.hasRoot(RepoScene::GraphType::OPTIMIZED));
	scene2.addStashGraph(empty, empty, empty, empty, transNodesStash);
	EXPECT_TRUE(scene2.hasRoot(RepoScene::GraphType::OPTIMIZED));

	scene2.clearStash();
	EXPECT_FALSE(scene2.hasRoot(RepoScene::GraphType::OPTIMIZED));
}

TEST(RepoSceneTest, getRoot)
{
	RepoScene scene;
	EXPECT_EQ(nullptr, scene.getRoot(defaultG));
	EXPECT_EQ(nullptr, scene.getRoot(RepoScene::GraphType::OPTIMIZED));

	RepoNodeSet transNodes, transNodesStash, empty;
	auto root = new TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1)));
	auto rootStash = new TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1)));
	transNodes.insert(root);
	transNodesStash.insert(rootStash);

	auto scene2 = RepoScene(std::vector<std::string>(), empty, empty, empty, empty, empty, transNodes);
	EXPECT_EQ(root, scene2.getRoot(defaultG));
	EXPECT_EQ(nullptr, scene2.getRoot(RepoScene::GraphType::OPTIMIZED));
	scene2.addStashGraph(empty, empty, empty, empty, transNodesStash);
	EXPECT_EQ(rootStash, scene2.getRoot(RepoScene::GraphType::OPTIMIZED));

	scene2.clearStash();
	EXPECT_EQ(nullptr, scene2.getRoot(RepoScene::GraphType::OPTIMIZED));
}

TEST(RepoSceneTest, getItemsInCurrentGraph)
{
	RepoScene scene;
	EXPECT_EQ(0, scene.getItemsInCurrentGraph(defaultG));
	EXPECT_EQ(0, scene.getItemsInCurrentGraph(RepoScene::GraphType::OPTIMIZED));

	RepoNodeSet transNodes, transNodesStash, empty;
	auto root = new TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1)));
	auto rootStash = new TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1)));
	transNodes.insert(root);
	transNodesStash.insert(rootStash);

	auto scene2 = RepoScene(std::vector<std::string>(), empty, empty, empty, empty, empty, transNodes);
	EXPECT_EQ(1, scene2.getItemsInCurrentGraph(defaultG));
	EXPECT_EQ(0, scene2.getItemsInCurrentGraph(RepoScene::GraphType::OPTIMIZED));
	scene2.addStashGraph(empty, empty, empty, empty, transNodesStash);
	EXPECT_EQ(1, scene2.getItemsInCurrentGraph(RepoScene::GraphType::OPTIMIZED));

	scene2.clearStash();
	EXPECT_EQ(0, scene2.getItemsInCurrentGraph(RepoScene::GraphType::OPTIMIZED));
}

TEST(RepoSceneTest, getOriginalFiles)
{
	RepoScene scene;
	EXPECT_EQ(0, scene.getOriginalFiles().size());

	RepoNodeSet empty;
	auto scene2 = RepoScene(std::vector<std::string>(), empty, empty, empty, empty, empty, empty);
	EXPECT_EQ(0, scene2.getOriginalFiles().size());


	std::vector<std::string> orgFiles;
	for (int i = 0; i < rand() % 10 + 1; ++i)
	{
		orgFiles.push_back(getRandomString(rand() % 10 + 1));
	}

	auto scene3 = RepoScene(orgFiles, empty, empty, empty, empty, empty, empty);
	auto orgFilesOut = scene3.getOriginalFiles();
	ASSERT_EQ(orgFiles.size(), orgFilesOut.size());

	for (int i = 0; i < orgFilesOut.size(); ++i)
	{
		EXPECT_TRUE(std::find(orgFilesOut.begin(), orgFilesOut.end(), orgFiles[i]) != orgFilesOut.end());
	}

	auto scene4 = RepoScene(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ);
	std::string errMsg;
	scene4.loadScene(getHandler(), errMsg);
	ASSERT_TRUE(errMsg.empty());
	auto orgFilesOut2 = scene4.getOriginalFiles();
	EXPECT_EQ(1, orgFilesOut2.size());
	EXPECT_EQ(getFileNameBIMModel, orgFilesOut2[0]);
}


TEST(RepoSceneTest, addNodes)
{
	std::vector<RepoNode *> newNodes;

	RepoScene scene;
	scene.addNodes(newNodes);
	EXPECT_EQ(0, scene.getItemsInCurrentGraph(defaultG));

	newNodes.push_back(new TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1))));
	newNodes.push_back(new TransformationNode(makeRandomNode(newNodes.back()->getSharedID())));
	newNodes.push_back(new MeshNode(makeRandomNode(newNodes.back()->getSharedID())));
	newNodes.push_back(new MeshNode(makeRandomNode(newNodes.back()->getSharedID())));
	newNodes.push_back(new MaterialNode(makeRandomNode(newNodes.back()->getSharedID())));
	newNodes.push_back(new MaterialNode(makeRandomNode(newNodes.back()->getSharedID())));
	newNodes.push_back(new TextureNode(makeRandomNode(newNodes.back()->getSharedID())));

	scene.addNodes(newNodes);
	EXPECT_EQ(newNodes.size(), scene.getItemsInCurrentGraph(defaultG));


	int currentSize = newNodes.size();
	newNodes.clear();
	newNodes.push_back(new TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1))));
	newNodes.push_back(new TransformationNode(makeRandomNode(newNodes.back()->getSharedID())));
	newNodes.push_back(new MeshNode(makeRandomNode(newNodes.back()->getSharedID())));
	newNodes.push_back(new MeshNode(makeRandomNode(newNodes.back()->getSharedID())));
	newNodes.push_back(new MaterialNode(makeRandomNode(newNodes.back()->getSharedID())));
	newNodes.push_back(new MaterialNode(makeRandomNode(newNodes.back()->getSharedID())));
	newNodes.push_back(new TextureNode(makeRandomNode(newNodes.back()->getSharedID())));

	scene.addNodes(newNodes);
	EXPECT_EQ(newNodes.size() + currentSize, scene.getItemsInCurrentGraph(defaultG));
}

TEST(RepoSceneTest, modifyNode)
{
	RepoScene scene;
	scene.modifyNode(defaultG, nullptr, nullptr);
	auto root = new TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1)));
	scene.addNodes({ root });
	RepoNode newFields = BSON("name" << "cream");
	scene.modifyNode(defaultG, root, &newFields);


	EXPECT_EQ(newFields.getName(), root->getName());

	RepoNode removedName = root->removeField("name");
	scene.modifyNode(defaultG, root, &removedName, true);
	EXPECT_FALSE(root->hasField("name"));
	scene.modifyNode(defaultG, root, nullptr, true);
}

TEST(RepoSceneTest, removeNode)
{
	RepoScene scene;
	scene.removeNode(defaultG, generateUUID());
	auto root = new TransformationNode(makeRandomNode(getRandomString(rand() % 10 + 1)));
	scene.addNodes({ root });
	scene.removeNode(defaultG, root->getSharedID());
	scene.removeNode(defaultG, root->getSharedID());

	delete root;
}

TEST(RepoSceneTest, resetChangeSet)
{
	RepoScene scene;
	scene.resetChangeSet();

	auto scene2 = RepoScene(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ);
	std::string errMsg;
	scene2.loadScene(getHandler(), errMsg);
	EXPECT_TRUE(scene2.isRevisioned());
	scene2.resetChangeSet();
	EXPECT_FALSE(scene2.isRevisioned());
}

TEST(RepoSceneTest, reorientateDirectXModel)
{
	RepoScene scene;
	scene.reorientateDirectXModel();
	EXPECT_FALSE(scene.hasRoot(defaultG));
	EXPECT_FALSE(scene.hasRoot(RepoScene::GraphType::OPTIMIZED));

	RepoNodeSet transNodes, transNodesStash, empty;
	auto root = new TransformationNode( RepoBSONFactory::makeTransformationNode());
	auto rootStash =new TransformationNode( RepoBSONFactory::makeTransformationNode());
	transNodes.insert(root);
	transNodesStash.insert(rootStash);

	ASSERT_TRUE(root->isIdentity());
	ASSERT_TRUE(rootStash->isIdentity());
	auto scene2 = RepoScene(std::vector<std::string>(), empty, empty, empty, empty, empty, transNodes);	
	scene2.addStashGraph(empty, empty, empty, empty, transNodesStash);
	scene2.reorientateDirectXModel();
	std::vector<float> rotatedMat =
	{
		1, 0, 0, 0,
		0, 0, 1, 0,
		0, -1, 0, 0,
		0, 0, 0, 1
	};

	EXPECT_TRUE(compareStdVectors(rotatedMat, root->getTransMatrix(false)));
	EXPECT_FALSE(scene2.hasRoot(RepoScene::GraphType::OPTIMIZED));

}