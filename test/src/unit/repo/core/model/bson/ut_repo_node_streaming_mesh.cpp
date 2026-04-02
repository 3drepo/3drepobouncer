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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>

#include <repo/core/model/bson/repo_node_streaming_mesh.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include "../../../../repo_test_mesh_utils.h"

using namespace repo::core::model;
using namespace testing;
using namespace repo::test::utils::mesh;

TEST(StreamingMeshNodeTest, EmptyConstructor)
{
	StreamingMeshNode empty;

	// Check basic data that should be accessible
	// though only containing default values
	EXPECT_EQ(empty.getNumVertices(), 0);
	EXPECT_TRUE(empty.getSharedId().isDefaultValue());
	EXPECT_TRUE(empty.getParent().isDefaultValue());	
	EXPECT_EQ(empty.getBoundingBox(), repo::lib::RepoBounds());

	// Check data that should not be accessible yet
	EXPECT_TRUE(empty.getUniqueId().isDefaultValue());
	EXPECT_EQ(empty.getNumLoadedFaces(), 0);	
	EXPECT_EQ(empty.getLoadedFaces().size(), 0);
	EXPECT_EQ(empty.getNumLoadedVertices(), 0);
	EXPECT_EQ(empty.getLoadedVertices().size(), 0);
	EXPECT_EQ(empty.getLoadedNormals().size(), 0);
	EXPECT_EQ(empty.getLoadedUVChannelsSeparated().size(), 0);
}

TEST(StreamingMeshNodeTest, LoadBasicStreamingMeshNode) {

	// Create mesh Data and StreamingMeshNode
	auto meshData = mesh_data(false, true, 3, 2, true, 1, 100, "");
	auto bson = meshNodeTestBSONFactory(meshData);
	StreamingMeshNode limited = StreamingMeshNode(bson);

	// Check basic data that should be accessible	
	EXPECT_EQ(limited.getNumVertices(), 100);
	EXPECT_EQ(limited.getSharedId(), meshData.sharedId);
	EXPECT_EQ(limited.getParent(), meshData.parents[0]);	
	EXPECT_EQ(limited.getBoundingBox(),
		repo::lib::RepoBounds(repo::lib::RepoVector3D(meshData.boundingBox[0]),
			repo::lib::RepoVector3D(meshData.boundingBox[1])));
	
	// Check data that should not be accessible yet
	EXPECT_TRUE(limited.getUniqueId().isDefaultValue());
	EXPECT_EQ(limited.getNumLoadedFaces(), 0);
	EXPECT_EQ(limited.getLoadedFaces().size(), 0);
	EXPECT_EQ(limited.getNumLoadedVertices(), 0);
	EXPECT_EQ(limited.getLoadedVertices().size(), 0);
	EXPECT_EQ(limited.getLoadedNormals().size(), 0);
	EXPECT_EQ(limited.getLoadedUVChannelsSeparated().size(), 0);
}

void TestLoadSupermeshingData(mesh_data meshData) {
	// Create mesh data	
	auto bson = meshNodeTestBSONFactory(meshData);

	// Create limited node
	StreamingMeshNode node = StreamingMeshNode(bson);

	// Check data that should not be accessible yet
	EXPECT_TRUE(node.getUniqueId().isDefaultValue());
	EXPECT_EQ(node.getNumLoadedFaces(), 0);
	EXPECT_EQ(node.getLoadedFaces().size(), 0);
	EXPECT_EQ(node.getNumLoadedVertices(), 0);
	EXPECT_EQ(node.getLoadedVertices().size(), 0);
	EXPECT_EQ(node.getLoadedNormals().size(), 0);
	EXPECT_EQ(node.getLoadedUVChannelsSeparated().size(), 0);

	// Inflate node
	auto data = bson.getBinariesAsBuffer();
	auto fakeRef = repo::core::handler::fileservice::DataRef("file", 0, data.second.size());
	bson.replaceBinaryWithReference(fakeRef.serialise(), data.first);
	node.loadSupermeshingData(bson, data.second, false);

	// Check the inflated node's data
	EXPECT_EQ(node.getUniqueId(), meshData.uniqueId);
	EXPECT_EQ(node.getNumLoadedFaces(), meshData.faces.size());
	EXPECT_EQ(node.getNumLoadedVertices(), meshData.vertices.size());
	EXPECT_THAT(node.getLoadedFaces(), ElementsAreArray(meshData.faces));
	EXPECT_THAT(node.getLoadedVertices(), ElementsAreArray(meshData.vertices));
	EXPECT_THAT(node.getLoadedNormals(), ElementsAreArray(meshData.normals));
	EXPECT_THAT(node.getLoadedUVChannelsSeparated(), ElementsAreArray(meshData.uvChannels));

	// Deflate node
	node.unloadSupermeshingData();

	// Check the deflated node's data
	// It should be back to the previous state		
	EXPECT_TRUE(node.getUniqueId().isDefaultValue());
	EXPECT_EQ(node.getNumLoadedFaces(), 0);
	EXPECT_EQ(node.getLoadedFaces().size(), 0);
	EXPECT_EQ(node.getNumLoadedVertices(), 0);
	EXPECT_EQ(node.getLoadedVertices().size(), 0);
	EXPECT_EQ(node.getLoadedNormals().size(), 0);
	EXPECT_EQ(node.getLoadedUVChannelsSeparated().size(), 0);
}

TEST(StreamingMeshNodeTest, LoadSupermeshingData) {
	TestLoadSupermeshingData(mesh_data(false, false, 0, 2, false, 0, 100, ""));
	TestLoadSupermeshingData(mesh_data(false, false, 0, 2, false, 1, 100, ""));
	TestLoadSupermeshingData(mesh_data(false, false, 0, 2, false, 2, 100, ""));
	TestLoadSupermeshingData(mesh_data(false, false, 0, 2, true, 0, 100, ""));
	TestLoadSupermeshingData(mesh_data(false, false, 0, 2, true, 1, 100, ""));
	TestLoadSupermeshingData(mesh_data(false, false, 0, 2, true, 2, 100, ""));
	TestLoadSupermeshingData(mesh_data(false, false, 0, 3, false, 0, 100, ""));
	TestLoadSupermeshingData(mesh_data(false, false, 0, 3, false, 1, 100, ""));
	TestLoadSupermeshingData(mesh_data(false, false, 0, 3, false, 2, 100, ""));
	TestLoadSupermeshingData(mesh_data(false, false, 0, 3, true, 0, 100, ""));
	TestLoadSupermeshingData(mesh_data(false, false, 0, 3, true, 1, 100, ""));
	TestLoadSupermeshingData(mesh_data(false, false, 0, 3, true, 2, 100, ""));

	TestLoadSupermeshingData(mesh_data(false, false, 0, 3, false, 0, 100, ""));
	TestLoadSupermeshingData(mesh_data(false, false, 1, 3, false, 0, 100, ""));
	TestLoadSupermeshingData(mesh_data(false, true, 0, 3, false, 0, 100, ""));
	TestLoadSupermeshingData(mesh_data(false, true, 1, 3, false, 0, 100, ""));
	TestLoadSupermeshingData(mesh_data(true, false, 0, 3, false, 0, 100, ""));
	TestLoadSupermeshingData(mesh_data(true, false, 1, 3, false, 0, 100, ""));
	TestLoadSupermeshingData(mesh_data(true, true, 1, 3, false, 0, 100, ""));

	TestLoadSupermeshingData(mesh_data(true, true, 3, 3, false, 0, 100, ""));
}

TEST(StreamingMeshNodeTest, TransformBounds) {
	// Create mesh Data and StreamingMeshNode
	auto meshData = mesh_data(false, true, 3, 2, true, 1, 100, "");
	auto bson = meshNodeTestBSONFactory(meshData);
	StreamingMeshNode limited = StreamingMeshNode(bson);

	// Create matrix
	auto m = repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D(10, 0, 0))
		* repo::lib::RepoMatrix::rotationX(0.12f)
		* repo::lib::RepoMatrix::rotationY(0.8f)
		* repo::lib::RepoMatrix::rotationZ(1.02f);

	// Get bounding box from the mesh data
	
	auto vMin = repo::lib::RepoVector3D64(
		meshData.boundingBox[0][0],
		meshData.boundingBox[0][1],
		meshData.boundingBox[0][2]);
	auto vMax = repo::lib::RepoVector3D64(
		meshData.boundingBox[1][0],
		meshData.boundingBox[1][1],
		meshData.boundingBox[1][2]);
	
	// Transform bounding box from the mesh data
	auto vMinTransformed = m * vMin;
	auto vMaxTransformed = m * vMax;
	auto boundsTransformed = repo::lib::RepoBounds(vMinTransformed, vMaxTransformed);

	// Transform via StreamingMeshNode and compare
	limited.transformBounds(m);	
	EXPECT_EQ(limited.getBoundingBox(), boundsTransformed);
}
TEST(StreamingMeshNodeTest, BakeMeshData) {
	// Create mesh Data and StreamingMeshNode
	auto meshData = mesh_data(false, true, 3, 2, true, 1, 100, "");
	auto bson = meshNodeTestBSONFactory(meshData);
	StreamingMeshNode node = StreamingMeshNode(bson);

	// Create matrix
	auto m = repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D(10, 0, 0))
		* repo::lib::RepoMatrix::rotationX(0.12f)
		* repo::lib::RepoMatrix::rotationY(0.8f)
		* repo::lib::RepoMatrix::rotationZ(1.02f);

	// Apply matrix to vertices
	std::vector<repo::lib::RepoVector3D> transformedVertices;
	for (auto v : meshData.vertices) {
		transformedVertices.push_back(m * v);
	}

	// Apply matrix to normals
	auto matInverse = m.inverse();
	auto worldMat = matInverse.transpose();
	auto mData = worldMat.getData();
	mData[3] = mData[7] = mData[11] = 0;
	mData[12] = mData[13] = mData[14] = 0;
	repo::lib::RepoMatrix multMat(mData);
	std::vector<repo::lib::RepoVector3D> transformedNormals;
	for (auto v : meshData.normals) {
		auto n = multMat * v;
		n.normalize();
		transformedNormals.push_back(n);
	}

	// Inflate node
	auto data = bson.getBinariesAsBuffer();
	auto fakeRef = repo::core::handler::fileservice::DataRef("file", 0, data.second.size());
	bson.replaceBinaryWithReference(fakeRef.serialise(), data.first);
	node.loadSupermeshingData(bson, data.second, false);

	// Apply transformation for baking
	node.bakeLoadedMeshes(m);
		
	// Check the transformed data
	EXPECT_THAT(node.getLoadedVertices(), ElementsAreArray(transformedVertices));
	EXPECT_THAT(node.getLoadedNormals(), ElementsAreArray(transformedNormals));
}