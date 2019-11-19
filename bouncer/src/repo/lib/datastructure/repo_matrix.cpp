/**
*  Copyright (C) 2016 3D Repo Ltd
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


#include "repo_matrix.h"
#include <sstream>

using namespace repo::lib;

RepoMatrix::RepoMatrix()
{
	data = { 1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1 };
}

RepoMatrix::RepoMatrix(const std::vector<float> &mat)
{
	data = { 1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1 };

	for (int i = 0; i < mat.size(); ++i)
	{
		if (i >= 16) break;
		data[i] = mat[i];
		
	}	
}		

RepoMatrix::RepoMatrix(const std::vector<double> &mat)
{
	data = { 1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1 };

	for (int i = 0; i < mat.size(); ++i)
	{
		if (i >= 16) break;
		data[i] = mat[i];

	}
}

RepoMatrix::RepoMatrix(const std::vector<std::vector<float>> &mat)
{

	data = { 1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1 };

	int counter = 0;
	for (const auto &row : mat)
	{
		for (const auto col : row)
			data[counter++] = col;
	}
	if (data.size() > 16) data.resize(16);
}


float RepoMatrix::determinant() const
{
	/*
	00 01 02 03
	04 05 06 07
	08 09 10 11
	12 13 14 15
	*/

	float a1 = data[0], a2 = data[1], a3 = data[2], a4 = data[3];
	float b1 = data[4], b2 = data[5], b3 = data[6], b4 = data[7];
	float c1 = data[8], c2 = data[9], c3 = data[10], c4 = data[11];
	float d1 = data[12], d2 = data[13], d3 = data[14], d4 = data[15];

	float a1b2 = (a1 * b2) *(c3 * d4 - c4 * d3);
	float a1b3 = (a1 * b3) *(c4 * d2 - c2 * d4);
	float a1b4 = (a1 * b4) *(c2 * d3 - c3 * d2);

	float a2b1 = -(a2 * b1) *(c3 * d4 - c4 * d3);
	float a2b3 = -(a2 * b3) *(c4 * d1 - c1 * d4);
	float a2b4 = -(a2 * b4) *(c1 * d3 - c3 * d1);

	float a3b1 = (a3 * b1) *(c2 * d4 - c4 * d2);
	float a3b2 = (a3 * b2) *(c4 * d1 - c1 * d4);
	float a3b4 = (a3 * b4) *(c1 * d2 - c2 * d1);

	float a4b1 = -(a4 * b1) *(c2 * d3 - c3 * d2);
	float a4b2 = -(a4 * b2) *(c3 * d1 - c1 * d3);
	float a4b3 = -(a4 * b3) *(c1 * d2 - c2 * d1);

	return a1b2 + a1b3 + a1b4
		+ a2b1 + a2b3 + a2b4
		+ a3b1 + a3b2 + a3b4
		+ a4b1 + a4b2 + a4b3;
}


bool RepoMatrix::equals(const RepoMatrix &other) const
{

	auto otherData = other.getData();
	bool equal = true;
	for (int i = 0; i < data.size(); ++i)
	{
		if (!(equal &= data[i] == otherData[i])) break;
	}
	return equal;
}

bool RepoMatrix::isIdentity(const float &eps) const
{
	//  00 01 02 03
	//  04 05 06 07
	//  08 09 10 11
	//  12 13 14 15

	bool iden = true;
	float threshold = fabs(eps);

	for (size_t i = 0; i < data.size(); ++i)
	{
		if (i % 5)
		{
			//This is suppose to be 0
			iden &= fabs(data[i]) <= threshold;
		}
		else
		{
			//This is suppose to be 1
			iden &= data[i] <= 1 + threshold && data[i] >= 1 - threshold;
		}
	}

	return iden;
}

RepoMatrix RepoMatrix::invert() const
{
	std::vector<float> result;
	result.resize(16);

	const float det = determinant();
	if (det == 0)
	{
		repoError << "Trying to invert a matrix with determinant = 0!";
	}
	else
	{
		const float inv_det = 1. / det;

		float a1 = data[0], a2 = data[1], a3 = data[2], a4 = data[3];
		float b1 = data[4], b2 = data[5], b3 = data[6], b4 = data[7];
		float c1 = data[8], c2 = data[9], c3 = data[10], c4 = data[11];
		float d1 = data[12], d2 = data[13], d3 = data[14], d4 = data[15];

		result[0] = inv_det * (b2 * (c3 * d4 - c4 * d3) + b3 * (c4 * d2 - c2 * d4) + b4 * (c2 * d3 - c3 * d2));
		result[1] = -inv_det * (a2 * (c3 * d4 - c4 * d3) + a3 * (c4 * d2 - c2 * d4) + a4 * (c2 * d3 - c3 * d2));
		result[2] = inv_det * (a2 * (b3 * d4 - b4 * d3) + a3 * (b4 * d2 - b2 * d4) + a4 * (b2 * d3 - b3 * d2));
		result[3] = -inv_det * (a2 * (b3 * c4 - b4 * c3) + a3 * (b4 * c2 - b2 * c4) + a4 * (b2 * c3 - b3 * c2));

		result[4] = -inv_det * (b1 * (c3 * d4 - c4 * d3) + b3 * (c4 * d1 - c1 * d4) + b4 * (c1 * d3 - c3 * d1));
		result[5] = inv_det * (a1 * (c3 * d4 - c4 * d3) + a3 * (c4 * d1 - c1 * d4) + a4 * (c1 * d3 - c3 * d1));
		result[6] = -inv_det * (a1 * (b3 * d4 - b4 * d3) + a3 * (b4 * d1 - b1 * d4) + a4 * (b1 * d3 - b3 * d1));
		result[7] = inv_det * (a1 * (b3 * c4 - b4 * c3) + a3 * (b4 * c1 - b1 * c4) + a4 * (b1 * c3 - b3 * c1));

		result[8] = inv_det * (b1 * (c2 * d4 - c4 * d2) + b2 * (c4 * d1 - c1 * d4) + b4 * (c1 * d2 - c2 * d1));
		result[9] = -inv_det * (a1 * (c2 * d4 - c4 * d2) + a2 * (c4 * d1 - c1 * d4) + a4 * (c1 * d2 - c2 * d1));
		result[10] = inv_det * (a1 * (b2 * d4 - b4 * d2) + a2 * (b4 * d1 - b1 * d4) + a4 * (b1 * d2 - b2 * d1));
		result[11] = -inv_det * (a1 * (b2 * c4 - b4 * c2) + a2 * (b4 * c1 - b1 * c4) + a4 * (b1 * c2 - b2 * c1));

		result[12] = -inv_det * (b1 * (c2 * d3 - c3 * d2) + b2 * (c3 * d1 - c1 * d3) + b3 * (c1 * d2 - c2 * d1));
		result[13] = inv_det * (a1 * (c2 * d3 - c3 * d2) + a2 * (c3 * d1 - c1 * d3) + a3 * (c1 * d2 - c2 * d1));
		result[14] = -inv_det * (a1 * (b2 * d3 - b3 * d2) + a2 * (b3 * d1 - b1 * d3) + a3 * (b1 * d2 - b2 * d1));
		result[15] = inv_det * (a1 * (b2 * c3 - b3 * c2) + a2 * (b3 * c1 - b1 * c3) + a3 * (b1 * c2 - b2 * c1));
	}


	return RepoMatrix(result);
}

std::string RepoMatrix::toString() const
{
	std::stringstream ss;
	for (int i = 0; i < data.size(); ++i)
	{
		ss << " " << data[i];
		if (i % 4 == 3)
		{
			ss << "\n";
		}
	}

	return ss.str();				
}

RepoMatrix RepoMatrix::transpose() const
{
	std::vector<float> result(data.begin(), data.end());


	/*
	00 01 02 03             00 04 08 12
	04 05 06 07   ----->    01 05 09 13
	08 09 10 11             02 06 10 14
	12 13 14 15             03 07 11 15
	*/

	result[1] = data[4];
	result[4] = data[1];
	result[2] = data[8];
	result[8] = data[2];
	result[3] = data[12];
	result[12] = data[3];
	result[6] = data[9];
	result[9] = data[6];
	result[7] = data[13];
	result[13] = data[7];
	result[11] = data[14];
	result[14] = data[11];


	return RepoMatrix(result);
}
