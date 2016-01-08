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
	AccessRight proAccess1 = AccessRight::READ;

	std::string proDB2 = "databaseb";
	std::string proName2 = "project2";
	AccessRight proAccess2 = AccessRight::WRITE;

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
			found |=  permissions[j].database == accessRights[i].database
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

	privileges.push_back({ "db1", "col1", {DBActions::FIND} });
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

	std::list<std::pair<std::string, std::string>>   projects;
	std::list<std::pair<std::string, std::string>>   roles;
	std::list<std::pair<std::string, std::string>>   groups;
	std::list<std::pair<std::string, std::string>>   apiKeys;
	std::vector<char>                                avatar;


	projects.push_back(std::pair<std::string, std::string >("database", "projectName"));
	roles.push_back(std::pair<std::string, std::string >("database", "roleName"));
	groups.push_back(std::pair<std::string, std::string >("database", "groupName"));
	apiKeys.push_back(std::pair<std::string, std::string >("database", "apiKey"));
	avatar.resize(10);

	RepoUser user = RepoBSONFactory::makeRepoUser(username, password, firstName, lastName, email, projects, roles, groups, apiKeys, avatar);

	EXPECT_EQ(username, user.getUserName());
	EXPECT_EQ(firstName, user.getFirstName());
	EXPECT_EQ(lastName, user.getLastName());
	EXPECT_EQ(email, user.getEmail());

	auto proOut = user.getProjectsList();
	auto rolesOut = user.getRolesList();
	auto groupsOut = user.getGroupsList();
	auto apiOut = user.getAPIKeysList();
	auto avatarOut = user.getAvatarAsRawData();


	EXPECT_EQ(projects.size(), proOut.size());
	EXPECT_EQ(roles.size(), rolesOut.size());
	EXPECT_EQ(groups.size(), groupsOut.size());
	EXPECT_EQ(apiKeys.size(), apiOut.size());
	EXPECT_EQ(avatar.size(), avatarOut.size());

	EXPECT_EQ(proOut.begin()->first, projects.begin()->first);
	EXPECT_EQ(proOut.begin()->second, projects.begin()->second);

	EXPECT_EQ(rolesOut.begin()->first, roles.begin()->first);
	EXPECT_EQ(rolesOut.begin()->second, roles.begin()->second);

	EXPECT_EQ(groupsOut.begin()->first, groups.begin()->first);
	EXPECT_EQ(groupsOut.begin()->second, groups.begin()->second);

	EXPECT_EQ(apiOut.begin()->first, apiKeys.begin()->first);
	EXPECT_EQ(apiOut.begin()->second, apiKeys.begin()->second);

	EXPECT_EQ(std::string(avatar.data()), std::string(avatarOut.data()));
}

TEST(RepoBSONFactoryTest, AppendDefaultsTest)
{
	RepoBSONBuilder builder;

	RepoBSONFactory::appendDefaults(
		builder, "test");

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

	RepoBSONFactory::appendDefaults(
		builderWithFields, "test");

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

	MapNode map = RepoBSONFactory::makeMapNode(width, zoom, tilt, tileSize, longit, latit, centrePoint, name);

	EXPECT_FALSE(map.isEmpty());
	EXPECT_EQ(name, map.getName());
	EXPECT_EQ(map.getTypeAsEnum(), NodeType::MAP);

	//TODO: There's no functionality to extract Map yet. once we have them we should replace these with the default functions
	EXPECT_EQ(width, map.getField(REPO_NODE_MAP_LABEL_WIDTH).Int());
	EXPECT_EQ(zoom, map.getField(REPO_NODE_MAP_LABEL_ZOOM).Int());
	EXPECT_EQ(tileSize, map.getField(REPO_NODE_MAP_LABEL_TILESIZE).Double());
	EXPECT_EQ(tilt, map.getField(REPO_NODE_MAP_LABEL_YROT).Double());
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
	std::vector<std::string> keys({ "one", "two", "three", "four", "five" }), values({"!", "!!", "!!!", "!!!!", "!!!!!"});


	std::string name = "MetaTest";

	MetadataNode metaNode = RepoBSONFactory::makeMetaDataNode(keys, values, name);

	EXPECT_FALSE(metaNode.isEmpty());
	EXPECT_EQ(name, metaNode.getName());
	EXPECT_EQ(metaNode.getTypeAsEnum(), NodeType::METADATA);

	for (uint32_t i = 0; i < keys.size(); ++i)
	{
		ASSERT_TRUE(metaNode.hasField(keys[i]));
		EXPECT_EQ(values[i], metaNode.getStringField(keys[i]));
	}
}

TEST(RepoBSONFactoryTest, MakeMeshNodeTest)
{

	//uint32_t nCount = 10;
	////using malloc to get un-initalised values to fill the memory. 
	//repo_vector_t *rawVec = (repo_vector_t*)malloc(sizeof(*rawVec) * nCount);
	//repo_vector_t *rawNorm = (repo_vector_t*)malloc(sizeof(*rawNorm) * nCount);
	//repo_vector2d_t *rawUV = (repo_vector2d_t*)malloc(sizeof(*rawUV) * nCount);
	//repo_color4d_t *rawColors = (repo_color4d_t*)malloc(sizeof(*rawColors) * nCount);

	//ASSERT_TRUE(rawVec != nullptr);
	//ASSERT_TRUE(rawNorm != nullptr);
	//ASSERT_TRUE(rawUV != nullptr);

	////Set up faces
	//std::vector<repo_face_t> faces;
	//faces.reserve(nCount);
	//for (uint32_t i = 0; i < nCount; ++i)
	//{
	//	repo_face_t f;
	//	f.numIndices = 3;
	//	f.indices = (uint32_t*)malloc(sizeof(*f.indices) * 3);
	//}

	//std::vector<repo_vector_t> vectors;
	//vectors.resize(nCount);
	//std::vector<repo_vector_t> normals;
	//normals.resize(nCount);

	//std::memcpy(vectors.data(), rawVec, nCount*sizeof(*rawVec));
	//std::memcpy(normals.data(), rawNorm, nCount*sizeof(*rawNorm));

	//std::vector<std::vector<float>> boundingBox, outLine;
	//boundingBox.resize(2);
	//boundingBox[0] = { 1.0, 1.0, 2.0 };
	//boundingBox[1] = { 1.5f, 10.1f, 23.1f };
	//outLine = boundingBox;

	//std::vector<std::vector<repo_vector2d_t>> uvChannels;
	//uvChannels.resize(1);
	//std::memcpy(uvChannels[0].data(), rawUV, nCount*sizeof(*rawUV));

	//std::vector<repo_color4d_t> colors;
	//colors.resize(nCount);
	//std::memcpy(colors.data(), rawColors, nCount*sizeof(*rawColors));

	//free(rawVec);
	//free(rawNorm);
	//free(rawUV);
	//free(rawColors);


	//std::string name = "meshTest";

	////End of setting up data... the actual testing happens here.

	//MeshNode mesh = RepoBSONFactory::makeMeshNode(vectors, faces, normals, boundingBox, uvChannels, colors, outLine, name);

	//auto vOut = mesh.getVertices();
	//auto nOut = mesh.getNormals();
	////auto fOut = mesh.getFaces();
	//EXPECT_TRUE(compareVectors(vectors, *vOut));
	//EXPECT_TRUE(compareVectors(normals, *nOut));
	////EXPECT_TRUE(compareVectors(faces, *fOut));

	//delete vOut;
	//delete nOut;
	////delete fOut;

	//for (repo_face_t face : faces)
	//{
	//	free(face.indices);
	//}


}
