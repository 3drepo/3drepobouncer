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

#pragma once

#include <repo/lib/datastructure/repo_vector3d.h>
#include <repo/lib/datastructure/repo_matrix.h>
#include <repo/lib/datastructure/repo_range.h>
#include <repo/lib/datastructure/repo_bounds.h>
#include <random>

namespace testing {
	struct RepoRandomGenerator
	{
		std::random_device rd;
		std::mt19937 gen;

		RepoRandomGenerator()
			:gen(rd())
		{
		}

		// Returns a vector with a length between lower and upper

		repo::lib::RepoVector3D64 vector(const repo::lib::RepoRange& range);

		repo::lib::RepoVector3D64 vector(
			const repo::lib::RepoRange& x, 
			const repo::lib::RepoRange& y, 
			const repo::lib::RepoRange& z
		);

		repo::lib::RepoVector3D64 direction();

		double scalar();

		int64_t range(int64_t lower, int64_t upper);

		double number(double range);

		double number(const repo::lib::RepoRange& range);

		double angle();

		double angle(const repo::lib::RepoRange& radians);

		repo::lib::RepoMatrix transform(
			bool rotation, 
			const repo::lib::RepoRange& translate, 
			const repo::lib::RepoRange& scale);

		repo::lib::RepoMatrix rotation(
			const repo::lib::RepoRange& x,
			const repo::lib::RepoRange& y,
			const repo::lib::RepoRange& z
		);

		repo::lib::RepoMatrix rotation(
			const repo::lib::RepoVector3D64& axis,
			const repo::lib::RepoRange& angle
		);

		bool boolean();

		repo::lib::RepoVector3D64 barycentric();
	};

	/*
	* BoundedContext is a wrapper around random that ensures all generated points
	* lie within a given range of each other, even if the absolute start and end
	* of this range relative to the origin is much larger.
	* An instance of this can be used to create vectors that all sit within a
	* RepoBounds of a given size, while leaving the center of the bounds
	* unconstrained.
	*/
	struct BoundedContext
	{
		RepoRandomGenerator& random;
		BoundedContext(RepoRandomGenerator& random, double limit);

	private:
		repo::lib::RepoVector3D64 min;
		repo::lib::RepoVector3D64 max;
		repo::lib::RepoBounds sampled;
		double limit;

	public:
		repo::lib::RepoVector3D64 vector(
			const repo::lib::RepoRange& x,
			const repo::lib::RepoRange& y,
			const repo::lib::RepoRange& z
		);

		void reset();
	};
}
