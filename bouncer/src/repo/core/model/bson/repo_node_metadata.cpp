/**
*  Copyright (C) 2015 3D Repo Ltd
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

/**
*  Metadata node
*/

#include "repo_node_metadata.h"
#include "repo_bson_builder.h"

using namespace repo::core::model;

MetadataNode::MetadataNode() :
RepoNode()
{
}

MetadataNode::MetadataNode(RepoBSON bson,
	const std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>> &binMapping) :
	RepoNode(bson, binMapping)
{
	deserialise(bson);
}

MetadataNode::~MetadataNode()
{
}

void MetadataNode::deserialise(RepoBSON& bson)
{
	auto arr = bson.getObjectField(REPO_NODE_LABEL_METADATA);
	auto names = arr.getFieldNames();
	for (auto& n : names) {
		auto field = arr.getObjectField(n);
		auto key = field.getStringField(REPO_NODE_LABEL_META_KEY);
		auto value = field.getField(REPO_NODE_LABEL_META_VALUE).repoVariant();
		metadataMap[key] = value;
	}
}

bool keyCheck(const char& c)
{
	return c == '$' || c == '.';
}

std::string sanitiseKey(const std::string& key)
{
	std::string cleanedKey(key);
	std::replace_if(cleanedKey.begin(), cleanedKey.end(), keyCheck, ':');
	return cleanedKey;
}

void MetadataNode::serialise(repo::core::model::RepoBSONBuilder& builder) const
{
	RepoNode::serialise(builder);

	std::vector<RepoBSON> metaEntries;
	for (const auto& entry : metadataMap) {
		std::string key = entry.first;
		repo::lib::RepoVariant value = entry.second;
		if (!key.empty())
		{
			RepoBSONBuilder metaEntryBuilder;
			metaEntryBuilder.append(REPO_NODE_LABEL_META_KEY, key);
			metaEntryBuilder.appendRepoVariant(REPO_NODE_LABEL_META_VALUE, value);

			metaEntries.push_back(metaEntryBuilder.obj());
		}
	}

	builder.appendArray(REPO_NODE_LABEL_METADATA, metaEntries);
}

void MetadataNode::setMetadata(const std::unordered_map<std::string, repo::lib::RepoVariant>& map)
{
	for (const auto& pair : map) {
		auto key = sanitiseKey(pair.first);
		auto value = pair.second;
		metadataMap[key] = value;
	}
}

/* Define the equality operator for tm, for sEqual below.
 * This static definition is only for this object.
 */

static bool operator== (tm a, tm b)
{
	return memcmp(&a, &b, sizeof(tm)) == 0;
}

bool MetadataNode::sEqual(const RepoNode &other) const
{
	if (other.getTypeAsEnum() != NodeType::METADATA)
	{
		return false;
	}

	auto o = dynamic_cast<const MetadataNode&>(other);

	if (metadataMap.size() != o.metadataMap.size()) 
	{
		return false;
	}

	for (auto& m : metadataMap)
	{
		auto it = o.metadataMap.find(m.first);
		if (it == o.metadataMap.end())
		{
			return false;
		}
	
		if ((*it).second != m.second)
		{
			return false;
		}
	}

	return true;
}