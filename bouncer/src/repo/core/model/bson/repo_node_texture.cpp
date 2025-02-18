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
#include <repo_log.h>
#include <boost/filesystem.hpp>
#include "repo_bson_builder.h"

using namespace repo::core::model;

TextureNode::TextureNode() :
	RepoNode()
{
	width = 0;
	height = 0;
}

TextureNode::TextureNode(RepoBSON bson) :
	RepoNode(bson)
{
	width = 0;
	height = 0;
	deserialise(bson);
}

TextureNode::~TextureNode()
{
}

void TextureNode::deserialise(RepoBSON& bson)
{
	if (bson.hasBinField(REPO_LABEL_DATA))
	{
		bson.getBinaryFieldAsVector(REPO_LABEL_DATA, data);
	}

	if (bson.hasField(REPO_LABEL_WIDTH))
	{
		width = bson.getIntField(REPO_LABEL_WIDTH);
	}

	if (bson.hasField(REPO_LABEL_HEIGHT))
	{
		height = bson.getIntField(REPO_LABEL_HEIGHT);
	}

	if (bson.hasField(REPO_NODE_LABEL_EXTENSION))
	{
		extension = bson.getStringField(REPO_NODE_LABEL_EXTENSION);
	}
}

void TextureNode::serialise(repo::core::model::RepoBSONBuilder& builder) const
{
	RepoNode::serialise(builder);

	builder.append(REPO_LABEL_WIDTH, (int32_t)width);
	builder.append(REPO_LABEL_HEIGHT, (int32_t)height);

	if (!extension.empty())
	{
		builder.append(REPO_NODE_LABEL_EXTENSION, extension);
	}

	if (data.size()) {
		builder.appendLargeArray(REPO_LABEL_DATA, data);
	}
	else
	{
		repoWarning << " Creating a texture node with no texture!";
	}
}

bool TextureNode::sEqual(const RepoNode &other) const
{
	if (other.getTypeAsEnum() != NodeType::TEXTURE)
	{
		return false;
	}

	auto node = dynamic_cast<const TextureNode&>(other);

	// The file extension and dimensions are just metadata of the underlying
	// buffer - if this is identical, then the textures are identical.

	auto buf1 = getRawData();
	auto buf2 = node.getRawData();

	return buf1 == buf2;
}

bool TextureNode::isEmpty() const
{
	return !data.size();
}