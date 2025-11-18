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

#include "repo/lib/datastructure/repo_uuid.h"

namespace repo {
	namespace manipulator {
		namespace modelutility {
			namespace clash {

				// The clash engine will return composite object a on the left, and b on the
				// right, always. Though it is good practice when matching the result
				// downstream to ignore the order, in case the user swaps the sets between
				// runs.

				struct OrderedPair
				{
					repo::lib::RepoUUID a;
					repo::lib::RepoUUID b;

					OrderedPair(const repo::lib::RepoUUID& a, const repo::lib::RepoUUID& b)
						:a(a), b(b)
					{
					}

					size_t getHash() const
					{
						return a.getHash() ^ b.getHash();
					}

					bool operator == (const OrderedPair& other) const
					{
						return (a == other.a && b == other.b);
					}
				};

				struct OrderedPairHasher
				{
					std::size_t operator()(const OrderedPair& p) const
					{
						return p.getHash();
					}
				};
			}
		}
	}
}