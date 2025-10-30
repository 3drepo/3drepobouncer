/**
*  Copyright (C) 2024 3D Repo Ltd
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

#include "repo_test_utils.h"
#include "repo_test_mesh_utils.h"
#include "repo/core/model/bson/repo_bson_factory.h"
#include "repo/core/model/bson/repo_bson_builder.h"

#include <numbers>

using namespace repo::core::model;
using namespace testing;

std::unique_ptr<MeshNode> repo::test::utils::mesh::createRandomMesh(
	const int nVertices,
	const bool hasUV,
	const int primitiveSize,
	const std::string grouping,
	const std::vector<repo::lib::RepoUUID>& parent)
{
	auto mesh = makeMeshNode(mesh_data(true, true, 0, primitiveSize, false, hasUV ? 1 : 0, nVertices, grouping));
	mesh.addParents(parent);

	// The new mpOpt drops geometry that has no material, so we set the default here
	mesh.setMaterial(repo::lib::repo_material_t::DefaultMaterial());	

	return std::make_unique<MeshNode>(mesh);
}

bool repo::test::utils::mesh::compareMeshes(
	std::string database,
	std::string projectName,
	repo::lib::RepoUUID revId,
	TestModelExport* mockExporter
) 
{
	// Get processed geometry from the mock exporter
	auto processedFaces = getFacesFromMockExporter(mockExporter);

	// Get unprocessed geometry from the database
	auto unprocessedFaces = getFacesFromDatabase(database, projectName, revId);

	// Check for length equality first
	if (unprocessedFaces.size() != processedFaces.size())
	{
		return false;
	}

	// Faces are compared exactly using a hash table for speed.
	std::map<long, std::vector<GenericFace>> actual;

	for (auto& face : processedFaces)
	{
		actual[face.hash()].push_back(face);
	}

	for (auto& face : unprocessedFaces)
	{
		auto& others = actual[face.hash()];
		for (auto& other : others) {
			if (other.hit)
			{
				continue; // Each actual face may only be matched once
			}
			if (other.equals(face))
			{
				face.hit++;
				other.hit++;
				break;
			}
		}
	}

	// Did we find a match for all faces?

	for (const auto& face : unprocessedFaces)
	{
		if (!face.hit)
		{
			return false;
		}
	}

	return true;
}


// Helper method for getting binary data from the nodes in getFacesFromDatabase(...)
template <class T>
void deserialiseVector(
	const repo::core::model::RepoBSON& bson,
	const std::vector<uint8_t>& buffer,
	std::vector<T>& vec)
{
	auto start = bson.getLongField(REPO_LABEL_BINARY_START);
	auto size = bson.getLongField(REPO_LABEL_BINARY_SIZE);

	vec.resize(size / sizeof(T));
	memcpy(vec.data(), buffer.data() + (sizeof(uint8_t) * start), size);
}

std::vector<repo::test::utils::mesh::GenericFace> repo::test::utils::mesh::getFacesFromDatabase(std::string database, std::string projectName, repo::lib::RepoUUID revId)
{
	std::vector<GenericFace> genericFaces;
	
	// Assemble query for db.
	auto handler = getHandler();
	std::string sceneCollection = projectName + "." + REPO_COLLECTION_SCENE;
	repo::core::handler::fileservice::BlobFilesHandler blobHandler(handler->getFileManager(), database, sceneCollection);

	repo::core::handler::database::query::RepoQueryBuilder filter;
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_REVISION_ID, revId));
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_LABEL_TYPE, std::string(REPO_NODE_TYPE_MESH)));

	// One test inserts unknown primitive types in the database to see whether the mpOpt processes them. For a accurate comparison we need to ignore them.
	filter.append(repo::core::handler::database::query::Or(
		repo::core::handler::database::query::Eq(REPO_NODE_MESH_LABEL_PRIMITIVE, 2),
		repo::core::handler::database::query::Eq(REPO_NODE_MESH_LABEL_PRIMITIVE, 3)
	));

	// Query
	auto cursor = handler->findCursorByCriteria(database, sceneCollection, filter);

	// Process the results
	for (auto bson : (*cursor)) {

		std::vector<repo::lib::RepoVector3D> vertices;
		std::vector<repo::lib::RepoVector3D> normals;
		std::vector<repo::lib::repo_face_t> faces;
		std::vector < std::vector<repo::lib::RepoVector2D>> channels;

		auto binRef = bson.getBinaryReference();
		auto dataRef = repo::core::handler::fileservice::DataRef::deserialise(binRef);
		auto buffer = blobHandler.readToBuffer(dataRef);

		auto blobRefBson = bson.getObjectField(REPO_LABEL_BINARY_REFERENCE);
		auto elementsBson = blobRefBson.getObjectField(REPO_LABEL_BINARY_ELEMENTS);

		if (elementsBson.hasField(REPO_NODE_MESH_LABEL_VERTICES)) {
			auto vertBson = elementsBson.getObjectField(REPO_NODE_MESH_LABEL_VERTICES);
			deserialiseVector(vertBson, buffer, vertices);
		}

		if (elementsBson.hasField(REPO_NODE_MESH_LABEL_NORMALS)) {
			auto normBson = elementsBson.getObjectField(REPO_NODE_MESH_LABEL_NORMALS);
			deserialiseVector(normBson, buffer, normals);
		}

		if (bson.hasField(REPO_NODE_MESH_LABEL_UV_CHANNELS_COUNT))
		{
			// The new multipart optimiser drops UVs of meshes that have no texture,
			// so we need to make sure that we drop it here too.
			auto matFilterBson = bson.getObjectField(REPO_FILTER_OBJECT_NAME);
			if (matFilterBson.hasField(REPO_FILTER_PROP_TEXTURE_ID)) {
								
				std::vector<repo::lib::RepoVector2D> serialisedChannels;
				auto uvBson = elementsBson.getObjectField(REPO_NODE_MESH_LABEL_UV_CHANNELS);
				deserialiseVector(uvBson, buffer, serialisedChannels);

				if (serialisedChannels.size())
				{
					//get number of channels and split the serialised.
					uint32_t nChannels = bson.getIntField(REPO_NODE_MESH_LABEL_UV_CHANNELS_COUNT);
					uint32_t vecPerChannel = serialisedChannels.size() / nChannels;
					channels.reserve(nChannels);
					for (uint32_t i = 0; i < nChannels; i++)
					{
						channels.push_back(std::vector<repo::lib::RepoVector2D>());
						channels[i].reserve(vecPerChannel);

						uint32_t offset = i * vecPerChannel;
						channels[i].insert(channels[i].begin(), serialisedChannels.begin() + offset,
							serialisedChannels.begin() + offset + vecPerChannel);
					}
				}
			}
		}

		if (elementsBson.hasField(REPO_NODE_MESH_LABEL_FACES)) {

			int32_t faceCount = bson.getIntField(REPO_NODE_MESH_LABEL_FACES_COUNT);
			faces.reserve(faceCount);

			std::vector<uint32_t> serialisedFaces = std::vector<uint32_t>();
			auto faceBson = elementsBson.getObjectField(REPO_NODE_MESH_LABEL_FACES);
			deserialiseVector(faceBson, buffer, serialisedFaces);

			int mNumIndicesIndex = 0;
			while (serialisedFaces.size() > mNumIndicesIndex)
			{
				int mNumIndices = serialisedFaces[mNumIndicesIndex];
				if (serialisedFaces.size() > mNumIndicesIndex + mNumIndices)
				{
					repo::lib::repo_face_t face;
					face.resize(mNumIndices);
					for (int i = 0; i < mNumIndices; ++i)
						face[i] = serialisedFaces[mNumIndicesIndex + 1 + i];
					faces.push_back(face);
					mNumIndicesIndex += mNumIndices + 1;
				}
				else
				{
					repoError << "Cannot copy all faces. Buffer size is smaller than expected!";
				}
			}

		}

		// Add this mesh's faces
		addFaces(vertices, normals, faces, channels, genericFaces);

	}
	
	return genericFaces;
}

std::vector<repo::test::utils::mesh::GenericFace> repo::test::utils::mesh::getFacesFromMockExporter(TestModelExport* mockExporter)
{
	std::vector<GenericFace> genericFaces;

	auto meshes = mockExporter->getSupermeshes();

	for (auto& supermesh : meshes) {
		addFaces(supermesh, genericFaces);
	}

	return genericFaces;
}

void repo::test::utils::mesh::addFaces(repo::core::model::MeshNode &mesh, std::vector<GenericFace>& faces)
{
	auto vertices = mesh.getVertices();
	auto channels = mesh.getUVChannelsSeparated();
	auto normals = mesh.getNormals();

	for (const auto face : mesh.getFaces())
	{
		GenericFace portableFace;
		portableFace.hit = 0;

		for (const auto index : face)
		{
			portableFace.push(vertices[index]);

			for (const auto channel : channels)
			{
				portableFace.push(channel[index]);
			}

			if (normals.size())
			{
				portableFace.push(normals[index]);
			}
		}

		faces.push_back(portableFace);
	}
}

void repo::test::utils::mesh::addFaces(
	const std::vector<repo::lib::RepoVector3D>& vertices,
	const std::vector<repo::lib::RepoVector3D>& normals,
	const std::vector<repo::lib::repo_face_t>& faces,
	const std::vector<std::vector<repo::lib::RepoVector2D>> &uvChannels,
	std::vector<GenericFace>& genericFaces)
{
	for (const auto face : faces)
	{
		GenericFace portableFace;
		portableFace.hit = 0;

		for (const auto index : face)
		{
			portableFace.push(vertices[index]);

			for (const auto channel : uvChannels)
			{
				portableFace.push(channel[index]);
			}

			if (normals.size())
			{
				portableFace.push(normals[index]);
			}
		}

		genericFaces.push_back(portableFace);
	}
}

/**
* This implementation should be the reference for the node database schema and
* should be effectively independent, but equivalent, to the serialise method
* in MeshNode.
*/
RepoBSON repo::test::utils::mesh::meshNodeTestBSONFactory(mesh_data data)
{
	RepoBSONBuilder builder;

	builder.append(REPO_NODE_LABEL_ID, data.uniqueId);

	if (!data.sharedId.isDefaultValue())
	{
		builder.append(REPO_NODE_LABEL_SHARED_ID, data.sharedId);
	}

	builder.append(REPO_NODE_LABEL_TYPE, "mesh");

	if (data.parents.size() > 0)
	{
		builder.appendArray(REPO_NODE_LABEL_PARENTS, data.parents);
	}

	if (!data.name.empty())
	{
		builder.append(REPO_NODE_LABEL_NAME, data.name);
	}

	if (data.boundingBox.size() > 0)
	{
		builder.append(REPO_NODE_MESH_LABEL_BOUNDING_BOX, data.boundingBox);
	}

	if (data.vertices.size() > 0)
	{
		builder.append(REPO_NODE_MESH_LABEL_VERTICES_COUNT, (int32_t)(data.vertices.size()));
		builder.appendLargeArray(REPO_NODE_MESH_LABEL_VERTICES, data.vertices);
	}

	if (data.faces.size() > 0)
	{
		builder.append(REPO_NODE_MESH_LABEL_FACES_COUNT, (int32_t)(data.faces.size()));

		// In API LEVEL 1, faces are stored as
		// [n1, v1, v2, ..., n2, v1, v2...]

		MeshNode::Primitive primitive = MeshNode::Primitive::UNKNOWN;

		std::vector<uint32_t> facesLevel1;
		for (auto& face : data.faces) {
			auto nIndices = face.size();
			if (!nIndices)
			{
				repoWarning << "number of indices in this face is 0!";
			}
			if (primitive == MeshNode::Primitive::UNKNOWN) // The primitive type is unknown, so attempt to infer it
			{
				if (nIndices == 2) {
					primitive = MeshNode::Primitive::LINES;
				}
				else if (nIndices == 3) {
					primitive = MeshNode::Primitive::TRIANGLES;
				}
				else // The primitive type is not one we support
				{
					repoWarning << "unsupported primitive type - only lines and triangles are supported but this face has " << nIndices << " indices!";
				}
			}
			else  // (otherwise check for consistency with the existing type)
			{
				if (nIndices != static_cast<int>(primitive))
				{
					repoWarning << "mixing different primitives within a mesh is not supported!";
				}
			}
			facesLevel1.push_back(nIndices);
			for (uint32_t ind = 0; ind < nIndices; ind++)
			{
				facesLevel1.push_back(face[ind]);
			}
		}

		builder.append(REPO_NODE_MESH_LABEL_PRIMITIVE, static_cast<int>(primitive));

		builder.appendLargeArray(REPO_NODE_MESH_LABEL_FACES, facesLevel1);
	}

	if (data.normals.size() > 0)
	{
		builder.appendLargeArray(REPO_NODE_MESH_LABEL_NORMALS, data.normals);
	}

	if (data.uvChannels.size() > 0)
	{
		std::vector<repo::lib::RepoVector2D> concatenated;

		for (auto it = data.uvChannels.begin(); it != data.uvChannels.end(); ++it)
		{
			std::vector<repo::lib::RepoVector2D> channel = *it;

			std::vector<repo::lib::RepoVector2D>::iterator cit;
			for (cit = channel.begin(); cit != channel.end(); ++cit)
			{
				concatenated.push_back(*cit);
			}
		}

		if (concatenated.size() > 0)
		{
			// Could be unsigned __int64 if BSON had such construct (the closest is only __int64)
			builder.append(REPO_NODE_MESH_LABEL_UV_CHANNELS_COUNT, (int32_t)(data.uvChannels.size()));
			builder.appendLargeArray(REPO_NODE_MESH_LABEL_UV_CHANNELS, concatenated);
		}
	}

	// Grouping
	if (!data.grouping.empty()) {
		builder.append(REPO_NODE_MESH_LABEL_GROUPING, data.grouping);
	}

	return builder.obj();
}

MeshNode repo::test::utils::mesh::makeMeshNode(mesh_data data)
{
	return MeshNode(meshNodeTestBSONFactory(data));
}

MeshNode repo::test::utils::mesh::makeDeterministicMeshNode(int primitive, bool normals, int uvs)
{
	restartRand();
	return makeMeshNode(mesh_data(false, false, false, primitive, normals, uvs, 100, ""));
}

std::vector<repo::lib::RepoVector3D> repo::test::utils::mesh::makeVertices(int num)
{
	std::vector<repo::lib::RepoVector3D> vertices;
	for (int i = 0; i < num; i++) {
		vertices.push_back(repo::lib::RepoVector3D(rand() - rand(), rand() - rand(), rand() - rand()));
	}
	return vertices;
}

std::vector<repo::lib::RepoVector3D> repo::test::utils::mesh::makeNormals(int num)
{
	std::vector<repo::lib::RepoVector3D> normals;
	for (int i = 0; i < num; i++) {
		auto n = repo::lib::RepoVector3D(rand(), rand(), rand());
		n.normalize();
		normals.push_back(n);
	}
	return normals;
}

std::vector<repo::lib::RepoVector2D> repo::test::utils::mesh::makeUVs(int num)
{
	std::vector<repo::lib::RepoVector2D> uvs;
	for (int i = 0; i < num; i++) {
		uvs.push_back(repo::lib::RepoVector2D(rand(), rand()));
	}
	return uvs;
}

std::vector<repo::lib::repo_face_t> repo::test::utils::mesh::makeFaces(MeshNode::Primitive primitive)
{
	std::vector<repo::lib::repo_face_t> faces;
	for (int i = 0; i < 100; i++)
	{
		repo::lib::repo_face_t face;
		for (int i = 0; i < (int)primitive; i++)
		{
			face.push_back(rand());
		}
		faces.push_back(face);
	}
	return faces;
}

repo::lib::RepoBounds repo::test::utils::mesh::getBoundingBox(std::vector< repo::lib::RepoVector3D> vertices)
{
	repo::lib::RepoBounds bounds;
	for (auto& v : vertices)
	{
		bounds.encapsulate(v);
	}
	return bounds;
}

repo::lib::RepoBounds repo::test::utils::mesh::getBoundingBox(std::vector<repo::lib::RepoVector3D64> vertices)
{
	repo::lib::RepoBounds bounds;
	for (auto& v : vertices)
	{
		bounds.encapsulate(v);
	}
	return bounds;
}

repo::test::utils::mesh::mesh_data::mesh_data(
	bool name,
	bool sharedId,
	int numParents,
	int faceSize,
	bool normals,
	int numUvChannels,
	int numVertices,
	std::string grouping
)
{
	if (name) {
		this->name = "Named Mesh";
	}

	if (sharedId) {
		this->sharedId = getRandUUID();
	}

	for (int i = 0; i < numParents; i++) {
		parents.push_back(getRandUUID());
	}

	uniqueId = getRandUUID();

	repo::lib::RepoVector3D min = repo::lib::RepoVector3D(FLT_MAX, FLT_MAX, FLT_MAX);
	repo::lib::RepoVector3D max = repo::lib::RepoVector3D(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (int i = 0; i < numVertices; i++)
	{
		vertices.push_back(repo::lib::RepoVector3D(rand() - rand(), rand() - rand(), rand() - rand()));
		min = repo::lib::RepoVector3D::min(min, vertices[vertices.size() - 1]);
		max = repo::lib::RepoVector3D::max(max, vertices[vertices.size() - 1]);
	}

	int vertexIndex = 0;
	do
	{
		repo::lib::repo_face_t face;
		for (int j = 0; j < faceSize; j++)
		{
			face.push_back((vertexIndex++ % vertices.size()));
		}
		faces.push_back(face);
	} while (vertexIndex < vertices.size());

	if (normals)
	{
		this->normals = makeNormals(numVertices);
	}

	for (int i = 0; i < numUvChannels; i++)
	{
		this->uvChannels.push_back(makeUVs(numVertices));
	}

	this->boundingBox.push_back({
		min.x, min.y, min.z
		});

	this->boundingBox.push_back({
		max.x, max.y, max.z
		});

	this->grouping = grouping;
}

repo::lib::RepoMatrix repo::test::utils::mesh::makeTransform(bool translation, bool rotation)
{
	repo::lib::RepoMatrix m;

	if (rotation)
	{
		m = m * repo::lib::RepoMatrix::rotationX((float)rand() / RAND_MAX) * repo::lib::RepoMatrix::rotationY((float)rand() / RAND_MAX) * repo::lib::RepoMatrix::rotationZ((float)rand() / RAND_MAX);
	}

	if (translation)
	{
		m = m * repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D(rand(), rand(), rand()));
	}

	return m;
}

void repo::test::utils::mesh::compareMeshNode(mesh_data expected, MeshNode actual)
{
	EXPECT_THAT(actual.getUniqueID(), Eq(expected.uniqueId));
	EXPECT_THAT(actual.getSharedID(), Eq(expected.sharedId));
	EXPECT_THAT(actual.getParentIDs(), UnorderedElementsAreArray(expected.parents));
	EXPECT_THAT(actual.getName(), Eq(expected.name));
	EXPECT_THAT(actual.getNumFaces(), Eq(expected.faces.size()));
	EXPECT_THAT(actual.getFaces(), ElementsAreArray(expected.faces));
	EXPECT_THAT(actual.getNumVertices(), Eq(expected.vertices.size()));
	EXPECT_THAT(actual.getVertices(), ElementsAreArray(expected.vertices));
	EXPECT_THAT(actual.getNormals(), ElementsAreArray(expected.normals));
	EXPECT_THAT(actual.getNumUVChannels(), Eq(expected.uvChannels.size()));
	for (int i = 0; i < expected.uvChannels.size(); i++)
	{
		EXPECT_THAT(actual.getUVChannelsSeparated()[i], ElementsAreArray(expected.uvChannels[i]));
	}
	EXPECT_EQ(actual.getGrouping(), expected.grouping);
}

float repo::test::utils::mesh::hausdorffDistance(const std::vector<repo::core::model::MeshNode>& meshes)
{
	float d = 0;
	for (auto& a : meshes) {
		for (auto& b : meshes) {
			d = std::max(d, hausdorffDistance(a, b));
		}
	}
	return d;
}

float repo::test::utils::mesh::hausdorffDistance(const repo::core::model::MeshNode& ma, const repo::core::model::MeshNode& mb)
{
	auto& A = ma.getVertices();
	auto& B = mb.getVertices();
	float h = FLT_MIN;
	for (auto& a : A) {
		float d = FLT_MAX;
		for (auto& b : B) {
			d = std::min(d, (a - b).norm2());
		}
		h = std::max(h, d);
	}
	return std::sqrt(h);
}

float repo::test::utils::mesh::shortestDistance(const repo::core::model::MeshNode& m, repo::lib::RepoVector3D p)
{
	return shortestDistance(m.getVertices(), p);
}

float repo::test::utils::mesh::shortestDistance(const std::vector<repo::core::model::MeshNode>& meshes, repo::lib::RepoVector3D p)
{
	float d = FLT_MAX;
	for (auto& m : meshes) {
		d = std::min(d, shortestDistance(m, p));
	}
	return d;
}

float repo::test::utils::mesh::shortestDistance(const std::vector<repo::lib::RepoVector3D>& vectors, repo::lib::RepoVector3D p)
{
	float d = FLT_MAX;
	for (auto& v : vectors) {
		d = std::min(d, (v - p).norm2());
	}
	return std::sqrt(d);
}

repo::core::model::MeshNode repo::test::utils::mesh::makeUnitCube()
{
	// Define the 8 vertices of the unit cube centered at (0,0,0)
	std::vector<repo::lib::RepoVector3D> vertices = {
		{-0.5, -0.5, -0.5}, // 0
		{ 0.5, -0.5, -0.5}, // 1
		{ 0.5,  0.5, -0.5}, // 2
		{-0.5,  0.5, -0.5}, // 3
		{-0.5, -0.5,  0.5}, // 4
		{ 0.5, -0.5,  0.5}, // 5
		{ 0.5,  0.5,  0.5}, // 6
		{-0.5,  0.5,  0.5}  // 7
	};

	// Define the 12 triangular faces of the cube
	std::vector<repo::lib::repo_face_t> faces = {
		{0, 1, 2}, {0, 2, 3}, // Bottom face
		{4, 5, 6}, {4, 6, 7}, // Top face
		{0, 1, 5}, {0, 5, 4}, // Front face
		{1, 2, 6}, {1, 6, 5}, // Right face
		{2, 3, 7}, {2, 7, 6}, // Back face
		{3, 0, 4}, {3, 4, 7}  // Left face
	};

	repo::lib::RepoBounds boundingBox = getBoundingBox(vertices);

	return RepoBSONFactory::makeMeshNode(
		vertices,
		faces,
		{},
		boundingBox,
		{},
		"UnitCube"
	);
}

repo::core::model::MeshNode repo::test::utils::mesh::makeUnitCone()
{
	// Number of segments for the circular base
	const size_t segments = 16;

	repo::lib::RepoVector3D apex(0.0, 0.0, 0.5);
	repo::lib::RepoVector3D baseCenter(0.0, 0.0, -0.5);

	// Define the vertices of the base
	std::vector<repo::lib::RepoVector3D> vertices;
	vertices.push_back(apex); // Add the apex as the first vertex

	for (size_t i = 0; i < segments; ++i)
	{
		double angle = 2.0 * std::numbers::pi * i / segments;
		double x = 0.5 * cos(angle);
		double y = 0.5 * sin(angle);
		vertices.emplace_back(x, y, -0.5);
	}
	vertices.push_back(baseCenter); // Add the center of the base

	// Define the faces of the cone
	std::vector<repo::lib::repo_face_t> faces;

	// Create faces for the sides of the cone
	for (size_t i = 1; i <= segments; ++i)
	{
		size_t next = (i % segments) + 1;
		faces.push_back({ 0, i, next }); // Apex to two consecutive base vertices
	}

	// Create faces for the base
	size_t baseCenterIndex = static_cast<size_t>(vertices.size() - 1);
	for (size_t i = 1; i <= segments; ++i)
	{
		size_t next = (i % segments) + 1;
		faces.push_back({ baseCenterIndex, next, i }); // Base center to two consecutive base vertices
	}

	// Compute the bounding box for the cone
	repo::lib::RepoBounds boundingBox = getBoundingBox(vertices);

	return RepoBSONFactory::makeMeshNode(
		vertices,
		faces,
		{},
		boundingBox,
		{},
		"UnitCone"
	);
}

repo::core::model::MeshNode repo::test::utils::mesh::makeUnitSphere()
{
	// Number of segments for latitude and longitude
	const size_t latitudeSegments = 16;
	const size_t longitudeSegments = 32;

	// Define the vertices
	std::vector<repo::lib::RepoVector3D> vertices;

	// Add the top vertex (north pole)
	vertices.emplace_back(0.0, 0.0, 0.5);

	// Add vertices for each latitude ring
	for (int lat = 1; lat < latitudeSegments; ++lat)
	{
		double theta = std::numbers::pi * lat / latitudeSegments; // Polar angle
		double sinTheta = sin(theta);
		double cosTheta = cos(theta);

		for (int lon = 0; lon < longitudeSegments; ++lon)
		{
			double phi = 2.0 * std::numbers::pi * lon / longitudeSegments; // Azimuthal angle
			double x = 0.5 * sinTheta * cos(phi);
			double y = 0.5 * sinTheta * sin(phi);
			double z = 0.5 * cosTheta;
			vertices.emplace_back(x, y, z);
		}
	}

	// Add the bottom vertex (south pole)
	vertices.emplace_back(0.0, 0.0, -0.5);

	// Define the faces
	std::vector<repo::lib::repo_face_t> faces;

	// Create faces for the top cap
	for (size_t lon = 0; lon < longitudeSegments; ++lon)
	{
		size_t nextLon = (lon + 1) % longitudeSegments;
		faces.push_back({ 0, 1 + lon, 1 + nextLon });
	}

	// Create faces for the middle latitude rings
	for (size_t lat = 0; lat < latitudeSegments - 2; ++lat)
	{
		for (size_t lon = 0; lon < longitudeSegments; ++lon)
		{
			size_t current = 1 + lat * longitudeSegments + lon;
			size_t next = 1 + lat * longitudeSegments + (lon + 1) % longitudeSegments;
			size_t below = current + longitudeSegments;
			size_t belowNext = next + longitudeSegments;

			faces.push_back({ current, below, next });
			faces.push_back({ next, below, belowNext });
		}
	}

	// Create faces for the bottom cap
	size_t bottomIndex = static_cast<size_t>(vertices.size() - 1);
	for (int lon = 0; lon < longitudeSegments; ++lon)
	{
		size_t current = bottomIndex - longitudeSegments + lon;
		size_t next = bottomIndex - longitudeSegments + (lon + 1) % longitudeSegments;
		faces.push_back({ bottomIndex, current, next });
	}

	repo::lib::RepoBounds boundingBox = getBoundingBox(vertices);

	return RepoBSONFactory::makeMeshNode(
		vertices,
		faces,
		{},
		boundingBox,
		{},
		"UnitSphere"
	);
}
