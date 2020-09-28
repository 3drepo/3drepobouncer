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
*  Texture node
*/

#include "repo_node_texture.h"
#include "../../../lib/repo_log.h"
#include <boost/filesystem.hpp>

using namespace repo::core::model;

TextureNode::TextureNode() :
	RepoNode()
{
}

TextureNode::TextureNode(RepoBSON bson, const std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>>&binMapping) :
	RepoNode(bson, binMapping)
{
}

TextureNode::~TextureNode()
{
}

std::vector<char> TextureNode::getRawData() const
{
	std::vector<char> dataVec;
	if (hasBinField(REPO_LABEL_DATA))
	{
		getBinaryFieldAsVector(REPO_LABEL_DATA, dataVec);
	}
	else {
		repoError << "Cannot find field for data in texture node!";
	}

	return dataVec;
}

std::string TextureNode::getFileExtension() const
{
	return getStringField(REPO_NODE_LABEL_EXTENSION);
}

bool TextureNode::sEqual(const RepoNode &other) const
{
	if (other.getTypeAsEnum() != NodeType::TEXTURE || other.getParentIDs().size() != getParentIDs().size())
	{
		return false;
	}

	TextureNode otherText = TextureNode(other);
	bool equal;

	if (equal = getFileExtension() == otherText.getFileExtension())
	{
		std::vector<char> raw, raw2;

		raw = getRawData();
		raw2 = otherText.getRawData();

		if (equal = (raw.size() == raw2.size()))
		{
			equal = !memcmp(raw.data(), raw2.data(), raw.size() * sizeof(*raw.data()));
		}
	}

	return equal;
}