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
#include "../../../../repo_test_utils.h"
#include <repo/core/model/bson/repo_bson_factory.h>
#include <repo/core/model/bson/repo_bson_builder.h>

using namespace repo::core::model;

TEST(RepoBSONFactoryTest, MakeRepoProjectSettingsTest)
{
	std::string projectName = "project";
	std::string owner = "repo";
	std::string type = "Structural";
	std::string description = "testing project";
	// TODO: add test values for pinSize, avatarHeight, visibilityLimit, speed, zNear, zFar

	RepoProjectSettings settings = RepoBSONFactory::makeRepoProjectSettings(projectName, owner, type,
		description);

	EXPECT_EQ(projectName, settings.getProjectName());
	EXPECT_EQ(description, settings.getDescription());
	EXPECT_EQ(owner, settings.getOwner());
	EXPECT_EQ(type, settings.getType());
}

TEST(RepoBSONFactoryTest, MakeRepoRoleTest)
{
	std::string roleName = "repoRole";
	std::string databaseName = "admin";
	std::vector<RepoPermission> permissions;

	std::string proDB1 = "database";
	std::string proName1 = "project1";
	AccessRight proAccess1 = AccessRight::READ_AND_COMMENT;

	std::string proDB2 = "databaseb";
	std::string proName2 = "project2";
	AccessRight proAccess2 = AccessRight::READ_ONLY;

	std::string proDB3 = "databasec";
	std::string proName3 = "project3";
	AccessRight proAccess3 = AccessRight::READ_WRITE;

	permissions.push_back({ proDB1, proName1, proAccess1 });
	permissions.push_back({ proDB2, proName2, proAccess2 });
	permissions.push_back({ proDB3, proName3, proAccess3 });

	RepoRole role = RepoBSONFactory::makeRepoRole(roleName, databaseName, permissions);

	EXPECT_EQ(databaseName, role.getDatabase());
	EXPECT_EQ(roleName, role.getName());
	EXPECT_EQ(0, role.getInheritedRoles().size());

	std::vector<RepoPermission> accessRights = role.getProjectAccessRights();

	ASSERT_EQ(permissions.size(), accessRights.size());

	for (int i = 0; i < accessRights.size(); ++i)
	{
		//Order is not guaranteed, this is a long winded way to find the same permission again
		//but it should be fine as it's only 3 members
		bool found = false;
		for (int j = 0; j < permissions.size(); ++j)
		{
			found |= permissions[j].database == accessRights[i].database
				&& permissions[j].project == accessRights[i].project
				&& permissions[j].permission == accessRights[i].permission;
		}
		EXPECT_TRUE(found);
	}
}

TEST(RepoBSONFactoryTest, MakeRepoRoleTest2)
{
	std::string roleName = "repoRole";
	std::string databaseName = "admin";
	std::vector<RepoPrivilege> privileges;
	std::vector <std::pair<std::string, std::string>> inheritedRoles;

	privileges.push_back({ "db1", "col1", { DBActions::FIND } });
	privileges.push_back({ "db2", "col2", { DBActions::INSERT, DBActions::CREATE_USER } });
	privileges.push_back({ "db1", "col2", { DBActions::FIND, DBActions::DROP_ROLE } });

	inheritedRoles.push_back(std::pair < std::string, std::string > {"orange", "superUser"});

	RepoRole role = RepoBSONFactory::_makeRepoRole(roleName, databaseName, privileges, inheritedRoles);

	EXPECT_EQ(databaseName, role.getDatabase());
	EXPECT_EQ(roleName, role.getName());

	auto inheritedRolesOut = role.getInheritedRoles();

	ASSERT_EQ(inheritedRoles.size(), inheritedRolesOut.size());

	for (int i = 0; i < inheritedRolesOut.size(); ++i)
	{
		EXPECT_EQ(inheritedRolesOut[i].first, inheritedRoles[i].first);
		EXPECT_EQ(inheritedRolesOut[i].second, inheritedRoles[i].second);
	}

	auto privOut = role.getPrivileges();

	ASSERT_EQ(privOut.size(), privileges.size());
	for (int i = 0; i < privOut.size(); ++i)
	{
		EXPECT_EQ(privOut[i].database, privileges[i].database);
		EXPECT_EQ(privOut[i].collection, privileges[i].collection);
		EXPECT_EQ(privOut[i].actions.size(), privileges[i].actions.size());
	}
}

TEST(RepoBSONFactoryTest, MakeRepoUserTest)
{
	std::string username = "user";
	std::string password = "password";
	std::string firstName = "firstname";
	std::string lastName = "lastName";
	std::string email = "email";

	std::list<std::pair<std::string, std::string>>   roles;
	std::list<std::pair<std::string, std::string>>   apiKeys;
	std::vector<char>                                avatar;

	roles.push_back(std::pair<std::string, std::string >("database", "roleName"));
	apiKeys.push_back(std::pair<std::string, std::string >("database", "apiKey"));
	avatar.resize(10);

	RepoUser user = RepoBSONFactory::makeRepoUser(username, password, firstName, lastName, email, roles, apiKeys, avatar);

	EXPECT_EQ(username, user.getUserName());
	EXPECT_EQ(firstName, user.getFirstName());
	EXPECT_EQ(lastName, user.getLastName());
	EXPECT_EQ(email, user.getEmail());

	auto rolesOut = user.getRolesList();
	auto apiOut = user.getAPIKeysList();
	auto avatarOut = user.getAvatarAsRawData();

	EXPECT_EQ(roles.size(), rolesOut.size());
	EXPECT_EQ(apiKeys.size(), apiOut.size());
	EXPECT_EQ(avatar.size(), avatarOut.size());

	EXPECT_EQ(rolesOut.begin()->first, roles.begin()->first);
	EXPECT_EQ(rolesOut.begin()->second, roles.begin()->second);

	EXPECT_EQ(apiOut.begin()->first, apiKeys.begin()->first);
	EXPECT_EQ(apiOut.begin()->second, apiKeys.begin()->second);

	EXPECT_EQ(std::string(avatar.data()), std::string(avatarOut.data()));
}

TEST(RepoBSONFactoryTest, AppendDefaultsTest)
{
	RepoBSONBuilder builder;

	auto defaults = RepoBSONFactory::appendDefaults("test");
	builder.appendElements(defaults);

	RepoNode n = builder.obj();

	EXPECT_FALSE(n.isEmpty());

	EXPECT_EQ(4, n.nFields());

	EXPECT_TRUE(n.hasField(REPO_NODE_LABEL_ID));
	EXPECT_TRUE(n.hasField(REPO_NODE_LABEL_SHARED_ID));
	EXPECT_TRUE(n.hasField(REPO_NODE_LABEL_API));
	EXPECT_TRUE(n.hasField(REPO_NODE_LABEL_TYPE));

	//Ensure existing fields doesnt' disappear

	RepoBSONBuilder builderWithFields;

	builderWithFields << "Number" << 1023;
	builderWithFields << "doll" << "Kitty";

	auto defaults2 = RepoBSONFactory::appendDefaults("test");
	builderWithFields.appendElements(defaults2);

	RepoNode nWithExists = builderWithFields.obj();
	EXPECT_FALSE(nWithExists.isEmpty());

	EXPECT_EQ(6, nWithExists.nFields());

	EXPECT_TRUE(nWithExists.hasField(REPO_NODE_LABEL_ID));
	EXPECT_TRUE(nWithExists.hasField(REPO_NODE_LABEL_SHARED_ID));
	EXPECT_TRUE(nWithExists.hasField(REPO_NODE_LABEL_API));
	EXPECT_TRUE(nWithExists.hasField(REPO_NODE_LABEL_TYPE));

	EXPECT_TRUE(nWithExists.hasField("doll"));
	EXPECT_EQ("Kitty", std::string(nWithExists.getStringField("doll")));
	EXPECT_TRUE(nWithExists.hasField("Number"));
	EXPECT_EQ(nWithExists.getField("Number").Int(), 1023);
}

TEST(RepoBSONFactoryTest, MakeCameraNodeTest)
{
	float aspectRatio = 1.0;
	float fCP = 10;
	float nCP = 100;
	float fov = 500;
	repo_vector_t lookAt = { 1.0, 2.0, 3.0 };
	repo_vector_t position = { 3.1, 2.2, 3.5 };
	repo_vector_t up = { 4.1, 12.2, 23.5 };

	std::string name = "CamTest";

	CameraNode camera = RepoBSONFactory::makeCameraNode(aspectRatio, fCP, nCP, fov, lookAt, position, up, name);

	EXPECT_FALSE(camera.isEmpty());
	EXPECT_EQ(aspectRatio, camera.getAspectRatio());
	EXPECT_EQ(fCP, camera.getFarClippingPlane());
	EXPECT_EQ(nCP, camera.getNearClippingPlane());
	EXPECT_EQ(fov, camera.getFieldOfView());

	EXPECT_TRUE(compareVectors(lookAt, camera.getLookAt()));
	EXPECT_TRUE(compareVectors(position, camera.getPosition()));
	EXPECT_TRUE(compareVectors(up, camera.getUp()));

	EXPECT_EQ(name, camera.getName());

	EXPECT_EQ(camera.getTypeAsEnum(), NodeType::CAMERA);
}

TEST(RepoBSONFactoryTest, MakeMaterialNodeTest)
{
	repo_material_t mat_struct;
	mat_struct.ambient.resize(4);
	mat_struct.diffuse.resize(4);
	mat_struct.specular.resize(4);
	mat_struct.emissive.resize(4);
	mat_struct.opacity = 0.9;
	mat_struct.shininess = 1.0;
	mat_struct.shininessStrength = 0.5;
	mat_struct.isWireframe = true;
	mat_struct.isTwoSided = false;

	std::string name = "MatTest";

	MaterialNode material = RepoBSONFactory::makeMaterialNode(mat_struct, name);

	EXPECT_FALSE(material.isEmpty());
	EXPECT_EQ(name, material.getName());
	EXPECT_EQ(material.getTypeAsEnum(), NodeType::MATERIAL);

	auto matOut = material.getMaterialStruct();

	compareMaterialStructs(mat_struct, matOut);

	repo_material_t emptyStruct;

	//See if it breaks if the vectors in the struct is never filled
	MaterialNode material2 = RepoBSONFactory::makeMaterialNode(emptyStruct, name);
	EXPECT_FALSE(material2.isEmpty());
	EXPECT_EQ(name, material2.getName());
	EXPECT_EQ(material2.getTypeAsEnum(), NodeType::MATERIAL);
}

TEST(RepoBSONFactoryTest, MakeMapNodeTest)
{
	uint32_t width = 1, zoom = 19;
	float tilt = 2.0, tileSize = 10.5, longit = 2.3546, latit = 5.3235;
	repo_vector_t centrePoint = { 3.12345, 54.3536, 435.32 };
	std::string name = "mapTest";
	std::string apiKey = "apiKey";

	MapNode map = RepoBSONFactory::makeMapNode(width, zoom, tilt, tileSize, longit, latit, centrePoint, apiKey, name);

	EXPECT_FALSE(map.isEmpty());
	EXPECT_EQ(name, map.getName());
	EXPECT_EQ(apiKey, map.getAPIKey());
	EXPECT_EQ(map.getTypeAsEnum(), NodeType::MAP);

	EXPECT_EQ(width, map.getWidth());
	EXPECT_EQ(zoom, map.getZoom());
	EXPECT_EQ(tileSize, map.getTileSize());
	EXPECT_EQ(tilt, map.getYRot());
	EXPECT_EQ(longit, map.getField(REPO_NODE_MAP_LABEL_LONG).Double());
	EXPECT_EQ(latit, map.getField(REPO_NODE_MAP_LABEL_LAT).Double());

	repo_vector_t vec;
	if (map.hasField(REPO_NODE_MAP_LABEL_TRANS))
	{
		std::vector<float> floatArr = map.getFloatArray(REPO_NODE_MAP_LABEL_TRANS);
		if (floatArr.size() >= 3)
		{
			//repo_vector_t is effectively float[3]
			std::copy(floatArr.begin(), floatArr.begin() + 3, (float*)&vec);
		}
	}

	EXPECT_TRUE(compareVectors(centrePoint, vec));
}

TEST(RepoBSONFactoryTest, MakeMetaDataNodeTest)
{
	RepoBSON data = BSON("something" << "Something else" << "something2" << "somethingelse2");
	std::string mimeType = "application/x-mswrite";
	std::string name = "MetaTest";

	MetadataNode metaNode = RepoBSONFactory::makeMetaDataNode(data, mimeType, name);

	EXPECT_FALSE(metaNode.isEmpty());
	EXPECT_EQ(name, metaNode.getName());
	EXPECT_EQ(metaNode.getTypeAsEnum(), NodeType::METADATA);

	EXPECT_EQ(mimeType, metaNode.getStringField(REPO_LABEL_MEDIA_TYPE));

	EXPECT_EQ(data.toString(), metaNode.getObjectField(REPO_NODE_LABEL_METADATA).toString());
}

TEST(RepoBSONFactoryTest, MakeMetaDataNodeTest2)
{
	std::vector<std::string> keys({ "one", "two", "three", "four", "five" }), values({ "!", "!!", "!!!", "!!!!", "!!!!!" });

	std::string name = "MetaTest";

	MetadataNode metaNode = RepoBSONFactory::makeMetaDataNode(keys, values, name);

	EXPECT_FALSE(metaNode.isEmpty());
	EXPECT_EQ(name, metaNode.getName());
	EXPECT_EQ(metaNode.getTypeAsEnum(), NodeType::METADATA);

	auto metaBSON = metaNode.getObjectField(REPO_NODE_LABEL_METADATA);

	ASSERT_FALSE(metaBSON.isEmpty());

	for (uint32_t i = 0; i < keys.size(); ++i)
	{
		ASSERT_TRUE(metaBSON.hasField(keys[i]));
		EXPECT_EQ(values[i], metaBSON.getStringField(keys[i]));
	}
}

TEST(RepoBSONFactoryTest, MakeMeshNodeTest)
{
	uint32_t nCount = 10;
	//using malloc to get un-initalised values to fill the memory.
	repo_vector_t *rawVec = (repo_vector_t*)malloc(sizeof(*rawVec) * nCount);
	repo_vector_t *rawNorm = (repo_vector_t*)malloc(sizeof(*rawNorm) * nCount);
	repo_vector2d_t *rawUV = (repo_vector2d_t*)malloc(sizeof(*rawUV) * nCount);
	repo_color4d_t *rawColors = (repo_color4d_t*)malloc(sizeof(*rawColors) * nCount);

	ASSERT_TRUE(rawVec);
	ASSERT_TRUE(rawNorm);
	ASSERT_TRUE(rawUV);

	//Set up faces
	std::vector<repo_face_t> faces;
	std::vector<repo_vector_t> vectors;
	std::vector<repo_vector_t> normals;
	std::vector<std::vector<repo_vector2d_t>> uvChannels;
	std::vector<repo_color4d_t> colors;
	uvChannels.resize(1);
	faces.reserve(nCount);
	vectors.reserve(nCount);
	normals.reserve(nCount);
	uvChannels[0].reserve(nCount);
	colors.reserve(nCount);
	for (uint32_t i = 0; i < nCount; ++i)
	{
		repo_face_t face = { (uint32_t)std::rand(), (uint32_t)std::rand(), (uint32_t)std::rand() };
		faces.push_back(face);

		vectors.push_back({ (float)std::rand() / 100.0f, (float)std::rand() / 100.0f, (float)std::rand() / 100.0f });
		normals.push_back({ (float)std::rand() / 100.0f, (float)std::rand() / 100.0f, (float)std::rand() / 100.0f });
		uvChannels[0].push_back({ (float)std::rand() / 100.0f, (float)std::rand() / 100.0f });
		colors.push_back({ (float)std::rand() / 100.0f, (float)std::rand() / 100.0f, (float)std::rand() / 100.0f, (float)std::rand() / 100.0f });
	}

	std::vector<std::vector<float>> boundingBox, outLine;
	boundingBox.resize(2);
	boundingBox[0] = { std::rand() / 100.f, std::rand() / 100.f, std::rand() / 100.f };
	boundingBox[1] = { std::rand() / 100.f, std::rand() / 100.f, std::rand() / 100.f };
	outLine = boundingBox;

	std::string name = "meshTest";

	//End of setting up data... the actual testing happens here.

	MeshNode mesh = RepoBSONFactory::makeMeshNode(vectors, faces, normals, boundingBox, uvChannels, colors, outLine, name);

	auto vOut = mesh.getVertices();
	auto nOut = mesh.getNormals();
	auto fOut = mesh.getFaces();
	auto cOut = mesh.getColors();
	auto uvOut = mesh.getUVChannelsSeparated();
	EXPECT_TRUE(compareVectors(vectors, vOut));
	EXPECT_TRUE(compareVectors(normals, nOut));
	EXPECT_TRUE(compareStdVectors(faces, fOut));
	EXPECT_TRUE(compareVectors(colors, cOut));
	EXPECT_TRUE(compareVectors(uvChannels, uvOut));

	auto bbox = mesh.getBoundingBox();
	ASSERT_EQ(boundingBox.size(), bbox.size());
	ASSERT_EQ(3, boundingBox[0].size());
	ASSERT_EQ(3, boundingBox[1].size());

	EXPECT_TRUE(compareVectors(bbox[0], { boundingBox[0][0], boundingBox[0][1], boundingBox[0][2] }));
	EXPECT_TRUE(compareVectors(bbox[1], { boundingBox[1][0], boundingBox[1][1], boundingBox[1][2] }));
}

TEST(RepoBSONFactoryTest, MakeReferenceNodeTest)
{
	std::string dbName = "testDB";
	std::string proName = "testProj";
	repoUUID revId = generateUUID();
	bool isUnique = true;
	std::string name = "refNodeName";

	ReferenceNode ref = RepoBSONFactory::makeReferenceNode(dbName, proName, revId, isUnique, name);

	ASSERT_FALSE(ref.isEmpty());

	EXPECT_EQ(dbName, ref.getDatabaseName());
	EXPECT_EQ(proName, ref.getProjectName());
	EXPECT_EQ(revId, ref.getRevisionID());
	EXPECT_EQ(isUnique, ref.useSpecificRevision());
	EXPECT_EQ(name, ref.getName());

	ReferenceNode ref2 = RepoBSONFactory::makeReferenceNode(dbName, proName, revId, !isUnique, name);
	EXPECT_EQ(!isUnique, ref2.useSpecificRevision());
}

TEST(RepoBSONFactoryTest, MakeRevisionNodeTest)
{
	std::string owner = "revOwner";
	repoUUID branchID = generateUUID();
	std::vector<repoUUID> currentNodes;
	size_t currCount = 10;
	currentNodes.reserve(currCount);
	for (size_t i = 0; i < currCount; ++i)
		currentNodes.push_back(generateUUID());
	std::vector<std::string> files = { "test1", "test5" };
	std::vector<repoUUID> parents;
	size_t parentCount = 5;
	parents.reserve(parentCount);
	for (size_t i = 0; i < parentCount; ++i)
		parents.push_back(generateUUID());
	std::string message = "this is some random message to test message";
	std::string tag = "this is a random tag to test tags";
	std::vector<double> offset = { std::rand() / 100., std::rand() / 100., std::rand() / 100. };

	RevisionNode rev = RepoBSONFactory::makeRevisionNode(owner, branchID, currentNodes, files, parents, offset, message, tag);
	EXPECT_EQ(owner, rev.getAuthor());
	EXPECT_EQ(branchID, rev.getSharedID());
	EXPECT_EQ(message, rev.getMessage());
	EXPECT_EQ(tag, rev.getTag());
	//fileNames changes after it gets into the bson, just check the size
	EXPECT_EQ(files.size(), rev.getOrgFiles().size());

	EXPECT_TRUE(compareStdVectors(currentNodes, rev.getCurrentIDs()));
	EXPECT_TRUE(compareStdVectors(parents, rev.getParentIDs()));
	EXPECT_TRUE(compareStdVectors(offset, rev.getCoordOffset()));

	//ensure no random parent being generated
	std::vector<repoUUID> emptyParents;
	RevisionNode rev2 = RepoBSONFactory::makeRevisionNode(owner, branchID, currentNodes, files, emptyParents, offset, message, tag);
	EXPECT_EQ(0, rev2.getParentIDs().size());
}

TEST(RepoBSONFactoryTest, MakeTextureNodeTest)
{
	std::string ext = "jpg";
	std::string name = "textureNode." + ext;
	std::string data = "The value of this texture is represented by this string as all it takes is a char*";
	int width = 100, height = 110;

	TextureNode tex = RepoBSONFactory::makeTextureNode(name, data.c_str(), data.size(), width, height);

	ASSERT_FALSE(tex.isEmpty());

	EXPECT_EQ(name, tex.getName());
	EXPECT_EQ(width, tex.getField(REPO_LABEL_WIDTH).Int());
	EXPECT_EQ(height, tex.getField(REPO_LABEL_HEIGHT).Int());
	EXPECT_EQ(ext, tex.getFileExtension());
	std::vector<char> rawOut = tex.getRawData();
	ASSERT_EQ(data.size(), rawOut.size());
	EXPECT_EQ(0, memcmp(data.c_str(), rawOut.data(), data.size()));

	//make sure the code doesn't fail over if for some reason the name does not contain the extension
	TextureNode tex2 = RepoBSONFactory::makeTextureNode("noExtensionName", data.c_str(), data.size(), width, height);
}

TEST(RepoBSONFactoryTest, MakeTransformationNodeTest)
{
	//If I make a transformation with no parameters, it should be identity matrix
	std::vector<float> identity =
	{ 1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1 };

	TransformationNode trans = RepoBSONFactory::makeTransformationNode();

	ASSERT_FALSE(trans.isEmpty());
	EXPECT_TRUE(compareStdVectors(identity, trans.getTransMatrix(false)));

	std::vector<std::vector<float>> transMat;
	std::vector<float> transMatFlat;
	transMat.resize(4);
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
		{
			transMat[i].push_back(std::rand() / 100.);
			transMatFlat.push_back(transMat[i][j]);
		}
	std::string name = "myTransTest";

	std::vector<repoUUID> parents;
	for (int i = 0; i < 10; ++i)
		parents.push_back(generateUUID());

	TransformationNode trans2 = RepoBSONFactory::makeTransformationNode(transMat, name, parents);

	ASSERT_FALSE(trans2.isEmpty());
	EXPECT_EQ(name, trans2.getName());
	std::vector<float> matrix = trans2.getTransMatrix(false);

	EXPECT_TRUE(compareStdVectors(transMatFlat, matrix));
	EXPECT_TRUE(compareStdVectors(parents, trans2.getParentIDs()));

	//ensure random parents aren't thrown in
	parents.clear();
	TransformationNode trans3 = RepoBSONFactory::makeTransformationNode(transMat, name, parents);
	EXPECT_EQ(parents.size(), trans3.getParentIDs().size());
}