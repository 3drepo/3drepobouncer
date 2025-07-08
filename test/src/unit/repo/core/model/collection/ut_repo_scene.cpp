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
#include <gmock/gmock.h>

#include <repo/core/model/bson/repo_node_mesh.h>
#include <repo/core/model/bson/repo_node_texture.h>
#include <repo/core/model/bson/repo_node_transformation.h>
#include <repo/core/model/bson/repo_bson_factory.h>
#include <repo/core/model/collection/repo_scene.h>
#include <repo/error_codes.h>

#include "../../../../repo_test_utils.h"
#include "../../../../repo_test_mesh_utils.h"
#include "../../../../repo_test_database_info.h"
#include "../../../../repo_test_fileservice_info.h"

using namespace repo::core::model;
using namespace testing;

const static RepoScene::GraphType defaultG = RepoScene::GraphType::DEFAULT;

static TransformationNode makeTransformationNode(
	const repo::lib::RepoUUID& parent,
	const std::string& name = "")
{
	return RepoBSONFactory::makeTransformationNode(repo::lib::RepoMatrix(), name, { parent });
}

static TransformationNode makeTransformationNode(
	const std::string& name = "")
{
	return RepoBSONFactory::makeTransformationNode(repo::lib::RepoMatrix(), name);
}

static MeshNode makeMeshNode(
	const repo::lib::RepoUUID& parent,
	const std::string& name = "")
{
	auto m = repo::test::utils::mesh::makeMeshNode(repo::test::utils::mesh::mesh_data(
		true,
		true,
		0,
		3,
		false,
		0,
		1024,
		""
	));
	m.changeName(name, true);
	m.setSharedID(repo::lib::RepoUUID::createUUID());
	m.addParent(parent);
	return m;
}

static MetadataNode makeMetadataNode(
	const repo::lib::RepoUUID& parent,
	const std::string& name = "")
{
	return RepoBSONFactory::makeMetaDataNode({}, name, { parent });
}

static MetadataNode makeMetadataNode(
	const std::string& name = "")
{
	return RepoBSONFactory::makeMetaDataNode({}, name);
}

static MaterialNode makeMaterialNode(
	const repo::lib::RepoUUID& parent)
{
	repo::lib::repo_material_t s;
	auto m = RepoBSONFactory::makeMaterialNode(s, "", { parent });
	m.setSharedID(repo::lib::RepoUUID::createUUID());  // These unit tests will set the parent of texture nodes as material nodes
	return m;
}

static TextureNode makeTextureNode(
	const repo::lib::RepoUUID& parent)
{
	return RepoBSONFactory::makeTextureNode("", 0, 0, 0, 0, { parent });
}

static ReferenceNode makeReferenceNode(
	const repo::lib::RepoUUID& parent)
{
	auto r = RepoBSONFactory::makeReferenceNode("db", "project");
	r.addParent(parent);
	return r;
}

TEST(RepoSceneTest, Constructor)
{
	//Just to make sure it doesn't throw exceptiosn with empty
	RepoScene scene;
	RepoNodeSet empty;
	std::vector<std::string> files;
	RepoScene scene2(files, empty, empty, empty, empty, empty, empty);
}

TEST(RepoSceneTest, ConstructOrphanMeshScene)
{
	RepoNodeSet empty, trans, meshes;
	std::vector<std::string> files;

	auto root = new TransformationNode(makeTransformationNode(getRandomString(rand() % 100 + 1)));
	trans.insert(root);

	auto t1 = new TransformationNode(makeTransformationNode(root->getSharedID(), getRandomString(rand() % 100 + 1) + root->getName()));
	trans.insert(t1);

	auto m1 = new MeshNode();
	auto m2 = new MeshNode();
	auto m3 = new MeshNode();
	meshes.insert(m1);
	meshes.insert(m2);
	meshes.insert(m3);

	RepoScene scene(files, empty, meshes, empty, empty, empty, trans);
	EXPECT_TRUE(scene.isMissingNodes());
}

TEST(RepoSceneTest, FilterNodesByType)
{
	std::vector<RepoNode*> nodes;
	TextureNode texNode;
	MeshNode meshNode;
	TransformationNode transNode;
	//Check empty vector doesn't crash
	EXPECT_EQ(0, RepoScene::filterNodesByType(nodes, NodeType::REFERENCE).size());

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

	auto root = makeTransformationNode(getRandomString(rand() % 100 + 1));

	auto t1 = makeTransformationNode(root.getSharedID(), getRandomString(rand() % 100 + 1) + root.getName());
	auto t2 = makeTransformationNode(root.getSharedID(), getRandomString(rand() % 100 + 1) + t1.getName());
	auto t3 = makeTransformationNode(root.getSharedID(), getRandomString(rand() % 100 + 1) + t2.getName());

	auto m1 = makeMeshNode(t1.getSharedID());
	auto m2 = makeMeshNode(t1.getSharedID());
	auto m3 = makeMeshNode(t2.getSharedID());

	auto mm1 = makeMetadataNode(t1.getName());
	auto mm2 = makeMetadataNode(t2.getName());
	auto mm3 = makeMetadataNode(t3.getName());

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
		scene2(std::vector<std::string>(), meshNodes, empty, empty, empty, transNodes);
	scene.addMetadata(metaNodes, true);
	EXPECT_EQ(0, scene.getAllMetadata(defaultG).size());
	scene2.addMetadata(metaNodes, true, false); //no propagation check
	auto scene2Meta = scene2.getAllMetadata(defaultG);
	EXPECT_EQ(metaNodes.size(), scene2Meta.size());
	for (const auto& s2meta : scene2Meta)
	{
		auto parents = s2meta->getParentIDs();
		EXPECT_EQ(1, parents.size());
		for (const auto& parent : parents)
		{
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

	RepoScene scene3(std::vector<std::string>(), meshNodes, empty, empty, empty, transNodes);

	scene3.addMetadata(metaNodes, true, true); //propagation check

	auto scene3Meta = scene3.getAllMetadata(defaultG);
	EXPECT_EQ(metaNodes.size(), scene2Meta.size());
	for (const auto& s3meta : scene3Meta)
	{
		auto parents = s3meta->getParentIDs();
		EXPECT_TRUE(parents.size() > 0);
		for (const auto& parent : parents)
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
	scene.addStashGraph(empty, empty, empty, empty);
	EXPECT_FALSE(scene.hasRoot(stashGraph));

	RepoNodeSet transNodes;
	RepoNodeSet meshNodes;
	RepoNodeSet metaNodes;

	auto root = makeTransformationNode(getRandomString(rand() % 10 + 1));

	auto t1 = makeTransformationNode(root.getSharedID(), getRandomString(rand() % 100 + 1));
	auto t2 = makeTransformationNode(root.getSharedID(), getRandomString(rand() % 100 + 1));
	auto t3 = makeTransformationNode(root.getSharedID(), getRandomString(rand() % 100 + 1));

	auto m1 = makeMeshNode(t1.getSharedID());
	auto m2 = makeMeshNode(t1.getSharedID());
	auto m3 = makeMeshNode(t2.getSharedID());

	auto rootNode = new TransformationNode(root);
	transNodes.insert(rootNode);
	transNodes.insert(new TransformationNode(t1));
	transNodes.insert(new TransformationNode(t2));
	transNodes.insert(new TransformationNode(t3));

	meshNodes.insert(new MeshNode(m1));
	meshNodes.insert(new MeshNode(m2));
	meshNodes.insert(new MeshNode(m3));

	scene.addStashGraph(meshNodes, empty, empty, transNodes);
	EXPECT_EQ(0, scene.getAllTextures(stashGraph).size());
	EXPECT_EQ(0, scene.getAllMaterials(stashGraph).size());
	EXPECT_EQ(meshNodes.size(), scene.getAllMeshes(stashGraph).size());
	EXPECT_EQ(transNodes.size(), scene.getAllTransformations(stashGraph).size());
	EXPECT_TRUE(scene.hasRoot(stashGraph));
	EXPECT_FALSE(scene.hasRoot(RepoScene::GraphType::DEFAULT));
	EXPECT_EQ(rootNode, scene.getRoot(stashGraph));

	scene.clearStash();

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

	auto handler = getHandler();

	//Commiting an empty scene should fail (fails on empty project/database name)
	EXPECT_EQ(REPOERR_UPLOAD_FAILED, scene.commit(handler.get(), handler->getFileManager().get(), errMsg, commitUser));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();

	scene.setDatabaseAndProjectName("sceneCommit", "test1");
	EXPECT_EQ(REPOERR_UPLOAD_FAILED, scene.commit(handler.get(), handler->getFileManager().get(), errMsg, commitUser));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();

	RepoNodeSet transNodes;
	RepoNodeSet meshNodes, empty;

	auto root = new TransformationNode(makeTransformationNode(getRandomString(rand() % 10 + 1)));

	auto t1 = new TransformationNode(makeTransformationNode(root->getSharedID(), getRandomString(rand() % 10 + 1)));

	auto m1 = new MeshNode(makeMeshNode(t1->getSharedID()));
	auto m2 = new MeshNode(makeMeshNode(t1->getSharedID()));

	transNodes.insert(root);
	transNodes.insert(t1);

	meshNodes.insert(m1);
	meshNodes.insert(m2);

	RepoScene scene2(std::vector<std::string>(), empty, meshNodes, empty, empty, empty, transNodes);
	scene2.setDatabaseAndProjectName("sceneCommit", "test2");
	EXPECT_EQ(REPOERR_UPLOAD_FAILED, scene2.commit(nullptr, nullptr, errMsg, commitUser));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();

	std::string commitMsg = "this is a commit message for this commit.";
	std::string commitTag = "test";

	EXPECT_EQ(REPOERR_OK, scene2.commit(handler.get(), handler->getFileManager().get(), errMsg, commitUser, commitMsg, commitTag));
	EXPECT_TRUE(errMsg.empty());

	EXPECT_TRUE(scene2.isRevisioned());
	EXPECT_EQ(scene2.getOwner(), commitUser);
	EXPECT_EQ(scene2.getTag(), commitTag);
	EXPECT_EQ(scene2.getMessage(), commitMsg);
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

TEST(RepoSceneTest, getSetRevisionID)
{
	RepoScene scene;
	EXPECT_TRUE(scene.isHeadRevision());
	repo::lib::RepoUUID id = repo::lib::RepoUUID::createUUID();
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

	auto root = new TransformationNode(makeTransformationNode(getRandomString(rand() % 10 + 1)));

	auto t1 = new TransformationNode(makeTransformationNode(root->getSharedID(), getRandomString(rand() % 10 + 1)));

	auto m1 = new MeshNode(makeMeshNode(t1->getSharedID()));
	auto m2 = new MeshNode(makeMeshNode(t1->getSharedID()));

	transNodes.insert(root);
	transNodes.insert(t1);

	meshNodes.insert(m1);
	meshNodes.insert(m2);
	std::vector<double> offset({ rand() / 1000., rand() / 1000., rand() / 1000. });

	auto handler = getHandler();

	RepoScene scene2(std::vector<std::string>(), empty, meshNodes, empty, empty, empty, transNodes);
	scene2.setDatabaseAndProjectName("sceneCommit", "test2");
	ASSERT_EQ(REPOERR_OK, scene2.commit(handler.get(), handler->getFileManager().get(), errMsg, commitUser, commitMessage, commitTag));

	scene2.setWorldOffset(offset);
	EXPECT_EQ(scene2.getOwner(), commitUser);
	EXPECT_EQ(scene2.getTag(), commitTag);
	EXPECT_EQ(scene2.getMessage(), commitMessage);
	EXPECT_TRUE(compareStdVectors(scene2.getWorldOffset(), offset));
}

TEST(RepoSceneTest, getSetBranchID)
{
	RepoScene scene;
	EXPECT_EQ(scene.getBranchID().toString(), REPO_HISTORY_MASTER_BRANCH);

	repo::lib::RepoUUID newBranch = repo::lib::RepoUUID::createUUID();
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

	auto root = new TransformationNode(makeTransformationNode(getRandomString(rand() % 10 + 1)));

	auto t1 = new TransformationNode(makeTransformationNode(root->getSharedID(), getRandomString(rand() % 10 + 1)));

	auto m1 = new MeshNode(makeMeshNode(t1->getSharedID()));
	auto m2 = new MeshNode(makeMeshNode(t1->getSharedID()));

	transNodes.insert(root);
	transNodes.insert(t1);

	meshNodes.insert(m1);
	meshNodes.insert(m2);

	transNodes2.insert(new TransformationNode(*root));
	transNodes2.insert(new TransformationNode(*t1));

	meshNodes2.insert(new MeshNode(*m1));
	meshNodes2.insert(new MeshNode(*m2));

	RepoScene scene2(std::vector<std::string>(), empty, meshNodes, empty, empty, empty, transNodes);
	scene2.addStashGraph(meshNodes2, empty, empty, transNodes2);

	EXPECT_EQ(scene2.getViewGraph(), RepoScene::GraphType::OPTIMIZED);
}

TEST(RepoSceneTest, loadRevision)
{
	auto handler = getHandler();
	std::string errMsg;
	EXPECT_FALSE(RepoScene().loadRevision(handler.get(), errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
	EXPECT_TRUE(RepoScene(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ).loadRevision(handler.get(), errMsg));
	EXPECT_TRUE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(RepoScene("nonexistantDatabase", "NonExistantProject").loadRevision(handler.get(), errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(RepoScene(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ).loadRevision(nullptr, errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
}

TEST(RepoSceneTest, loadScene)
{
	auto handler = getHandler();
	std::string errMsg;
	EXPECT_FALSE(RepoScene().loadScene(handler.get(), errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
	EXPECT_TRUE(RepoScene(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ).loadScene(handler.get(), errMsg));
	EXPECT_TRUE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(RepoScene("nonexistantDatabase", "NonExistantProject").loadScene(handler.get(), errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(RepoScene(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ).loadScene(nullptr, errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
}

TEST(RepoSceneTest, loadStash)
{
	auto handler = getHandler();
	std::string errMsg;
	EXPECT_FALSE(RepoScene().loadStash(handler.get(), errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
	EXPECT_TRUE(RepoScene(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ).loadStash(handler.get(), errMsg));
	EXPECT_TRUE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(RepoScene("nonexistantDatabase", "NonExistantProject").loadStash(handler.get(), errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(RepoScene(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ).loadStash(nullptr, errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
}

TEST(RepoSceneTest, updateRevisionStatus)
{
	auto handler = getHandler();
	RepoScene scene;

	EXPECT_FALSE(scene.updateRevisionStatus(handler.get(), ModelRevisionNode::UploadStatus::GEN_DEFAULT));
	EXPECT_FALSE(scene.updateRevisionStatus(nullptr, ModelRevisionNode::UploadStatus::GEN_DEFAULT));

	scene = RepoScene(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ);
	std::string errMsg;
	scene.loadRevision(handler.get(), errMsg);
	EXPECT_TRUE(scene.updateRevisionStatus(handler.get(), ModelRevisionNode::UploadStatus::GEN_DEFAULT));
	EXPECT_TRUE(scene.updateRevisionStatus(handler.get(), ModelRevisionNode::UploadStatus::COMPLETE));
}

TEST(RepoSceneTest, abandonChild)
{
	RepoNodeSet transNodes, meshNodes, empty;

	auto root = new TransformationNode(makeTransformationNode(getRandomString(rand() % 10 + 1)));

	auto m1 = new MeshNode(makeMeshNode(root->getSharedID()));

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

	scene.abandonChild(RepoScene::GraphType::DEFAULT, repo::lib::RepoUUID::createUUID(), m1, false, false); //shoudln't work with unrecognised parent
	scene.abandonChild(RepoScene::GraphType::DEFAULT, root->getSharedID(), nullptr, false, false); //shoudln't work with unrecognised parent
}

TEST(RepoSceneTest, addInheritance)
{
	RepoNodeSet transNodes, meshNodes, empty;

	auto root = new TransformationNode(makeTransformationNode(getRandomString(rand() % 10 + 1)));

	auto m1 = new MeshNode(makeMeshNode(root->getSharedID()));
	auto m2 = new MeshNode(makeMeshNode(root->getSharedID()));

	transNodes.insert(root);
	meshNodes.insert(m1);
	meshNodes.insert(m2);

	RepoScene scene(std::vector<std::string>(), empty, meshNodes, empty, empty, empty, transNodes);

	scene.addInheritance(RepoScene::GraphType::DEFAULT, nullptr, nullptr); //nothing should happen and should not crash
	scene.addInheritance(RepoScene::GraphType::DEFAULT, repo::lib::RepoUUID::createUUID(), repo::lib::RepoUUID::createUUID()); //nothing should happen and should not crash

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

	auto root = new TransformationNode(makeTransformationNode(getRandomString(rand() % 10 + 1)));

	auto m1 = new MeshNode(makeMeshNode(root->getSharedID()));
	auto m2 = new MeshNode(makeMeshNode(root->getSharedID()));

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
	EXPECT_EQ(0, scene.getChildrenAsNodes(RepoScene::GraphType::DEFAULT, repo::lib::RepoUUID::createUUID()).size());

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

	auto root = new TransformationNode(makeTransformationNode(getRandomString(rand() % 10 + 1)));

	auto m1 = new MeshNode(makeMeshNode(root->getSharedID()));
	auto m2 = new MeshNode(makeMeshNode(root->getSharedID()));

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
	auto handler = getHandler();
	RepoScene scene;
	EXPECT_FALSE(scene.getSceneFromReference(defaultG, repo::lib::RepoUUID::createUUID()));
	scene = RepoScene(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_FED);
	std::string errMsg;
	scene.loadScene(handler.get(), errMsg);
	ASSERT_TRUE(errMsg.empty());

	auto references = scene.getAllReferences(defaultG);
	ASSERT_TRUE(references.size());
	for (const auto &ref : references)
		EXPECT_TRUE(scene.getSceneFromReference(defaultG, ref->getSharedID()));
	EXPECT_FALSE(scene.getSceneFromReference(defaultG, repo::lib::RepoUUID::createUUID()));
}

TEST(RepoSceneTest, getTextureIDForMesh)
{
	RepoScene scene;
	EXPECT_TRUE(scene.getTextureIDForMesh(defaultG, repo::lib::RepoUUID::createUUID()).empty());

	RepoNodeSet transNodes, meshNodes, empty, matNodes, texNodes;

	auto root = new TransformationNode(makeTransformationNode(getRandomString(rand() % 10 + 1)));
	auto m1 = new MeshNode(makeMeshNode(root->getSharedID()));
	auto m2 = new MeshNode(makeMeshNode(root->getSharedID()));
	auto mat1 = new MaterialNode(makeMaterialNode(m1->getSharedID()));
	auto mat2 = new MaterialNode(makeMaterialNode(m2->getSharedID()));
	auto tex1 = new TextureNode(makeTextureNode(mat1->getSharedID()));

	transNodes.insert(root);
	meshNodes.insert(m1);
	meshNodes.insert(m2);

	matNodes.insert(mat1);
	matNodes.insert(mat2);
	texNodes.insert(tex1);

	auto scene2 = RepoScene(std::vector<std::string>(), meshNodes, matNodes, empty, texNodes, transNodes);

	EXPECT_EQ(tex1->getUniqueID().toString(), scene2.getTextureIDForMesh(defaultG, m1->getSharedID()));
	EXPECT_TRUE(scene2.getTextureIDForMesh(defaultG, m2->getSharedID()).empty());
}

TEST(RepoSceneTest, getAllNodes)
{
	RepoScene scene;
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

	RepoNodeSet transNodes, meshNodes, metaNodes, matNodes, texNodes, refNodes;

	auto root = new TransformationNode(makeTransformationNode(getRandomString(rand() % 10 + 1)));
	meshNodes.insert(new MeshNode(makeMeshNode(root->getSharedID())));
	meshNodes.insert(new MeshNode(makeMeshNode(root->getSharedID())));
	matNodes.insert(new MaterialNode(makeMaterialNode(root->getSharedID())));
	matNodes.insert(new MaterialNode(makeMaterialNode(root->getSharedID())));
	texNodes.insert(new TextureNode(makeTextureNode(root->getSharedID())));
	metaNodes.insert(new MetadataNode(makeMetadataNode(root->getSharedID())));
	metaNodes.insert(new MetadataNode(makeMetadataNode(root->getSharedID())));
	metaNodes.insert(new MetadataNode(makeMetadataNode(root->getSharedID())));
	refNodes.insert(new ReferenceNode(makeReferenceNode(root->getSharedID())));

	transNodes.insert(root);

	auto scene2 = RepoScene(std::vector<std::string>(), meshNodes, matNodes, metaNodes, texNodes, transNodes, refNodes);

	auto allSharedIDs = scene2.getAllSharedIDs(defaultG);
	EXPECT_EQ(0, scene2.getAllSharedIDs(RepoScene::GraphType::OPTIMIZED).size());

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
}

TEST(RepoSceneTest, getAllDescendantsByType)
{
	RepoScene scene;

	RepoNodeSet transNodes, meshNodes, empty, matNodes, texNodes;

	auto root = new TransformationNode(makeTransformationNode(getRandomString(rand() % 10 + 1)));
	auto trans2 = new TransformationNode(makeTransformationNode(root->getSharedID()));
	auto m1 = new MeshNode(makeMeshNode(root->getSharedID()));
	auto m2 = new MeshNode(makeMeshNode(trans2->getSharedID()));
	auto mat1 = new MaterialNode(makeMaterialNode(m1->getSharedID()));
	auto mat2 = new MaterialNode(makeMaterialNode(m2->getSharedID()));
	auto tex1 = new TextureNode(makeTextureNode(mat1->getSharedID()));

	transNodes.insert(root);
	transNodes.insert(trans2);
	meshNodes.insert(m1);
	meshNodes.insert(m2);

	matNodes.insert(mat1);
	matNodes.insert(mat2);
	texNodes.insert(tex1);

	auto scene2 = RepoScene(std::vector<std::string>(), meshNodes, matNodes, empty, texNodes, transNodes);
	auto trans = scene2.getAllDescendantsByType(defaultG, root->getSharedID(), NodeType::TRANSFORMATION);
	ASSERT_EQ(1, trans.size());
	EXPECT_EQ(trans2, trans[0]);
	EXPECT_EQ(0, scene2.getAllDescendantsByType(defaultG, trans2->getSharedID(), NodeType::TRANSFORMATION).size());

	auto meshes = scene2.getAllDescendantsByType(defaultG, root->getSharedID(), NodeType::MESH);
	ASSERT_EQ(2, meshes.size());
	EXPECT_FALSE(std::find(meshes.begin(), meshes.end(), m1) == meshes.end());
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
	auto handler = getHandler();
	std::string errMsg;
	RepoScene scene(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ);
	scene.loadScene(handler.get(), errMsg);
	auto bb = scene.getSceneBoundingBox();
	EXPECT_THAT(bb, Eq(getGoldenDataForBBoxTest()));
}

TEST(RepoSceneTest, getNodeBySharedID)
{
	RepoNodeSet transNodes, meshNodes, empty, matNodes, texNodes;

	auto root = new TransformationNode(makeTransformationNode(getRandomString(rand() % 10 + 1)));
	auto trans2 = new TransformationNode(makeTransformationNode(root->getSharedID()));
	auto m1 = new MeshNode(makeMeshNode(root->getSharedID()));
	auto m2 = new MeshNode(makeMeshNode(trans2->getSharedID()));
	auto mat1 = new MaterialNode(makeMaterialNode(m1->getSharedID()));
	auto mat2 = new MaterialNode(makeMaterialNode(m2->getSharedID()));
	auto tex1 = new TextureNode(makeTextureNode(mat1->getSharedID()));

	transNodes.insert(root);
	transNodes.insert(trans2);
	meshNodes.insert(m1);
	meshNodes.insert(m2);

	matNodes.insert(mat1);
	matNodes.insert(mat2);
	texNodes.insert(tex1);

	auto scene2 = RepoScene(std::vector<std::string>(), meshNodes, matNodes, empty, texNodes, transNodes);

	EXPECT_EQ(root, scene2.getNodeBySharedID(defaultG, root->getSharedID()));
	EXPECT_EQ(nullptr, scene2.getNodeBySharedID(defaultG, repo::lib::RepoUUID::createUUID()));
}

TEST(RepoSceneTest, getNodeByUniqueID)
{
	RepoNodeSet transNodes, meshNodes, empty, matNodes, texNodes;

	auto root = new TransformationNode(makeTransformationNode(getRandomString(rand() % 10 + 1)));
	auto trans2 = new TransformationNode(makeTransformationNode(root->getSharedID()));
	auto m1 = new MeshNode(makeMeshNode(root->getSharedID()));
	auto m2 = new MeshNode(makeMeshNode(trans2->getSharedID()));
	auto mat1 = new MaterialNode(makeMaterialNode(m1->getSharedID()));
	auto mat2 = new MaterialNode(makeMaterialNode(m2->getSharedID()));
	auto tex1 = new TextureNode(makeTextureNode(mat1->getSharedID()));

	transNodes.insert(root);
	transNodes.insert(trans2);
	meshNodes.insert(m1);
	meshNodes.insert(m2);

	matNodes.insert(mat1);
	matNodes.insert(mat2);
	texNodes.insert(tex1);

	auto scene2 = RepoScene(std::vector<std::string>(), meshNodes, matNodes, empty, texNodes, transNodes);

	EXPECT_EQ(root, scene2.getNodeByUniqueID(defaultG, root->getUniqueID()));
	EXPECT_EQ(nullptr, scene2.getNodeBySharedID(defaultG, repo::lib::RepoUUID::createUUID()));
}

TEST(RepoSceneTest, hasRoot)
{
	RepoScene scene;
	EXPECT_FALSE(scene.hasRoot(defaultG));
	EXPECT_FALSE(scene.hasRoot(RepoScene::GraphType::OPTIMIZED));

	RepoNodeSet transNodes, transNodesStash, empty;
	auto root = new TransformationNode(makeTransformationNode(getRandomString(rand() % 10 + 1)));
	auto rootStash = new TransformationNode(makeTransformationNode(getRandomString(rand() % 10 + 1)));
	transNodes.insert(root);
	transNodesStash.insert(rootStash);

	auto scene2 = RepoScene(std::vector<std::string>(), empty, empty, empty, empty, transNodes);
	EXPECT_TRUE(scene2.hasRoot(defaultG));
	EXPECT_FALSE(scene2.hasRoot(RepoScene::GraphType::OPTIMIZED));
	scene2.addStashGraph(empty, empty, empty, transNodesStash);
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
	auto root = new TransformationNode(makeTransformationNode(getRandomString(rand() % 10 + 1)));
	auto rootStash = new TransformationNode(makeTransformationNode(getRandomString(rand() % 10 + 1)));
	transNodes.insert(root);
	transNodesStash.insert(rootStash);

	auto scene2 = RepoScene(std::vector<std::string>(), empty, empty, empty, empty, transNodes);
	EXPECT_EQ(root, scene2.getRoot(defaultG));
	EXPECT_EQ(nullptr, scene2.getRoot(RepoScene::GraphType::OPTIMIZED));
	scene2.addStashGraph(empty, empty, empty, transNodesStash);
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
	auto root = new TransformationNode(makeTransformationNode(getRandomString(rand() % 10 + 1)));
	auto rootStash = new TransformationNode(makeTransformationNode(getRandomString(rand() % 10 + 1)));
	transNodes.insert(root);
	transNodesStash.insert(rootStash);

	auto scene2 = RepoScene(std::vector<std::string>(), empty, empty, empty, empty, transNodes);
	EXPECT_EQ(1, scene2.getItemsInCurrentGraph(defaultG));
	EXPECT_EQ(0, scene2.getItemsInCurrentGraph(RepoScene::GraphType::OPTIMIZED));
	scene2.addStashGraph(empty, empty, empty, transNodesStash);
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

	auto scene3 = RepoScene(orgFiles, empty, empty, empty, empty, empty);
	auto orgFilesOut = scene3.getOriginalFiles();
	ASSERT_EQ(orgFiles.size(), orgFilesOut.size());

	for (int i = 0; i < orgFilesOut.size(); ++i)
	{
		EXPECT_TRUE(std::find(orgFilesOut.begin(), orgFilesOut.end(), orgFiles[i]) != orgFilesOut.end());
	}

	auto handler = getHandler();
	auto scene4 = RepoScene(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ);
	std::string errMsg;
	scene4.loadScene(handler.get(), errMsg);
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

	newNodes.push_back(new TransformationNode(makeTransformationNode(getRandomString(rand() % 10 + 1))));
	newNodes.push_back(new TransformationNode(makeTransformationNode(newNodes.back()->getSharedID())));
	newNodes.push_back(new MeshNode(makeMeshNode(newNodes.back()->getSharedID())));
	newNodes.push_back(new MeshNode(makeMeshNode(newNodes.back()->getSharedID())));
	newNodes.push_back(new MaterialNode(makeMaterialNode(newNodes.back()->getSharedID())));
	newNodes.push_back(new MaterialNode(makeMaterialNode(newNodes.back()->getSharedID())));
	newNodes.push_back(new TextureNode(makeTextureNode(newNodes.back()->getSharedID())));

	scene.addNodes(newNodes);
	EXPECT_EQ(newNodes.size(), scene.getItemsInCurrentGraph(defaultG));

	int currentSize = newNodes.size();
	newNodes.clear();
	newNodes.push_back(new TransformationNode(makeTransformationNode(scene.getRoot(defaultG)->getSharedID(), getRandomString(rand() % 10 + 1))));
	newNodes.push_back(new TransformationNode(makeTransformationNode(newNodes.back()->getSharedID())));
	newNodes.push_back(new MeshNode(makeMeshNode(newNodes.back()->getSharedID())));
	newNodes.push_back(new MeshNode(makeMeshNode(newNodes.back()->getSharedID())));
	newNodes.push_back(new MaterialNode(makeMaterialNode(newNodes.back()->getSharedID())));
	newNodes.push_back(new MaterialNode(makeMaterialNode(newNodes.back()->getSharedID())));
	newNodes.push_back(new TextureNode(makeTextureNode(newNodes.back()->getSharedID())));

	scene.addNodes(newNodes);
	EXPECT_EQ(newNodes.size() + currentSize, scene.getItemsInCurrentGraph(defaultG));
}

TEST(RepoSceneTest, removeNode)
{
	RepoScene scene;
	scene.removeNode(defaultG, repo::lib::RepoUUID::createUUID());
	auto root = new TransformationNode(makeTransformationNode(getRandomString(rand() % 10 + 1)));
	scene.addNodes({ root });
	scene.removeNode(defaultG, root->getSharedID());
	scene.removeNode(defaultG, root->getSharedID());

	delete root;
}

TEST(RepoSceneTest, resetChangeSet)
{
	auto handler = getHandler();
	RepoScene scene;
	scene.resetChangeSet();

	auto scene2 = RepoScene(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ);
	std::string errMsg;
	scene2.loadScene(handler.get(), errMsg);
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
	auto root = new TransformationNode(RepoBSONFactory::makeTransformationNode());
	auto rootStash = new TransformationNode(RepoBSONFactory::makeTransformationNode());
	transNodes.insert(root);
	transNodesStash.insert(rootStash);

	ASSERT_TRUE(root->isIdentity());
	ASSERT_TRUE(rootStash->isIdentity());
	auto scene2 = RepoScene(std::vector<std::string>(), empty, empty, empty, empty, transNodes);
	scene2.addStashGraph(empty, empty, empty, transNodesStash);
	scene2.reorientateDirectXModel();
	std::vector<float> rotatedMat =
	{
		1, 0, 0, 0,
		0, 0, 1, 0,
		0, -1, 0, 0,
		0, 0, 0, 1
	};

	EXPECT_EQ(repo::lib::RepoMatrix(rotatedMat), root->getTransMatrix());
	EXPECT_FALSE(scene2.hasRoot(RepoScene::GraphType::OPTIMIZED));
}