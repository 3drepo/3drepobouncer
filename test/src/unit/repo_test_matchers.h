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
#include "repo/lib/datastructure/repo_line.h"
#include "repo/lib/datastructure/repo_triangle.h"

/* As we sometimes deal with very large coordinate systems, we expect positions
 * to have a low tolerance, approaching floating point quantisation itself.
 * Directions are usually normalised, which introduces a lot more quantisation
 * error but is far less noticable, so the tolerance is higher.
 */

#define POSITION_TOLERANCE 0.0000001
#define DIRECTION_TOLERANCE 0.00001

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

	MATCHER_P2(VectorNear, a, tolerance, "")
	{
		if (!::testing::ExplainMatchResult(::testing::NanSensitiveDoubleNear((double)arg.x, tolerance), (double)a.x, result_listener))
		{
			*result_listener << " in the z axis.";
			return false;
		}
		if (!::testing::ExplainMatchResult(::testing::NanSensitiveDoubleNear((double)arg.y, tolerance), (double)a.y, result_listener))
		{
			*result_listener << " in the y axis.";
			return false;
		}
		if (!::testing::ExplainMatchResult(::testing::NanSensitiveDoubleNear((double)arg.z, tolerance), (double)a.z, result_listener))
		{
			*result_listener << " in the z axis.";
			return false;
		}
		return true;
	}

	MATCHER_P(VectorNear, a, "")
	{
		return ::testing::ExplainMatchResult(VectorNear(a, POSITION_TOLERANCE), arg, result_listener);
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

	static bool compareBounds(const repo::lib::RepoBounds& a, const repo::lib::RepoBounds& b, double tolerance, testing::MatchResultListener* listener)
	{
		if (abs(a.min().x - b.min().x) > tolerance) { *listener << "Bounds Min x exceeds tolerance."; return false; }
		if (abs(a.min().y - b.min().y) > tolerance) { *listener << "Bounds Min y exceeds tolerance."; return false; }
		if (abs(a.min().z - b.min().z) > tolerance) { *listener << "Bounds Min z exceeds tolerance."; return false; }
		if (abs(a.max().x - b.max().x) > tolerance) { *listener << "Bounds Max x exceeds tolerance."; return false; }
		if (abs(a.max().y - b.max().y) > tolerance) { *listener << "Bounds Max y exceeds tolerance."; return false; }
		if (abs(a.max().z - b.max().z) > tolerance) { *listener << "Bounds Max z exceeds tolerance."; return false; }
		return true;
	}

	MATCHER_P2(BoundsAre, bounds, tolerance, "")
	{
		return compareBounds(arg, bounds, tolerance, result_listener);
	}

	MATCHER_P2(UnorderedBoundsAre, elements, tolerance, "")
	{
		std::vector<repo::lib::RepoBounds> bounds(elements);

		for (auto& r : arg) {
			for (auto i = 0; i < bounds.size(); i++) {
				if (compareBounds(r, bounds[i], tolerance, result_listener)) {
					bounds.erase(bounds.begin() + i);
					break;
				}
			}
		}

		return !bounds.size(); // If every element has been matched, they should by now have all been removed.
	}

	static bool compareVectors(const repo::lib::RepoVector3D64& a, const repo::lib::RepoVector3D64& b, double tolerance, testing::MatchResultListener* listener)
	{
		if (abs(a.x - b.x) > tolerance) { *listener << "Vector x exceeds tolerance."; return false; }
		if (abs(a.y - b.y) > tolerance) { *listener << "Vector y exceeds tolerance."; return false; }
		if (abs(a.z - b.z) > tolerance) { *listener << "Vector z exceeds tolerance."; return false; }
		return true;
	}

	MATCHER_P2(UnorderedVectorsAre, elements, tolerance, "")
	{
		std::vector<repo::lib::RepoVector3D64> vectors(elements);
		for (auto& r : arg) {
			for (auto i = 0; i < vectors.size(); i++) {
				if (compareVectors(r, vectors[i], tolerance, result_listener)) {
					vectors.erase(vectors.begin() + i);
					break;
				}
			}
		}
		return !vectors.size(); // If every element has been matched, they should by now have all been removed.
	}

	namespace vectors {

		// The vectors namespace contains a set of types to manage comparisons between
		// RepoVectors, including types that are composed of RepoVectors, such as
		// Containers, but also Lines and Triangles.

		// GMOCK matchers are templated, however they do have constraints; for example,
		// the EXPECT statement does not support initialiser lists as its first
		// argument. The types below work together, to transform input arguments for
		// both the macro and matcher itself, so that EXPECT can be used cleanly, and
		// inside the common matcher to implement specialised behaviour where necessary.

		// The actual matcher - this macro creates a templated class. The overloads below
		// should return an instance of this type.

		MATCHER_P2(RepoVectorsGeneric, elements, tolerance, "")
		{
			// elements is always expected to be an iterable type, even if it has
			// only one elment.
			// arg may be one of a number of high level types, which we convert
			// using one of the above overloads.

			size_t matched = 0;
			size_t expected = 0;
			for (auto& a : arg) {
				expected++;
				for(auto& b : elements) {
					if (compareVectors(a, b, tolerance, result_listener)) {
						matched++;
						break;
					}
				}

				if (matched < expected) {
					*result_listener << " No match for element " << a.toString() << ".";
				}
			}

			if (matched != expected) {
				*result_listener << " Only " << matched << " of " << expected << " elements matched.";
				return false;
			}

			return true;
		}

		// The following overloads perform type-specific transforms of the arguments
		// into a form the vector matcher can deal with. These must all return a suitable
		// matcher.

		static RepoVectorsGenericMatcherP2<
			std::initializer_list<repo::lib::RepoVector3D64>,
			double
		> AreSubsetOf(
			std::initializer_list<repo::lib::RepoVector3D64> elements,
			double tolerance = POSITION_TOLERANCE)
		{
			return RepoVectorsGeneric(elements, tolerance);
		}

		// Helper object to present different types as iterators of vectors, where
		// necessary. This is used to pass high level types to the EXPECT macro,
		// which is constrained in what it can accept, as well as make it clear
		// that we want to iterate over the vectors within the type.

		template<typename T>
		struct Iterator
		{
			Iterator(const repo::lib::_RepoTriangle<T>& triangle)
			{
				_begin = &triangle.a;
				_end = &triangle.c + 1;
			}

			Iterator(const repo::lib::_RepoLine<T>& line)
			{
				_begin = &line.start;
				_end = &line.end + 1;
			}

			const repo::lib::_RepoVector3D<T>* begin() const {
				return _begin;
			}

			const repo::lib::_RepoVector3D<T>* end() const {
				return _end;
			}

		private:
			const repo::lib::_RepoVector3D<T>* _begin;
			const repo::lib::_RepoVector3D<T>* _end;
		};
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