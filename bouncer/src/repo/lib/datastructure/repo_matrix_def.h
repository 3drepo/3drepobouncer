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

#pragma once

#include "repo_vector.h"
#include <repo_log.h>
#include <string>

namespace repo {
	namespace lib {
		template <class T>
		class REPO_API_EXPORT _RepoMatrix
		{
		public:

			_RepoMatrix() {
				data = { 1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					0, 0, 0, 1 };
			}

			_RepoMatrix(const std::vector<float> &mat)
			{
				data = { 1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					0, 0, 0, 1 };

				for (int i = 0; i < mat.size(); ++i)
				{
					if (i >= 16) break;
					data[i] = (T)mat[i];
				}
			}

			_RepoMatrix(const std::vector<double> &mat)

			{
				data = { 1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					0, 0, 0, 1 };

				for (int i = 0; i < mat.size(); ++i)
				{
					if (i >= 16) break;
					data[i] = (T)mat[i];
				}
			}

			_RepoMatrix(const std::vector<std::vector<T>> &mat)
			{
				data = { 1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					0, 0, 0, 1 };

				int counter = 0;
				for (const auto &row : mat)
				{
					for (const auto col : row)
						data[counter++] = (T)col;
				}
				if (data.size() > 16) data.resize(16);
			}

			template<typename T2>
			_RepoMatrix(const T2* coefficients, bool rowMajor = true)
			{
				data.resize(16);
				for (int i = 0; i < 4; i++) {
					for (int j = 0; j < 4; j++) {
						auto c = rowMajor ? (i * 4) + j : i + (j * 4);
						data[c] = (T)*coefficients++;
					}
				}
			}

			float determinant() const {
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

			bool equals(const _RepoMatrix<T> &other) const {
				auto otherData = other.getData();
				bool equal = true;
				for (int i = 0; i < data.size(); ++i)
				{
					if (!(equal &= data[i] == otherData[i])) break;
				}
				return equal;
			}

			std::vector<T> getData() const { return data; }

			std::vector<T>& getData() { return data; }

			_RepoMatrix<T> invert() const {
				std::vector<T> result;
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

				return _RepoMatrix<T>(result);
			}

			bool isIdentity(const float &eps = 10e-5) const {
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

			repo::lib::RepoVector3D transformDirection(const repo::lib::RepoVector3D& vec) const
			{
				repo::lib::RepoVector3D result;
				result.x = data[0] * vec.x + data[1] * vec.y + data[2] * vec.z;
				result.y = data[4] * vec.x + data[5] * vec.y + data[6] * vec.z;
				result.z = data[8] * vec.x + data[9] * vec.y + data[10] * vec.z;
				return result;
			}

			std::string toString() const {
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

			_RepoMatrix<T> transpose() const {
				std::vector<T> result(data.begin(), data.end());

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

				return _RepoMatrix<T>(result);
			}

			static _RepoMatrix<T> rotationX(T angle)
			{
				return _RepoMatrix<T>(std::vector<T>({
					1, 0, 0, 0,
					0, cos(angle), -sin(angle), 0,
					0, sin(angle), cos(angle), 0,
					0, 0, 0, 1
				}));
			}

			static _RepoMatrix<T> rotationY(T angle)
			{
				return _RepoMatrix<T>(std::vector<T>({
					cos(angle), 0, sin(angle), 0,
					0, 1, 0, 0,
					-sin(angle), 0, cos(angle), 0,
					0, 0, 0, 1
				}));
			}

			static _RepoMatrix<T> rotationZ(T angle)
			{
				return _RepoMatrix<T>(std::vector<T>({
					cos(angle), -sin(angle), 0, 0,
					sin(angle), cos(angle), 0, 0,
					0, 0, 1, 0,
					0, 0, 0, 1
				}));
			}

			static _RepoMatrix<T> translate(lib::_RepoVector3D<T> t)
			{
				return _RepoMatrix<T>(std::vector<T>({
					1, 0, 0, t.x,
					0, 1, 0, t.y,
					0, 0, 1, t.z,
					0, 0, 0, 1
				}));
			}

			explicit operator repo::lib::_RepoMatrix<float>() const
			{
				auto m = repo::lib::_RepoMatrix<float>();
				for (auto i = 0; i < data.size(); i++) {
					m.getData()[i] = (float)data[i];
				}
				return m;
			}

		private:
			std::vector<T> data;
		};

		/**
		* Matrix x vector multiplication
		* NOTE: this assumes matrix has row as fast dimension!
		* @param mat 4x4 matrix
		* @param vec vector
		* @return returns the resulting vector.
		*/
		template <class T>
		inline repo::lib::RepoVector3D operator*(const _RepoMatrix<T> &matrix, const repo::lib::RepoVector3D &vec)
		{
			repo::lib::RepoVector3D result;
			auto mat = matrix.getData();
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
		inline repo::lib::RepoVector3D64 operator*(const _RepoMatrix<T> &matrix, const repo::lib::RepoVector3D64 &vec)
		{
			repo::lib::RepoVector3D64 result;
			auto mat = matrix.getData();
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
		inline _RepoMatrix<T> operator*(const _RepoMatrix<T> &matrix1, const _RepoMatrix<T> &matrix2)
		{
			std::vector<float> result;
			result.resize(16);

			auto mat1 = matrix1.getData();
			auto mat2 = matrix2.getData();

			for (int i = 0; i < 4; ++i)
			{
				for (int j = 0; j < 4; ++j)
				{
					size_t resultIdx = i * 4 + j;
					result[resultIdx] = 0;
					for (int k = 0; k < 4; ++k)
					{
						result[resultIdx] += mat1[i * 4 + k] * mat2[k * 4 + j];
					}
				}
			}
			return _RepoMatrix<T>(result);
		}

		template <class T>
		inline bool operator==(const _RepoMatrix<T> &matrix1, const _RepoMatrix<T> &matrix2)
		{
			return matrix1.equals(matrix2);
		}

		template <class T>
		inline bool operator!=(const _RepoMatrix<T> &matrix1, const _RepoMatrix<T> &matrix2)
		{
			return !(matrix1 == matrix2);
		}
	}
}
