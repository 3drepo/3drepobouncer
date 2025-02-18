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
#include "repo_bson_builder.h"

using namespace repo::core::model;

MaterialNode::MaterialNode() :
RepoNode()
{
}

MaterialNode::MaterialNode(RepoBSON bson) :
RepoNode(bson)
{
	deserialise(bson);
}

MaterialNode::~MaterialNode()
{
}

void MaterialNode::deserialise(RepoBSON& bson)
{
	std::vector<float> tempVec = bson.getFloatArray(REPO_NODE_MATERIAL_LABEL_AMBIENT);
	if (tempVec.size() > 0)
		material.ambient = tempVec;

	tempVec = bson.getFloatArray(REPO_NODE_MATERIAL_LABEL_DIFFUSE);
	if (tempVec.size() > 0)
		material.diffuse = tempVec;

	tempVec = bson.getFloatArray(REPO_NODE_MATERIAL_LABEL_SPECULAR);
	if (tempVec.size() > 0)
		material.specular = tempVec;

	tempVec = bson.getFloatArray(REPO_NODE_MATERIAL_LABEL_EMISSIVE);
	if (tempVec.size() > 0)
		material.emissive = tempVec;

	material.isWireframe = bson.hasField(REPO_NODE_MATERIAL_LABEL_WIREFRAME) &&
		bson.getBoolField(REPO_NODE_MATERIAL_LABEL_WIREFRAME);

	material.isTwoSided = bson.hasField(REPO_NODE_MATERIAL_LABEL_TWO_SIDED) &&
		bson.getBoolField(REPO_NODE_MATERIAL_LABEL_TWO_SIDED);

	material.opacity = bson.hasField(REPO_NODE_MATERIAL_LABEL_OPACITY) ?
		bson.getDoubleField(REPO_NODE_MATERIAL_LABEL_OPACITY) :
		std::numeric_limits<float>::quiet_NaN();

	material.shininess = bson.hasField(REPO_NODE_MATERIAL_LABEL_SHININESS) ?
		bson.getDoubleField(REPO_NODE_MATERIAL_LABEL_SHININESS) :
		std::numeric_limits<float>::quiet_NaN();

	material.shininessStrength = bson.hasField(REPO_NODE_MATERIAL_LABEL_SHININESS_STRENGTH) ?
		bson.getDoubleField(REPO_NODE_MATERIAL_LABEL_SHININESS_STRENGTH) :
		std::numeric_limits<float>::quiet_NaN();

	material.lineWeight = bson.hasField(REPO_NODE_MATERIAL_LABEL_LINE_WEIGHT) ?
		bson.getDoubleField(REPO_NODE_MATERIAL_LABEL_LINE_WEIGHT) :
		std::numeric_limits<float>::quiet_NaN();
}


void MaterialNode::serialise(repo::core::model::RepoBSONBuilder& builder) const
{
	RepoNode::serialise(builder);
	if (material.ambient)
		builder.append(REPO_NODE_MATERIAL_LABEL_AMBIENT, material.ambient);
	if (material.diffuse > 0)
		builder.append(REPO_NODE_MATERIAL_LABEL_DIFFUSE, material.diffuse);
	if (material.specular)
		builder.append(REPO_NODE_MATERIAL_LABEL_SPECULAR, material.specular);
	if (material.emissive)
		builder.append(REPO_NODE_MATERIAL_LABEL_EMISSIVE, material.emissive);

	if (material.isWireframe)
		builder.append(REPO_NODE_MATERIAL_LABEL_WIREFRAME, material.isWireframe);
	if (material.isTwoSided)
		builder.append(REPO_NODE_MATERIAL_LABEL_TWO_SIDED, material.isTwoSided);

	// Checking for NaN values

	if (material.opacity == material.opacity)
		builder.append(REPO_NODE_MATERIAL_LABEL_OPACITY, material.opacity);
	if (material.shininess == material.shininess)
		builder.append(REPO_NODE_MATERIAL_LABEL_SHININESS, material.shininess);
	if (material.shininessStrength == material.shininessStrength)
		builder.append(REPO_NODE_MATERIAL_LABEL_SHININESS_STRENGTH, material.shininessStrength);
	if (material.lineWeight == material.lineWeight)
		builder.append(REPO_NODE_MATERIAL_LABEL_LINE_WEIGHT, material.lineWeight);
}

bool MaterialNode::sEqual(const RepoNode &other) const
{
	if (other.getTypeAsEnum() != NodeType::MATERIAL || other.getParentIDs().size() != getParentIDs().size())
	{
		return false;
	}

	try{
		auto otherMaterialNode = dynamic_cast<const MaterialNode&>(other);
		return getMaterialStruct().checksum() == otherMaterialNode.getMaterialStruct().checksum();
	}
	catch (std::bad_cast)
	{
		return false;
	}
}