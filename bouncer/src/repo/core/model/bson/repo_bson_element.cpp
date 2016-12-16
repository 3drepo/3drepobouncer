#include "repo_bson_element.h"

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