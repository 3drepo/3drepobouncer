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

	RepoProjectSettings settings = RepoBSONFactory::makeRepoProjectSettings(projectName, owner, false, type,
		description);

	EXPECT_EQ(projectName, settings.getProjectName());
	EXPECT_EQ(description, settings.getDescription());
	EXPECT_EQ(owner, settings.getOwner());
	EXPECT_EQ(type, settings.getType());
	EXPECT_FALSE(settings.isFederate());

	RepoProjectSettings settings2 = RepoBSONFactory::makeRepoProjectSettings(projectName, owner, true, type,
		description);

	EXPECT_EQ(projectName, settings2.getProjectName());
	EXPECT_EQ(description, settings2.getDescription());
	EXPECT_EQ(owner, settings2.getOwner());
	EXPECT_EQ(type, settings2.getType());
	EXPECT_TRUE(settings2.isFederate());
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

	EXPECT_EQ(3, n.nFields());

	EXPECT_TRUE(n.hasField(REPO_NODE_LABEL_ID));
	EXPECT_TRUE(n.hasField(REPO_NODE_LABEL_SHARED_ID));
	EXPECT_TRUE(n.hasField(REPO_NODE_LABEL_TYPE));

	//Ensure existing fields doesnt' disappear

	RepoBSONBuilder builderWithFields;

	builderWithFields.append("Number", 1023);
	builderWithFields.append("doll", "Kitty");

	auto defaults2 = RepoBSONFactory::appendDefaults("test");
	builderWithFields.appendElements(defaults2);

	RepoNode nWithExists = builderWithFields.obj();
	EXPECT_FALSE(nWithExists.isEmpty());

	EXPECT_EQ(5, nWithExists.nFields());

	EXPECT_TRUE(nWithExists.hasField(REPO_NODE_LABEL_ID));
	EXPECT_TRUE(nWithExists.hasField(REPO_NODE_LABEL_SHARED_ID));
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
	repo::lib::RepoVector3D lookAt = { 1.0f, 2.0f, 3.0f };
	repo::lib::RepoVector3D position = { 3.1f, 2.2f, 3.5f };
	repo::lib::RepoVector3D up = { 4.1f, 12.2f, 23.5f };

	std::string name = "CamTest";

	CameraNode camera = RepoBSONFactory::makeCameraNode(aspectRatio, fCP, nCP, fov, lookAt, position, up, name);

	EXPECT_FALSE(camera.isEmpty());
	EXPECT_EQ(aspectRatio, camera.getAspectRatio());
	EXPECT_EQ(fCP, camera.getFarClippingPlane());
	EXPECT_EQ(nCP, camera.getNearClippingPlane());
	EXPECT_EQ(fov, camera.getFieldOfView());

	EXPECT_EQ(lookAt, camera.getLookAt());
	EXPECT_EQ(position, camera.getPosition());
	EXPECT_EQ(up, camera.getUp());

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
	mat_struct.lineWeight = 3;
	mat_struct.isWireframe = true;
	mat_struct.isTwoSided = false;

	std::string name = "MatTest";

	MaterialNode material = RepoBSONFactory::makeMaterialNode(mat_struct, name);

	EXPECT_FALSE(material.isEmpty());
	EXPECT_EQ(name, material.getName());
	EXPECT_EQ(material.getTypeAsEnum(), NodeType::MATERIAL);

	auto matOut = material.getMaterialStruct();

	EXPECT_TRUE(compareMaterialStructs(mat_struct, matOut));

	repo_material_t emptyStruct;

	//See if it breaks if the vectors in the struct is never filled
	MaterialNode material2 = RepoBSONFactory::makeMaterialNode(emptyStruct, name);
	EXPECT_FALSE(material2.isEmpty());
	EXPECT_EQ(name, material2.getName());
	EXPECT_EQ(material2.getTypeAsEnum(), NodeType::MATERIAL);
}

TEST(RepoBSONFactoryTest, MakeMetaDataNodeTest)
{
	std::string name = "MetaTest";
	std::unordered_map<std::string, repo::lib::RepoVariant> metaDataUnMap;

	metaDataUnMap["one"]=1;
	metaDataUnMap["two"]= 2;
	metaDataUnMap["three"]= 3;
	metaDataUnMap["four"]= 4;
	metaDataUnMap["five"]= 5;
	MetadataNode metaNode = RepoBSONFactory::makeMetaDataNode(metaDataUnMap, name);
	EXPECT_FALSE(metaNode.isEmpty());
	EXPECT_EQ(name, metaNode.getName());
	EXPECT_EQ(metaNode.getTypeAsEnum(), NodeType::METADATA);

	auto metaBSON = metaNode.getObjectField(REPO_NODE_LABEL_METADATA);

	ASSERT_FALSE(metaBSON.isEmpty());
	for (uint32_t i = 0; i < metaDataUnMap.size(); ++i)
	{
		auto index = std::to_string(i);
		ASSERT_TRUE(metaBSON.hasField(index));
		auto metaEntry = metaBSON.getObjectField(index);
		auto key = metaEntry.getStringField(REPO_NODE_LABEL_META_KEY);
		auto value = metaEntry.getIntField(REPO_NODE_LABEL_META_VALUE);
		auto keyIt = metaDataUnMap.find(key);
		auto endIt = metaDataUnMap.end();
		ASSERT_NE(keyIt, endIt);
		EXPECT_EQ(key, keyIt->first);
		int itValue;
		if((keyIt->second).getBaseData<int>(itValue))
			EXPECT_EQ(value,itValue);
	}
}

TEST(RepoBSONFactoryTest, MakeMeshNodeTest)
{
	uint32_t nCount = 10;

	//Set up faces
	std::vector<repo_face_t> faces;
	std::vector<repo::lib::RepoVector3D> vectors;
	std::vector<repo::lib::RepoVector3D> normals;
	std::vector<std::vector<repo::lib::RepoVector2D>> uvChannels;
	std::vector<repo_color4d_t> colors;
	std::vector<float> ids;
	uvChannels.resize(1);
	faces.reserve(nCount);
	vectors.reserve(nCount);
	normals.reserve(nCount);
	uvChannels[0].reserve(nCount);
	colors.reserve(nCount);
	ids.reserve(nCount);
	for (uint32_t i = 0; i < nCount; ++i)
	{
		repo_face_t face = { (uint32_t)std::rand(), (uint32_t)std::rand(), (uint32_t)std::rand() };
		faces.push_back(face);

		vectors.push_back({ (float)std::rand() / 100.0f, (float)std::rand() / 100.0f, (float)std::rand() / 100.0f });
		normals.push_back({ (float)std::rand() / 100.0f, (float)std::rand() / 100.0f, (float)std::rand() / 100.0f });
		uvChannels[0].push_back({ (float)std::rand() / 100.0f, (float)std::rand() / 100.0f });
		colors.push_back({ (float)std::rand() / 100.0f, (float)std::rand() / 100.0f, (float)std::rand() / 100.0f, (float)std::rand() / 100.0f });
		ids.push_back(std::rand());
	}
	std::vector<std::vector<float>> boundingBox;
	boundingBox.resize(2);
	boundingBox[0] = { std::rand() / 100.f, std::rand() / 100.f, std::rand() / 100.f };
	boundingBox[1] = { std::rand() / 100.f, std::rand() / 100.f, std::rand() / 100.f };

	std::string name = "meshTest";

	//End of setting up data... the actual testing happens here.

	MeshNode mesh = RepoBSONFactory::makeMeshNode(vectors, faces, normals, boundingBox, uvChannels, colors, ids, name);

	repoTrace << mesh.toString();

	auto vOut = mesh.getVertices();
	auto nOut = mesh.getNormals();
	auto fOut = mesh.getFaces();
	auto cOut = mesh.getColors();
	auto uvOut = mesh.getUVChannelsSeparated();
	auto idOut = mesh.getSubmeshIds();
	EXPECT_TRUE(compareStdVectors(vectors, vOut));
	EXPECT_TRUE(compareStdVectors(normals, nOut));
	EXPECT_TRUE(compareStdVectors(faces, fOut));
	EXPECT_TRUE(compareVectors(colors, cOut));
	EXPECT_TRUE(compareStdVectors(uvChannels, uvOut));
	EXPECT_TRUE(compareStdVectors(ids, idOut));

	ASSERT_EQ(MeshNode::Primitive::TRIANGLES, mesh.getPrimitive());

	auto bbox = mesh.getBoundingBox();
	ASSERT_EQ(boundingBox.size(), bbox.size());
	ASSERT_EQ(3, boundingBox[0].size());
	ASSERT_EQ(3, boundingBox[1].size());

	EXPECT_EQ(bbox[0], repo::lib::RepoVector3D(boundingBox[0][0], boundingBox[0][1], boundingBox[0][2]));
	EXPECT_EQ(bbox[1], repo::lib::RepoVector3D(boundingBox[1][0], boundingBox[1][1], boundingBox[1][2]));

	faces.clear();
	for (uint32_t i = 0; i < nCount; ++i)
	{
		repo_face_t face = { (uint32_t)std::rand(), (uint32_t)std::rand() };
		faces.push_back(face);
	}

	// Re-create the mesh but using lines instead of triangles. This should change the primitive type, but otherwise all properties should be handled identically.

	mesh = RepoBSONFactory::makeMeshNode(vectors, faces, normals, boundingBox, uvChannels, colors, ids, name);

	repoTrace << mesh.toString();

	ASSERT_EQ(MeshNode::Primitive::LINES, mesh.getPrimitive());

	vOut = mesh.getVertices();
	nOut = mesh.getNormals();
	fOut = mesh.getFaces();
	cOut = mesh.getColors();
	uvOut = mesh.getUVChannelsSeparated();
	idOut = mesh.getSubmeshIds();
	EXPECT_TRUE(compareStdVectors(vectors, vOut));
	EXPECT_TRUE(compareStdVectors(normals, nOut));
	EXPECT_TRUE(compareStdVectors(faces, fOut));
	EXPECT_TRUE(compareVectors(colors, cOut));
	EXPECT_TRUE(compareStdVectors(uvChannels, uvOut));
	EXPECT_TRUE(compareStdVectors(ids, idOut));

	bbox = mesh.getBoundingBox();
	ASSERT_EQ(boundingBox.size(), bbox.size());
	ASSERT_EQ(3, boundingBox[0].size());
	ASSERT_EQ(3, boundingBox[1].size());

	EXPECT_EQ(bbox[0], repo::lib::RepoVector3D(boundingBox[0][0], boundingBox[0][1], boundingBox[0][2]));
	EXPECT_EQ(bbox[1], repo::lib::RepoVector3D(boundingBox[1][0], boundingBox[1][1], boundingBox[1][2]));

	// Re-create the mesh but with an unsupported primitive type. If the mesh does not have a type set, the API should return triangles,
	// but if the primitive has *attempted* to be inferred and failed, the type should report as unknown.

	faces.clear();
	for (uint32_t i = 0; i < nCount; ++i)
	{
		repo_face_t face; // empty faces should result in an unknown primitive type
		faces.push_back(face);
	}

	mesh = RepoBSONFactory::makeMeshNode(vectors, faces, normals, boundingBox, uvChannels, colors, ids, name);

	ASSERT_EQ(MeshNode::Primitive::UNKNOWN, mesh.getPrimitive());
}

TEST(RepoBSONFactoryTest, MakeReferenceNodeTest)
{
	std::string dbName = "testDB";
	std::string proName = "testProj";
	repo::lib::RepoUUID revId = repo::lib::RepoUUID::createUUID();
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
	repo::lib::RepoUUID branchID = repo::lib::RepoUUID::createUUID();
	std::vector<std::string> files = { "test1", "test5" };
	std::vector<repo::lib::RepoUUID> parents;
	size_t parentCount = 5;
	parents.reserve(parentCount);
	for (size_t i = 0; i < parentCount; ++i)
		parents.push_back(repo::lib::RepoUUID::createUUID());
	std::string message = "this is some random message to test message";
	std::string tag = "this is a random tag to test tags";
	std::vector<double> offset = { std::rand() / 100., std::rand() / 100., std::rand() / 100. };
	repo::lib::RepoUUID revId = repo::lib::RepoUUID::createUUID();

	RevisionNode rev = RepoBSONFactory::makeRevisionNode(owner, branchID, revId, files, parents, offset, message, tag);
	EXPECT_EQ(owner, rev.getAuthor());
	EXPECT_EQ(branchID, rev.getSharedID());
	EXPECT_EQ(revId, rev.getUniqueID());
	EXPECT_EQ(message, rev.getMessage());
	EXPECT_EQ(tag, rev.getTag());
	//fileNames changes after it gets into the bson, just check the size
	EXPECT_EQ(files.size(), rev.getOrgFiles().size());

	EXPECT_TRUE(compareStdVectors(parents, rev.getParentIDs()));
	EXPECT_TRUE(compareStdVectors(offset, rev.getCoordOffset()));

	//ensure no random parent being generated
	std::vector<repo::lib::RepoUUID> emptyParents;
	RevisionNode rev2 = RepoBSONFactory::makeRevisionNode(owner, branchID, revId, files, emptyParents, offset, message, tag);
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
	EXPECT_TRUE(compareStdVectors(identity, trans.getTransMatrix(false).getData()));

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

	std::vector<repo::lib::RepoUUID> parents;
	for (int i = 0; i < 10; ++i)
		parents.push_back(repo::lib::RepoUUID::createUUID());

	TransformationNode trans2 = RepoBSONFactory::makeTransformationNode(transMat, name, parents);

	ASSERT_FALSE(trans2.isEmpty());
	EXPECT_EQ(name, trans2.getName());
	auto matrix = trans2.getTransMatrix(false);

	EXPECT_TRUE(compareStdVectors(transMatFlat, matrix.getData()));
	EXPECT_TRUE(compareStdVectors(parents, trans2.getParentIDs()));

	//ensure random parents aren't thrown in
	parents.clear();
	TransformationNode trans3 = RepoBSONFactory::makeTransformationNode(transMat, name, parents);
	EXPECT_EQ(parents.size(), trans3.getParentIDs().size());
}