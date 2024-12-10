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
#include <repo/repo_controller.h>
#include <repo/core/model/bson/repo_node_metadata.h>
#include "repo_test_database_info.h"
#include "repo_test_fileservice_info.h"
#include "repo/lib/datastructure/repo_variant.h"
#include "repo/lib/datastructure/repo_variant_utils.h"
#include "repo/lib/datastructure/repo_vector.h"
#include <fstream>

namespace testing {

	repo::RepoController::RepoToken* initController(
		repo::RepoController* controller);

	bool projectExists(
		const std::string& db,
		const std::string& project);

	bool projectSettingsCheck(
		const std::string& dbName,
		const std::string& projectName,
		const std::string& owner,
		const std::string& tag,
		const std::string& desc);

	bool projectHasValidRevision(
		const std::string& dbName,
		const std::string& projectName);

	bool fileExists(const std::string& file);

	bool filesCompare(
		const std::string& fileA,
		const std::string& fileB);

	bool compareMaterialStructs(
		const repo_material_t& m1,
		const repo_material_t& m2);

	bool compareVectors(
		const repo_color4d_t& v1,
		const repo_color4d_t& v2);

	bool compareVectors(
		const std::vector<repo_color4d_t>& v1,
		const std::vector<repo_color4d_t>& v2);

	std::string getRandomString(const uint32_t& iLen);

	tm getRandomTm();

	// Searches all elements in a project for one with metadata matching the
	// key-value provided, and returns true if that element has geometry. Keys
	// and Values are case-sensitive.
	bool projectHasGeometryWithMetadata(
		std::string dbName,
		std::string projectName,
		std::string key,
		std::string value);

	// Finds all meta nodes that match the metadata criteria, and compares their
	// position in the transformation tree to the list in expected.
	bool projectHasMetaNodesWithPaths(
		std::string dbName,
		std::string projectName,
		std::string key,
		std::string value,
		std::vector<std::string> expected);

	repo::lib::RepoVector3D makeRepoVector();

	std::vector<uint8_t> makeRandomBinary(size_t size = 1000);

	repo::core::model::RepoBSON makeRandomRepoBSON(
		int seed,
		size_t numBinFiles,
		size_t binFileSize = 1000);

	// Returns a UUID that is a function of the random seed. If the seed is reset
	// with resetRand(), the sequence will restart.
	repo::lib::RepoUUID getRandUUID();

	// Resets the random seed. Use this instead of std::srand in order to restart
	// the UUID sequence as well.
	void restartRand(int seed = 1);

	template <typename T>
	static bool compareStdVectors(const std::vector<T>& v1, const std::vector<T>& v2)
	{
		bool identical;
		if (identical = v1.size() == v2.size())
		{
			for (int i = 0; i < v1.size(); ++i)
			{
				identical &= v1[i] == v2[i];
			}
		}
		return identical;
	}

	// Gets the number of fields in a RepoBSON - this is a test utility rather than
	// a RepoBSON method because it is best not to count methods at all, as usually
	// the same result for whatever reason there is to count them can be achieved
	// by iterating them directly, which is more performant.
	int nFields(const repo::core::model::RepoBSON& bson);
}