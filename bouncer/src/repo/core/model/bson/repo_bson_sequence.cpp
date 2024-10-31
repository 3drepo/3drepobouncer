/**
*  Copyright (C) 2019 3D Repo Ltd
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

#include "repo_bson_sequence.h"
#include "repo_bson_builder.h"
#include "../../../lib/datastructure/repo_uuid.h"

using namespace repo::core::model;

RepoSequence::RepoSequence()
{
	uniqueId = repo::lib::RepoUUID::createUUID();
	revisionId = repo::lib::RepoUUID::defaultValue;
	firstFrame = 0;
	lastFrame = 0;
}

RepoSequence::RepoSequence(
	const std::string& name,
	const repo::lib::RepoUUID& uniqueId,
	const repo::lib::RepoUUID& revisionId,
	const long long& firstFrameTimestamp,
	const long long& lastFrameTimestamp,
	const std::vector<FrameData>& frames
)
	:name(name),
	uniqueId(uniqueId),
	revisionId(revisionId),
	firstFrame(firstFrameTimestamp),
	lastFrame(lastFrameTimestamp),
	frameData(frames)
{
}

RepoSequence RepoSequence::cloneAndAddRevision(
	const repo::lib::RepoUUID &rid
) const {

	RepoSequence copy = *this;
	copy.revisionId = rid;
	return copy;
}

RepoSequence::operator RepoBSON() const
{
	RepoBSONBuilder builder;

	builder.append(REPO_LABEL_ID, uniqueId);
	builder.append(REPO_SEQUENCE_LABEL_NAME, name);
	builder.append(REPO_SEQUENCE_LABEL_START_DATE, (long long)firstFrame);
	builder.append(REPO_SEQUENCE_LABEL_END_DATE, (long long)lastFrame);
	if (!revisionId.isDefaultValue())
	{
		builder.append(REPO_SEQUENCE_LABEL_REV_ID, revisionId);
	}

	std::vector<RepoBSON> frames;

	for (const auto& frameEntry : frameData) {
		RepoBSONBuilder bsonBuilder;
		bsonBuilder.appendTime(REPO_SEQUENCE_LABEL_DATE, frameEntry.timestamp);
		bsonBuilder.append(REPO_SEQUENCE_LABEL_STATE, frameEntry.ref);

		frames.push_back(bsonBuilder.obj());
	}

	builder.appendArray(REPO_SEQUENCE_LABEL_FRAMES, frames);

	return builder.obj();
}

bool RepoSequence::checkSize()
{
	RepoBSON bson = *this;
	if (bson.objsize() > REPO_MAX_OBJ_SIZE)
	{
		return false;
	}
	return true;
}