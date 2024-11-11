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

#define NOMINMAX

#include "repo_test_utils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>
#include <time.h>

using namespace repo::core::model;
using namespace testing;

// Hidden pool of UUIDs for use by getRandUUID
std::vector<repo::lib::RepoUUID> uuidPool;
int uuidPoolCounter = 0;

repo::RepoController::RepoToken* testing::initController(repo::RepoController* controller) {
	repo::lib::RepoConfig config = { REPO_GTEST_DBADDRESS, REPO_GTEST_DBPORT,
		REPO_GTEST_DBUSER, REPO_GTEST_DBPW };

	config.configureFS("./", 2);
	std::string errMsg;
	return controller->init(errMsg, config);
}

repo::lib::RepoVector3D testing::makeRepoVector()
{
	repo::lib::RepoVector3D v;
	v.x = (double)(rand() - rand()) / rand();
	v.y = (double)(rand() - rand()) / rand();
	v.z = (double)(rand() - rand()) / rand();
	return v;
}

std::vector<uint8_t> testing::makeRandomBinary(size_t size)
{
	std::vector<uint8_t> bin;
	for (int i = 0; i < size; i++)
	{
		bin.push_back(rand() % 256);
	}
	return bin;
}

repo::core::model::RepoBSON testing::makeRandomRepoBSON(int seed, size_t numBinFiles, size_t binFileSize)
{
	restartRand();
	RepoBSONBuilder builder;

	int counter = 0;
	std::string prefix = std::string("field_");

	builder.append(prefix + std::to_string(counter++), std::to_string(rand()));
	builder.append(prefix + std::to_string(counter++), (int)rand());
	builder.append(prefix + std::to_string(counter++), (long long)rand());
	builder.append(prefix + std::to_string(counter++), (double)rand() / (double)rand());
	builder.append(prefix + std::to_string(counter++), (float)rand() / (double)rand());
	builder.append(prefix + std::to_string(counter++), getRandUUID());
	std::vector<float> matrixData;
	for (size_t i = 0; i < 16; i++)
	{
		matrixData.push_back(rand());
	}
	repo::lib::RepoMatrix m(matrixData);
	builder.append(prefix + std::to_string(counter++), m);
	builder.append(prefix + std::to_string(counter++), makeRepoVector());
	builder.appendVector3DObject(prefix + std::to_string(counter++), makeRepoVector());
	builder.append(prefix + std::to_string(counter++), getRandomTm());
	builder.append(prefix + std::to_string(counter++), rand() % 2 == 0);

	return builder.obj();
}

repo::lib::RepoUUID testing::getRandUUID()
{
	if (!uuidPool.size()) {
		for (auto i = 0; i < 100000; i++) {
			uuidPool.push_back(repo::lib::RepoUUID::createUUID());
		}
	}
	return uuidPool[uuidPoolCounter++]; // Tests should not use more than 100000 UUIDs, but if so, this will throw
}

void testing::restartRand()
{
	std::srand(1);
	uuidPoolCounter = 0;
}

bool testing::projectHasMetaNodesWithPaths(std::string dbName, std::string projectName, std::string key, std::string value, std::vector<std::string> expected)
{
	repo::RepoController* controller = new repo::RepoController();
	auto token = initController(controller);

	std::vector<std::string> paths;

	if (token)
	{
		auto scene = controller->fetchScene(token, dbName, projectName, REPO_HISTORY_MASTER_BRANCH, true, false, false, { repo::core::model::ModelRevisionNode::UploadStatus::MISSING_BUNDLES });
		if (scene)
		{
			auto metadata = scene->getAllMetadata(repo::core::model::RepoScene::GraphType::DEFAULT);
			auto transforms = scene->getAllTransformations(repo::core::model::RepoScene::GraphType::DEFAULT);

			std::unordered_map<repo::lib::RepoUUID, repo::core::model::RepoNode*, repo::lib::RepoUUIDHasher> sharedIdToNode;

			for (auto m : transforms) {
				sharedIdToNode[m->getSharedID()] = m;
			}

			for (auto m : metadata) {
				auto metaDataNode = dynamic_cast<repo::core::model::MetadataNode*>(m);
				auto metaDataArray = metaDataNode->getAllMetadata();
				for (auto entry : metaDataArray)
				{
					auto aKey = entry.first;
					std::string aValue = boost::apply_visitor(repo::lib::StringConversionVisitor(), entry.second);

					if (aKey == key && aValue == value)
					{
						// This metadata node contains the key-value pair we are looking for. Now
						// check its position in the tree.

						auto path = metaDataNode->getName();
						auto parents = metaDataNode->getParentIDs();

						while (parents.size() > 0)
						{
							auto parent = sharedIdToNode[parents[0]];
							path = parent->getName() + "->" + path;
							parents = parent->getParentIDs();
						}

						paths.push_back(path);
					}
				}
			}
			delete scene;
		}
	}
	controller->disconnectFromDatabase(token);
	delete controller;

	if (paths.size() != expected.size())
	{
		return false;
	}

	int found = 0;

	for (auto p : expected)
	{
		for (size_t i = 0; i < paths.size(); i++)
		{
			if (paths[i] == p)
			{
				found++;
				paths.erase(paths.begin() + i);
				break;
			}
		}
	}

	if (found != expected.size())
	{
		return false;
	}

	return true;
}

bool testing::projectHasGeometryWithMetadata(std::string dbName, std::string projectName, std::string key, std::string value)
{
	bool res = false;
	repo::RepoController* controller = new repo::RepoController();
	auto token = initController(controller);

	if (token)
	{
		auto scene = controller->fetchScene(token, dbName, projectName, REPO_HISTORY_MASTER_BRANCH, true, true, false, { repo::core::model::ModelRevisionNode::UploadStatus::MISSING_BUNDLES });
		if (scene)
		{
			auto metadata = scene->getAllMetadata(repo::core::model::RepoScene::GraphType::DEFAULT);
			auto meshes = scene->getAllMeshes(repo::core::model::RepoScene::GraphType::DEFAULT);

			std::unordered_map<repo::lib::RepoUUID, repo::core::model::MeshNode*, repo::lib::RepoUUIDHasher > sharedIdToMeshNode;

			for (auto m : meshes) {
				auto meshNode = dynamic_cast<repo::core::model::MeshNode*>(m);
				sharedIdToMeshNode[meshNode->getSharedID()] = meshNode;
			}

			for (auto m : metadata) {
				auto metaDataNode = dynamic_cast<repo::core::model::MetadataNode*>(m);
				auto metaDataArray = metaDataNode->getAllMetadata();
				for (auto entry : metaDataArray)
				{
					auto aKey = entry.first;
					std::string aValue = boost::apply_visitor(repo::lib::StringConversionVisitor(), entry.second);

					// Note: the string conversion encloses values that are stored as strings in the DB with \"
					// We will need to remove them to guarantee a correct comparison
					std::string sanitisedValue;
					if (aValue.length() > 2 && aValue.substr(0, 1) == "\"" && aValue.substr(aValue.length() - 1, 1) == "\"") {
						sanitisedValue = aValue.substr(1, aValue.length() - 2);
					}
					else {
						sanitisedValue = aValue;
					}


					if (aKey == key && sanitisedValue == value)
					{
						// This metadata node contains the key-value pair we are looking for. Now
						// check if there is geometry associated with it.

						for (auto p : metaDataNode->getParentIDs())
						{
							if (sharedIdToMeshNode.find(p) != sharedIdToMeshNode.end())
							{
								if (sharedIdToMeshNode[p]->getNumVertices() >= 0)
								{
									res = true;
									break;
								}
							}
						}
					}

					if (res)
					{
						break;
					}
				}

				if (res)
				{
					break;
				}
			}
			delete scene;
		}
	}
	controller->disconnectFromDatabase(token);
	delete controller;
	return res;
}

bool testing::projectExists(
	const std::string& db,
	const std::string& project)
{
	auto handler = getHandler();
	for (auto collection : handler->getCollections(db)) {
		if (collection == (project + ".history")) {
			return true;
		}
	}
	return false;
}

bool testing::projectSettingsCheck(
	const std::string& dbName, const std::string& projectName, const std::string& owner, const std::string& tag, const std::string& desc)
{
	bool res = false;
	repo::RepoController* controller = new repo::RepoController();
	auto token = initController(controller);

	if (token)
	{
		auto scene = controller->fetchScene(token, dbName, projectName, REPO_HISTORY_MASTER_BRANCH, true, true, true, { repo::core::model::ModelRevisionNode::UploadStatus::MISSING_BUNDLES });
		if (scene)
		{
			res = scene->getOwner() == owner && scene->getTag() == tag && scene->getMessage() == desc;
			delete scene;
		}
	}
	controller->disconnectFromDatabase(token);
	delete controller;
	return res;
}

bool testing::projectHasValidRevision(
	const std::string& dbName, const std::string& projectName)
{
	bool res = false;
	repo::RepoController* controller = new repo::RepoController();
	auto token = initController(controller);
	if (token)
	{
		auto scene = controller->fetchScene(token, dbName, projectName, REPO_HISTORY_MASTER_BRANCH, true, true);
		if (res = scene)
		{
			delete scene;
		}
	}
	controller->disconnectFromDatabase(token);
	delete controller;
	return res;
}

bool testing::fileExists(
	const std::string& file)
{
	std::ifstream ofs(file);
	const bool valid = ofs.good();
	ofs.close();
	return valid;
}

bool testing::filesCompare(
	const std::string& fileA,
	const std::string& fileB)
{
	bool match = false;
	std::ifstream fA(fileA), fB(fileB);
	if (fA.good() && fB.good())
	{
		std::string lineA, lineB;
		bool endofA, endofB;
		while ((endofA = (bool)std::getline(fA, lineA)) && (endofB = (bool)std::getline(fB, lineB)))
		{
			match = lineA == lineB;
			if (!match)
			{
				std::cout << "Failed match. " << std::endl;
				std::cout << "line A: " << lineA << std::endl;
				std::cout << "line B: " << lineB << std::endl;
				break;
			}
		}

		if (!endofA)
		{
			//if endofA is false then end of B won't be found as getline wouldn't have ran for fB
			endofB = (bool)std::getline(fB, lineB);
		}

		match &= (!endofA && !endofB);
	}

	return match;
}

bool testing::compareMaterialStructs(const repo_material_t& m1, const repo_material_t& m2)
{
	return compareStdVectors(m1.ambient, m2.ambient)
		&& compareStdVectors(m1.diffuse, m2.diffuse)
		&& compareStdVectors(m1.specular, m2.specular)
		&& compareStdVectors(m1.emissive, m2.emissive)
		&& m1.opacity == m2.opacity
		&& m1.shininess == m2.shininess
		&& m1.shininessStrength == m2.shininessStrength
		&& m1.lineWeight == m2.lineWeight
		&& m1.isWireframe == m2.isWireframe
		&& m1.isTwoSided == m2.isTwoSided;
}

std::string testing::getRandomString(const uint32_t& iLen)
{
	std::string sStr;
	sStr.reserve(iLen);
	char syms[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	unsigned int Ind = 0;
	srand(time(NULL) + rand());
	for (int i = 0; i < iLen; ++i);
	{
		sStr.push_back(syms[rand() % 62]);
	}

	return sStr;
}

bool testing::compareVectors(const repo_color4d_t& v1, const repo_color4d_t& v2)
{
	return v1.r == v2.r && v1.g == v2.g && v1.b == v2.b && v1.a == v2.a;
}

bool testing::compareVectors(const std::vector<repo_color4d_t>& v1, const std::vector<repo_color4d_t>& v2)
{
	if (v1.size() != v2.size()) return false;
	bool match = true;
	for (int i = 0; i < v1.size(); ++i)
	{
		match &= compareVectors(v1[i], v2[i]);
	}

	return match;
}

tm testing::getRandomTm()
{
	// Do this instead of building a tm struct directly, because the struct
	// has day entries etc that can be quite tricky to get right...
	time_t ten_years = 365 * 12 * 30 * 24 * 60;
	auto t = time(0) + ((time_t)rand() % ten_years) - ((time_t)rand() % ten_years);
	tm tm = *localtime(&t);
	return tm;
}