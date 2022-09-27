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
			columns.append(std::to_string(j), resultData[idx]);
		}
		rows.appendArray(std::to_string(i), columns.obj());
	}
	builder.appendArray(REPO_NODE_LABEL_MATRIX, rows.obj());
	

	builder.appendElementsUnique(*this);

	return TransformationNode(RepoBSON(builder.mongoObj(), bigFiles));
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
			columns.append(std::to_string(j), (i == j ? 1.0 : 0.0));
		}
		rows.appendArray(std::to_string(i), columns.obj());
	}
	builder.appendArray(REPO_NODE_LABEL_MATRIX, rows.obj());

	builder.appendElementsUnique(*this);

	return TransformationNode(RepoBSON(builder.mongoObj(), bigFiles));
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

// Keeping these vectors around avoids the allocations on the very frequent
// calls to getTransMatrix, as the contents are always the same size.
// (Static here means they are not exported.)
static std::vector<mongo::BSONElement> rows;
static std::vector<mongo::BSONElement> cols;
static std::vector<float> transformationMatrix;

repo::lib::RepoMatrix TransformationNode::getTransMatrix(const bool &rowMajor) const
{
	uint32_t rowInd = 0, colInd = 0;
	if (hasField(REPO_NODE_LABEL_MATRIX))
	{
		transformationMatrix.resize(16);
		float *transArr = &transformationMatrix.at(0);

		// matrix is stored as array of arrays
		auto matrixObj =
			getField(REPO_NODE_LABEL_MATRIX).embeddedObject();

		rows.clear();
		matrixObj.elems(rows);
		for (size_t rowInd = 0; rowInd < 4; rowInd++)
		{
			cols.clear();
			rows[rowInd].embeddedObject().elems(cols);
			for (size_t colInd = 0; colInd < 4; colInd++)
			{
				uint32_t index;
				if (rowMajor) // Whether to return the matrix transposed - matrices are always *stored* row-major.
				{
					index = colInd * 4 + rowInd;
				}
				else
				{
					index = rowInd * 4 + colInd;
				}

				transArr[index] = cols[colInd].number();
			}
		}
		return repo::lib::RepoMatrix(transformationMatrix);
	}
	else
	{
		repoWarning << "This transformation has no matrix field, returning identity";
		return repo::lib::RepoMatrix();
	}
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