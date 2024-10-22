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

using namespace repo::core::model;
using namespace testing;

std::vector<repo::lib::RepoUUID> constantUUIDPool(
	{
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID(),
	}
);

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
	std::srand(seed);
	RepoBSONBuilder builder;

	int counter = 0;
	std::string prefix = std::string("field_");

	builder.append(prefix + std::to_string(counter++), std::to_string(rand()));
	builder.append(prefix + std::to_string(counter++), (int)rand());
	builder.append(prefix + std::to_string(counter++), (long long)rand());
	builder.append(prefix + std::to_string(counter++), (double)rand() / (double)rand());
	builder.append(prefix + std::to_string(counter++), (float)rand() / (double)rand());
	builder.append(prefix + std::to_string(counter++), constantUUIDPool[rand() % constantUUIDPool.size()]);
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