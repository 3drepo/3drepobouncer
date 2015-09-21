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

#include <boost/functional/hash.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp> 
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>


#include <sstream>

//abstract out the use of boost inside the node codes 
//incase we want to change it in the future
typedef boost::uuids::uuid repoUUID;

typedef struct{
	std::vector<float> ambient;
	std::vector<float> diffuse;
	std::vector<float> specular;
	std::vector<float> emissive;
	float opacity;
	float shininess;
	float shininessStrength;
	bool isWireframe;
	bool isTwoSided;
}repo_material_t;


typedef struct{
	float r;
	float g;
	float b;
	float a;

}repo_color4d_t;

typedef struct{
	float x;
	float y;
	float z;

}repo_vector_t;

typedef struct{
	float x;
	float y;

}repo_vector2d_t;

typedef struct{
	uint32_t numIndices;
	uint32_t *indices;
}repo_face_t;


//This is used to map info for multipart optimization
typedef struct{
	repo_vector_t min;
	repo_vector_t max;
	repoUUID      mesh_id;
	repoUUID      material_id;
	int32_t       vertFrom;
	int32_t       vertTo;
	int32_t       triFrom;
	int32_t       triTo;
}repo_mesh_mapping_t;

static repoUUID generateUUID(){
	return  boost::uuids::random_generator()();
}

//FIXME: scope this

/*!
* Returns a valid uuid representation of a given string. If empty, returns
* a randomly generated uuid. If the string is not a uuid representation,
* the string is hashed and appended with given suffix to prevent
* uuid clashes in cases where two objects such as a mesh and a
* transformation share the same name.
*
* \param text Can be any string including a valid UUID representation
*             without '{' and '}'.
* \param suffix Numerical suffix to prevent name clashes, eg "01".
* \return valid uuid
*/
static repoUUID stringToUUID(
	const std::string &text,
	const std::string &suffix = std::string())
{
	boost::uuids::uuid uuid;
	if (text.empty())
		uuid = generateUUID();
	else
	{
		try
		{
			boost::uuids::string_generator gen;
			if (text.substr(0, 1) != "{")
				uuid = gen("{" + text + "}");
			else
				uuid = gen(text);
		}
		catch (std::runtime_error e)
		{
			// uniformly distributed hash
			boost::hash<std::string> string_hash;
			std::string hashedUUID;
			std::stringstream str;
			str << string_hash(text);
			str >> hashedUUID;

			// uuid: 8 + 4 + 4 + 4 + 12 = 32
			// pad with zero, leave last places empty for suffix
			while (hashedUUID.size() < 32 - suffix.size())
				hashedUUID.append("0");
			hashedUUID.append(suffix);
			uuid = stringToUUID(hashedUUID, suffix);
		}
	}
	return uuid;
}

/**
* Converts a RepoUUID to string
* @param id repoUUID to convert
* @return a string representation of repoUUID
*/
static std::string UUIDtoString(const repoUUID &id)
{
	return boost::lexical_cast<std::string>(id);
}

static std::string toString(const repo_face_t &f)
{
	std::string str;
	unsigned int mNumIndices = f.numIndices;

	str += "[";
	for (unsigned int i = 0; i < f.numIndices; i++)
	{
		str += std::to_string(f.indices[i]);
		if (i != mNumIndices - 1)
			str += ", ";
	}
	str += "]";
	return str;
}

static std::string toString(const repo_color4d_t &color)
{
	std::stringstream sstr;
	sstr << "[" << color.r << ", " << color.g << ", " << color.b << ", " << color.a << "]";
	return sstr.str();
}

static std::string toString(const repo_vector_t &vec)
{
	std::stringstream sstr;
	sstr << "[" << vec.x << ", " << vec.y << ", " << vec.z << "]";
	return sstr.str();
}

static std::string toString(const repo_vector2d_t &vec)
{
	std::stringstream sstr;
	sstr << "[" << vec.x << ", " << vec.y << "]";
	return sstr.str();
}

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

static float dotProduct(const repo_vector_t a, const repo_vector_t b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

static repo_vector_t crossProduct(const repo_vector_t a, const repo_vector_t b)
{
	repo_vector_t product;

	product.x = (a.y * b.z) - (a.z * b.y);
	product.y = (a.z * b.x) - (a.x * b.z);
	product.z = (a.x * b.y) - (a.y * b.x);

	return product;
}

static void normalize(repo_vector_t &a)
{
	float length = std::sqrt(a.x*a.x + a.y*a.y + a.z*a.z);

	a.x /= length;
	a.y /= length;
	a.z /= length;
}

static bool nameCheck(const char &c)
{
	return c == ' ' || c == '$';
}

static bool dbNameCheck(const char &c)
{
	return c == '/' || c == '\\' || c == '.' || c == ' '
		|| c == '\"' || c == '$' || c == '*' || c == '<'
		|| c == '>"' || c == ':' || c == '?';
}


static std::string sanitizeName(const std::string& name)
{
	// http://docs.mongodb.org/manual/reference/limits/#Restriction-on-Collection-Names
	std::string newName(name);
	std::replace_if(newName.begin(), newName.end(), nameCheck, '_');
	auto strPos = newName.find("system.");
	if ( strPos != std::string::npos)
	{
		newName.replace(strPos, sizeof("system."), "");
	}
	return newName;
}

static std::string sanitizeDatabaseName(const std::string& name)
{
	// http://docs.mongodb.org/manual/reference/limits/#naming-restrictions

	// Cannot contain any of /\. "$*<>:|?
	std::string newName(name);
	std::replace_if(newName.begin(), newName.end(), dbNameCheck, '_');
	auto strPos = newName.find("system.");
	if (strPos != std::string::npos)
	{
		newName.replace(strPos, sizeof("system."), "");
	}
	return newName;
}
