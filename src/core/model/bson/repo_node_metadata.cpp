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

using namespace repo::core::model::bson;

MetadataNode::MetadataNode() :
RepoNode()
{
}

MetadataNode::MetadataNode(RepoBSON bson) :
RepoNode(bson)
{

}

MetadataNode::~MetadataNode()
{
}

MetadataNode* MetadataNode::createMetadataNode(
	RepoBSON			         &metadata,
	const std::string            &mimeType,
	const std::string            &name,
	const std::vector<repo_uuid> &parents,
	const int                    &apiLevel)
{
	RepoBSONBuilder builder;

	// Compulsory fields such as _id, type, api as well as path
	// and optional name
	appendDefaults(builder, REPO_NODE_TYPE_METADATA, apiLevel, generateUUID(), name, parents);

	//--------------------------------------------------------------------------
	// Media type
	if (!mimeType.empty())
		builder << REPO_LABEL_MEDIA_TYPE << mimeType;

	//--------------------------------------------------------------------------
	// Add metadata subobject
	if (!metadata.isEmpty())
		builder << REPO_NODE_LABEL_METADATA << metadata;

	return new MetadataNode(builder.obj());
}