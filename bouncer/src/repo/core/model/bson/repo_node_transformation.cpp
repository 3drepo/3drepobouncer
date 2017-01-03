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
*  Transformation node
*/

#include "repo_node_transformation.h"
#include "repo_bson_builder.h"

using namespace repo::core::model;

TransformationNode::TransformationNode() :
RepoNode()
{
}

TransformationNode::TransformationNode(RepoBSON bson) :
RepoNode(bson)
{
}

TransformationNode::~TransformationNode()
{
}

RepoNode TransformationNode::cloneAndApplyTransformation(
	const repo::lib::RepoMatrix &matrix) const
{
	RepoNode resultNode;
	RepoBSONBuilder builder;
	
	auto currentTrans = getTransMatrix(false);
	auto resultTrans = currentTrans * matrix;
	auto resultData = resultTrans.getData();

	RepoBSONBuilder rows;
	for (uint32_t i = 0; i < 4; ++i)
	{
		RepoBSONBuilder columns;
		for (uint32_t j = 0; j < 4; ++j){
			size_t idx = i * 4 + j;
			columns << std::to_string(j) << resultData[idx];
		}
		rows.appendArray(std::to_string(i), columns.obj());
	}
	builder.appendArray(REPO_NODE_LABEL_MATRIX, rows.obj());
	

	builder.appendElementsUnique(*this);

	return TransformationNode(RepoBSON(builder.obj(), bigFiles));
}

TransformationNode TransformationNode::cloneAndResetMatrix() const
{
	RepoNode resultNode;
	RepoBSONBuilder builder;

	RepoBSONBuilder rows;
	for (uint32_t i = 0; i < 4; ++i)
	{
		RepoBSONBuilder columns;
		for (uint32_t j = 0; j < 4; ++j){
			columns << std::to_string(j) << (i == j ? 1.0 : 0.0);
		}
		rows.appendArray(std::to_string(i), columns.obj());
	}
	builder.appendArray(REPO_NODE_LABEL_MATRIX, rows.obj());

	builder.appendElementsUnique(*this);

	return TransformationNode(RepoBSON(builder.obj(), bigFiles));
}

std::vector<std::vector<float>> TransformationNode::identityMat()
{
	std::vector<std::vector<float>> idMat;
	idMat.push_back({ 1, 0, 0, 0 });
	idMat.push_back({ 0, 1, 0, 0 });
	idMat.push_back({ 0, 0, 1, 0 });
	idMat.push_back({ 0, 0, 0, 1 });
	return idMat;
}

bool TransformationNode::isIdentity(const float &eps) const
{
	
	return  getTransMatrix(false).isIdentity(eps);
}

repo::lib::RepoMatrix TransformationNode::getTransMatrix(const bool &rowMajor) const
{
	std::vector<float> transformationMatrix;
	uint32_t rowInd = 0, colInd = 0;
	if (hasField(REPO_NODE_LABEL_MATRIX))
	{
		transformationMatrix.resize(16);
		float *transArr = &transformationMatrix.at(0);

		// matrix is stored as array of arrays
		RepoBSON matrixObj =
			getField(REPO_NODE_LABEL_MATRIX).embeddedObject();

		std::set<std::string> mFields;
		matrixObj.getFieldNames(mFields);
		for (auto &field : mFields)
		{
			rowInd = std::stoi(field);
			RepoBSON arrayObj = matrixObj.getField(field).embeddedObject();
			std::set<std::string> aFields;
			arrayObj.getFieldNames(aFields);
			for (auto &aField : aFields)
			{
				colInd = std::stoi(aField);

				//figure out the index depending on if it's row or col major
				uint32_t index;
				if (rowMajor)
				{
					index = colInd * 4 + rowInd;
				}
				else
				{
					index = rowInd * 4 + colInd;
				}

				auto f = arrayObj.getField(aField);
				if (f.type() == ElementType::DOUBLE)
					transArr[index] = (float)f.Double();
				else if (f.type() == ElementType::INT)
					transArr[index] = (float)f.Int();
				else
				{
					repoError << "Unexpected type within transformation matrix!";
				}
			}
		}
	}
	else
	{
		repoWarning << "This transformation has no matrix field, returning identity";
		transformationMatrix = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
	}
	return repo::lib::RepoMatrix(transformationMatrix);
}

bool TransformationNode::sEqual(const RepoNode &other) const
{
	if (other.getTypeAsEnum() != NodeType::TRANSFORMATION || other.getParentIDs().size() != getParentIDs().size())
	{
		repoTrace << "Failed at start";
		return false;
	}

	const TransformationNode otherTrans = TransformationNode(other);

	auto mat = getTransMatrix(false);
	auto otherMat = otherTrans.getTransMatrix(false);

	return mat == otherMat;
}