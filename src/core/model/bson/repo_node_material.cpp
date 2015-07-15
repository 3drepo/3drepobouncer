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
*  Material node
*/

#include "repo_node_material.h"

using namespace repo::core::model::bson;

MaterialNode::MaterialNode() :
RepoNode()
{
}

MaterialNode::MaterialNode(RepoBSON bson) :
RepoNode(bson)
{

}

MaterialNode::~MaterialNode()
{
}

MaterialNode* MaterialNode::createMaterialNode(
	const repo_material_t &material,
	const std::string     &name,
	const int     &apiLevel)
{
	RepoBSONBuilder builder;

	// Compulsory fields such as _id, type, api as well as path
	// and optional name
	appendDefaults(builder, REPO_NODE_TYPE_MATERIAL, apiLevel, generateUUID(), name);


	builder << REPO_NODE_LABEL_AMBIENT   << material.ambient;
	builder << REPO_NODE_LABEL_DIFFUSE   << material.diffuse;
	builder << REPO_NODE_LABEL_SPECULAR  << material.specular;
	builder << REPO_NODE_LABEL_EMISSIVE  << material.emissive;
	builder << REPO_NODE_LABEL_WIREFRAME << material.isWireframe;
	builder << REPO_NODE_LABEL_TWO_SIDED << material.isTwoSided;
	builder << REPO_NODE_LABEL_OPACITY   << material.opacity;
	builder << REPO_NODE_LABEL_SHININESS << material.shininess;
	builder << REPO_NODE_LABEL_SHININESS_STRENGTH << material.shininessStrength;

	return new MaterialNode(builder.obj());
}
