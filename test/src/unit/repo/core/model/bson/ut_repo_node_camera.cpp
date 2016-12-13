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