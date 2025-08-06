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

#include "repo_test_common_tests.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>

using namespace testing;
using namespace testing::common;

void testing::common::checkMetadataInheritence(SceneUtils& scene)
{
	for (auto& metadata : scene.getMetadataNodes())
	{
		auto meshParents = metadata.getParents({ repo::core::model::NodeType::MESH });

		for (auto& parent : metadata.getParents({ repo::core::model::NodeType::TRANSFORMATION }))
		{
			// When a metadata node is a child of transformation leaf node (i.e.
			// with only unnamed meshes), it should also be a child of those mesh
			// nodes.

			if (parent.isLeaf())
			{
				auto meshSiblings = parent.getMeshes();
				EXPECT_THAT(meshSiblings, IsSubsetOf(meshParents));
			}
		}
	}
}