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


#include "repo_metadataVariantHelper.h"



using namespace repo::lib;

bool MetadataVariantHelper::TryConvert(aiMetadataEntry& assimpMetaEntry, MetadataVariant& v)
{
	// Dissect the entry object
	switch (assimpMetaEntry.mType)
	{
	case AI_BOOL:
	{
		v = *(static_cast<bool*>(assimpMetaEntry.mData));
		break;
	}
	case AI_INT32:
	{
		v = *(static_cast<int*>(assimpMetaEntry.mData));
		break;
	}
	case AI_UINT64:
	{
		uint64_t value = *(static_cast<uint64_t*>(assimpMetaEntry.mData));
		v = static_cast<long long>(value); // Potentially losing precision here, but mongo does not accept uint64_t
		break;
	}
	case AI_FLOAT:
	{
		float value = *(static_cast<float*>(assimpMetaEntry.mData));
		v = static_cast<double>(value); // Potentially losing precision here, but mongo does not accept float
		break;
	}
	case AI_DOUBLE:
	{
		v = *(static_cast<double*>(assimpMetaEntry.mData));
		break;
	}
	case AI_AIVECTOR3D:
	{
		aiVector3D* vector = (static_cast<aiVector3D*>(assimpMetaEntry.mData));
		RepoVector3D repoVector = { (float)vector->x, (float)vector->y, (float)vector->z };
		v = repoVector.toString(); // not the best way to store a vector, but this appears to be the way it is done at the moment.
		break;
	}
	default:
	{
		// The other cases (AI_AISTRING and FORCE_32BIT) need extra treatment.
		return false;
	}
	}

	return true;

}

bool repo::lib::MetadataVariantHelper::TryConvert(OdNwPropertyPtr& metaProperty, MetadataVariant& v)
{

	switch (metaProperty->getValueType())
	{
	case NwPropertyValueType::value_type_default: {
		return false;
	}
	case NwPropertyValueType::value_type_bool: {
		bool value;
		metaProperty->getValue(value);
		v = value;
		break;
	}
	case NwPropertyValueType::value_type_double: {
		double value;
		metaProperty->getValue(value);
		v = value;
		break;
	}
	case NwPropertyValueType::value_type_float: {
		float value;
		metaProperty->getValue(value);
		v = static_cast<double>(value); // Potentially losing precision here, but mongo does not accept float
		break;
	}
	case NwPropertyValueType::value_type_OdInt8: {
		OdInt8 value;
		metaProperty->getValue(value);
		v = static_cast<int>(value);
		break;
	}
	case NwPropertyValueType::value_type_OdUInt8: {
		OdUInt8 value;
		metaProperty->getValue(value);
		v = static_cast<int>(value);
		break;
	}
	case NwPropertyValueType::value_type_OdInt32: {
		OdInt32 value;
		metaProperty->getValue(value);
		v = static_cast<long long>(value);
		break;
	}
	case NwPropertyValueType::value_type_OdUInt32: {
		OdUInt32 value;
		metaProperty->getValue(value);
		v = static_cast<long long>(value);
		break;
	}
	case NwPropertyValueType::value_type_OdUInt64: {
		OdUInt64 value;
		metaProperty->getValue(value);
		v = static_cast<long long>(value); // Potentially losing precision here since mongo does not support unsigned long long
		break;
	}
	case NwPropertyValueType::value_type_OdString: {
		OdString value;
		metaProperty->getValue(value);
		v = repo::manipulator::modelconvertor::odaHelper::convertToStdString(value);
		break;
	}
	case NwPropertyValueType::value_type_OdStringArray: {
		OdStringArray value;
		metaProperty->getValue(value);
		std::string combined;
		for (auto str : value)
		{
			combined += repo::manipulator::modelconvertor::odaHelper::convertToStdString(str) + ";";
		}
		v = combined;
		break;
	}
	case NwPropertyValueType::value_type_OdGeVector3d: {
		OdGeVector3d value;
		metaProperty->getValue(value);
		std::string str = std::to_string(value.x) + ", " + std::to_string(value.y) + ", " + std::to_string(value.z);
		v = str;
		break;
	}
	case NwPropertyValueType::value_type_OdNwColor: {
		OdNwColor value;
		metaProperty->getValue(value);
		std::string str = std::to_string(value.R()) + ", " + std::to_string(value.G()) + ", " + std::to_string(value.B());
		v = str;
		break;
	}
	case NwPropertyValueType::value_type_tm: {
		tm value;
		metaProperty->getValue(value);
		v = value;
		break;
	}
	default: {
		// All other cases can not be handled by this converter.
		return false;
	}
	}

	return true;
}

bool repo::lib::MetadataVariantHelper::TryConvert(OdTfVariant& metaEntry, OdBmLabelUtilsPEPtr labelUtils, OdBmParamDefPtr paramDef, OdBmDatabase* database, OdBm::BuiltInParameter::Enum param, MetadataVariant& v)
{
	switch (metaEntry.type()) {
	case OdVariant::kVoid: {
		return false;
	}
	case OdVariant::kString: {
		v = repo::manipulator::modelconvertor::odaHelper::convertToStdString(metaEntry.getString());
		break;
	}
	case OdVariant::kBool:
	{
		v = metaEntry.getBool();
		break;
	}
	case OdVariant::kInt8: {
		OdInt8 value = metaEntry.getInt8();
		v = static_cast<int>(value);
		break;
	}
	case OdVariant::kInt16: {
		OdInt16 value = metaEntry.getInt16();
		v = static_cast<int>(value);
		break;
	}
	case OdVariant::kInt32: {
		if (paramDef->getParameterTypeId() == OdBmSpecTypeId::Boolean::kYesNo)
		{
			(metaEntry.getInt32()) ? v = true : v = false;
		}
		else
		{
			OdInt32 value = metaEntry.getInt32();
			v = static_cast<long long>(value);
		}
		break;
	}
	case OdVariant::kInt64: {
		OdInt64 value = metaEntry.getInt64();
		v = static_cast<long long>(value);
		break;
	}
	case OdVariant::kDouble: {
		OdDouble value = metaEntry.getDouble();
		v = static_cast<double>(value);
		break;
	}
	break;
	case OdVariant::kAnsiString: {
		v = std::string(metaEntry.getAnsiString().c_str());
		break;
	}
	case OdTfVariant::kDbStubPtr: {
		// A stub is effectively a pointer to another database, or built-in, object. We don't recurse these objects, but will try to extract
		// their names or identities where possible (e.g. if a key pointed to a wall object, we would get the name of the wall object).
		
		OdDbStub* stub = metaEntry.getDbStubPtr();
		OdBmObjectId bmId = OdBmObjectId(stub);
		if (stub)
		{
			OdDbHandle hdl = bmId.getHandle();
			if (param == OdBm::BuiltInParameter::ELEM_CATEGORY_PARAM || param == OdBm::BuiltInParameter::ELEM_CATEGORY_PARAM_MT)
			{
				if (OdBmObjectId::isRegularHandle(hdl)) // A regular handle points to a database entry; if it is not regular, it is built-in.
				{
					v = std::to_string((OdUInt64)hdl);
				}
				else
				{
					OdBm::BuiltInCategory::Enum builtInValue = static_cast<OdBm::BuiltInCategory::Enum>((OdUInt64)hdl);
					auto category = labelUtils->getLabelFor(OdBm::BuiltInCategory::Enum(builtInValue));
					if (!category.isEmpty()) {
						v = repo::manipulator::modelconvertor::odaHelper::convertToStdString(category);
					}
					else {
						v = repo::manipulator::modelconvertor::odaHelper::convertToStdString(OdBm::BuiltInCategory(builtInValue).toString());
					}
				}
			}
			else
			{
				OdBmObjectPtr bmPtr = bmId.openObject();
				if (!bmPtr.isNull())
				{
					// The object class is unknown - some superclasses we can handle explicitly.

					auto bmPtrClass = bmPtr->isA();
					if (!bmPtrClass->isKindOf(OdBmElement::desc()))
					{
						OdBmElementPtr elem = bmPtr;
						if (elem->getElementName() == OdString::kEmpty) {
							v = std::to_string((OdUInt64)bmId.getHandle());
						}
						else {
							v = repo::manipulator::modelconvertor::odaHelper::convertToStdString(elem->getElementName());
						}
					}
					else
					{
						repoError << "Unsupported metadata value type (class) " << repo::manipulator::modelconvertor::odaHelper::convertToStdString(bmPtrClass->name()) << " currently only OdBmElement's are supported.";
					}
				}
			}
		}
	}
	default:
		return false;
	}

	return true;
}
