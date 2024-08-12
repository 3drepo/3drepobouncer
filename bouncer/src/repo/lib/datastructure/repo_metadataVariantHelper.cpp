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

bool repo::lib::MetadataVariantHelper::TryConvert(OdNwDataPropertyPtr& metaProperty, MetadataVariant& v)
{

	switch (metaProperty->getDataType())
	{
	case NwDataType::dt_NONE: {
		return false;
	}
	case NwDataType::dt_DOUBLE: {
		OdNwVariant odvar;
		metaProperty->getValue(odvar);
		v = odvar.getDouble();
		break;
	}
	case NwDataType::dt_INT32: {
		OdNwVariant odvar;
		metaProperty->getValue(odvar);
		v = static_cast<long long>(odvar.getInt32()); // Incoming is long, convert it to long long since int won't fit.
		break;
	}
	case NwDataType::dt_BOOL: {
		OdNwVariant odvar;
		metaProperty->getValue(odvar);
		v = odvar.getBool();
		break;
	}
	case NwDataType::dt_DISPLAY_STRING: {
		OdNwVariant odvar;
		metaProperty->getValue(odvar);
		OdString value = odvar.getString();
		v = repo::manipulator::modelconvertor::odaHelper::convertToStdString(value);
		break;
	}
	case NwDataType::dt_DATETIME: {
		OdNwVariant odvar;
		metaProperty->getValue(odvar);
		v = odvar.getTime();
		break;
	}
	case NwDataType::dt_DOUBLE_LENGTH: {
		OdNwVariant odvar;
		metaProperty->getValue(odvar);
		v = odvar.getMeasuredDouble();
		break;
	}
	case NwDataType::dt_DOUBLE_ANGLE: {
		OdNwVariant odvar;
		metaProperty->getValue(odvar);
		v = odvar.getMeasuredDouble();
		break;
	}
	case NwDataType::dt_NAME: {
		OdNwVariant odvar;
		metaProperty->getValue(odvar);
		OdRxObjectPtr ptr = odvar.getRxObjectPtr();
		OdNwNamePtr namePtr = static_cast<OdNwNamePtr>(ptr);
		OdString displayNameOd = namePtr->getDisplayName();
		std::string displayName = repo::manipulator::modelconvertor::odaHelper::convertToStdString(displayNameOd);
		v = displayName;
		break;
	}
	case NwDataType::dt_IDENTIFIER_STRING: {
		OdNwVariant odvar;
		metaProperty->getValue(odvar);
		OdString value = odvar.getString();
		v = repo::manipulator::modelconvertor::odaHelper::convertToStdString(value);
		break;
	}
	case NwDataType::dt_DOUBLE_AREA: {
		OdNwVariant odvar;
		metaProperty->getValue(odvar);
		v = odvar.getMeasuredDouble();
		break;
	}
	case NwDataType::dt_DOUBLE_VOLUME: {
		OdNwVariant odvar;
		metaProperty->getValue(odvar);
		v = odvar.getMeasuredDouble();
		break;
	}
	case NwDataType::dt_POINT2D: {
		OdNwVariant odvar;
		metaProperty->getValue(odvar);
		OdGePoint2d point = odvar.getPoint2d();
		std::string str = std::to_string(point.x) + ", " + std::to_string(point.y);
		v = str;
		break;
	}
	case NwDataType::dt_POINT3D: {
		OdNwVariant odvar;
		metaProperty->getValue(odvar);
		OdGePoint3d point = odvar.getPoint3d();
		std::string str = std::to_string(point.x) + ", " + std::to_string(point.y) + ", " + std::to_string(point.z);
		v = str;
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
