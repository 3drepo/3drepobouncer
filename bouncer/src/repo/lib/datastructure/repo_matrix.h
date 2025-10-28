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

#include <string>
#include <vector>
#include "repo_vector.h"

namespace repo {
	namespace lib {

		template <class T>
		class REPO_API_EXPORT _RepoMatrix
		{
		public:
			_RepoMatrix();

			_RepoMatrix(const std::vector<float>& mat);
			_RepoMatrix(const std::vector<double>& mat);

			_RepoMatrix(const std::vector<std::vector<T>>& mat);

			template<typename T2>
			_RepoMatrix(const T2* coefficients, bool rowMajor = true);

			T determinant() const;

			bool equals(const _RepoMatrix<T>& other) const;

			const T* getData() const;

			T* getData();

			_RepoMatrix<T> inverse() const;

			bool isIdentity(const float& eps = 10e-5) const;

			repo::lib::RepoVector3D transformDirection(const repo::lib::RepoVector3D& vec) const;

			repo::lib::RepoVector3D64 transformDirection(const repo::lib::RepoVector3D64& vec) const;

			std::string toString() const;

			_RepoMatrix<T> transpose() const;

			_RepoMatrix<T> rotation() const;

			repo::lib::_RepoVector3D<T> translation() const;

			repo::lib::_RepoVector3D<T> scale() const;

			static _RepoMatrix<T> rotationX(T angle);

			static _RepoMatrix<T> rotationY(T angle);

			static _RepoMatrix<T> rotationZ(T angle);

			static _RepoMatrix<T> rotation(const repo::lib::_RepoVector3D<T>& axis, T angle);

			static _RepoMatrix<T> translate(lib::_RepoVector3D<T> t);

			static _RepoMatrix<T> scale(lib::_RepoVector3D<T> s);

			static _RepoMatrix<T> scale(T s);

		private:
			T data[16];
		};

		template <class T>
		inline repo::lib::RepoVector3D operator*(const _RepoMatrix<T>& matrix, const repo::lib::RepoVector3D& vec);

		template <class T>
		inline repo::lib::RepoVector3D64 operator*(const _RepoMatrix<T>& matrix, const repo::lib::RepoVector3D64& vec);

		template <class T>
		inline _RepoMatrix<T> operator*(const _RepoMatrix<T>& matrix1, const _RepoMatrix<T>& matrix2);

		template <class T>
		inline bool operator==(const _RepoMatrix<T>& matrix1, const _RepoMatrix<T>& matrix2);

		template <class T>
		inline bool operator!=(const _RepoMatrix<T>& matrix1, const _RepoMatrix<T>& matrix2);

		using RepoMatrix = _RepoMatrix<double>;
	}
}
