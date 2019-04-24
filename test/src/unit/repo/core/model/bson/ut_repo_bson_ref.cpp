/**
*  Copyright (C) 2019 3D Repo Ltd
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

#include <repo/core/model/bson/repo_bson_ref.h>
#include <repo/core/model/bson/repo_bson_factory.h>

using namespace repo::core::model;

TEST(RepoRefTest, OperatorEqualTest)
{
	RepoNode typicalNode = makeTypicalNode();
	EXPECT_NE(makeRandomNode(), typicalNode);
	EXPECT_NE(makeRandomNode(), makeRandomNode());
	EXPECT_EQ(typicalNode, typicalNode);

	repo::lib::RepoUUID sharedID = repo::lib::RepoUUID::createUUID();
	EXPECT_NE(makeNode(repo::lib::RepoUUID::createUUID(), sharedID), makeNode(repo::lib::RepoUUID::createUUID(), sharedID));
	EXPECT_NE(makeNode(sharedID, repo::lib::RepoUUID::createUUID()), makeNode(sharedID, repo::lib::RepoUUID::createUUID()));
}
