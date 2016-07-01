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

using namespace repo::core::model;

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
//
//TEST(RepoSceneTest, AddMetadata)
//{
//	RepoNodeSet transNodes;
//	RepoNodeSet meshNodes;
//
//	auto root = TransformationNode(makeRandomNode());
//
//	auto t1 = TransformationNode(makeRandomNode(root.getSharedID));
//	auto t2 = TransformationNode(makeRandomNode(root.getSharedID));
//	auto t3 = TransformationNode(makeRandomNode(root.getSharedID));
//
//	auto m1 = MeshNode(makeRandomNode(t1.getSharedID()));
//	auto m2 = MeshNode(makeRandomNode(t1.getSharedID()));
//	auto m3 = MeshNode(makeRandomNode(t2.getSharedID()));
//}