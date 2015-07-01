/**
*  Copyright (C) 2014 3D Repo Ltd
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
* Repo Node - A node representation of RepoBSON
*/

#include "repo_node.h"
#include "repo_bson_builder.h"
using namespace repo::core::model::bson;

RepoNode RepoNode::createRepoNode(
	const std::string &type,
	const unsigned int api,
	const repo_uuid &sharedId,
	const std::string &name,
	const std::vector<repo_uuid> &parents)
{
	RepoBSONBuilder builder;
	//--------------------------------------------------------------------------
	// ID field (UUID)
	builder.append(REPO_NODE_LABEL_ID, generateUUID());

	//--------------------------------------------------------------------------
	// Shared ID (UUID)
	builder.append(REPO_NODE_LABEL_SHARED_ID, sharedId);
	
	//--------------------------------------------------------------------------
	// Paths
	// 
	// Paths are stored as array of arrays of shared_id (uuids)
//FIXME: we don't use paths? Either way this should be a scene graph operation. Node document shouldn't even know what this means.
	//const std::vector<std::vector<boost::uuids::uuid>> paths =
	//	getPaths(this);
	//if (paths.size() > 0)
	//	RepoTranscoderBSON::append(REPO_NODE_LABEL_PATHS, paths, builder);

	//--------------------------------------------------------------------------
	// Type
	if (!type.empty())
		builder << REPO_NODE_LABEL_TYPE << type;

	//--------------------------------------------------------------------------
	// API level
	builder << REPO_NODE_LABEL_API << api;

	//--------------------------------------------------------------------------
	// Parents
	builder.appendArray(REPO_NODE_LABEL_PARENTS, builder.createArrayBSON(parents));

	//--------------------------------------------------------------------------
	// Name
	if (!name.empty())
		builder << REPO_NODE_LABEL_NAME << name;

	return RepoNode(builder.obj());
}

RepoNode::~RepoNode()
{
}
