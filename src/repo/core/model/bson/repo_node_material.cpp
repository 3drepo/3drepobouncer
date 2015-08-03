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

	if (material.ambient.size() > 0)
		builder.appendArray(REPO_NODE_MATERIAL_LABEL_AMBIENT, builder.createArrayBSON(material.ambient));
	if (material.diffuse.size() > 0)
		builder.appendArray(REPO_NODE_MATERIAL_LABEL_DIFFUSE, builder.createArrayBSON(material.diffuse));
	if (material.specular.size() > 0)
		builder.appendArray(REPO_NODE_MATERIAL_LABEL_SPECULAR, builder.createArrayBSON(material.specular));
	if (material.emissive.size() > 0)
		builder.appendArray(REPO_NODE_MATERIAL_LABEL_EMISSIVE, builder.createArrayBSON(material.emissive));


	if (material.isWireframe)
		builder << REPO_NODE_MATERIAL_LABEL_WIREFRAME << material.isWireframe;
	if (material.isTwoSided)
		builder << REPO_NODE_MATERIAL_LABEL_TWO_SIDED << material.isTwoSided;
	
	if (material.opacity == material.opacity)
		builder << REPO_NODE_MATERIAL_LABEL_OPACITY   << material.opacity;
	
	if (material.shininess == material.shininess)
		builder << REPO_NODE_MATERIAL_LABEL_SHININESS << material.shininess;
	
	if (material.shininessStrength == material.shininessStrength)
		builder << REPO_NODE_MATERIAL_LABEL_SHININESS_STRENGTH << material.shininessStrength;

	return new MaterialNode(builder.obj());
}


repo_material_t MaterialNode::getMaterialStruct() const
{
	repo_material_t mat;


	std::vector<float> tempVec = getFloatArray(REPO_NODE_MATERIAL_LABEL_AMBIENT);
	if (tempVec.size() > 0)
		mat.ambient.insert(mat.ambient.end(), tempVec.begin(), tempVec.end());

	tempVec = getFloatArray(REPO_NODE_MATERIAL_LABEL_DIFFUSE);
	if (tempVec.size() > 0)
		mat.diffuse.insert(mat.diffuse.end(), tempVec.begin(), tempVec.end());

	tempVec = getFloatArray(REPO_NODE_MATERIAL_LABEL_SPECULAR);
	if (tempVec.size() > 0)
		mat.specular.insert(mat.specular.end(), tempVec.begin(), tempVec.end());

	tempVec = getFloatArray(REPO_NODE_MATERIAL_LABEL_EMISSIVE);
	if (tempVec.size() > 0)
		mat.emissive.insert(mat.emissive.end(), tempVec.begin(), tempVec.end());

	mat.isWireframe = hasField(REPO_NODE_MATERIAL_LABEL_WIREFRAME) && 
		getField(REPO_NODE_MATERIAL_LABEL_WIREFRAME).boolean();

	mat.isTwoSided = hasField(REPO_NODE_MATERIAL_LABEL_TWO_SIDED) &&
		getField(REPO_NODE_MATERIAL_LABEL_TWO_SIDED).boolean();

	mat.opacity = hasField(REPO_NODE_MATERIAL_LABEL_OPACITY) ?
		getField(REPO_NODE_MATERIAL_LABEL_OPACITY).numberDouble() :
		std::numeric_limits<float>::quiet_NaN();

	mat.shininess = hasField(REPO_NODE_MATERIAL_LABEL_SHININESS) ?
		getField(REPO_NODE_MATERIAL_LABEL_SHININESS).numberDouble() :
		std::numeric_limits<float>::quiet_NaN();

	mat.shininessStrength = hasField(REPO_NODE_MATERIAL_LABEL_SHININESS_STRENGTH) ?
		getField(REPO_NODE_MATERIAL_LABEL_SHININESS_STRENGTH).numberDouble() :
		std::numeric_limits<float>::quiet_NaN();

	return mat;
}