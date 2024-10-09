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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>

/* As we sometimes deal with very large coordinate systems, we expect positions
 * to have a low tolerance, approaching floating point quantisation itself. 
 * Directions are usually normalised, which introduces a lot more quantisation
 * error but is far less noticable, so the tolerance is higher. 
 */

#define POSITION_TOLERANCE 0.0000001f
#define DIRECTION_TOLERANCE 0.00001f

MATCHER(DirectionsNear, "")
{
	auto dx = std::get<0>(arg).x - std::get<1>(arg).x;
	auto dy = std::get<0>(arg).y - std::get<1>(arg).y;
	auto dz = std::get<0>(arg).z - std::get<1>(arg).z;
	return std::abs(dx) < DIRECTION_TOLERANCE && std::abs(dy) < DIRECTION_TOLERANCE && std::abs(dz) < DIRECTION_TOLERANCE;
}

MATCHER(PositionsNear, "")
{
	auto dx = std::get<0>(arg).x - std::get<1>(arg).x;
	auto dy = std::get<0>(arg).y - std::get<1>(arg).y;
	auto dz = std::get<0>(arg).z - std::get<1>(arg).z;
	return std::abs(dx) < POSITION_TOLERANCE && std::abs(dy) < POSITION_TOLERANCE && std::abs(dz) < POSITION_TOLERANCE;
}

/*
* Compares two RepoVector3Ds for semantic equivalence - that is, if they
* effectively represent the same point subject to rounding error. Use this
* matcher to compare vectors having undergone transformations such as
* rotations or scales.
*/
MATCHER_P(VectorNear, a, "")
{
	if (!::testing::ExplainMatchResult(::testing::FloatNear(arg.x, POSITION_TOLERANCE), a.x, result_listener))
	{
		*result_listener << " in the z axis.";
		return false;
	}
	if (!::testing::ExplainMatchResult(::testing::FloatNear(arg.y, POSITION_TOLERANCE), a.y, result_listener))
	{
		*result_listener << " in the y axis.";
		return false;
	}
	if (!::testing::ExplainMatchResult(::testing::FloatNear(arg.z, POSITION_TOLERANCE), a.z, result_listener))
	{
		*result_listener << " in the z axis.";
		return false;
	}
	return true;
}

MATCHER_P(VectorGe, a, "") 
{
	if (!::testing::ExplainMatchResult(::testing::Ge(a.x), arg.x, result_listener))
	{
		*result_listener << " lt in the x axis.";
		return false;
	}
	if (!::testing::ExplainMatchResult(::testing::Ge(a.y), arg.y, result_listener))
	{
		*result_listener << " lt in the y axis.";
		return false;
	}
	if (!::testing::ExplainMatchResult(::testing::Ge(a.z), arg.z, result_listener))
	{
		*result_listener << " lt in the z axis.";
		return false;
	}
	return true;
}

MATCHER_P(VectorLe, a, "")
{
	if (!::testing::ExplainMatchResult(::testing::Le(a.x), arg.x, result_listener))
	{
		*result_listener << " gt in the x axis.";
		return false;
	}
	if (!::testing::ExplainMatchResult(::testing::Le(a.y), arg.y, result_listener))
	{
		*result_listener << " gt in the y axis.";
			return false;
	}
	if (!::testing::ExplainMatchResult(::testing::Le(a.z), arg.z, result_listener))
	{
		*result_listener << " gt in the z axis.";
			return false;
	}
	return true;
}

MATCHER(IsNow, "")
{
	return ((time_t)arg > std::time(0) - (time_t)1000) && ((time_t)arg < std::time(0) + (time_t)1000);
}