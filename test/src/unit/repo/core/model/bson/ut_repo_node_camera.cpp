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
	EXPECT_TRUE(compareVectors({ 0, 0, -1 }, camEmpty.getLookAt()));
	EXPECT_TRUE(compareVectors({ 0, 1, 0 }, camEmpty.getUp()));
	EXPECT_TRUE(compareVectors({ 0, 0, 0 }, camEmpty.getPosition()));
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

TEST(CameraTest, GetOrientation)
{

	CameraNode camEmpty;
	std::vector<float> expectedEmptyVector = { 0, 0, 0, 1 };
	EXPECT_TRUE(compareStdVectors(expectedEmptyVector, camEmpty.getOrientation()));

	repo_vector_t lookat = { 30, 54, 13.235 };
	repo_vector_t position = { 1.234, 23450, 356.43 };
	repo_vector_t up = { 0.01345, 1.0, 0.5654 };
	auto camNode = RepoBSONFactory::makeCameraNode(0.4, 0.6, 0.1, 0.9, lookat, position, up);
	std::vector<float> expectedOrientation = { 0.82826995849609375f, -0.54004347324371338f, -0.14940512180328369f, 1.62119138240814210f };
	EXPECT_TRUE(compareStdVectors(expectedOrientation, camNode.getOrientation()));

}

TEST(CameraTest, GetCameraMatrix)
{
	CameraNode emptyCam;
	std::vector<float> goldenDataEmpty = 
	{
		-1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, -1, 0,
		0, 0, 0, 1
	};

	EXPECT_TRUE(compareStdVectors(goldenDataEmpty, emptyCam.getCameraMatrix(false)));
	EXPECT_TRUE(compareStdVectors(goldenDataEmpty, emptyCam.getCameraMatrix(true)));

	repo_vector_t lookat = { 30, 54, 13.235 };
	repo_vector_t position = { 1.234, 23450, 356.43 };
	repo_vector_t up = { 0.01345, 1.0, 0.5654 };
	auto camNode = RepoBSONFactory::makeCameraNode(0.4, 0.6, 0.1, 0.9, lookat, position, up);

	auto matrix = camNode.getCameraMatrix(true);

	std::vector<float> camMat1 = 
	{
		-0.456150203943253f, 0.011707351543009f, 0.474866360425949f, 0.000000000000000f,
		0.442631483078003f, 0.870435059070587f, 0.854759454727173f, 0.000000000000000f,
		-0.772013247013092f, 0.492143988609314f, 0.209495201706886f, 0.000000000000000f,
		-10103.976562500000000f, -20587.128906250000000f, -20119.365234375000000f, 1.000000000000000f
	};


	std::vector<float> camMat2 =
	{
		-0.456150203943253f, 0.442631483078003f, -0.772013247013092f, -10103.976562500000000f,
		0.011707351543009f, 0.870435059070587f, 0.492143988609314f, -20587.128906250000000f,
		0.474866360425949f, 0.854759454727173f, 0.209495201706886f, -20119.365234375000000f,
		0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 1.000000000000000f
	};


	EXPECT_TRUE(compareStdVectors(camMat1, camNode.getCameraMatrix(false)));
	EXPECT_TRUE(compareStdVectors(camMat2, camNode.getCameraMatrix(true)));
}



TEST(CameraTest, SEqual)
{
	CameraNode emptyCam;
	EXPECT_TRUE(emptyCam.sEqual(emptyCam));

	float aspRat = rand() % 1000 / 1000.;
	float fov = rand() % 1000 / 1000.;
	float nearClip = rand() % 1000 / 1000.;
	float farClip = rand() % 1000 / 1000.;
	std::string name = getRandomString(10);
	repo_vector_t lookat = { rand() % 1000 / 1000., rand() % 1000 / 1000., rand() % 1000 / 1000. };
	repo_vector_t position = { rand() % 1000 / 1000., rand() % 1000 / 1000., rand() % 1000 / 1000. };
	repo_vector_t up = { rand() % 1000 / 1000., rand() % 1000 / 1000., rand() % 1000 / 1000. };
	auto camNode = RepoBSONFactory::makeCameraNode(aspRat, farClip, nearClip, fov, lookat, position, up, name);
	auto camNode2 = RepoBSONFactory::makeCameraNode(aspRat, farClip, nearClip, fov, lookat, position, up, name);

	EXPECT_TRUE(camNode.sEqual(camNode2));
	EXPECT_TRUE(camNode2.sEqual(camNode));

	auto camNode3 = RepoBSONFactory::makeCameraNode(1.3, 1, 0.1, 1, { 1, 2, 3 }, position, up, name);
	EXPECT_FALSE(camNode.sEqual(camNode3));
	EXPECT_FALSE(emptyCam.sEqual(camNode3));



}