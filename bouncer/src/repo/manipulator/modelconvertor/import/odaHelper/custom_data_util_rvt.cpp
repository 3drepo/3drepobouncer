/**
*  Copyright (C) 2019 3D Repo Ltd
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
#include "custom_data_util_rvt.h"
#include <Database/Entities/BmParamElem.h>
#include <Database/Entities/BmParamValueAString.h>
#include <Database/Entities/BmParamValueSetAString.h>
#include <Database/Entities/BmParamValueInt.h>
#include <Database/Entities/BmParamValueSetInt.h>
#include <Database/Entities/BmParamValueDouble.h>
#include <Database/Entities/BmParamValueSetDouble.h>
#include "helper_functions.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

void CustomDataProcessorRVT::fillCustomMetadata(
	std::unordered_map<std::string, std::string>& metadata
) {
	fetchStringData(metadata);
	/*fetchIntData(metadata);
	fetchDoubleData(metadata);*/
}

void CustomDataProcessorRVT::fetchStringData(
	std::unordered_map<std::string, std::string>& dataStore
) {
	//In accordance to https://forum.opendesign.com/showthread.php?18030-How-to-access-the-custom-parameter-added-in-model-group-using-ODA-BimRv-API
	OdBmParamValueAStringPtrArray aStringParams;
	if (!element->getStringParams().isNull())
	{
		OdBmParamValueSetAStringPtr pStringParams = element->getStringParams();
		pStringParams->getParamSet(aStringParams);
		for (const auto &param : aStringParams) {
			OdBmObjectId paramID = param->getParamId();
			if ((OdInt64)paramID.getHandle() < 0) continue; //negative id is built in values
			auto value = convertToStdString(param->getValue());

			OdBmParamElemPtr paramElemPtr = paramID.safeOpenObject();
			OdBmParamDefPtr pDescParam = paramElemPtr->getParamDef();
			auto metaKey = convertToStdString(pDescParam->getCaption());
			dataStore[metaKey] = value;
		}
	}
}

void CustomDataProcessorRVT::fetchIntData(
	std::unordered_map<std::string, std::string>& dataStore
) {

	OdBmParamValueIntPtrArray aIntParams;
	if (!element->getStringParams().isNull())
	{
		OdBmParamValueSetIntPtr intParams = element->getIntParams();
		intParams->getParamSet(aIntParams);
		for (const auto &param : aIntParams) {
			OdBmObjectId paramID = param->getParamId();
			if ((OdInt64)paramID.getHandle() < 0) continue; //negative id is built in values
			auto value = std::to_string(param->getValue());

			OdBmParamElemPtr paramElemPtr = paramID.safeOpenObject();
			OdBmParamDefPtr pDescParam = paramElemPtr->getParamDef();
			auto metaKey = convertToStdString(pDescParam->getCaption());
			dataStore[metaKey] = value;
		}
	}
}

void CustomDataProcessorRVT::fetchDoubleData(
	std::unordered_map<std::string, std::string>& dataStore
) {

	OdBmParamValueDoublePtrArray aDoubleParams;
	if (!element->getStringParams().isNull())
	{
		OdBmParamValueSetDoublePtr doubleParams = element->getDoubleParams();
		doubleParams->getParamSet(aDoubleParams);
		for (const auto &param : aDoubleParams) {
			OdBmObjectId paramID = param->getParamId();
			if ((OdInt64)paramID.getHandle() < 0) continue; //negative id is built in values
			auto value = std::to_string(param->getValue());

			OdBmParamElemPtr paramElemPtr = paramID.safeOpenObject();
			OdBmParamDefPtr pDescParam = paramElemPtr->getParamDef();
			auto metaKey = convertToStdString(pDescParam->getCaption());
			dataStore[metaKey] = value;
		}
	}
}