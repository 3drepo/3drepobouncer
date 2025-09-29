/**
*  Copyright (C) 2024 3D Repo Ltd
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

#include "repo/repo_bouncer_global.h"
#include <repo/lib/datastructure/repo_vector.h>

namespace repo {
	namespace lib {

		template <typename T>
		class _RepoMatrix;

		/*
		* Represents a bounding box in 3D
		*/
		REPO_API_EXPORT class RepoBounds
		{
		public:
			RepoBounds();
			RepoBounds(const RepoVector3D64& min, const RepoVector3D64& max);
			RepoBounds(const RepoVector3D& min, const RepoVector3D& max);
			RepoBounds(std::initializer_list<RepoVector3D64> points);

			// Expands the bounds to include the point
			void encapsulate(const RepoVector3D64& p);
			void encapsulate(const RepoBounds& other);

			bool operator==(const RepoBounds& other) const;

			bool contains(const RepoVector3D64& p) const;

			const RepoVector3D64& min() const;
			const RepoVector3D64& max() const;

			RepoVector3D64 size() const;

		private:
			RepoVector3D64 bmin;
			RepoVector3D64 bmax;
		};

		REPO_API_EXPORT repo::lib::RepoBounds operator*(const _RepoMatrix<double>& matrix, const repo::lib::RepoBounds& bounds);
	}
}
