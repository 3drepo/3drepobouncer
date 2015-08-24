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
*  Reference node
*/

#include "repo_node_reference.h"

using namespace repo::core::model;

ReferenceNode::ReferenceNode() :
RepoNode()
{
}

ReferenceNode::ReferenceNode(RepoBSON bson) :
RepoNode(bson)
{

}

ReferenceNode::~ReferenceNode()
{
}

ReferenceNode ReferenceNode::createReferenceNode(
	const std::string &database,
	const std::string &project,
	const repoUUID    &revisionID,
	const bool        &isUniqueID,
	const std::string &name,
	const int         &apiLevel)
{
	RepoBSONBuilder builder;
	std::string nodeName = name.empty()? database+ "." + project : name;

	appendDefaults(builder, REPO_NODE_TYPE_REFERENCE, apiLevel, generateUUID(), nodeName);

	//--------------------------------------------------------------------------
	// Project owner (company or individual)
	if (!database.empty())
		builder << REPO_NODE_REFERENCE_LABEL_OWNER << database;

	//--------------------------------------------------------------------------
	// Project name
	if (!project.empty())
		builder << REPO_NODE_REFERENCE_LABEL_PROJECT << project;

	//--------------------------------------------------------------------------
	// Revision ID (specific revision if UID, branch if SID)
	builder.append(
		REPO_NODE_REFERENCE_LABEL_REVISION_ID,
		revisionID);

	//--------------------------------------------------------------------------
	// Unique set if the revisionID is UID, not set if SID (branch)
	if (isUniqueID)
		builder << REPO_NODE_REFERENCE_LABEL_UNIQUE << isUniqueID;

	return ReferenceNode(builder.obj());

}