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
#include "../repo_log.h" 
#include <string>

namespace repo{
	namespace lib{
		class REPO_API_EXPORT RepoMatrix
		{
		public:

			RepoMatrix();

			RepoMatrix(const std::vector<float> &mat);

			RepoMatrix(const std::vector<std::vector<float>> &mat);

			float determinant() const;

			bool equals(const RepoMatrix &other) const;

			std::vector<float> getData() const { return data; }

			RepoMatrix invert() const;

			bool isIdentity(const float &eps = 10e-5) const;

			std::string toString() const;

			RepoMatrix transpose() const;

		private:
			std::vector<float> data;
			  
		};


		/**
		* Matrix x vector multiplication
		* NOTE: this assumes matrix has row as fast dimension!
		* @param mat 4x4 matrix
		* @param vec vector
		* @return returns the resulting vector.
		*/
		inline repo::lib::RepoVector3D operator*(const RepoMatrix &matrix, const repo::lib::RepoVector3D &vec)
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
			}

			return result;
		}


		inline RepoMatrix operator*(const RepoMatrix &matrix1, const RepoMatrix &matrix2)
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
			return RepoMatrix(result);
		}
		
		inline bool operator==(const RepoMatrix &matrix1, const RepoMatrix &matrix2)
		{
			return matrix1.equals(matrix2);
		}

		inline bool operator!=(const RepoMatrix &matrix1, const RepoMatrix &matrix2)
		{
			return !(matrix1 == matrix2);
		}

	}	
}
