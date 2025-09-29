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

		repo::lib::RepoVector3D64 vector(double scale);

		repo::lib::RepoVector3D64 direction();

		double scalar();

		int range(int lower, int upper);

		double number(double range);

		double number(double upper, double lower);

		double angle();

		repo::lib::RepoMatrix transform(bool rotation, bool scale);

		bool boolean();
	};
}
