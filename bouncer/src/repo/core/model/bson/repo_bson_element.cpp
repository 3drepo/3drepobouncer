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
#include "repo/lib/datastructure/repo_variant.h"
#include "repo/lib/repo_exception.h"
#include "repo/core/model/bson/repo_bson.h"

#include <bsoncxx/document/element.hpp>
#include <bsoncxx/document/view.hpp>
#include <bsoncxx/array/element.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/types/bson_value/value.hpp>
#include <bsoncxx/exception/exception.hpp>

using namespace repo::core::model;
using namespace bsoncxx::document;

RepoBSONElement::~RepoBSONElement()
{
}

ElementType RepoBSONElement::type() const
{
	ElementType elementType;
	switch (element::type())
	{
	case bsoncxx::type::k_array: // array
		elementType = ElementType::ARRAY;
		break;
	case bsoncxx::type::k_binary: // binary data
		{
			const auto& b = element::get_binary();
			switch (b.sub_type)
			{
			case bsoncxx::binary_sub_type::k_uuid:
			case bsoncxx::binary_sub_type::k_uuid_deprecated:
				elementType = ElementType::UUID;
				break;
			default:
				elementType = ElementType::BINARY;
				break;
			}
		}
		break;
	case bsoncxx::type::k_bool:
		elementType = ElementType::BOOL;
		break;
	case bsoncxx::type::k_date:
		elementType = ElementType::DATE;
		break;
	case bsoncxx::type::k_oid: // ObjectId
		elementType = ElementType::OBJECTID;
		break;
	case bsoncxx::type::k_double: // double precision floating point value
		elementType = ElementType::DOUBLE;
		break;
	case bsoncxx::type::k_int32: // 32 bit signed integer
		elementType = ElementType::INT;
		break;
	case bsoncxx::type::k_int64: // 64 bit signed integer
		elementType = ElementType::LONG;
		break;
	case bsoncxx::type::k_document: // embedded object
		elementType = ElementType::OBJECT;
		break;
	case bsoncxx::type::k_string: // character string, stored in utf8
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
	case ElementType::UUID:
		v = UUID();
		break;
	default:
		throw repo::lib::RepoException("Cannot convert BSONElement to Variant because Variant will not accept the type.");
	}
	return v;
}

std::string RepoBSONElement::String() const
{
	const auto& b = element::get_string();
	return std::string(b.value.data(), b.value.size());
}

time_t RepoBSONElement::TimeT() const
{
	// By convention, this API considers time_t as seconds since the unix epoch
	return std::chrono::duration_cast<std::chrono::seconds>(element::get_date().value).count();
}

tm RepoBSONElement::Tm() const
{
	time_t time = TimeT();
	tm buf = *gmtime(&time);
	return buf;
}

bool RepoBSONElement::Bool() const
{
	return element::get_bool();
}

int RepoBSONElement::Int() const
{
	return element::get_int32();
}

int64_t RepoBSONElement::Long() const
{
	return element::get_int64();
}

double RepoBSONElement::Double() const
{
	return element::get_double();
}

repo::lib::RepoUUID RepoBSONElement::UUID() const
{
	const auto& b = element::get_binary();
	if (b.sub_type == bsoncxx::binary_sub_type::k_uuid || b.sub_type == bsoncxx::binary_sub_type::k_uuid_deprecated)
	{
		boost::uuids::uuid id;
		memcpy(id.data, b.bytes, b.size);
		return repo::lib::RepoUUID(id);
	}
	throw repo::lib::RepoFieldTypeException("Cannot convert UUID because the binary has the wrong subtype");
}

size_t RepoBSONElement::size() const
{
	return element::length();
}

bool RepoBSONElement::operator==(const RepoBSONElement& other) const
{
	return element::get_value() == other.get_value();
}

bool RepoBSONElement::operator!=(const RepoBSONElement& other) const
{
	return element::get_value() != other.get_value();
}

RepoBSON RepoBSONElement::Object() const
{
	return RepoBSON(get_document().value, {});
}