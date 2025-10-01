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

#include "repo_test_random_generator.h"

using namespace testing;

#define PI 3.14159265358979323846

repo::lib::RepoVector3D64 RepoRandomGenerator::vector(const repo::lib::RepoRange& range)
{
	auto v = direction();
	std::uniform_real_distribution<> d(range.x, range.y);
	return v * d(gen);
}

repo::lib::RepoVector3D64 RepoRandomGenerator::vector(
	const repo::lib::RepoRange& x,
	const repo::lib::RepoRange& y,
	const repo::lib::RepoRange& z
)
{
	std::uniform_real_distribution<> dx(x.min(), x.max());
	std::uniform_real_distribution<> dy(y.min(), y.max());
	std::uniform_real_distribution<> dz(z.min(), z.max());
	return repo::lib::RepoVector3D64(
		dx(gen),
		dy(gen),
		dz(gen)
	);
}

repo::lib::RepoVector3D64 RepoRandomGenerator::direction()
{
	std::uniform_real_distribution<> d(-1, 1);
	auto v = repo::lib::RepoVector3D64(
		d(gen),
		d(gen),
		d(gen)
	);
	v.normalize();
	return v;
}

double RepoRandomGenerator::scalar()
{
	std::uniform_real_distribution<> d(0, 1);
	return d(gen);
}

double RepoRandomGenerator::number(double upper)
{
	std::uniform_real_distribution<> d(0, upper);
	return d(gen);
}

int64_t RepoRandomGenerator::range(int64_t lower, int64_t upper)
{
	if (lower > upper) {
		std::swap(lower, upper);
	}
	std::uniform_int_distribution<int64_t> d(lower, upper);
	return d(gen);
}

double RepoRandomGenerator::number(const repo::lib::RepoRange& range)
{
	std::uniform_real_distribution<> d(range.x, range.y);
	return d(gen);
}

double RepoRandomGenerator::angle() // in radians
{
	std::uniform_real_distribution<> d(-2 * PI, 2 * PI);
	return d(gen);
}

bool RepoRandomGenerator::boolean()
{
	return scalar() > 0.5;
}

repo::lib::RepoMatrix RepoRandomGenerator::transform(bool rotation,
	const repo::lib::RepoRange& translate,
	const repo::lib::RepoRange& scale)
{
	repo::lib::RepoMatrix m;

	if (rotation) {
		m = m *
			repo::lib::RepoMatrix::rotationX(angle()) *
			repo::lib::RepoMatrix::rotationY(angle()) *
			repo::lib::RepoMatrix::rotationZ(angle());
	}

	if (scale.length()) {
		m = m * repo::lib::RepoMatrix::scale(vector(scale));
	}

	if (translate.length()) {
		m = m * repo::lib::RepoMatrix::translate(vector(translate));
	}

	return m;
}