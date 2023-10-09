/**
*  Copyright (C) 2016 3D Repo Ltd
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

#pragma once
#include <repo/core/handler/repo_database_handler_mongo.h>
#include <repo/core/handler/fileservice/repo_file_manager.h>
#include <repo/lib/repo_config.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include <boost/filesystem.hpp>

//Test Database address
const static std::string REPO_GTEST_DBADDRESS = "localhost";
const static int REPO_GTEST_DBPORT = 27017;
const static std::string REPO_GTEST_AUTH_DATABASE = "admin";
const static std::string REPO_GTEST_DBUSER = "testUser";
const static std::string REPO_GTEST_DBPW = "3drepotest";
const static std::string REPO_GTEST_DBNAME1 = "sampleDataReadOnly";
const static std::string REPO_GTEST_DBNAME2 = "sampleDataReadOnly2";
const static std::string REPO_GTEST_DBNAME1_PROJ = "3drepoBIM";
const static std::string REPO_GTEST_DBNAME2_PROJ = "sphere";
const static std::string REPO_GTEST_DBNAME1_FED = "fedTest";
const static std::string REPO_GTEST_DBNAME_ROLEUSERTEST = "sampleDataRWRolesUsers";
const static std::string REPO_GTEST_DBNAME_FILE_MANAGER = "testFileManager";

const static std::string REPO_GTEST_COLNAME_FILE_MANAGER = "testFileUpload";

const static std::string connectionConfig = "config/config.json";

const static std::string clientExe = "3drepobouncerClient";
const static std::string simpleModel = "cube.obj";
const static std::string texturedModel = "texturedPlane.dae";
const static std::string missingNodesModel = "Wall.ifc";
const static std::string texturedModel2 = "texturedPlane2.dae"; //With Texture
const static std::string badExtensionFile = "cube.exe";
const static std::string ifcModel = "duplex.ifc";
const static std::string ifcModel_InfiniteLoop = "infiniteLoop.ifc";
const static std::string ifc4Model = "ifc4Test.ifc";
const static std::string dgnModel = "sample.dgn";
const static std::string dwgModel = "box.dwg";
const static std::string dxfModel = "box.dxf";
const static std::string rvtModel = "sample.rvt";
const static std::string rvtModel2021 = "sample2021.rvt";
const static std::string rvtModel2022 = "sample2022.rvt";
const static std::string nwdModel2022 = "sample2022.nwd";
const static std::string rvtModel2024 = "sample2024.rvt";
const static std::string nwdModel2024 = "sample2024.nwd";
const static std::string nwcModel = "test.nwc";
const static std::string rvtNo3DViewModel = "sample_bad.rvt";
const static std::string rvtRoofTest = "RoofTest.rvt";
const static std::string rvtMeta1 = "MetaTest1.rvt";
const static std::string rvtMeta2 = "MetaTest2.rvt";
const static std::string rvtHouse = "sampleHouse.rvt";
const static std::string unsupportedFBXVersion = "unsupported.FBX";
const static std::string synchroFile = "synchro.spm";
const static std::string synchroWithTransform = "withTransformations.spm";
const static std::string synchroOldVersion = "synchro6_1.spm";
const static std::string synchroVersion6_3 = "synchro6_3.spm";
const static std::string synchroVersion6_4 = "synchro6_4.spm";
const static std::string synchroVersion6_5 = "synchro6_5.spm";

const static std::string emptyFile = "empty.json";
const static std::string emptyJSONFile = "empty2.json";
const static std::string noSubProjectJSONFile = "noSubPro.json";
const static std::string noDbNameJSONFile = "noDb.json";
const static std::string noProNameJSONFile = "noPro.json";
const static std::string invalidJSONFile = "invalid.json";
const static std::string validGenFedJSONFile = "validFed.json";

const static std::string  importSuccess = "successFileImport.json";
const static std::string  importSuccess2 = "successFileImport2.json";
const static std::string  importNoFile = "importNoFile.json";
const static std::string  importbadDir = "importBadDir.json";
const static std::string  importbadDir2 = "importBadDir2.json";
const static std::string  importNoDatabase = "importNoDatabase.json";
const static std::string  importNoDatabase2 = "importNoDatabase2.json";
const static std::string  importNoProject = "importNoProject.json";
const static std::string  importNoProject2 = "importNoProject2.json";
const static std::string  importNoOwner = "importNoOwner.json";
const static std::string  importNoOwner2 = "importNoOwner2.json";

const static std::string  importSuccessPro = "success1";
const static std::string  importSuccessPro2 = "success2";
const static std::string  importSuccess2Tag = "taggg";
const static std::string  importSuccess2Desc = "desccc";
const static std::string  importNoOwnerPro = "owner1";
const static std::string  importNoOwnerProTag = "thisTag";
const static std::string  importNoOwnerProDesc = "MyUpload";
const static std::string  importNoOwnerPro2 = "owner2";
const static std::string  importNoOwnerPro2Tag = "thisTag";
const static std::string  importNoOwnerPro2Desc = "MyUpload";

const static std::string getFileFileName = "a0205d17-e73c-4d3f-ad1b-8b875cb5f342cube_obj";
const static std::string getFileNameBIMModel = "5be1aca9-e4d0-4cec-987d-80d2fde3dade3DrepoBIM_obj";

const static std::string genFedDB = "genFedTest";
const static std::string genFedNoSubProName = "noSubPro";
const static std::string genFedSuccessName = "fedTest";

const static mongo::BSONObj REPO_GTEST_DROPROLETEST = BSON("db" << REPO_GTEST_DBNAME_ROLEUSERTEST << "role" << "dropRoleTest");
const static mongo::BSONObj REPO_GTEST_DROPUSERTEST = BSON("db" << "admin" << "user" << "dropUserTest");
const static mongo::BSONObj REPO_GTEST_UPDATEROLETEST = BSON("db" << REPO_GTEST_DBNAME_ROLEUSERTEST << "role" << "updateRole");
const static mongo::BSONObj REPO_GTEST_UPDATEUSERTEST = BSON("db" << "admin" << "user" << "updateUserTest");
const static std::vector<repo::lib::RepoUUID> uuidsToSearch = { repo::lib::RepoUUID("0ab45528-9258-421a-927c-c51bf40fc478"), repo::lib::RepoUUID("126f9de3-c942-4d66-862a-16cc4f11841b") };

const static std::pair<std::string, std::string> REPO_GTEST_DROPCOL_TESTCASE = { "sampleDataRW", "collectionToDrop" };
const static std::string REPO_GTEST_RAWFILE_FETCH_TEST = "gridFSFile";
const static size_t REPO_GTEST_RAWFILE_FETCH_SIZE = 1024;

static repo::lib::RepoConfig getConfig()
{
	auto config = repo::lib::RepoConfig(REPO_GTEST_DBADDRESS, REPO_GTEST_DBPORT,
		REPO_GTEST_DBUSER, REPO_GTEST_DBPW);
	config.configureFS(".");

	return config;
}

static repo::core::handler::MongoDatabaseHandler* getHandler()
{
	std::string errMsg;

	auto handler = repo::core::handler::MongoDatabaseHandler::getHandler(errMsg, REPO_GTEST_DBADDRESS, REPO_GTEST_DBPORT,
		1,
		REPO_GTEST_AUTH_DATABASE,
		REPO_GTEST_DBUSER, REPO_GTEST_DBPW);

	static bool first = true;

	if (first) {
		repo::core::handler::fileservice::FileManager::instantiateManager(getConfig(), handler);
		first = false;
	}

	return handler;
}

static std::vector<repo::lib::RepoVector3D> getGoldenDataForBBoxTest()
{
	return{ repo::lib::RepoVector3D(-30.00954627990722700f, -15.00000476837158200f, -0.00000199999999495f),
			repo::lib::RepoVector3D(30.05025100708007800f, 60.69493103027343800f, 30.00000953674316400f) };
}

static std::string getClientExePath()
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

static std::string getDataPath(
	const std::string &file)
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

static std::string getConnConfig() {
	return getDataPath(connectionConfig);
}

/*
* Get expected #items in collection count within testCases
* be sure to update this with any changes within the testCases database
*/
static std::unordered_map<std::string, uint32_t> getCollectionCounts(
	const std::string &databaseName)
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

static std::vector<std::string> getCollectionList(
	const std::string &databaseName)
{
	if (databaseName == REPO_GTEST_DBNAME1)
	{
		return{ "3drepoBIM.history", "3drepoBIM.history.ref", "3drepoBIM.issues", "3drepoBIM.scene", "3drepoBIM.scene.ref", "3drepoBIM.stash.3drepo", "3drepoBIM.stash.3drepo.ref",
			"fedTest.history", "fedTest.issues", "fedTest.scene"
			, "settings" };
	}
	else
	{
		return{ "sphere.history","sphere.history.ref", "sphere.issues", "sphere.scene", "sphere.scene.ref", "sphere.stash.3drepo", "sphere.stash.3drepo.ref", "settings" };
	}
}

static std::pair <std::pair<std::string, std::string>, mongo::BSONObj> getCollectionStats()
{
	std::pair <std::pair<std::string, std::string>, mongo::BSONObj> results;

	results.first = { REPO_GTEST_DBNAME1, "3drepoBIM.scene" };
	results.second = BSON("ns" << "sampleDataReadOnly.3drepoBIM.scene"
		<< "count" << 14
		//<< "size" << 18918176
		//<< "avgObjSize" <<  1351298
		//<< "storageSize" << 33562624
		//<< "numExtents" << 2
		<< "nindexes" << 1
		//<< "lastExtentSize" << 33554432
		//<< "paddingFactor" << 1.0000000000000000
		//<< "systemFlags" << 1
		//<< "userFlags" << 1
		/*<< "totalIndexSize" << 8176
		<< "indexSizes"
		<< BSON("_id_" << 8176)*/
		<< "ok" << 1.0000000000000000
	);

	return results;
}

static std::pair<std::pair<std::string, std::string>, mongo::BSONObj> getDataForDropCase()
{
	std::pair<std::pair<std::string, std::string>, mongo::BSONObj> result;
	result.first = { "sampleDataRW", "collectionToDrop" };
	repo::core::model::RepoBSONBuilder builder;
	builder.append("_id", repo::lib::RepoUUID("0ab45528-9258-421a-927c-c51bf40fc478"));
	result.second = builder.mongoObj();

	return result;
}

static std::pair<std::pair<std::string, std::string>, std::vector<std::string>> getGoldenForGetAllFromCollectionTailable()
{
	std::pair<std::pair<std::string, std::string>, std::vector<std::string>> results;

	results.first = { REPO_GTEST_DBNAME2, "sphere.stash.3drepo" };
	results.second = {
		"{ _id: BinData(3, 816B8599779B44439CB35C5D8D14871F), rev_id: BinData(3, 297DB5A5E7024ECAB4F2EF098D13C278), parents: [ BinData(3, 8E7F856993704399896EE407731B947E) ], shared_id: BinData(3, 2B82A769B4784C0CA7FD51D40289050E), type: \"material\", api: 1, ambient: [ 0.05000000074505806, 0.05000000074505806, 0.05000000074505806 ], diffuse: [ 0.6000000238418579, 0.6000000238418579, 0.6000000238418579 ], specular: [ 0.6000000238418579, 0.6000000238418579, 0.6000000238418579 ], wireframe: true, two_sided: true }",
		"{ _id: BinData(3, E625D5737A694313873AB231E8708360), rev_id: BinData(3, 297DB5A5E7024ECAB4F2EF098D13C278), shared_id: BinData(3, 11553A7A497C4E748D60F9B99B3AFF57), type: \"transformation\", api: 1, name: \"<dummy_root>\", matrix: [ [ 1.0, 0.0, 0.0, 0.0 ], [ 0.0, 1.0, 0.0, 0.0 ], [ 0.0, 0.0, 1.0, 0.0 ], [ 0.0, 0.0, 0.0, 1.0 ] ] }",
		"{ _extRef: { normals: \"6b7d6af3-850e-463e-a450-3593b75b3ea3_normals\", vertices: \"6b7d6af3-850e-463e-a450-3593b75b3ea3_vertices\" }, _id: BinData(3, 6B7D6AF3850E463EA4503593B75B3EA3), rev_id: BinData(3, 297DB5A5E7024ECAB4F2EF098D13C278), m_map: [ { map_id: BinData(3, C3ED4CB1D9A94747963E2B89ACBF7096), mat_id: BinData(3, 816B8599779B44439CB35C5D8D14871F), v_from: 0, v_to: 1497000, t_from: 0, t_to: 499000, bounding_box: [ [ -1.0, -1.0, -1.0 ], [ 1.0, 1.0, 1.0 ] ] } ], parents: [ BinData(3, 11553A7A497C4E748D60F9B99B3AFF57) ], shared_id: BinData(3, 8E7F856993704399896EE407731B947E), type: \"mesh\", api: 1, bounding_box: [ [ -1.0, -1.0, -1.0 ], [ 1.0, 1.0, 1.0 ] ], outline: [ [ -1.0, -1.0 ], [ 1.0, -1.0 ], [ 1.0, 1.0 ], [ -1.0, 1.0 ] ], faces_count: 499000, faces: BinData(0, 03000000000000000100000002000000030000000300000004000000050000000300000006000000070000000800000003000000090000000A0000000B000000030000000C00...) }"
	};
	return results;
}