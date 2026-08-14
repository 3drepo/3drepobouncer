/**
*  Copyright (C) 2018 3D Repo Ltd
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

#include <string>
#include <sstream>
#include <iostream>
#include <unordered_set>
#include <algorithm>
#include <cctype>
#include <boost/locale/encoding_utf.hpp>
#include "helper_functions.h"
#include <DbXrecord.h>
#include <toString.h>
#include <RxProperty.h>
#include <RxValue.h>
#include <RxValueTypeUtil.h>
#include <RxMember.h>
#include <RxAttribute.h>
#include <DbSymbolTableRecord.h>
#include <DbMaterial.h>
#include <CmColorBase.h>
#include <repo_log.h>

using namespace boost::locale::conv;

const double repo::manipulator::modelconvertor::odaHelper::DOUBLE_TOLERANCE = 1E-6;

std::string repo::manipulator::modelconvertor::odaHelper::convertToStdString(const OdString &value)
{
	std::wstring wstr((wchar_t*)value.c_str());
	std::string str = utf_to_utf<char>(wstr.c_str(), wstr.c_str() + wstr.size());
	return str;
}

void repo::manipulator::modelconvertor::odaHelper::forEachBmDBView(OdBmDatabasePtr database, std::function<void(OdBmDBViewPtr viewPtr)> func)
{
	OdDbBaseDatabasePEPtr pDbPE(database);
	OdRxIteratorPtr layouts = pDbPE->layouts(database);
	for (; !layouts->done(); layouts->next())
	{
		OdBmDBDrawingPtr pDBDrawing = layouts->object();
		if (pDBDrawing->getBaseViewType() != OdBm::ViewType::ThreeD)
			continue;

		OdBmViewportPtr pViewport = pDBDrawing->getBaseViewportId().safeOpenObject();
		if (pViewport.isNull())
			continue;

		OdBmDBViewPtr pDBView = pViewport->getDbViewId().safeOpenObject();
		if (pDBView.isNull())
			continue;

		func(pDBView);
	}
}

int repo::manipulator::modelconvertor::odaHelper::compare(double d1, double d2)
{
	float diff = d1 - d2;
	if (diff > DOUBLE_TOLERANCE)
		return 1;
	if (diff < -DOUBLE_TOLERANCE)
		return -1;

	return 0;
}

bool repo::manipulator::modelconvertor::odaHelper::samePoint(const repo::lib::RepoVector3D64& a, const repo::lib::RepoVector3D64& b)
{
	return compare(a.x, b.x) == 0 &&
		compare(a.y, b.y) == 0 &&
		compare(a.z, b.z) == 0;
}

std::string repo::manipulator::modelconvertor::odaHelper::pointKey(const repo::lib::RepoVector3D64& p)
{
	// Quantise to the same tolerance samePoint()/compare() use, so points that
	// compare equal always produce the same key.
	const double keyScale = 1.0 / DOUBLE_TOLERANCE;
	return std::to_string(static_cast<long long>(p.x * keyScale)) + "," +
		std::to_string(static_cast<long long>(p.y * keyScale)) + "," +
		std::to_string(static_cast<long long>(p.z * keyScale));
}

std::string repo::manipulator::modelconvertor::odaHelper::edgeKey(const repo::lib::RepoVector3D64& a, const repo::lib::RepoVector3D64& b)
{
	auto keyA = pointKey(a);
	auto keyB = pointKey(b);
	return keyA < keyB ? keyA + "|" + keyB : keyB + "|" + keyA;
}

void repo::manipulator::modelconvertor::odaHelper::extractProxyDictionaryProperties(
	OdDbDictionaryPtr pDict,
	const std::vector<std::string>& dictNames,
	const std::string& metadataPrefix,
	const std::vector<std::string>& triggerSubstrings,
	bool includeInt32,
	std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	for (const auto& dictName : dictNames)
	{
		OdResult pStatus;
		OdDbObjectId entryId = pDict->getAt(OdString(dictName.c_str()), &pStatus);
		if (pStatus != eOk || entryId.isNull()) continue;

		OdDbXrecordPtr pXRec = OdDbXrecord::cast(entryId.safeOpenObject());
		if (pXRec.isNull()) continue;

		std::string propName = "";
		for (OdResBufPtr pRb = pXRec->rbChain(); !pRb.isNull(); pRb = pRb->next())
		{
			int resType = pRb->restype();

			if (resType == OdResBuf::kDxfText || resType == OdResBuf::kDxfXTextString)
			{
				std::string text = convertToStdString(pRb->getString());

				bool isPropertyName = false;
				for (const auto& trigger : triggerSubstrings)
				{
					if (text.find(trigger) != std::string::npos)
					{
						isPropertyName = true;
						break;
					}
				}

				if (isPropertyName)
				{
					propName = metadataPrefix + "::" + text;
				}
				else if (!propName.empty())
				{
					metadata[propName] = text;
					propName = "";
				}
			}
			else if (!propName.empty() && resType == OdResBuf::kDxfReal)
			{
				metadata[propName] = pRb->getDouble();
				propName = "";
			}
			else if (!propName.empty() && includeInt32 && resType == OdResBuf::kDxfInt32)
			{
				metadata[propName] = (int64_t)pRb->getInt32();
				propName = "";
			}
		}
	}
}

repo::lib::RepoVector3D64 repo::manipulator::modelconvertor::odaHelper::calcNormal(repo::lib::RepoVector3D64 p1, repo::lib::RepoVector3D64 p2, repo::lib::RepoVector3D64 p3)
{
	repo::lib::RepoVector3D64 vecA = p2 - p1;
	repo::lib::RepoVector3D64 vecB = p3 - p2;
	repo::lib::RepoVector3D64 vecC = vecA.crossProduct(vecB);
	vecC.normalize();
	return vecC;
}

repo::lib::RepoVector3D64 repo::manipulator::modelconvertor::odaHelper::toRepoVector(const OdGePoint3d& p)
{
	return repo::lib::RepoVector3D64(p.x, p.y, p.z);
}

repo::lib::RepoVector3D64 repo::manipulator::modelconvertor::odaHelper::toRepoVector(const OdGeVector3d& p)
{
	return repo::lib::RepoVector3D64(p.x, p.y, p.z);
}

repo::lib::RepoBounds repo::manipulator::modelconvertor::odaHelper::toRepoBounds(const OdGeExtents3d& b)
{
	return *(repo::lib::RepoBounds*)(&b);
}

void repo::manipulator::modelconvertor::odaHelper::removeDuplicateGeneralMetadata(
	std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	auto canonicalGeneralKey = [](const std::string& key) {
		const std::string prefix = "General::";
		if (key.rfind(prefix, 0) != 0) return std::string();

		std::string normalized;
		for (auto ch : key.substr(prefix.size()))
		{
			unsigned char c = static_cast<unsigned char>(ch);
			if (std::isalnum(c))
			{
				normalized.push_back(static_cast<char>(std::tolower(c)));
			}
		}

		if (normalized == "color" || normalized == "truecolor") return std::string("General::True Color");
		if (normalized == "layer") return std::string("General::Layer");
		if (normalized == "linetype") return std::string("General::Linetype");
		if (normalized == "linetypescale") return std::string("General::Linetype scale");
		if (normalized == "plotstyle" || normalized == "plotstylename") return std::string("General::Plot style");
		if (normalized == "lineweight") return std::string("General::Lineweight");
		if (normalized == "hyperlink") return std::string("General::Hyperlink");
		if (normalized == "visibility") return std::string("General::Visibility");
		return std::string();
	};

	std::vector<std::string> eraseKeys;
	std::unordered_set<std::string> pendingCanonicalKeys;
	std::vector<std::pair<std::string, repo::lib::RepoVariant>> insertValues;

	for (const auto& [key, value] : metadata)
	{
		auto canonicalKey = canonicalGeneralKey(key);
		if (canonicalKey.empty() || canonicalKey == key) continue;

		if (metadata.find(canonicalKey) == metadata.end() && pendingCanonicalKeys.insert(canonicalKey).second)
		{
			insertValues.push_back({ canonicalKey, value });
		}
		eraseKeys.push_back(key);
	}

	for (const auto& [key, value] : insertValues)
	{
		metadata[key] = value;
	}
	for (const auto& key : eraseKeys)
	{
		metadata.erase(key);
	}
}

void repo::manipulator::modelconvertor::odaHelper::extractEntityProperties(OdDbEntityPtr pEntity, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	if (pEntity.isNull()) return;

	try {
		// Use ODA's Common Data Access (CDA) to enumerate all properties.
		// This reads the same properties shown in the AutoCAD Properties panel:
		// General (Color, Layer, Linetype, Lineweight, Transparency, etc.)
		// Pattern (for hatches: Type, Pattern name, Angle, Scale, etc.)
		// Geometry (Elevation, Area, Cumulative Area, etc.)

		OdRxMemberIteratorPtr pIter = OdRxMemberQueryEngine::theEngine()->newMemberIterator(pEntity);
		if (pIter.isNull()) return;

		for (; !pIter->done(); pIter->next())
		{
			OdRxMember* pMember = pIter->current();
			if (!pMember) continue;

			// Only process properties (not methods or other members)
			OdRxProperty* pProp = OdRxProperty::cast(pMember);
			if (!pProp) continue;

			try {
				// Get property name
				std::string propName = convertToStdString(pProp->name());

				// Skip internal/system properties
				if (propName.empty() || propName[0] == '_') continue;

				// Get the category (General, Pattern, Geometry, etc.)
				std::string category = "";
				OdRxAttributePtr pAttr = pProp->attributes().get(OdRxUiPlacementAttribute::desc());
				if (!pAttr.isNull())
				{
					OdRxUiPlacementAttribute* pPlacement = OdRxUiPlacementAttribute::cast(pAttr);
					if (pPlacement)
					{
						category = convertToStdString(pPlacement->getCategory(pProp));
					}
				}
				if (category.empty()) continue; // Skip properties without a category

				// Build the metadata key with category prefix
				std::string metaKey = category.empty() ? propName : (category + "::" + propName);

				// Read the property value
				OdRxValue value;
				OdResult res = pProp->getValue(pEntity, value);
				if (res != eOk) continue;

				// Resolve the OdRxValue to a displayable RepoVariant
				const auto& vtype = value.type();

				// --- Object ID resolution (Layer, Linetype, Material, etc.) ---
				if (vtype == OdRxValueType::Desc<OdDbObjectId>::value())
				{
					OdDbObjectId objId = *rxvalue_cast<OdDbObjectId>(&value);
					if (!objId.isNull())
					{
						try {
							OdDbObjectPtr pObj = objId.safeOpenObject();
							if (!pObj.isNull())
							{
								OdDbSymbolTableRecordPtr pSymRec = OdDbSymbolTableRecord::cast(pObj);
								if (!pSymRec.isNull())
								{
									metadata[metaKey] = convertToStdString(pSymRec->getName());
								}
								else
								{
									OdDbMaterialPtr pMat = OdDbMaterial::cast(pObj);
									metadata[metaKey] = !pMat.isNull()
										? convertToStdString(pMat->name())
										: convertToStdString(pObj->isA()->name()) + ":" + convertToStdString(toString(objId.getHandle()));
								}
							}
						}
						catch (...) {
							metadata[metaKey] = convertToStdString(toString(objId.getHandle()));
						}
					}
				}
				// --- Transparency ---
				else if (vtype == OdRxValueType::Desc<OdCmTransparency>::value())
				{
					OdCmTransparency trans = *rxvalue_cast<OdCmTransparency>(&value);
					if (trans.isByLayer())       metadata[metaKey] = std::string("ByLayer");
					else if (trans.isByBlock())  metadata[metaKey] = std::string("ByBlock");
					else if (trans.isClear())    metadata[metaKey] = std::string("0");
					else                         metadata[metaKey] = std::to_string((int)((1.0 - trans.alpha() / 255.0) * 100.0 + 0.5));
				}
				// --- Color ---
				else if (vtype == OdRxValueType::Desc<OdCmColor>::value())
				{
					OdCmColor clr = *rxvalue_cast<OdCmColor>(&value);
					switch (clr.colorMethod())
					{
						case OdCmEntityColor::kByLayer:
							metadata[metaKey] = std::string("ByLayer");
							break;
						case OdCmEntityColor::kByBlock:
							metadata[metaKey] = std::string("ByBlock");
							break;
						case OdCmEntityColor::kByColor:
						{
							// AutoCAD displays true colors as "Red,Green,Blue"
							// If a color book name is set, it displays that instead
							OdString bookName = clr.bookName();
							OdString colorName = clr.colorName();
							if (!colorName.isEmpty())
							{
								std::string display = convertToStdString(colorName);
								if (!bookName.isEmpty())
									display = convertToStdString(bookName) + "$" + display;
								metadata[metaKey] = display;
							}
							else
							{
								metadata[metaKey] = std::to_string(clr.red()) + ","
									+ std::to_string(clr.green()) + ","
									+ std::to_string(clr.blue());
							}
							break;
						}
						case OdCmEntityColor::kByACI:
						case OdCmEntityColor::kByPen:
						case OdCmEntityColor::kByDgnIndex:
						{
							// AutoCAD Civil 3D displays ACI 1-7 by name, others as "Color N"
							// SDK: Autodesk.AutoCAD.Colors.Color.ColorIndex
							int aci = clr.colorIndex();
							switch (aci)
							{
							case 0:   metadata[metaKey] = std::string("ByBlock"); break;
							case 1:   metadata[metaKey] = std::string("Red"); break;
							case 2:   metadata[metaKey] = std::string("Yellow"); break;
							case 3:   metadata[metaKey] = std::string("Green"); break;
							case 4:   metadata[metaKey] = std::string("Cyan"); break;
							case 5:   metadata[metaKey] = std::string("Blue"); break;
							case 6:   metadata[metaKey] = std::string("Magenta"); break;
							case 7:   metadata[metaKey] = std::string("White"); break;
							case 256: metadata[metaKey] = std::string("ByLayer"); break;
							default:  metadata[metaKey] = "RGB(" + std::to_string(clr.red()) + ","
								+ std::to_string(clr.green()) + ","
								+ std::to_string(clr.blue())+ ")";
								break;
							}
							break;
						}
						case OdCmEntityColor::kForeground:
							metadata[metaKey] = std::string("White");
							break;
						case OdCmEntityColor::kNone:
							metadata[metaKey] = std::string("None");
							break;
						default:
							metadata[metaKey] = "RGB(" + std::to_string(clr.red()) + ","
								+ std::to_string(clr.green()) + ","
								+ std::to_string(clr.blue()) + ")";
							break;
					}
				}
				// --- LineWeight ---
				else if (vtype == OdRxValueType::Desc<OdDb::LineWeight>::value())
				{
					switch (OdDb::LineWeight lw = *rxvalue_cast<OdDb::LineWeight>(&value); lw)
					{
					case OdDb::kLnWtByLayer:    metadata[metaKey] = std::string("ByLayer"); break;
					case OdDb::kLnWtByBlock:    metadata[metaKey] = std::string("ByBlock"); break;
					case OdDb::kLnWtByLwDefault: metadata[metaKey] = std::string("Default"); break;
					default: metadata[metaKey] = std::to_string((int)lw / 100.0) + " mm"; break;
					}
				}
				// --- Numeric types ---
				else if (vtype == OdRxValueType::Desc<double>::value())
				{
					metadata[metaKey] = *rxvalue_cast<double>(&value);
				}
				else if (vtype == OdRxValueType::Desc<int>::value())
				{
					metadata[metaKey] = (int64_t)*rxvalue_cast<int>(&value);
				}
				else if (vtype == OdRxValueType::Desc<OdInt16>::value())
				{
					metadata[metaKey] = (int64_t)*rxvalue_cast<OdInt16>(&value);
				}
				else if (vtype == OdRxValueType::Desc<OdInt32>::value())
				{
					metadata[metaKey] = (int64_t)*rxvalue_cast<OdInt32>(&value);
				}
				else if (vtype == OdRxValueType::Desc<OdUInt32>::value())
				{
					metadata[metaKey] = (int64_t)*rxvalue_cast<OdUInt32>(&value);
				}
				else if (vtype == OdRxValueType::Desc<bool>::value())
				{
					metadata[metaKey] = *rxvalue_cast<bool>(&value) ? std::string("Yes") : std::string("No");
				}
				// --- String ---
				else if (vtype == OdRxValueType::Desc<OdString>::value())
				{
					OdString str = *rxvalue_cast<OdString>(&value);
					if (!str.isEmpty())
						metadata[metaKey] = convertToStdString(str);
				}
				// --- Geometry types ---
				else if (vtype == OdRxValueType::Desc<OdGePoint3d>::value())
				{
					OdGePoint3d pt = *rxvalue_cast<OdGePoint3d>(&value);
					metadata[metaKey] = "(" + std::to_string(pt.x) + ", " +
						std::to_string(pt.y) + ", " + std::to_string(pt.z) + ")";
				}
				else if (vtype == OdRxValueType::Desc<OdGePoint2d>::value())
				{
					OdGePoint2d pt = *rxvalue_cast<OdGePoint2d>(&value);
					metadata[metaKey] = "(" + std::to_string(pt.x) + ", " + std::to_string(pt.y) + ")";
				}
				else if (vtype == OdRxValueType::Desc<OdGeVector3d>::value())
				{
					OdGeVector3d v = *rxvalue_cast<OdGeVector3d>(&value);
					metadata[metaKey] = "(" + std::to_string(v.x) + ", " +
						std::to_string(v.y) + ", " + std::to_string(v.z) + ")";
				}
				// --- Fallback: toString() ---
				else
				{
					OdString strVal = value.toString();
					if (!strVal.isEmpty())
						metadata[metaKey] = convertToStdString(strVal);
				}
			}
			catch (...) {
				// Skip properties that throw during read
				continue;
			}
		}
	}
	catch (OdError& e) {
		repoTrace << "CDA property extraction failed: " << convertToStdString(e.description());
	}
	catch (...) {
		repoTrace << "CDA property extraction failed with unknown error";
	}
}