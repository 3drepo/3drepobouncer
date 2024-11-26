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

#include <cstdlib>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>

#include <repo/core/model/bson/repo_bson_builder.h>
#include <repo/core/model/bson/repo_bson_factory.h>

#include "../../../../repo_test_utils.h"
#include "../../../../repo_test_matchers.h"

using namespace repo::core::model;
using namespace testing;

TEST(RepoTaskTest, Serialise)
{
	auto name = "name";
	long long startTime = std::time(0) - 1000;
	long long endTime = std::time(0) + 1000;
	auto sequenceId = repo::lib::RepoUUID::createUUID();

	std::unordered_map<std::string, std::string> data;
	data[repo::lib::RepoUUID::createUUID().toString()] = repo::lib::RepoUUID::createUUID().toString();
	data[repo::lib::RepoUUID::createUUID().toString()] = repo::lib::RepoUUID::createUUID().toString();
	data[repo::lib::RepoUUID::createUUID().toString()] = repo::lib::RepoUUID::createUUID().toString();
	data[repo::lib::RepoUUID::createUUID().toString()] = repo::lib::RepoUUID::createUUID().toString();

	std::vector<repo::lib::RepoUUID> resources;
	resources.push_back(repo::lib::RepoUUID::createUUID());
	resources.push_back(repo::lib::RepoUUID::createUUID());
	resources.push_back(repo::lib::RepoUUID::createUUID());
	resources.push_back(repo::lib::RepoUUID::createUUID());

	auto parent = repo::lib::RepoUUID::createUUID();
	auto id = repo::lib::RepoUUID::createUUID();

	RepoBSON task = RepoBSONFactory::makeTask(
		name,
		startTime,
		endTime,
		sequenceId,
		data,
		resources,
		parent,
		id
	);

	EXPECT_THAT(task.getUUIDField(REPO_LABEL_ID), Eq(id));
	EXPECT_THAT(task.getStringField(REPO_TASK_LABEL_NAME), Eq(name));
	EXPECT_THAT(task.getLongField (REPO_TASK_LABEL_START), Eq(startTime));
	EXPECT_THAT(task.getLongField(REPO_TASK_LABEL_END), Eq(endTime));
	EXPECT_THAT(task.getUUIDField(REPO_TASK_LABEL_SEQ_ID), Eq(sequenceId));
	EXPECT_THAT(task.getUUIDField(REPO_TASK_LABEL_PARENT), Eq(parent));

	EXPECT_THAT(task.hasField(REPO_TASK_LABEL_RESOURCES), IsTrue());
	auto resourcesField = task.getObjectField(REPO_TASK_LABEL_RESOURCES);
	EXPECT_THAT(resourcesField.getUUIDFieldArray(REPO_TASK_SHARED_IDS), UnorderedElementsAreArray(resources));

	EXPECT_THAT(task.hasField(REPO_TASK_LABEL_DATA), IsTrue());
	auto metaField = task.getObjectField(REPO_TASK_LABEL_DATA);

	// This test doesn't contain any metadata that may be converted to integers or
	// doubles, so a direct comparison will work

	EXPECT_THAT(metaField.nFields(), Eq(data.size()));
	for (auto& n : metaField.getFieldNames()) {
		auto entry = metaField.getObjectField(n);
		auto key = entry.getStringField(REPO_TASK_META_KEY);
		auto value = entry.getStringField(REPO_TASK_META_VALUE);
		EXPECT_THAT(data[key], Eq(value));
	}

	// Parent and id should be given values

	task = RepoBSONFactory::makeTask(
		name,
		startTime,
		endTime,
		sequenceId,
		data,
		resources
	);

	EXPECT_THAT(task.getUUIDField(REPO_LABEL_ID), Not(Eq(id)));
	EXPECT_THAT(task.getUUIDField(REPO_TASK_LABEL_PARENT), Not(Eq(parent)));

	EXPECT_THAT(task.getUUIDField(REPO_LABEL_ID).isDefaultValue(), IsFalse());
	EXPECT_THAT(task.getUUIDField(REPO_TASK_LABEL_PARENT).isDefaultValue(), IsFalse());

	// Check key sanitisation

	data.clear();
	data["$Key$0"] = "Data0";
	data["Key$1"] = "Data1";
	data["Key$$2"] = "Data2";
	data["Key.3."] = "Data3";
	data["Key..4"] = "Data4";
	data["Key$.$5"] = "Data5";

	task = RepoBSONFactory::makeTask(
		name,
		startTime,
		endTime,
		sequenceId,
		data,
		resources
	);

	std::unordered_map<std::string, std::string> actual;

	metaField = task.getObjectField(REPO_TASK_LABEL_DATA);
	for (auto& n : metaField.getFieldNames()) {
		auto entry = metaField.getObjectField(n);
		auto key = entry.getStringField(REPO_TASK_META_KEY);
		auto value = entry.getStringField(REPO_TASK_META_VALUE);
		actual[key] = value;
	}

	std::unordered_map<std::string, std::string> expected;

	expected[":Key:0"] = "Data0";
	expected["Key:1"] = "Data1";
	expected["Key::2"] = "Data2";
	expected["Key:3:"] = "Data3";
	expected["Key::4"] = "Data4";
	expected["Key:::5"] = "Data5";

	EXPECT_THAT(actual, Eq(expected));

	// Check metadata types - RepoTask should attempt a lexical cast of metadata
	// values, storing them as long longs or doubles where possible

	data.clear();
	data["string"] = "I am a string";
	data["long"] = "1024";
	data["long_max"] = "9223372036854775807";
	data["long_min"] = "-9223372036854775806";
	data["double"] = "123.5";
	data["double_min"] = "2.2250738585072014e-308";
	data["double_max"] = "1.7976931348623158e+308";

	metaField = task.getObjectField(REPO_TASK_LABEL_DATA);
	for (auto& n : metaField.getFieldNames()) {
		auto entry = metaField.getObjectField(n);
		auto key = entry.getStringField(REPO_TASK_META_KEY);

		if (key == "string") {
			EXPECT_THAT(entry.getStringField(REPO_TASK_META_VALUE), Eq("I am a string"));
		}
		else if (key == "long") {
			EXPECT_THAT(entry.getLongField(REPO_TASK_META_VALUE), Eq(1024));
		}
		else if (key == "long_max") {
			EXPECT_THAT(entry.getLongField(REPO_TASK_META_VALUE), Eq(9223372036854775807));
		}
		else if (key == "long_min") {
			EXPECT_THAT(entry.getLongField(REPO_TASK_META_VALUE), Eq(-9223372036854775806));
		}
		else if (key == "double") {
			EXPECT_THAT(entry.getDoubleField(REPO_TASK_META_VALUE), Eq(123.5));
		}
		else if (key == "double_min") {
			EXPECT_THAT(entry.getDoubleField(REPO_TASK_META_VALUE), Eq(2.2250738585072014e-308));
		}
		else if (key == "double_max") {
			EXPECT_THAT(entry.getDoubleField(REPO_TASK_META_VALUE), Eq(1.7976931348623158e+308));
		}
	}
}