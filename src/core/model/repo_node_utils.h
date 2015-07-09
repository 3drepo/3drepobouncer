/**
*  Copyright (C) 2014 3D Repo Ltd
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
#include <boost/functional/hash.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp> 
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>


#include <sstream>

//abstract out the use of boost inside the node codes 
//incase we want to change it in the future
typedef boost::uuids::uuid repo_uuid;

static repo_uuid generateUUID(){
	return  boost::uuids::random_generator()();
}


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
static repo_uuid stringToUUID(
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

static std::string UUIDtoString(const repo_uuid &id)
{
	return boost::lexical_cast<std::string>(id);
}