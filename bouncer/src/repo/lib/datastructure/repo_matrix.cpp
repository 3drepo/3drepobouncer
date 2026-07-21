/**
*  Copyright (C) 2025 3D Repo Ltd
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
#include <repo_log.h>

using namespace repo::lib;

template<typename T>
_RepoMatrix<T>::_RepoMatrix() {

	memset(data, 0, 16 * sizeof(T));
	data[0] = (T)1;
	data[5] = (T)1;
	data[10] = (T)1;
	data[15] = (T)1;
}

template<typename T>
template<typename T2>
repo::lib::_RepoMatrix<T>::_RepoMatrix(const T2* coefficients, bool rowMajor)
{
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			auto c = rowMajor ? (i * 4) + j : i + (j * 4);
			data[c] = (T)*coefficients++;
		}
	}
}

template<typename T>
_RepoMatrix<T>::_RepoMatrix(const std::vector<float>& mat)
	:_RepoMatrix()
{
	for (int i = 0; i < mat.size(); ++i)
	{
		if (i >= 16) break;
		data[i] = (T)mat[i];
	}
}

template<typename T>
_RepoMatrix<T>::_RepoMatrix(const std::vector<double>& mat)
	:_RepoMatrix()
{
	for (int i = 0; i < mat.size(); ++i)
	{
		if (i >= 16) break;
		data[i] = (T)mat[i];
	}
}

template<typename T>
_RepoMatrix<T>::_RepoMatrix(const std::vector<std::vector<T>>& mat)
	:_RepoMatrix()
{
	int counter = 0;
	for (const auto& row : mat)
	{
		for (const auto col : row)
			data[counter++] = (T)col;
	}
}

template<typename T>
T _RepoMatrix<T>::determinant() const {
	/*
	00 01 02 03
	04 05 06 07
	08 09 10 11
	12 13 14 15
	*/

	T a1 = data[0], a2 = data[1], a3 = data[2], a4 = data[3];
	T b1 = data[4], b2 = data[5], b3 = data[6], b4 = data[7];
	T c1 = data[8], c2 = data[9], c3 = data[10], c4 = data[11];
	T d1 = data[12], d2 = data[13], d3 = data[14], d4 = data[15];

	T a1b2 = (a1 * b2) * (c3 * d4 - c4 * d3);
	T a1b3 = (a1 * b3) * (c4 * d2 - c2 * d4);
	T a1b4 = (a1 * b4) * (c2 * d3 - c3 * d2);

	T a2b1 = -(a2 * b1) * (c3 * d4 - c4 * d3);
	T a2b3 = -(a2 * b3) * (c4 * d1 - c1 * d4);
	T a2b4 = -(a2 * b4) * (c1 * d3 - c3 * d1);

	T a3b1 = (a3 * b1) * (c2 * d4 - c4 * d2);
	T a3b2 = (a3 * b2) * (c4 * d1 - c1 * d4);
	T a3b4 = (a3 * b4) * (c1 * d2 - c2 * d1);

	T a4b1 = -(a4 * b1) * (c2 * d3 - c3 * d2);
	T a4b2 = -(a4 * b2) * (c3 * d1 - c1 * d3);
	T a4b3 = -(a4 * b3) * (c1 * d2 - c2 * d1);

	return a1b2 + a1b3 + a1b4
		+ a2b1 + a2b3 + a2b4
		+ a3b1 + a3b2 + a3b4
		+ a4b1 + a4b2 + a4b3;
}

template<typename T>
bool _RepoMatrix<T>::equals(const _RepoMatrix<T>& other) const {
	auto otherData = other.getData();
	bool equal = true;
	for (int i = 0; i < 16; ++i)
	{
		if (!(equal &= data[i] == otherData[i])) break;
	}
	return equal;
}

template<typename T>
const T* _RepoMatrix<T>::getData() const { return data; }

template<typename T>
T* _RepoMatrix<T>::getData() { return data; }

template<typename T>
_RepoMatrix<T> _RepoMatrix<T>::inverse() const {
	std::vector<T> result;
	result.resize(16);

	const T det = determinant();
	if (det == 0)
	{
		repoError << "Trying to invert a matrix with determinant = 0!";
	}
	else
	{
		const T inv_det = 1. / det;

		T a1 = data[0], a2 = data[1], a3 = data[2], a4 = data[3];
		T b1 = data[4], b2 = data[5], b3 = data[6], b4 = data[7];
		T c1 = data[8], c2 = data[9], c3 = data[10], c4 = data[11];
		T d1 = data[12], d2 = data[13], d3 = data[14], d4 = data[15];

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

	return _RepoMatrix<T>(result);
}

template<typename T>
bool _RepoMatrix<T>::isIdentity(const float& eps) const {
	//  00 01 02 03
	//  04 05 06 07
	//  08 09 10 11
	//  12 13 14 15

	bool iden = true;
	float threshold = fabs(eps);

	for (size_t i = 0; i < 16; ++i)
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

template<typename T>
repo::lib::RepoVector3D _RepoMatrix<T>::transformDirection(const repo::lib::RepoVector3D& vec) const
{
	repo::lib::RepoVector3D result;
	result.x = data[0] * vec.x + data[1] * vec.y + data[2] * vec.z;
	result.y = data[4] * vec.x + data[5] * vec.y + data[6] * vec.z;
	result.z = data[8] * vec.x + data[9] * vec.y + data[10] * vec.z;
	return result;
}

template<typename T>
repo::lib::RepoVector3D64 _RepoMatrix<T>::transformDirection(const repo::lib::RepoVector3D64& vec) const
{
	repo::lib::RepoVector3D64 result;
	result.x = data[0] * vec.x + data[1] * vec.y + data[2] * vec.z;
	result.y = data[4] * vec.x + data[5] * vec.y + data[6] * vec.z;
	result.z = data[8] * vec.x + data[9] * vec.y + data[10] * vec.z;
	return result;
}

template<typename T>
std::string _RepoMatrix<T>::toString() const {
	std::stringstream ss;
	for (int i = 0; i < 16; ++i)
	{
		ss << " " << data[i];
		if (i % 4 == 3)
		{
			ss << "\n";
		}
	}

	return ss.str();
}

template<typename T>
_RepoMatrix<T> _RepoMatrix<T>::transpose() const {

	/*
	00 01 02 03             00 04 08 12
	04 05 06 07   ----->    01 05 09 13
	08 09 10 11             02 06 10 14
	12 13 14 15             03 07 11 15
	*/

	return _RepoMatrix<T>(getData(), false);
}

template<typename T>
_RepoMatrix<T> _RepoMatrix<T>::rotation() const {
	auto result = *this;
	auto data = result.getData();
	data[3] = 0;
	data[7] = 0;
	data[11] = 0;
	data[12] = 0;
	data[13] = 0;
	data[14] = 0;
	return result;
}

template<typename T>
repo::lib::_RepoVector3D<T> _RepoMatrix<T>::translation() const {
	return repo::lib::_RepoVector3D<T>(data[3], data[7], data[11]);
}

template<typename T>
repo::lib::_RepoVector3D<T> _RepoMatrix<T>::scale() const {
	return repo::lib::_RepoVector3D<T>(
		std::sqrt(data[0] * data[0] + data[4] * data[4] + data[8] * data[8]),
		std::sqrt(data[1] * data[1] + data[5] * data[5] + data[9] * data[9]),
		std::sqrt(data[2] * data[2] + data[6] * data[6] + data[10] * data[10])
	);
}

template<typename T>
_RepoMatrix<T> _RepoMatrix<T>::rotationX(T angle)
{
	_RepoMatrix<T> m;
	m.data[5] = cos(angle);
	m.data[6] = -sin(angle);
	m.data[9] = sin(angle);
	m.data[10] = cos(angle);
	return m;
}

template<typename T>
_RepoMatrix<T> _RepoMatrix<T>::rotationY(T angle)
{
	_RepoMatrix<T> m;
	m.data[0] = cos(angle);
	m.data[2] = sin(angle);
	m.data[8] = -sin(angle);
	m.data[10] = cos(angle);
	return m;
}

template<typename T>
_RepoMatrix<T> _RepoMatrix<T>::rotationZ(T angle)
{
	_RepoMatrix<T> m;
	m.data[0] = cos(angle);
	m.data[1] = -sin(angle);
	m.data[4] = sin(angle);
	m.data[5] = cos(angle);
	return m;
}

template<typename T>
_RepoMatrix<T> _RepoMatrix<T>::translate(lib::_RepoVector3D<T> t)
{
	_RepoMatrix<T> m;
	m.data[3] = t.x;
	m.data[7] = t.y;
	m.data[11] = t.z;
	return m;
}

template<typename T>
_RepoMatrix<T> _RepoMatrix<T>::rotation(const repo::lib::_RepoVector3D<T>& axis, T angle)
{
	T c = cos(angle);
	T s = sin(angle);
	T t = 1.0 - c;

	_RepoMatrix<T> m;

	m.data[0] = c + axis.x * axis.x * t;
	m.data[5] = c + axis.y * axis.y * t;
	m.data[10] = c + axis.z * axis.z * t;

	T j = axis.x * axis.y * t;
	T k = axis.z * s;
	m.data[4] = j + k;
	m.data[1] = j - k;
	
	j = axis.x * axis.z * t;
	k = axis.y * s;
	m.data[2] = j + k;
	m.data[8] = j - k;

	j = axis.y * axis.z * t;
	k = axis.x * s;
	m.data[9] = j + k;
	m.data[6] = j - k;

	m.data[15] = 1;

	return m;
}

template<typename T>
_RepoMatrix<T> _RepoMatrix<T>::scale(lib::_RepoVector3D<T> s)
{
	_RepoMatrix<T> m;
	m.data[0] = s.x;
	m.data[5] = s.y;
	m.data[10] = s.z;
	return m;
}

template<typename T>
_RepoMatrix<T> _RepoMatrix<T>::scale(T s)
{
	_RepoMatrix<T> m;
	m.data[0] = s;
	m.data[5] = s;
	m.data[10] = s;
	return m;
}

/**
* Matrix x vector multiplication
* NOTE: this assumes matrix has row as fast dimension!
* @param mat 4x4 matrix
* @param vec vector
* @return returns the resulting vector.
*/
template <class T>
repo::lib::RepoVector3D repo::lib::operator*(const _RepoMatrix<T>& matrix, const repo::lib::RepoVector3D& vec)
{
	repo::lib::RepoVector3D result;
	auto mat = matrix.getData();

	/*
	00 01 02 03
	04 05 06 07
	08 09 10 11
	12 13 14 15
	*/

	result.x = (float)(mat[0] * (double)vec.x + mat[1] * (double)vec.y + mat[2] * (double)vec.z + mat[3]);
	result.y = (float)(mat[4] * (double)vec.x + mat[5] * (double)vec.y + mat[6] * (double)vec.z + mat[7]);
	result.z = (float)(mat[8] * (double)vec.x + mat[9] * (double)vec.y + mat[10] * (double)vec.z + mat[11]);

	float sig = 1e-5;

	if (fabs(mat[12]) > sig || fabs(mat[13]) > sig || fabs(mat[14]) > sig || fabs(mat[15] - 1) > sig)
	{
		repoWarning << "Potentially incorrect transformation : does not expect the last row to have values!";
		repoWarning << matrix.toString();
		exit(0);
	}

	return result;
}

template <class T>
repo::lib::RepoVector3D64 repo::lib::operator*(const _RepoMatrix<T>& matrix, const repo::lib::RepoVector3D64& vec)
{
	repo::lib::RepoVector3D64 result;
	const auto mat = matrix.getData();

	/*
	00 01 02 03
	04 05 06 07
	08 09 10 11
	12 13 14 15
	*/

	result.x = mat[0] * vec.x + mat[1] * vec.y + mat[2] * vec.z + mat[3];
	result.y = mat[4] * vec.x + mat[5] * vec.y + mat[6] * vec.z + mat[7];
	result.z = mat[8] * vec.x + mat[9] * vec.y + mat[10] * vec.z + mat[11];

	float sig = 1e-5;

	if (fabs(mat[12]) > sig || fabs(mat[13]) > sig || fabs(mat[14]) > sig || fabs(mat[15] - 1) > sig)
	{
		repoWarning << "Potentially incorrect transformation : does not expect the last row to have values!";
		repoWarning << matrix.toString();
		exit(0);
	}

	return result;
}

template <class T>
_RepoMatrix<T> repo::lib::operator*(const _RepoMatrix<T>& matrix1, const _RepoMatrix<T>& matrix2)
{
	_RepoMatrix<T> result;
	auto resultData = result.getData();

	auto mat1 = matrix1.getData();
	auto mat2 = matrix2.getData();

	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			size_t resultIdx = i * 4 + j;
			resultData[resultIdx] = 0;
			for (int k = 0; k < 4; ++k)
			{
				resultData[resultIdx] += mat1[i * 4 + k] * mat2[k * 4 + j];
			}
		}
	}
	return result;
}

template<class T>
bool repo::lib::operator==(const _RepoMatrix<T>& matrix1, const _RepoMatrix<T>& matrix2)
{
	return matrix1.equals(matrix2);
}

template <class T>
bool repo::lib::operator!=(const _RepoMatrix<T>& matrix1, const _RepoMatrix<T>& matrix2)
{
	return !(matrix1 == matrix2);
}

// Create the explicit instantations for all the supported types.

template class REPO_API_EXPORT _RepoMatrix<double>;

// In the latest version of bouncer, we no longer support single precision
// matrices, though we do allow upcasting explicitly.

template _RepoMatrix<double>::_RepoMatrix(const double* coefficients, bool rowMajor);
template _RepoMatrix<double>::_RepoMatrix(const float* coefficients, bool rowMajor);

template repo::lib::RepoVector3D repo::lib::operator*(const _RepoMatrix<double>& matrix, const repo::lib::RepoVector3D& vec);
template repo::lib::RepoVector3D64 repo::lib::operator*(const _RepoMatrix<double>& matrix, const repo::lib::RepoVector3D64& vec);
template _RepoMatrix<double> repo::lib::operator*(const _RepoMatrix<double>& matrix1, const _RepoMatrix<double>& matrix2);
template bool repo::lib::operator==(const _RepoMatrix<double>& matrix1, const _RepoMatrix<double>& matrix2);
template bool repo::lib::operator!=(const _RepoMatrix<double>& matrix1, const _RepoMatrix<double>& matrix2);