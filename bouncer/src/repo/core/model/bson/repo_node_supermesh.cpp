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

#include "repo_node_supermesh.h"
#include "repo_bson_builder.h"
#include "repo/lib/repo_exception.h"

using namespace repo::core::model;

SupermeshNode::SupermeshNode(RepoBSON bson)
	: MeshNode(bson)
{
	deserialise(bson);
}

SupermeshNode::SupermeshNode()
	: MeshNode()
{
}

void SupermeshNode::deserialise(RepoBSON& bson)
{
	throw repo::lib::RepoException("Supermesh Nodes cannot be deserialised");
}

void SupermeshNode::serialise(repo::core::model::RepoBSONBuilder& builder) const
{
	throw repo::lib::RepoException("Supermesh Nodes cannot be serialised");
}