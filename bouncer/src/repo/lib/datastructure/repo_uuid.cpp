/**
*  Copyright (C) 2016 3D Repo Ltd
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
* A (attempt to be) thread safe stack essentially that would push/pop items as required
* Incredibly weird, I know.
*/

#include "repo_uuid.h"
using namespace repo::lib;

#include <boost/functional/hash.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

const std::string defaultValue = "00000000-0000-0000-0000-000000000000";

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
static boost::uuids::uuid stringToUUID(
	const std::string &text,
	const std::string &suffix = std::string())
{
	boost::uuids::uuid uuid;
	if (text.empty())
		return stringToUUID(defaultValue);
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

size_t RepoUUIDHasher::operator()(const RepoUUID& uid) const
{
	return boost::hash<boost::uuids::uuid>()(uid);
}

RepoUUID::RepoUUID(const std::string &stringRep)
	: id(stringToUUID(stringRep))
{
}

RepoUUID createUUID()
{
	static boost::uuids::random_generator gen;
	return RepoUUID(gen());
}

std::string RepoUUID::toString() const
{
	return boost::lexical_cast<std::string>(id);
}

std::ostream& operator<<(std::ostream& stream, const RepoUUID& uuid)
{
	stream << uuid.toString();
}