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


#include <string>
#include "boost/variant.hpp"
#include <boost/variant/static_visitor.hpp>
#include "repo_variant.h"
#include <ctime>
#include <iostream>


namespace repo {
	namespace lib {
		class DuplicationVisitor : public boost::static_visitor<bool> {
		public:

			template <typename T1, typename T2>
			bool operator()(const T1&, const T2&) const {
				return false; // Different types so it can't be equal
			}

			template <typename T>
			bool operator()(const T& lhs, const T& rhs) const {
				return lhs == rhs;
			}

			// Special case for tm, because it does not have an == operator
			bool operator()(const tm lhs, const tm rhs) const {
				tm lhsCpy = lhs; // Copy because mktime can alter the struct
				tm rhsCpy = rhs; // Copy because mktime can alter the struct

				time_t lhsTime = mktime(&lhsCpy);
				time_t rhsTime = mktime(&rhsCpy);

				return lhsTime == rhsTime;
			}
		};

		class StringConversionVisitor : public boost::static_visitor<std::string> {
		public:

			std::string operator()(const bool& b) const {
				return std::to_string(b);
			}

			std::string operator()(const int& i) const {
				return std::to_string(i);
			}

			std::string operator()(const int64_t& ll) const {
				return std::to_string(ll);
			}

			std::string operator()(const double& d) const {
				return std::to_string(d);
			}

			std::string operator()(const std::string& s) const {
				return s;
			}

			std::string operator()(const tm& t) const {
				char buffer[80];

				strftime(buffer, sizeof(buffer), "%d-%m-%Y %H-%M-%S", &t);
				return std::string(buffer);
			}

			std::string operator()(const repo::lib::RepoUUID& u) const {
				return u.toString();
			}
		};
	}
}