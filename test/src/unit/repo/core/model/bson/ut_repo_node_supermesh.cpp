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
#include <gtest/gtest-matchers.h>

#include <repo/core/model/bson/repo_node_mesh.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include <repo/core/model/bson/repo_bson_factory.h>

#include "../../../../repo_test_utils.h"
#include "../../../../repo_test_mesh_utils.h"
#include "../../../../repo_test_matchers.h"

using namespace repo::core::model;
using namespace repo::test::utils::mesh;
using namespace testing;

/**
* SupermeshNode is special in that it is expected to exist only at runtime; that
* is, it is not written to the database and cannot serialise itself.
* It may deserialise itself, for legacy reasons only.
*/

repo_mesh_mapping_t makeMeshMapping()
{
	repo_mesh_mapping_t m;
	m.material_id = repo::lib::RepoUUID::createUUID();
	m.max = repo::lib::RepoVector3D(rand() - rand(), rand() - rand(), rand() - rand());
	m.min = repo::lib::RepoVector3D(rand() - rand(), rand() - rand(), rand() - rand());
	m.triFrom = rand();
	m.triFrom = m.triFrom + rand();
	m.vertFrom = rand();
	m.vertTo = m.vertFrom + rand();
	m.mesh_id = repo::lib::RepoUUID::createUUID();
	m.shared_id = repo::lib::RepoUUID::createUUID();
	return m;
}

std::vector<float> makeSubmeshIds(int num)
{
	std::vector<float> uvs;
	for (int i = 0; i < num; i++) {
		uvs.push_back(rand());
	}
	return uvs;
}

TEST(SupermeshNodeTest, Constructor)
{
	SupermeshNode node;

	EXPECT_THAT(node.getVertices(), IsEmpty());
	EXPECT_THAT(NodeType::MESH, node.getTypeAsEnum());
	EXPECT_THAT(MeshNode::Primitive::TRIANGLES, node.getPrimitive());
	EXPECT_THAT(node.getMeshMapping(), IsEmpty());
	EXPECT_THAT(node.getSubmeshIds(), IsEmpty());
}

bool operator== (const repo_mesh_mapping_t& a, const repo_mesh_mapping_t& b)
{
	return
		a.min == b.min &&
		a.max == b.max &&
		a.mesh_id == b.mesh_id &&
		a.shared_id == b.shared_id &&
		a.material_id == b.material_id &&
		a.vertFrom == b.vertFrom &&
		a.vertTo == b.vertTo &&
		a.triFrom == b.triFrom &&
		a.triTo == b.triTo;
}

TEST(SupermeshNodeTest, Methods)
{
	SupermeshNode node;

	auto mappings1 = std::vector<repo_mesh_mapping_t>({
		makeMeshMapping(),
		makeMeshMapping(),
		makeMeshMapping(),
	});

	auto mappings2 = std::vector<repo_mesh_mapping_t>({
		makeMeshMapping(),
		makeMeshMapping(),
		makeMeshMapping(),
	});

	node.setMeshMapping(mappings1);
	// EXPECT_THAT(node.getMeshMapping(), ElementsAreArray(mappings1));

	node.setMeshMapping(mappings2);
	// EXPECT_THAT(node.getMeshMapping(), Eq(mappings2));

	auto ids = makeSubmeshIds(1000);
	node.setSubmeshIds(ids);
	EXPECT_THAT(node.getSubmeshIds(), Eq(ids));
}
