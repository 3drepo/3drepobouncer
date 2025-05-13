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
#include "repo/lib/datastructure/repo_variant_utils.h"
#include "repo/lib/datastructure/repo_bounds.h"

/* As we sometimes deal with very large coordinate systems, we expect positions
 * to have a low tolerance, approaching floating point quantisation itself.
 * Directions are usually normalised, which introduces a lot more quantisation
 * error but is far less noticable, so the tolerance is higher.
 */

#define POSITION_TOLERANCE 0.0000001f
#define DIRECTION_TOLERANCE 0.00001f

namespace testing {

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

	// Is within a second or so of the current time.
	MATCHER(IsNow, "")
	{
		return ((time_t)arg > std::time(0) - (time_t)1) && ((time_t)arg < std::time(0) + (time_t)1);
	}

	//Verifies that two arrays have at least one element in common
	MATCHER_P(AnyElementsOverlap, a, "")
	{
		for (auto& r : arg)
		{
			if (std::find(a.begin(), a.end(), r) != a.end()) {
				return true;
			}
		}
		return false;
	}

	//Runs the specified matcher on the result of the getFieldNames() method
	MATCHER_P(GetFieldNames, matcher, "")
	{
		return ExplainMatchResult(matcher, arg.getFieldNames(), result_listener);
	}

	//Checks the value of the hasOversizeFiles() method
	MATCHER(HasBinaryFields, "")
	{
		return arg.hasOversizeFiles();
	}

	// Allows using the string value of a variant directly in an expects statement
	MATCHER_P(Vs, s, "")
	{
		auto asString = boost::apply_visitor(repo::lib::StringConversionVisitor(), arg);
		return asString == s;
	}

	MATCHER_P(Vq, s, "")
	{
		return boost::apply_visitor(repo::lib::DuplicationVisitor(), arg, repo::lib::RepoVariant(s));
	}

	static bool compareBounds(const repo::lib::RepoBounds& a, const repo::lib::RepoBounds& b, double tolerance)
	{
		return
			abs(a.min().x - b.min().x) < tolerance &&
			abs(a.min().y - b.min().y) < tolerance &&
			abs(a.min().z - b.min().z) < tolerance &&
			abs(a.max().x - b.max().x) < tolerance &&
			abs(a.max().y - b.max().y) < tolerance &&
			abs(a.max().z - b.max().z) < tolerance;
	}

	MATCHER_P2(BoundsAre, bounds, tolerance, "")
	{
		return compareBounds(arg, bounds, tolerance);
	}

	MATCHER_P2(UnorderedBoundsAre, elements, tolerance, "")
	{
		std::vector<repo::lib::RepoBounds> bounds(elements);

		for (auto& r : arg) {
			for (auto i = 0; i < bounds.size(); i++) {
				if (compareBounds(r, bounds[i], tolerance)) {
					bounds.erase(bounds.begin() + i);
					break;
				}
			}
		}

		return !bounds.size(); // If every element has been matched, they should by now have all been removed.
	}
}

namespace repo {
	namespace core {
		namespace model {
			// This method takes precedence over all the other gtest Printers when printing
			// RepoBSON. This is required as otherwise gtest will try and use begin/end,
			// which are privately inherited and so inaccessible.
			void PrintTo(const class RepoBSON& point, std::ostream* os);
		}
	}
}

// These operators allow gmock to work with tm structs as if they are typical
// primitive types. These should not be defined within any namespace.

bool operator== (tm a, tm b);

void operator<< (std::basic_ostream<char, std::char_traits<char>>& out, tm a);