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

TextureNode::TextureNode(RepoBSON bson) :
RepoNode(bson)
{

}

TextureNode::~TextureNode()
{
}


std::vector<char>* TextureNode::getRawData() const
{

	std::vector<char> *dataVec = nullptr;
	if (hasField(REPO_LABEL_DATA) &&
		hasField(REPO_NODE_LABEL_DATA_BYTE_COUNT))
	{
		dataVec = new std::vector<char>();
		getBinaryFieldAsVector(REPO_LABEL_DATA,
			getField(REPO_NODE_LABEL_DATA_BYTE_COUNT).numberInt(), dataVec);
	}
	else{
		repoError << "Cannot find field for data in texture node!";
	}

	return dataVec;
}

std::string TextureNode::getFileExtension() const
{
	return getStringField(REPO_NODE_LABEL_EXTENSION);
}