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
*  Camera node
*/

#include "repo_node_camera.h"

#define _USE_MATH_DEFINES
#include <math.h>

using namespace repo::core::model;

CameraNode::CameraNode() :
RepoNode()
{
}

CameraNode::CameraNode(RepoBSON bson) :
RepoNode(bson)
{
}

CameraNode::~CameraNode()
{
}

RepoNode CameraNode::cloneAndApplyTransformation(
	const std::vector<float> &matrix) const
{
	RepoBSONBuilder builder;
	if (hasField(REPO_NODE_LABEL_LOOK_AT))
	{
		builder.append(REPO_NODE_LABEL_LOOK_AT, multiplyMatVec(matrix, getLookAt()));
	}

	if (hasField(REPO_NODE_LABEL_POSITION))
	{
		builder.append(REPO_NODE_LABEL_POSITION, multiplyMatVec(matrix, getPosition()));
	}

	if (hasField(REPO_NODE_LABEL_UP))
	{
		builder.append(REPO_NODE_LABEL_UP, multiplyMatVec(matrix, getUp()));
	}
	return CameraNode(builder.appendElementsUnique(*this));
}

repo_vector_t CameraNode::getPosition() const
{
	repo_vector_t vec;
	if (hasField(REPO_NODE_LABEL_POSITION))
	{
		std::vector<float> floatArr = getFloatArray(REPO_NODE_LABEL_POSITION);
		if (floatArr.size() >= 3)
		{
			//repo_vector_t is effectively float[3]
			std::copy(floatArr.begin(), floatArr.begin() + 3, (float*)&vec);
		}
	}

	return vec;
}

repo_vector_t CameraNode::getLookAt() const
{
	repo_vector_t vec = { 0, 0, -1 };
	if (hasField(REPO_NODE_LABEL_LOOK_AT))
	{
		std::vector<float> floatArr = getFloatArray(REPO_NODE_LABEL_LOOK_AT);
		if (floatArr.size() >= 3)
		{
			//repo_vector_t is effectively float[3]
			std::copy(floatArr.begin(), floatArr.begin() + 3, (float*)&vec);
		}
	}

	// if look at is 0 0 0, treat it as non existent (FIXME: is this really correct?)
	if (0 == vec.x == vec.y == vec.z)
		return{ 0, 0, -1 };
	else
		return vec;
}

repo_vector_t CameraNode::getUp() const
{
	repo_vector_t vec = { 0, 1, 0 };
	if (hasField(REPO_NODE_LABEL_UP))
	{
		std::vector<float> floatArr = getFloatArray(REPO_NODE_LABEL_UP);
		if (floatArr.size() >= 3)
		{
			//repo_vector_t is effectively float[3]
			std::copy(floatArr.begin(), floatArr.begin() + 3, (float*)&vec);
		}
	}

	return vec;
}

std::vector<float> CameraNode::getCameraMatrix(
	const bool &rowMajor) const
{
	std::vector<float> mat;
	mat.resize(16);

	uint8_t a1, a2, a3, a4, b1, b2, b3, b4, c1, c2, c3, c4, d1, d2, d3, d4;

	if (rowMajor)
	{
		//row is the fast dimension
		a1 = 0;
		a2 = 1;
		a3 = 2;
		a4 = 3;

		b1 = 4;
		b2 = 5;
		b3 = 6;
		b4 = 7;

		c1 = 8;
		c2 = 9;
		c3 = 10;
		c4 = 11;

		d1 = 12;
		d2 = 13;
		d3 = 14;
		d4 = 15;
	}
	else
	{
		//col is fast dimension
		a1 = 0;
		a2 = 4;
		a3 = 8;
		a4 = 12;

		b1 = 1;
		b2 = 5;
		b3 = 9;
		b4 = 13;

		c1 = 2;
		c2 = 6;
		c3 = 10;
		c4 = 14;

		d1 = 3;
		d2 = 7;
		d3 = 11;
		d4 = 15;
	}

	/** We don't know whether these vectors are already normalized ...*/
	repo_vector_t zaxis = getLookAt();
	repo_vector_t yaxis = getUp();
	repo_vector_t xaxis = crossProduct(yaxis, zaxis);

	normalize(zaxis);
	normalize(yaxis);
	normalize(xaxis);

	repo_vector_t position = getPosition();

	mat[a4] = -dotProduct(xaxis, position);
	mat[b4] = -dotProduct(yaxis, position);
	mat[c4] = -dotProduct(zaxis, position);

	mat[a1] = xaxis.x;
	mat[a2] = xaxis.y;
	mat[a3] = xaxis.z;

	mat[b1] = yaxis.x;
	mat[b2] = yaxis.y;
	mat[b3] = yaxis.z;

	mat[c1] = zaxis.x;
	mat[c2] = zaxis.y;
	mat[c3] = zaxis.z;

	mat[d1] = mat[d2] = mat[d3] = 0.f;
	mat[d4] = 1.f;

	return mat;
}

std::vector<float> CameraNode::getOrientation() const
{
	repo_vector_t lookAt = getLookAt();
	repo_vector_t up = getUp();
	repo_vector_t forward = { -lookAt.x, -lookAt.y, -lookAt.z };
	normalize(forward);
	normalize(up);
	repo_vector_t right = crossProduct(up, forward);

	float a = up.x - right.y;
	float b = forward.x - right.z;
	float c = forward.y - up.z;
	float tr = right.x + up.y + forward.z;

	float x = 0, y = 0, z = 0, angle = 1;

	float eps = 0.0001;
	float sqrtHalf = sqrtf(0.5);

	if (fabs(a) < eps && fabs(b) < eps && fabs(c) < eps)
	{
		float d = up.x + right.y;
		float e = forward.x + right.z;
		float f = forward.y + up.z;

		if (!((fabs(d) < eps) && (fabs(e) < eps) && (fabs(f) < eps) && (fabs(tr - 3.0) < eps)))
		{
			angle = M_PI;

			float xx = (right.x + 1.) / 2.;
			float yy = (up.y + 1.) / 2;
			float zz = (forward.z + 1.) / 2;
			float xy = d / 4.;
			float xz = e / 4.;
			float yz = f / 4.;

			if ((xx - yy) > eps && (xx - zz) > eps)
			{
				repoDebug << " case a";
				if (xx < eps)
				{
					x = 0;
					y = sqrtHalf;
					z = sqrtHalf;
				}
				else
				{
					x = sqrtf(xx);
					y = xy / x;
					z = xz / x;
				}
			}
			else if ((yy - zz) > eps)
			{
				if (yy < eps)
				{
					x = sqrtHalf;
					y = 0;
					z = sqrtHalf;
				}
				else
				{
					y = sqrt(yy);
					x = xy / y;
					z = yz / y;
				}
			}
			else
			{
				if (zz < eps)
				{
					x = sqrtHalf;
					y = sqrtHalf;
					z = 0;
				}
				else
				{
					z = sqrtf(zz);
					x = xz / z;
					y = yz / z;
				}
			}
		}//if (!(fabs(d) < eps && fabs(e) < eps && fabs(f) < eps && fabs(tr - 3) < eps))
	}//if (fabs(a) < eps && fabs(b) < eps && fabs(c) < eps)
	else
	{
		float s = sqrtf(a*a + b*b + c*c);
		if (s < eps)
			s = 1;

		x = -c / s;
		y = b / s;
		z = -a / s;

		angle = acosf((tr - 1.) / 2.);
	}

	return{ x, y, z, angle };
}

bool CameraNode::sEqual(const RepoNode &other) const
{
	if (other.getTypeAsEnum() != NodeType::CAMERA || other.getParentIDs().size() != getParentIDs().size())
	{
		return false;
	}

	const CameraNode otherCam = CameraNode(other);

	std::vector<float> mat = getCameraMatrix();
	std::vector<float> otherMat = otherCam.getCameraMatrix();

	return !memcmp(mat.data(), otherMat.data(), mat.size() *sizeof(*mat.data()));
}