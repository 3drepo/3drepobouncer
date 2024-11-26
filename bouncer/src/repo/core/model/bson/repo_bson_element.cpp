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

#include "repo_bson_element.h"
#include "../../../lib/datastructure/repo_variant.h"
#include "../../../lib/repo_exception.h"

#include <mongo/bson/bson.h>
#include <repo/core/model/bson/repo_bson.h>

using namespace repo::core::model;

RepoBSONElement::~RepoBSONElement()
{
}

ElementType RepoBSONElement::type() const
{
	ElementType elementType;
	switch (mongo::BSONElement::type())
	{
	case mongo::Array: // array
		elementType = ElementType::ARRAY;
		break;
	case mongo::BinData: // binary data
		if (mongo::bdtUUID == binDataType() || mongo::newUUID == binDataType())
		{
			elementType = ElementType::UUID;
		}
		else
		{
			elementType = ElementType::BINARY;
		}
		break;
	case mongo::Bool:
		elementType = ElementType::BOOL;
		break;
	case mongo::Date:
		elementType = ElementType::DATE;
		break;
	case mongo::jstOID: // ObjectId
		elementType = ElementType::OBJECTID;
		break;
	case mongo::NumberDouble: // double precision floating point value
		elementType = ElementType::DOUBLE;
		break;
	case mongo::NumberInt: // 32 bit signed integer
		elementType = ElementType::INT;
		break;
	case mongo::NumberLong: // 64 bit signed integer
		elementType = ElementType::LONG;
		break;
	case mongo::Object: // embedded object
		elementType = ElementType::OBJECT;
		break;
	case mongo::String: // character string, stored in utf8
		elementType = ElementType::STRING;
		break;
		//case JSTypeMax : /** max type that is not MaxKey */
		//case MaxKey : // larger than all other types
		//case Timestamp : /** Updated to a Date with value next OpTime on insert */
		//case Undefined : // Undefined type
		//case jstNULL : // null type
		//case RegEx : // regular expression, a pattern with options
		//case DBRef : // deprecated / will be redesigned
		//case Code : // deprecated / use CodeWScope
		//case Symbol : // a programming language (e.g., Python) symbol
		//case CodeWScope : // javascript code that can execute on the database server, with SavedContext
	default:
		elementType = ElementType::UNKNOWN;
		break;
	}

	return elementType;
}

repo::lib::RepoVariant RepoBSONElement::repoVariant() const
{
	repo::lib::RepoVariant v;
	switch (type())
	{
	case ElementType::BOOL:
		v = Bool();
		break;
	case ElementType::DATE:
		{
		v = Tm();
		}
		break;
	case ElementType::INT:
		v = Int();
		break;
	case ElementType::LONG:
		v = Long();
		break;
	case ElementType::DOUBLE:
		v = Double();
		break;
	case ElementType::STRING:
		v = String();
		break;
	default:
		throw repo::lib::RepoException("Cannot convert BSONElement to Variant because Variant will not accept the type.");
	}
	return v;
}

std::string RepoBSONElement::String() const
{
	return mongo::BSONElement::String();
}

time_t RepoBSONElement::TimeT() const
{
	return mongo::BSONElement::Date().toTimeT();
}

tm RepoBSONElement::Tm() const
{
	tm buf;
	mongo::BSONElement::Date().toTm(&buf);
	return buf;
}

bool RepoBSONElement::Bool() const
{
	return mongo::BSONElement::Bool();
}

int RepoBSONElement::Int() const
{
	return mongo::BSONElement::Int();
}

long long RepoBSONElement::Long() const
{
	return mongo::BSONElement::Long();
}

double RepoBSONElement::Double() const
{
	return mongo::BSONElement::Double();
}

size_t RepoBSONElement::size() const
{
	return mongo::BSONElement::size();
}

const char* RepoBSONElement::binData(int& length) const
{
	return mongo::BSONElement::binData(length);
}

bool RepoBSONElement::operator==(const RepoBSONElement& other) const
{
	return mongo::BSONElement::operator==(other);
}

bool RepoBSONElement::operator!=(const RepoBSONElement& other) const
{
	return mongo::BSONElement::operator!=(other);
}

std::string RepoBSONElement::toString() const
{
	return mongo::BSONElement::toString();
}

bool RepoBSONElement::eoo() const
{
	return mongo::BSONElement::eoo();
}

bool RepoBSONElement::isNull() const
{
	return mongo::BSONElement::isNull();
}

RepoBSON RepoBSONElement::Object() const
{
	return RepoBSON(mongo::BSONElement::embeddedObject(), {});
}

RepoBSONElement::operator const std::string& () const
{
	return toString();
}

std::vector<RepoBSONElement> RepoBSONElement::Array()
{
	//FIXME: potentially slow.
	//This is done so we can hide mongo representation from the bouncer world.
	std::vector<RepoBSONElement> arr;

	if (!eoo())
	{
		std::vector<mongo::BSONElement> mongoArr = mongo::BSONElement::Array();
		arr.reserve(mongoArr.size());

		for (auto const& ele : mongoArr)
		{
			arr.push_back(RepoBSONElement(ele));
		}
	}

	return arr;
}