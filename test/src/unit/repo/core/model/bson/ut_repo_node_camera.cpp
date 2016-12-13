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

#include <repo/core/model/bson/repo_node_camera.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include <repo/core/model/bson/repo_bson_factory.h>

#include "../../../../repo_test_utils.h"

using namespace repo::core::model;


TEST(CameraTest, Constructor)
{
	CameraNode empty;

	EXPECT_TRUE(empty.isEmpty());
	EXPECT_EQ(NodeType::CAMERA, empty.getTypeAsEnum());

	auto repoBson = RepoBSON(BSON("test" << "blah" << "test2" << 2));

	auto fromRepoBSON = CameraNode(repoBson);
	EXPECT_EQ(NodeType::CAMERA, fromRepoBSON.getTypeAsEnum());
	EXPECT_EQ(fromRepoBSON.nFields(), repoBson.nFields());
	EXPECT_EQ(0, fromRepoBSON.getFileList().size());
}

TEST(CameraTest, TypeTest)
{
	CameraNode node;

	EXPECT_EQ(REPO_NODE_TYPE_CAMERA, node.getType());
	EXPECT_EQ(NodeType::CAMERA, node.getTypeAsEnum());
}

TEST(CameraTest, PositionDependant)
{
	CameraNode empty;
	EXPECT_TRUE(empty.positionDependant());
}

TEST(CameraTest, CloneAndApplyTransformation)
{
	std::vector<float> empty, identity, random;

	identity = {
		1, 0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1
	};

	random = {
		2, 0.5f, 0.45f, -1032,
		0.6f, 1, 0.3f, 1032,
		0.4f, 0.4f, 2, -1034,
		0, 0, 0, 1
	};

	CameraNode camEmpty;
	camEmpty.cloneAndApplyTransformation(empty);
	repo_vector_t lookat = { 30, 54, 13.235 };
	repo_vector_t position = { 1.234, 23450, 356.43 };
	repo_vector_t up = { 0.01345, 1.0, 0.5654 };
	auto camNode = RepoBSONFactory::makeCameraNode(1.0, 1.0, 1.0, 1.0, lookat, position, up);

	CameraNode sameNode = camNode.cloneAndApplyTransformation(identity);
	compareVectors(lookat, sameNode.getLookAt());
	compareVectors(position, sameNode.getPosition());
	compareVectors(up, sameNode.getUp());

	CameraNode transformedNode = camNode.cloneAndApplyTransformation(random);
	compareVectors({ -939.044, 1107.971, -973.93 }, transformedNode.getLookAt());
	compareVectors({ 10855.86, 24589.67, 9059.354 }, transformedNode.getPosition());
	compareVectors({ -1031.22, 1033.178, -1032.46 }, transformedNode.getUp());
}

TEST(CameraTest, GetFields)
{
	CameraNode camEmpty;
	EXPECT_EQ(1.0, camEmpty.getAspectRatio());
	EXPECT_EQ(1.0, camEmpty.getHorizontalFOV());
	EXPECT_EQ(1.0, camEmpty.getFieldOfView());
	EXPECT_EQ(1.0, camEmpty.getNearClippingPlane());
	EXPECT_EQ(1.0, camEmpty.getFarClippingPlane());
	EXPECT_TRUE(compareVectors({ 0, 0, -1}, camEmpty.getLookAt()));
	EXPECT_TRUE(compareVectors({ 0, 1,  0}, camEmpty.getUp()));
	EXPECT_TRUE(compareVectors({ 0, 0,  0}, camEmpty.getPosition()));
	EXPECT_TRUE(camEmpty.getName().empty());

	float aspRat = rand() % 1000 / 1000.;
	float fov = rand() % 1000 / 1000.;
	float nearClip = rand() % 1000 / 1000.;
	float farClip = rand() % 1000 / 1000.;
	std::string name = getRandomString(10);
	repo_vector_t lookat = { rand() % 1000 / 1000., rand() % 1000 / 1000., rand() % 1000 / 1000. };
	repo_vector_t position = { rand() % 1000 / 1000., rand() % 1000 / 1000., rand() % 1000 / 1000. };
	repo_vector_t up = { rand() % 1000 / 1000., rand() % 1000 / 1000., rand() % 1000 / 1000. };
	auto camNode = RepoBSONFactory::makeCameraNode(aspRat, farClip, nearClip, fov, lookat, position, up, name);

	EXPECT_EQ(aspRat, camNode.getAspectRatio());
	EXPECT_EQ(fov, camNode.getHorizontalFOV());
	EXPECT_EQ(fov, camNode.getFieldOfView());
	EXPECT_EQ(nearClip, camNode.getNearClippingPlane());
	EXPECT_EQ(farClip, camNode.getFarClippingPlane());
	EXPECT_EQ(name, camNode.getName());

	EXPECT_TRUE(compareVectors(lookat, camNode.getLookAt()));
	EXPECT_TRUE(compareVectors(up, camNode.getUp()));
	EXPECT_TRUE(compareVectors(position, camNode.getPosition()));


}