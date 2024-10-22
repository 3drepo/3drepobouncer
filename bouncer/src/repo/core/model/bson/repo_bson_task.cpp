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

#include "repo_bson_task.h"
#include "repo_bson_builder.h"
#include "repo/lib/datastructure/repo_uuid.h"

#include <boost/lexical_cast.hpp>

using namespace repo::core::model;

RepoTask::RepoTask(
	const std::string& name,
	const repo::lib::RepoUUID& uniqueId,
	const repo::lib::RepoUUID& parentId,
	const repo::lib::RepoUUID& sequenceId,
	const long long& startTime,
	const long long& endTime,
	const std::vector<repo::lib::RepoUUID>& resources,
	const std::unordered_map<std::string, std::string>& metadata
):
	name(name),
	uniqueId(uniqueId),
	parentId(parentId),
	sequenceId(sequenceId),
	startTime(startTime),
	endTime(endTime),
	resources(resources),
	metadata(metadata)
{
}

static bool keyCheck(const char& c)
{
	return c == '$' || c == '.';
}

static std::string sanitiseKey(const std::string& key)
{
	std::string cleanedKey(key);
	std::replace_if(cleanedKey.begin(), cleanedKey.end(), keyCheck, ':');
	return cleanedKey;
}

RepoTask::operator RepoBSON() const
{
	RepoBSONBuilder builder;

	builder.append(REPO_LABEL_ID, uniqueId);
	builder.append(REPO_TASK_LABEL_NAME, name);
	builder.append(REPO_TASK_LABEL_START, (long long)startTime);
	builder.append(REPO_TASK_LABEL_END, (long long)endTime);
	builder.append(REPO_TASK_LABEL_SEQ_ID, sequenceId);

	if (!parentId.isDefaultValue())
		builder.append(REPO_TASK_LABEL_PARENT, parentId);

	if (resources.size()) {
		RepoBSONBuilder resourceBuilder;
		resourceBuilder.appendArray(REPO_TASK_SHARED_IDS, resources);
		builder.append(REPO_TASK_LABEL_RESOURCES, resourceBuilder.obj());
	}

	std::vector<RepoBSON> metaEntries;
	for (const auto& entry : metadata) {
		std::string key = sanitiseKey(entry.first);
		std::string value = entry.second;

		if (!key.empty() && !value.empty())
		{
			RepoBSONBuilder metaBuilder;
			//Check if it is a number, if it is, store it as a number

			metaBuilder.append(REPO_TASK_META_KEY, key);
			try {
				long long valueInt = boost::lexical_cast<long long>(value);
				metaBuilder.append(REPO_TASK_META_VALUE, valueInt);
			}
			catch (boost::bad_lexical_cast&)
			{
				//not an int, try a double
				try {
					double valueFloat = boost::lexical_cast<double>(value);
					metaBuilder.append(REPO_TASK_META_VALUE, valueFloat);
				}
				catch (boost::bad_lexical_cast&)
				{
					//not an int or float, store as string
					metaBuilder.append(REPO_TASK_META_VALUE, value);
				}
			}

			metaEntries.push_back(metaBuilder.obj());
		}
	}

	builder.appendArray(REPO_TASK_LABEL_DATA, metaEntries);

	return builder.obj();
}