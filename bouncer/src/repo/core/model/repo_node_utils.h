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






/**
* Matrix x vector multiplication
* NOTE: this assumes matrix has row as fast dimension!
* This takes a 4x4 matrix but only use the 3x3 part.
* @param mat 4x4 matrix
* @param vec vector
* @return returns the resulting vector.
*/
static repo::lib::RepoVector3D multiplyMatVecFake3x3(const repo::lib::RepoMatrix &matrix, const repo::lib::RepoVector3D &vec)
{
	repo::lib::RepoVector3D result;
	
	auto data =  matrix.getData();
	data[3] = data[7] = data[11] = 0;

	repo::lib::RepoMatrix multMat(data);
	

	return multMat * result;
}



static bool nameCheck(const char &c)
{
	return c == ' ' || c == '$' || c == '.';
}

static bool dbNameCheck(const char &c)
{
	return c == '/' || c == '\\' || c == '.' || c == ' '
		|| c == '\"' || c == '$' || c == '*' || c == '<'
		|| c == '>' || c == ':' || c == '?' || c == '|';
}

static bool extNameCheck(const char &c)
{
	return c == ' ' || c == '$';
}

static std::string sanitizeExt(const std::string& name)
{
	// http://docs.mongodb.org/manual/reference/limits/#Restriction-on-Collection-Names
	std::string newName(name);
	std::replace_if(newName.begin(), newName.end(), extNameCheck, '_');

	return newName;
}

static std::string sanitizeName(const std::string& name)
{
	// http://docs.mongodb.org/manual/reference/limits/#Restriction-on-Collection-Names
	std::string newName(name);
	std::replace_if(newName.begin(), newName.end(), nameCheck, '_');

	return newName;
}

static std::string sanitizeDatabaseName(const std::string& name)
{
	// http://docs.mongodb.org/manual/reference/limits/#naming-restrictions

	// Cannot contain any of /\. "$*<>:|?
	std::string newName(name);
	std::replace_if(newName.begin(), newName.end(), dbNameCheck, '_');

	return newName;
}
