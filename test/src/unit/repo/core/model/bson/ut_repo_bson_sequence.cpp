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

TEST(RepoSequenceTest, Methods)
{
	auto id = repo::lib::RepoUUID::createUUID();

	RepoSequence sequence(
		std::string(),
		id,
		repo::lib::RepoUUID::createUUID(),
		0,
		0,
		{}
	);

	EXPECT_THAT(sequence.getUniqueId(), Eq(id));
}

bool repo::core::model::RepoSequence::FrameData::operator== (const repo::core::model::RepoSequence::FrameData& b) const
{
	return ref == b.ref && timestamp == b.timestamp;
}

TEST(RepoSequenceTest, Serialise)
{
	// We should not be serialising an empty Sequence, but if we did, this is
	// what it should look like.

	RepoSequence sequence;
	auto bson = (RepoBSON)sequence;

	EXPECT_THAT(bson.getUUIDField(REPO_LABEL_ID).isDefaultValue(), IsFalse());
	EXPECT_THAT(bson.getStringField(REPO_SEQUENCE_LABEL_NAME), IsEmpty());
	EXPECT_THAT(bson.getLongField(REPO_SEQUENCE_LABEL_START_DATE), Eq(0));
	EXPECT_THAT(bson.getLongField(REPO_SEQUENCE_LABEL_END_DATE), Eq(0));
	EXPECT_THAT(nFields(bson.getObjectField(REPO_SEQUENCE_LABEL_FRAMES)), Eq(0));
}

TEST(RepoSequenceTest, Factory)
{
	RepoSequence sequence;

	auto id = repo::lib::RepoUUID::createUUID();
	auto name = "name";
	auto start = 10000;
	auto end = 90000;

	auto frames = std::vector<RepoSequence::FrameData>();
	RepoSequence::FrameData f1;
	f1.ref = "ref1";
	f1.timestamp = 10000;
	frames.push_back(f1);

	RepoSequence::FrameData f2;
	f2.ref = "myRef";
	f2.timestamp = 20000;
	frames.push_back(f2);

	RepoSequence::FrameData f3;
	f3.ref = "secondRef";
	f3.timestamp = 30000;
	frames.push_back(f3);

	RepoBSON bson = RepoBSONFactory::makeSequence(frames, name, id, start, end);

	EXPECT_THAT(bson.getUUIDField(REPO_LABEL_ID), Eq(id));
	EXPECT_THAT(bson.getStringField(REPO_SEQUENCE_LABEL_NAME), Eq(name));

	// The bounding dates are Unix epochs encoded as long long

	EXPECT_THAT(bson.getLongField(REPO_SEQUENCE_LABEL_START_DATE), Eq(start));
	EXPECT_THAT(bson.getLongField(REPO_SEQUENCE_LABEL_END_DATE), Eq(end));

	auto framesField = bson.getObjectField(REPO_SEQUENCE_LABEL_FRAMES);

	// For individual Frames, the underlying type is a Timestamp

	EXPECT_THAT(framesField.getObjectField("0").getStringField(REPO_SEQUENCE_LABEL_STATE), Eq(f1.ref));
	EXPECT_THAT(framesField.getObjectField("0").getTimeStampField(REPO_SEQUENCE_LABEL_DATE), Eq(f1.timestamp));

	EXPECT_THAT(framesField.getObjectField("1").getStringField(REPO_SEQUENCE_LABEL_STATE), Eq(f2.ref));
	EXPECT_THAT(framesField.getObjectField("1").getTimeStampField(REPO_SEQUENCE_LABEL_DATE), Eq(f2.timestamp));

	EXPECT_THAT(framesField.getObjectField("2").getStringField(REPO_SEQUENCE_LABEL_STATE), Eq(f3.ref));
	EXPECT_THAT(framesField.getObjectField("2").getTimeStampField(REPO_SEQUENCE_LABEL_DATE), Eq(f3.timestamp));
}

TEST(RepoSequenceTest, Empty)
{
	RepoSequence sequence;

	EXPECT_THAT(((RepoBSON)sequence).getUUIDField(REPO_LABEL_ID), Not(Eq(repo::lib::RepoUUID::defaultValue)));
	EXPECT_THAT(((RepoBSON)sequence).hasField(REPO_SEQUENCE_LABEL_NAME), IsTrue());
	EXPECT_THAT(((RepoBSON)sequence).getLongField(REPO_SEQUENCE_LABEL_START_DATE), Eq(0LL));
	EXPECT_THAT(((RepoBSON)sequence).getLongField(REPO_SEQUENCE_LABEL_END_DATE), Eq(0LL));
	EXPECT_THAT(((RepoBSON)sequence).hasField(REPO_SEQUENCE_LABEL_REV_ID), IsFalse());
	EXPECT_THAT(((RepoBSON)sequence).getStringArray(REPO_SEQUENCE_LABEL_FRAMES), IsEmpty()); //Can be cast to anything because it is array...
}

TEST(RepoSequenceTest, Revision)
{
	RepoSequence sequence;

	EXPECT_THAT(((RepoBSON)sequence).hasField(REPO_SEQUENCE_LABEL_REV_ID), IsFalse());

	auto rid = repo::lib::RepoUUID::createUUID();
	sequence.setRevision(rid);
	EXPECT_THAT(((RepoBSON)sequence).getUUIDField(REPO_SEQUENCE_LABEL_REV_ID), Eq(rid));
	EXPECT_THAT(((RepoBSON)sequence).hasField(REPO_SEQUENCE_LABEL_NAME), IsTrue());
	EXPECT_THAT(((RepoBSON)sequence).getLongField(REPO_SEQUENCE_LABEL_START_DATE), Eq(0LL));
	EXPECT_THAT(((RepoBSON)sequence).getLongField(REPO_SEQUENCE_LABEL_END_DATE), Eq(0LL));
}