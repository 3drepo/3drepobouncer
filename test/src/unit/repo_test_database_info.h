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
#include <repo/lib/datastructure/repo_vector.h>
#include <repo/lib/datastructure/repo_bounds.h>
#include <repo/lib/repo_config.h>
#include <boost/filesystem.hpp>

//Test Database address
const static std::string REPO_GTEST_DBADDRESS = "localhost";
const static int REPO_GTEST_DBPORT = 27017;
const static std::string REPO_GTEST_AUTH_DATABASE = "admin";
const static std::string REPO_GTEST_DBUSER = "testUser";
const static std::string REPO_GTEST_DBPW = "3drepotest";
const static std::string REPO_GTEST_DBNAME1 = "sampleDataReadOnly";
const static std::string REPO_GTEST_DBNAME2 = "sampleDataReadOnly2";
const static std::string REPO_GTEST_DBNAME3 = "sandbox";
const static std::string REPO_GTEST_DBNAME4 = "testMongoDatabaseHandler";
const static std::string REPO_GTEST_DBNAME1_PROJ = "3drepoBIM";
const static std::string REPO_GTEST_DBNAME2_PROJ = "sphere";
const static std::string REPO_GTEST_DBNAME1_FED = "fedTest";
const static std::string REPO_GTEST_DBNAME_ROLEUSERTEST = "sampleDataRWRolesUsers";
const static std::string REPO_GTEST_DBNAME_FILE_MANAGER = "testFileManager";

const static std::string REPO_GTEST_COLNAME_FILE_MANAGER = "testFileUpload";
const static std::string REPO_GTEST_COLNAME_GOLDEN1 = "golden1";

const static std::string connectionConfig = "config/config.json";

const static std::string clientExe = "3drepobouncerClient";
const static std::string simpleModel = "cube.obj";
const static std::string texturedModel = "texturedPlane.dae";
const static std::string ifcModel_invalidSchema = "Wall.ifc";
const static std::string texturedModel2 = "texturedPlane2.dae"; //With Texture
const static std::string badExtensionFile = "cube.exe";
const static std::string ifcModel = "duplex.ifc";
const static std::string ifcModel2 = "duct.ifc";
const static std::string ifcModel_InfiniteLoop = "infiniteLoop.ifc";
const static std::string ifc4Model = "ifc4.ifc";
const static std::string ifc2x3Model = "ifc2x3.ifc";
const static std::string ifc4x3Model = "ifc4x3.ifc";
const static std::string ifc4x3_add2Model = "ifc4x3_add2.ifc";
const static std::string ifcSimpleHouse1 = "simpleHouse1.ifc";
const static std::string ifcSimpleHouse_doublyPlacedObjects = "simpleHouse_doublyPlacedObjects.ifc";
const static std::string ifcSimpleHouse_classification = "simpleHouse_classification.ifc";
const static std::string ifcSimpleHouse_valuesAndUnits = "simpleHouse_valuesAndUnits.ifc";
const static std::string ifcSimpleHouse_propertySets = "simpleHouse_propertySets.ifc";
const static std::string ifcSimpleHouse_propertySetsOfSets = "simpleHouse_propertySetsOfSets.ifc";
const static std::string ifcSimpleHouse_predefinedPropertySets = "simpleHouse_predefinedPropertySets.ifc";
const static std::string ifcSimpleHouse_quantitySets = "simpleHouse_quantitySets.ifc";
const static std::string ifcSimpleHouse_imperialUnits = "simpleHouse_imperialUnits.ifc";
const static std::string ifcSimpleHouse_definesByObject = "simpleHouse_definesByObject.ifc";
const static std::string ifcSimpleHouse_definesByType = "simpleHouse_definesByType.ifc";
const static std::string dgnModel = "sample.dgn";
const static std::string dwgModel = "box.dwg";
const static std::string dgnDrawing = "square.dgn";
const static std::string dwgDrawing = "square.dwg";
const static std::string dwgNestedBlocks = "nestedBlocks.dwg";
const static std::string dwgPasswordProtected = "passwordProtected.dwg";
const static std::string dxfModel = "box.dxf";
const static std::string rvtModel = "sample.rvt";
const static std::string rvtModel2021 = "sample2021.rvt";
const static std::string rvtModel2022 = "sample2022.rvt";
const static std::string nwdModel2022 = "sample2022.nwd";
const static std::string rvtModel2024 = "sample2024.rvt";
const static std::string nwdModel2024 = "sample2024.nwd";
const static std::string rvtModel2025 = "sample2025.rvt";
const static std::string nwdModel2025 = "sample2025.nwd";
const static std::string rvtModel2026 = "sample2026.rvt";
const static std::string nwdModel2026 = "sample2026.nwd";
const static std::string nwdPasswordProtected = "passwordProtected.nwd";
const static std::string nwcModel = "test.nwc";
const static std::string rvtNo3DViewModel = "sample_bad.rvt";
const static std::string rvtRoofTest = "RoofTest.rvt";
const static std::string rvtMeta1 = "MetaTest1.rvt";
const static std::string rvtMeta2 = "MetaTest2.rvt";
const static std::string rvtMeta3 = "MetaTest3.rvt";
const static std::string rvtHouse = "sampleHouse.rvt";
const static std::string unsupportedFBXVersion = "unsupported.FBX";
const static std::string synchroFile = "synchro.spm";
const static std::string synchroWithTransform = "withTransformations.spm";
const static std::string synchroOldVersion = "synchro6_1.spm";
const static std::string synchroVersion6_3 = "synchro6_3.spm";
const static std::string synchroVersion6_4 = "synchro6_4.spm";
const static std::string synchroVersion6_5 = "synchro6_5.spm";
const static std::string synchroVersion6_5_3_7 = "synchro6_5_3_7.spm";

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

const static std::string  processDrawingConfig = "processDrawingConfig.json";

const static std::string getFileFileName = "a0205d17-e73c-4d3f-ad1b-8b875cb5f342cube_obj";
const static std::string getFileNameBIMModel = "5be1aca9-e4d0-4cec-987d-80d2fde3dade3DrepoBIM_obj";

const static std::string genFedDB = "genFedTest";
const static std::string genFedNoSubProName = "noSubPro";
const static std::string genFedSuccessName = "fedTest";

const static std::pair<std::string, std::string> REPO_GTEST_DROPCOL_TESTCASE = { "sampleDataRW", "collectionToDrop" };
const static std::string REPO_GTEST_RAWFILE_FETCH_TEST = "gridFSFile";
const static size_t REPO_GTEST_RAWFILE_FETCH_SIZE = 1024;

std::string getConnConfig();

std::string getClientExePath();

repo::lib::RepoConfig getConfig();

std::string getDataPath(const std::string& file);

std::shared_ptr<repo::core::handler::MongoDatabaseHandler> getHandler();

repo::lib::RepoBounds getGoldenDataForBBoxTest();

/*
* Get expected #items in collection count within testCases
* be sure to update this with any changes within the testCases database
*/
std::unordered_map<std::string, uint32_t> getCollectionCounts(
	const std::string& databaseName
);

std::vector<std::string> getCollectionList(
	const std::string& databaseName
);

struct GoldenDocument
{
	repo::lib::RepoUUID _id;
	int32_t counter;
	std::string name;
	std::vector<uint8_t> myData1;
	std::vector<uint8_t> myData2;
};

std::vector<GoldenDocument> getGoldenData();