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

#include "repo_test_database_info.h"
#include <repo/core/model/bson/repo_bson_builder.h>

using namespace repo::core::model;
using namespace testing;

std::shared_ptr<repo::core::handler::MongoDatabaseHandler> getHandler()
{
	auto handler = repo::core::handler::MongoDatabaseHandler::getHandler(
		REPO_GTEST_DBADDRESS,
		REPO_GTEST_DBPORT,
		REPO_GTEST_DBUSER,
		REPO_GTEST_DBPW
	);

	auto config = repo::lib::RepoConfig::fromFile(getDataPath("config/withFS.json"));
	config.configureFS(getDataPath("fileShare"));
	handler->setFileManager(std::make_shared<repo::core::handler::fileservice::FileManager>(config, handler));
	return handler;
}

std::string getClientExePath()
{
	char* pathChr = getenv("REPO_CLIENT_PATH");
	std::string returnPath = clientExe;

	if (pathChr)
	{
		std::string path = pathChr;
		path.erase(std::remove(path.begin(), path.end(), '"'), path.end());
		boost::filesystem::path fileDir(path);
		auto fullPath = fileDir / boost::filesystem::path(clientExe);
		returnPath = fullPath.string();
	}
	return returnPath;
}

std::string getDataPath(
	const std::string& file)
{
	char* pathChr = getenv("REPO_MODEL_PATH");
	std::string returnPath = simpleModel;

	if (pathChr)
	{
		std::string path = pathChr;
		path.erase(std::remove(path.begin(), path.end(), '"'), path.end());
		boost::filesystem::path fileDir(path);
		auto fullPath = fileDir / boost::filesystem::path(file);
		returnPath = fullPath.string();
	}
	return returnPath;
}

std::string getConnConfig() {
	return getDataPath(connectionConfig);
}

repo::lib::RepoConfig getConfig()
{
	auto config = repo::lib::RepoConfig(REPO_GTEST_DBADDRESS, REPO_GTEST_DBPORT,
		REPO_GTEST_DBUSER, REPO_GTEST_DBPW);
	config.configureFS(".");

	return config;
}

std::unordered_map<std::string, uint32_t> getCollectionCounts(
	const std::string& databaseName)
{
	std::unordered_map<std::string, uint32_t> results;

	if (databaseName == REPO_GTEST_DBNAME1)
	{
		results["3drepoBIM.history"] = 1;
		results["3drepoBIM.issues"] = 0;
		results["3drepoBIM.scene"] = 14;
		results["3drepoBIM.stash.3drepo"] = 17;
	}
	else
	{
		results["sphere.history"] = 1;
		results["sphere.issues"] = 0;
		results["sphere.scene"] = 3;
		results["sphere.stash.3drepo"] = 3;
	}

	return results;
}

std::vector<std::string> getCollectionList(
	const std::string& databaseName)
{
	if (databaseName == REPO_GTEST_DBNAME1)
	{
		return {
			"3drepoBIM.history",
			"3drepoBIM.history.ref",
			"3drepoBIM.issues",
			"3drepoBIM.scene",
			"3drepoBIM.scene.ref",
			"3drepoBIM.stash.3drepo",
			"3drepoBIM.stash.3drepo.ref",
			"fedTest.history",
			"fedTest.issues",
			"fedTest.scene",
			"settings",
		};
	}
	else
	{
		return{ "sphere.history","sphere.history.ref", "sphere.issues", "sphere.scene", "sphere.scene.ref", "sphere.stash.3drepo", "sphere.stash.3drepo.ref", "settings" };
	}
}

repo::lib::RepoBounds getGoldenDataForBBoxTest()
{
	return repo::lib::RepoBounds(
		repo::lib::RepoVector3D(-30.00954627990722700f, -15.00000476837158200f, -0.00000199999999495f),
		repo::lib::RepoVector3D(30.05025100708007800f, 60.69493103027343800f, 30.00000953674316400f)
	);
}

/* A set of preallocated & valid UUIDs used to produce golden data sets */

std::vector<repo::lib::RepoUUID> goldenUUIDs = std::vector<repo::lib::RepoUUID>(
	{
		repo::lib::RepoUUID("95577a50-1632-4c4c-90bd-e32087efba6a"),
		repo::lib::RepoUUID("bd00a0b1-f581-4b17-9336-6059ebcca759"),
		repo::lib::RepoUUID("455485a4-68af-43d7-9baf-6bed9a19c98d"),
		repo::lib::RepoUUID("52dbaab2-97cf-4d11-8e1c-fdf4b9d8e2ce"),
		repo::lib::RepoUUID("ede439fe-871b-4018-9fea-f86f2d95d57d"),
		repo::lib::RepoUUID("4ab8feb4-c173-4f1a-9b2b-0ce136f78786"),
		repo::lib::RepoUUID("89909db4-3a3c-47db-a8b0-1d151e519be1"),
		repo::lib::RepoUUID("4f451946-3bea-4344-b37e-9552f6fbbf80"),
		repo::lib::RepoUUID("42a29126-f6f6-47ec-a019-c17f2a7d1ccf"),
		repo::lib::RepoUUID("fa6a02dc-e241-4bd2-a6ee-8165fd2e5b18"),
		repo::lib::RepoUUID("7f99f62e-fd6f-45e1-8cbe-173eae284add"),
		repo::lib::RepoUUID("aa89469d-a43a-4722-a83e-5b7930264853"),
		repo::lib::RepoUUID("9c20a649-bbbe-4078-a823-2b18fcb06148"),
		repo::lib::RepoUUID("bf183884-730d-42ee-98bf-44bae337fade"),
		repo::lib::RepoUUID("2af28878-96a4-4499-be8d-87785e8f04da"),
		repo::lib::RepoUUID("21aa1237-af2e-436a-a9b5-951900845542"),
		repo::lib::RepoUUID("ddd963fa-c66a-45ee-8b38-403760c0393b"),
		repo::lib::RepoUUID("c0a0047b-f91a-445e-9186-37a49a77c62c"),
		repo::lib::RepoUUID("b1c92040-8d44-43c5-ba80-dd402f3254a6"),
		repo::lib::RepoUUID("e7db1a01-67e6-44ba-a3c8-392a1c3efcf4"),
		repo::lib::RepoUUID("3435fe9c-161b-4807-bcf3-57f62a658d41"),
		repo::lib::RepoUUID("56955cc7-49ca-45fd-85f1-200fc9fb20af"),
		repo::lib::RepoUUID("e70f2ee7-2a21-4fba-98f2-bdeff7586dde"),
		repo::lib::RepoUUID("52e1d22e-33ec-47ec-a8ca-190a32efd175"),
		repo::lib::RepoUUID("cc57de49-67f8-44ee-a99f-074b0cfa8987"),
		repo::lib::RepoUUID("bd50f843-9993-43bb-aba6-1a2521ccc092"),
		repo::lib::RepoUUID("7d1d15d7-0fc6-4e99-aa3d-0705c88e1bb7"),
		repo::lib::RepoUUID("e0c52572-1efd-4fcb-9a08-4785a3088bbc"),
		repo::lib::RepoUUID("6ddb79e0-1c7d-4463-9df3-e2708a4718f0"),
		repo::lib::RepoUUID("54a2d573-b16c-4b38-ba10-6c8cdcbad824"),
		repo::lib::RepoUUID("1f055426-d491-46e3-9527-ea8e5d2c7731"),
		repo::lib::RepoUUID("585879b9-492c-4ccd-aae2-72f3a717d13b"),
		repo::lib::RepoUUID("9c0eaf76-046a-440c-9c45-18a460074aa0"),
		repo::lib::RepoUUID("622a8667-ef76-417c-bba0-0c9ff9eae085"),
		repo::lib::RepoUUID("9054960f-ed2e-4548-9f59-1bbe766fbd49"),
		repo::lib::RepoUUID("7bbea295-0cbf-451d-a6dd-401b7bf550d3"),
		repo::lib::RepoUUID("b45c9fdf-3868-4534-8d30-f76419243230"),
		repo::lib::RepoUUID("87771979-a537-408b-abe0-457a29d9ea69"),
		repo::lib::RepoUUID("72a11c5e-0500-423b-8094-448f5d97e63b"),
		repo::lib::RepoUUID("134c8b04-c1fe-4988-a3be-32e5b899d679"),
		repo::lib::RepoUUID("fe45efe2-b84c-4442-aec1-fc328c045856"),
		repo::lib::RepoUUID("1015cfa4-93b4-4cfb-bccc-cb397889f553"),
		repo::lib::RepoUUID("5c99baf6-c344-44fa-871c-1ba5eacec984"),
		repo::lib::RepoUUID("a1a09e82-c65e-4c8e-bf05-04b20e4f42fe"),
		repo::lib::RepoUUID("0ea350ca-1cd2-46a2-97f0-b6c27f0d3998"),
		repo::lib::RepoUUID("d4b780ce-d2e2-4fe1-b297-a7a440fa969f"),
		repo::lib::RepoUUID("71e40bfc-2242-4b0d-b95f-aef0ce1abe3c"),
		repo::lib::RepoUUID("37234d13-bfc5-426b-b778-4a7776b5fff6"),
		repo::lib::RepoUUID("a20212e3-b308-4da3-8dab-d314f841d4fd"),
		repo::lib::RepoUUID("7a3054c7-9810-4136-aaec-fe9eafc0f6e6")
	}
);

std::vector<GoldenDocument> getGoldenData()
{
	// The golden data function produces a set of records procedurally, using the
	// golden UUIDs as keys.  These records should by byte identical in content and
	// order, regardless of the platform.
	// A collection in the database dump (golden1) mirrors these records.

	std::set<uint32_t> existing;
	std::vector<GoldenDocument> records;
	for (auto uuid : goldenUUIDs)
	{
		// We generate the data using pseudo-hash functions, using the contents
		// of the known UUIDs as seeds. To ensure repeatability, these functions
		// do not use system APIs, as they don't need to be strong - just
		// something that would not occur by chance that could lead to a false
		// negative during the tests.

		int32_t x0 = uuid.toString()[0] + uuid.toString()[1] + uuid.toString()[2];
		int32_t x1 = uuid.toString()[3] + uuid.toString()[12] + uuid.toString()[28];
		int32_t x2 = uuid.toString()[9] + uuid.toString()[10] + uuid.toString()[11];
		int32_t x3 = uuid.toString()[14] + uuid.toString()[15] + uuid.toString()[16];

		int32_t counter = (x0 ^ x1) + 1;
		while (existing.find(counter) != existing.end())
		{
			counter++;
		}
		existing.insert(counter);

		std::string name = uuid.toString();
		for (auto i = 0; i < name.length(); i++) {
			name[i] = (name[i] + i) % 254;
		}

		std::vector<uint8_t> bytes1;
		for (int i = 0; i < counter * 100; i++)
		{
			bytes1.push_back(i + x1 ^ x2 * x3);
		}

		std::vector<uint8_t> bytes2;
		for (int i = 0; i < counter * 100; i++)
		{
			bytes2.push_back(i * x1 | x2 ^ x3);
		}

		records.push_back({
			uuid,
			counter,
			name,
			bytes1,
			bytes2
		});
	}

	return records;
}