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

#include <repo/core/model/bson/repo_node_drawing_revision.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include <repo/core/model/bson/repo_bson_factory.h>

#include "../../../../repo_test_utils.h"

using namespace repo::core::model;

TEST(DrawingRevisionNodeTest, Node)
{
	// Bouncer is never intended to create a Drawing Revision Node, so for this
	// test we mock up a BSON using the expected schema. Note that not all
	// fields are required by bouncer so not all have accessors implemented.

	auto model = repo::lib::RepoUUID::createUUID().toString();
	auto project = repo::lib::RepoUUID::createUUID();
	auto file = repo::lib::RepoUUID::createUUID();
	auto format = "dwg";

	RepoBSONBuilder builder;
	builder.append("model", model);
	builder.append("project", project);
	builder.append("format", format);
	std::vector< repo::lib::RepoUUID> files;
	files.push_back(file);
	builder.appendArray("rFile", files);

	DrawingRevisionNode revision(builder.obj());

	// Check that the methods we support work

	EXPECT_EQ(revision.getFiles().size(), 1);
	EXPECT_EQ(revision.getFiles()[0], file);
	EXPECT_EQ(revision.getProject(), project);
	EXPECT_EQ(revision.getModel(), model);
	EXPECT_EQ(revision.getFormat(), format);

	// Check that clone and add image works

	auto image = repo::lib::RepoUUID::createUUID();
	auto revision1 = revision.cloneAndAddImage(image);
	EXPECT_EQ(revision1.getUUIDField("image"), image);

	// Check that clone and add image overwrites existing images

	image = repo::lib::RepoUUID::createUUID();
	auto revision2 = revision1.cloneAndAddImage(image);
	EXPECT_EQ(revision2.getUUIDField("image"), image);
}