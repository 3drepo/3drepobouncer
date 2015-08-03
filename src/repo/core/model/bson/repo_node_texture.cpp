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

#include <boost/filesystem.hpp>

using namespace repo::core::model::bson;

TextureNode::TextureNode() :
RepoNode()
{
}

TextureNode::TextureNode(RepoBSON bson) :
RepoNode(bson)
{

}

TextureNode::~TextureNode()
{
}

TextureNode* TextureNode::createTextureNode(
	const std::string &name,
	const char        *data,
	const uint32_t    &byteCount,
	const uint32_t    &width,
	const uint32_t    &height,
	const int &apiLevel
	)
{

	RepoBSONBuilder builder;
	RepoNode::appendDefaults(builder, REPO_NODE_TYPE_TEXTURE, apiLevel, generateUUID(), name);
	//
	// Width
	//
	builder << REPO_LABEL_WIDTH << width;

	//
	// Height
	//
	builder << REPO_LABEL_HEIGHT << height;

	//
	// Format
	//
	if (name.empty())
	{
		boost::filesystem::path file{ name };
		builder << REPO_NODE_LABEL_EXTENSION << file.extension().c_str();
	}

	//
	// Data
	//

	if (NULL != data && byteCount > 0)
		builder.appendBinary(
		REPO_LABEL_DATA,
		data,
		byteCount,
		REPO_NODE_LABEL_DATA_BYTE_COUNT);

	return new TextureNode(builder.obj());
}

std::vector<char>* TextureNode::getRawData() const
{

	std::vector<char> *dataVec = nullptr;
	if (hasField(REPO_LABEL_DATA) &&
		hasField(REPO_NODE_LABEL_DATA_BYTE_COUNT))
	{
		getBinaryFieldAsVector(getField(REPO_LABEL_DATA),
			getField(REPO_NODE_LABEL_DATA_BYTE_COUNT).numberInt(), dataVec);
	}

	return dataVec;
}