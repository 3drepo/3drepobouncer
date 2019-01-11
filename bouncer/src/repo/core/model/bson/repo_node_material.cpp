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

using namespace repo::core::model;

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
	
	mat.texturePath = hasField(REPO_NODE_MATERIAL_LABEL_TEXTURE_PATH) ?
		getField(REPO_NODE_MATERIAL_LABEL_TEXTURE_PATH).valuestrsafe() : std::string();

	mat.missingTexture = hasField(REPO_NODE_MATERIAL_LABEL_MISSING_TEXTURE) &&
		getField(REPO_NODE_MATERIAL_LABEL_MISSING_TEXTURE).boolean();

	return mat;
}

std::vector<float> MaterialNode::getDataAsBuffer() const
{
	repo_material_t mat = getMaterialStruct();

	//flatten the material struct
	std::vector<float> buffer;
	buffer.reserve(sizeof(mat) / sizeof(float)); //FIXME: bigger than this, since we have 4 vector points with each having 3 members.

	for (const float &n : mat.ambient)
	{
		buffer.push_back(n);
	}

	for (const float &n : mat.diffuse)
	{
		buffer.push_back(n);
	}

	for (const float &n : mat.specular)
	{
		buffer.push_back(n);
	}

	for (const float &n : mat.emissive)
	{
		buffer.push_back(n);
	}

	buffer.push_back(mat.opacity);
	buffer.push_back(mat.shininess);
	buffer.push_back(mat.shininessStrength);
	uint32_t mask = (((int)mat.isWireframe) << 1) | mat.isTwoSided;

	buffer.push_back((float)mask);

	return buffer;
}

bool MaterialNode::sEqual(const RepoNode &other) const
{
	if (other.getTypeAsEnum() != NodeType::MATERIAL || other.getParentIDs().size() != getParentIDs().size())
	{
		return false;
	}

	auto mat = getDataAsBuffer();
	auto otherMat = MaterialNode(other).getDataAsBuffer();

	return mat.size() == otherMat.size() && !memcmp(mat.data(), otherMat.data(), mat.size() * sizeof(*mat.data()));
}