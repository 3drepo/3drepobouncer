/**
*  Copyright (C) 2015 3D Repo Ltd
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

/**
* Static utility functions for nodes
*/

#pragma once
#include <iostream>
#include <algorithm>

#include <sstream>

#include "../../lib/repo_log.h"
#include "../../lib/datastructure/repo_uuid.h"
#include "../../lib/datastructure/repo_vector.h"
#include "../../lib/datastructure/repo_matrix.h"

/**
* \brief Returns a compacted string representation of a given vector
* as [toString(0) ... toString(n)] where only the very first and the very
* last elements are displayed.
* \sa toString()
* @param vec vector to convert to string
* @return returns a string representing the vector
*/
template <class T>
static std::string vectorToString(const std::vector<T> &vec)
{
	{
		std::string str;
		if (vec.size() > 0)
		{
			str += "[" + toString(vec.at(0));
			if (vec.size() > 1)
				str += ", ..., " + toString(vec.at(vec.size() - 1));
			str += "]";
		}
		return str;
	}
}
