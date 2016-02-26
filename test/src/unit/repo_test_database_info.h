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


#include <repo/core/model/bson/repo_bson_builder.h>


//Test Database address
const static std::string REPO_GTEST_DBADDRESS = "localhost";
const static uint32_t    REPO_GTEST_DBPORT = 27017;
const static std::string REPO_GTEST_AUTH_DATABASE = "admin";
const static std::string REPO_GTEST_DBUSER = "testUser";
const static std::string REPO_GTEST_DBPW   = "3drepotest";
const static std::string REPO_GTEST_DBNAME1 = "sampleDataReadOnly";
const static std::string REPO_GTEST_DBNAME2 = "sampleDataReadOnly2";
const static std::string REPO_GTEST_DBNAME1_PROJ = "3drepoBIM";
const static std::string REPO_GTEST_DBNAME2_PROJ = "sphere";
const static std::string REPO_GTEST_DBNAME_ROLEUSERTEST = "sampleDataRWRolesUsers";

const static mongo::BSONObj REPO_GTEST_DROPROLETEST = BSON("db" << REPO_GTEST_DBNAME_ROLEUSERTEST << "role" <<  "dropRoleTest");
const static mongo::BSONObj REPO_GTEST_DROPUSERTEST = BSON("db" << "admin" << "user" << "dropUserTest");
const static mongo::BSONObj REPO_GTEST_UPDATEROLETEST = BSON("db" << REPO_GTEST_DBNAME_ROLEUSERTEST << "role" << "updateRole");
const static mongo::BSONObj REPO_GTEST_UPDATEUSERTEST = BSON("db" << "admin" << "user" << "updateUserTest");
const static std::vector<repoUUID> uuidsToSearch = { stringToUUID("0ab45528-9258-421a-927c-c51bf40fc478"), stringToUUID("126f9de3-c942-4d66-862a-16cc4f11841b") };

const static std::pair<std::string, std::string> REPO_GTEST_DROPCOL_TESTCASE = {"sampleDataRW", "collectionToDrop"};
const static std::string REPO_GTEST_RAWFILE_FETCH_TEST = "5be1aca9-e4d0-4cec-987d-80d2fde3dade3DrepoBIM_obj";
const static size_t REPO_GTEST_RAWFILE_FETCH_SIZE = 6050508;

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
		results["3drepoBIM.history.chunks"] = 24;
		results["3drepoBIM.history.files"] = 1;
		results["3drepoBIM.issues"] = 0;
		results["3drepoBIM.scene"] = 14;
		results["3drepoBIM.stash.3drepo"] = 17; 
	}
	else
	{
		results["sphere.history"] = 1;
		results["sphere.history.chunks"] = 20;
		results["sphere.history.files"] = 1;
		results["sphere.issues"] = 0;
		results["sphere.scene"] = 3;
		results["sphere.scene.chunks"] = 69;
		results["sphere.scene.files"] = 1;
		results["sphere.stash.3drepo"] = 3;
		results["sphere.stash.3drepo.chunks"] = 138;
		results["sphere.stash.3drepo.files"] = 2;
	}


	return results;

}

static std::vector<std::string> getCollectionList(
	const std::string &databaseName)
{
	if (databaseName == REPO_GTEST_DBNAME1)
	{
		return { "3drepoBIM.history", "3drepoBIM.history.chunks", "3drepoBIM.history.files", "3drepoBIM.issues", "3drepoBIM.scene", "3drepoBIM.stash.3drepo", "settings", "system.indexes"};
	}
	else
	{
		return{ "sphere.history", "sphere.history.chunks", "sphere.history.files", "sphere.issues", "sphere.scene", "sphere.scene.files", "sphere.scene.chunks", "sphere.stash.3drepo", "sphere.stash.3drepo.chunks", "sphere.stash.3drepo.files", "settings", "system.indexes" };
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
		<< "paddingFactor" << 1.0000000000000000
		<< "systemFlags" << 1
		//<< "userFlags" << 1
		<< "totalIndexSize" << 8176
		<< "indexSizes" 
		<< BSON("_id_" << 8176)
		<< "ok" << 1.0000000000000000
		);

	return results;
}

static std::pair<std::pair<std::string, std::string>, mongo::BSONObj> getDataForDropCase()
{
	std::pair<std::pair<std::string, std::string>, mongo::BSONObj> result;
	result.first = { "sampleDataRW", "collectionToDrop" };
	repo::core::model::RepoBSONBuilder builder;
	builder.append("_id", stringToUUID("0ab45528-9258-421a-927c-c51bf40fc478"));
	result.second = builder.obj();

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