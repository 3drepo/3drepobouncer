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

#include <OdaCommon.h>
#include <DbEntity.h>
#include <DbLayout.h>
#include <OdDbGeoDataMarker.h>
#include <DbBlockTableRecord.h>
#include <DbBlockReference.h>
#include <OdString.h>
#include <toString.h>
#include <DbProxyEntity.h>
#include <DbEntityWithGrData.h>
#include <DbRegAppTable.h>
#include <DbRegAppTableRecord.h>
#include <DbDictionary.h>
#include <DbXrecord.h>
#include <RxProperty.h>
#include <RxValue.h>
#include <RxValueTypeUtil.h>
#include <RxMember.h>
#include <RxObjectImpl.h>
#include <RxAttribute.h>
#include <DbDatabase.h>
#include <DbSymbolTableRecord.h>
#include <DbMaterial.h>
#include <CmColorBase.h>
#include "helper_functions.h"
#include "data_processor_dwg.h"
#include <repo_log.h>
#include <algorithm>
#include <cmath>
#include <cctype>

using namespace repo::manipulator::modelconvertor::odaHelper;

// samePoint(), pointKey(), edgeKey() and extractProxyDictionaryProperties() now
// live alongside compare() and the other shared geometry/proxy utilities in
// helper_functions.h. Civil3D/Plant3D-specific data and logic live in
// civil3d_proxy_handler.h/.cpp and plant3d_proxy_handler.h/.cpp.

DataProcessorDwg::~DataProcessorDwg()
{
	// This exists so we can use unique_ptr with a forward declaration of DwgDrawContext
	printDiagnostics();
}

ProxyAppType DataProcessorDwg::classifyApplication(const std::string& originalClass, ProxyAppHandler*& outHandler)
{
	outHandler = nullptr;
	if (originalClass.empty() || originalClass == "Unknown") return ProxyAppType::Unknown;

	for (auto* handler : proxyHandlers)
	{
		if (handler->matches(originalClass))
		{
			outHandler = handler;
			return handler->appType();
		}
	}

	return ProxyAppType::Custom;
}

std::string DataProcessorDwg::formatApplicationDisplayString(const ProxyInfo& info)
{
	if (!info.originalClass.empty() && info.originalClass != "Unknown")
	{
		if (info.matchedHandler) return info.matchedHandler->appName() + " (" + info.originalClass + ")";
		return "CustomApp (" + info.originalClass + ")";
	}

	for (const auto& app : info.xDataApps)
	{
		if (app.find("Aecc") != std::string::npos) return "Civil3D (XData)";
		if (app.find("AcPp") != std::string::npos) return "Plant3D (XData)";
	}

	return "";
}

bool DataProcessorDwg::getProxyInfo(OdDbEntityPtr entity, ProxyInfo& info, const ProxyReadOptions& options)
{
	info = ProxyInfo();

	if (entity.isNull()) return false;

	info.entity = OdDbProxyEntity::cast(entity);
	if (info.entity.isNull()) return false;

	try
	{
		OdString originalClassName = info.entity->originalClassName();
		if (!originalClassName.isEmpty())
		{
			info.originalClass = convertToStdString(originalClassName);
		}
		else
		{
			OdString appDescription = info.entity->applicationDescription();
			info.originalClass = !appDescription.isEmpty() ? convertToStdString(appDescription) : "Unknown";
		}
	}
	catch (OdError& e)
	{
		repoTrace << "Failed to get proxy class: " << convertToStdString(e.description());
		info.originalClass = "Unknown";
	}

	OdString originalDxfName = info.entity->originalDxfName();
	if (!originalDxfName.isEmpty()) info.originalDxfName = convertToStdString(originalDxfName);

	OdString appDescription = info.entity->applicationDescription();
	if (!appDescription.isEmpty()) info.applicationDescription = convertToStdString(appDescription);

	info.graphicsType = info.entity->graphicsMetafileType();
	info.hasFullGraphicsFlag = info.graphicsType == OdDbProxyEntity::kFullGraphics;

	try { info.graphicsPE = OdDbEntityWithGrDataPE::cast(entity); }
	catch (...) {}

	if (options.readXData) ensureProxyXData(info, options);
	if (options.readExtensionDictionary) ensureProxyExtensionDictionary(info);

	info.appType = classifyApplication(info.originalClass, info.matchedHandler);

	return true;
}

void DataProcessorDwg::ensureProxyXData(ProxyInfo& info, const ProxyReadOptions& options)
{
	if (info.xDataLoaded || info.entity.isNull()) return;
	info.xDataLoaded = true;

	try
	{
		info.xData = info.entity->xData();
		if (!info.xData.isNull())
		{
			for (OdResBufPtr pRb = info.xData; !pRb.isNull(); pRb = pRb->next())
			{
				if (pRb->restype() == OdResBuf::kDxfRegAppName)
				{
					OdString appName = pRb->getString();
					if (!appName.isEmpty()) info.xDataApps.push_back(convertToStdString(appName));
				}
			}
		}
		else if (options.scanRegisteredAppsFallback && info.entity->database() != nullptr)
		{
			repoTrace << "xData() returned null - trying database query method";

			OdDbObjectId entityId = info.entity->objectId();
			if (!entityId.isNull())
			{
				OdDbRegAppTablePtr pAppTable = info.entity->database()->getRegAppTableId().safeOpenObject();
				if (!pAppTable.isNull())
				{
					repoTrace << "Checking registered application table...";
					OdDbSymbolTableIteratorPtr pIter = pAppTable->newIterator();

					for (; !pIter->done(); pIter->step())
					{
						OdDbRegAppTableRecordPtr pApp = pIter->getRecord();
						if (pApp.isNull()) continue;

						OdString appName = pApp->getName();
						std::string appStr = convertToStdString(appName);

						OdResBufPtr pAppData = info.entity->xData(appName);
						if (!pAppData.isNull())
						{
							repoTrace << "  Entity has XData for: " << appStr;
							info.xDataApps.push_back(appStr);
						}
					}
				}
			}
		}
	}
	catch (OdError& e)
	{
		repoTrace << "XData access failed: " << convertToStdString(e.description());
	}
	catch (...)
	{
		repoTrace << "XData access failed with unknown exception";
	}
}

void DataProcessorDwg::ensureProxyExtensionDictionary(ProxyInfo& info)
{
	if (info.extensionDictionaryLoaded || info.entity.isNull()) return;
	info.extensionDictionaryLoaded = true;

	try
	{
		info.extensionDictionaryId = info.entity->extensionDictionary();
		if (!info.extensionDictionaryId.isNull())
		{
			info.extensionDictionary = info.extensionDictionaryId.safeOpenObject();
		}
	}
	catch (...)
	{
		repoTrace << "Proxy extension dictionary access failed";
	}
}

bool DataProcessorDwg::drawStoredProxyGraphics(OdDbEntityPtr pEntity, const ProxyInfo& info)
{
	if (pEntity.isNull() || !info.isProxy()) return false;

	// Both of these cases are already reported, deduplicated and with handle and
	// layer context, by logProxyWithoutRenderableGeometry().
	if (!info.hasFullGraphics()) return false;
	if (info.graphicsPE.isNull()) return false;

	try
	{
		return info.graphicsPE->worldDraw(pEntity, this);
	}
	catch (OdError& e)
	{
		repoTrace << "Stored proxy graphics replay failed: "
			<< convertToStdString(e.description());
	}
	catch (...)
	{
		repoTrace << "Stored proxy graphics replay failed with unknown error";
	}

	return false;
}

void DataProcessorDwg::printDiagnostics() const
{
	if (stats.totalEntities == 0) return;

	repoInfo << "DWG import: " << stats.totalEntities << " entities, "
		<< stats.entitiesWithGeometry << " with geometry, "
		<< stats.entitiesWithoutGeometry << " without";
	repoInfo << "DWG import: " << stats.civil3dEntities << " Civil3D, "
		<< stats.plant3dEntities << " Plant3D, "
		<< stats.proxyEntities << " proxy entities";

	if (suppressedProxyGeometryFailures)
	{
		repoInfo << "DWG import: " << suppressedProxyGeometryFailures
			<< " further proxies without renderable geometry were not reported individually";
	}

	if (!stats.entityTypeCount.empty() && (stats.proxyEntities > 0 || stats.civil3dEntities > 0 || stats.plant3dEntities > 0))
	{
		std::string customTypes;
		for (const auto& [type, count] : stats.entityTypeCount)
		{
			if (type.find("Aecc") != std::string::npos ||
				type.find("AcPp") != std::string::npos ||
				type == "AcDbProxyEntity")
			{
				if (!customTypes.empty()) customTypes += ", ";
				customTypes += type + "=" + std::to_string(count);
			}
		}

		if (!customTypes.empty())
		{
			repoInfo << "DWG import: custom entity types: " << customTypes;
		}
	}
}

void DataProcessorDwg::logProxyWithoutRenderableGeometry(
	OdDbEntityPtr pEntity,
	ProxyInfo& info,
	bool replayedStoredProxyGraphics,
	bool replayReturnedGeometry)
{
	if (!info.isProxy()) return;

	std::string handle = "Unknown";
	try
	{
		handle = convertToStdString(toString(pEntity->objectId().getHandle()));
	}
	catch (...) {}

	// Report each distinct proxy once, and only up to the cap. Everything past
	// that is counted and summarised by printDiagnostics(), so a drawing with
	// thousands of unrenderable proxies cannot flood the log.
	if (!loggedProxyGeometryFailures.insert(handle).second) return;

	if (loggedProxyGeometryFailures.size() > kMaxLoggedProxyGeometryFailures)
	{
		if (++suppressedProxyGeometryFailures == 1)
		{
			repoWarning << "[DWG_PROXY_NO_RENDERABLE_GEOMETRY] reached "
				<< kMaxLoggedProxyGeometryFailures
				<< " reported proxies; further ones are counted only";
		}
		return;
	}

	// Only needed for proxies we are actually going to report on.
	ensureProxyXData(info);

	std::string layer = "Unknown";
	try
	{
		layer = convertToStdString(toString(pEntity->layer()));
	}
	catch (...) {}

	std::string graphicsTypeName = "Unknown";
	std::string reason = "No geometry was produced by the vectorizer";

	if (info.graphicsType == OdDbProxyEntity::kNoMetafile)
	{
		graphicsTypeName = "No Metafile";
		reason = "Proxy entity has no saved graphics metafile";
	}
	else if (info.graphicsType == OdDbProxyEntity::kBoundingBox)
	{
		graphicsTypeName = "Bounding Box";
		reason = "Proxy entity only has bounding-box proxy graphics";
	}
	else if (info.graphicsType == OdDbProxyEntity::kFullGraphics)
	{
		graphicsTypeName = "Full Graphics";
		if (!replayedStoredProxyGraphics)
		{
			reason = "Stored proxy graphics replay was not attempted";
		}
		else if (!replayReturnedGeometry)
		{
			reason = "Stored proxy graphics replay returned false";
		}
		else
		{
			reason = "Stored proxy graphics replay produced no supported mesh or line geometry";
		}
	}

	std::string xdataApps;
	for (size_t i = 0; i < info.xDataApps.size(); ++i)
	{
		if (i > 0) xdataApps += ",";
		xdataApps += info.xDataApps[i];
	}

	// Built as a single record: the optional fields used to be separate log
	// lines, which multiplied the volume for no extra information.
	std::string message = "[DWG_PROXY_NO_RENDERABLE_GEOMETRY] handle=" + handle +
		" layer=\"" + layer + "\"" +
		" originalClass=\"" + info.originalClass + "\"" +
		" originalDxfName=\"" + info.originalDxfName + "\"" +
		" graphicsMetafile=\"" + graphicsTypeName + "\"" +
		" replayAttempted=" + (replayedStoredProxyGraphics ? "true" : "false") +
		" replayReturned=" + (replayReturnedGeometry ? "true" : "false") +
		" hasGraphicsPE=" + (info.graphicsPE.isNull() ? "false" : "true") +
		" reason=\"" + reason + "\"";

	if (!info.applicationDescription.empty())
	{
		message += " applicationDescription=\"" + info.applicationDescription + "\"";
	}

	if (!xdataApps.empty())
	{
		message += " xdataApps=\"" + xdataApps + "\"";
	}

	repoWarning << message;
}

std::unordered_map<std::string, repo::lib::RepoVariant> DataProcessorDwg::getProxyEntityMetadata(OdDbEntityPtr pEntity, ProxyInfo& info)
{
	std::unordered_map<std::string, repo::lib::RepoVariant> metadata;
	if (!info.isProxy()) return metadata;

	// Metadata is the one path that genuinely needs both of the expensive reads.
	ensureProxyXData(info);
	ensureProxyExtensionDictionary(info);

	try
	{
		// First ask ODA Common Data Access for palette-style properties. When a
		// native object enabler is available this can include custom object data;
		// otherwise it normally returns the proxy/general entity properties.
		addProxyBasicMetadata(pEntity, info, metadata);
		addProxyGeneralMetadata(pEntity, metadata);
		addProxyGeometryMetadata(pEntity, metadata);
		addProxyXDataMetadata(info, metadata);
		addProxyDictionaryMetadata(info, metadata);
		extractTextPropertiesFromProxy(info.entity, metadata);
	}
	catch (OdError& e)
	{
		metadata["Proxy::Metadata Error"] = convertToStdString(e.description());
	}
	catch (...)
	{
		metadata["Proxy::Metadata Error"] = std::string("Unknown error reading proxy metadata");
	}

	return metadata;
}

void DataProcessorDwg::addProxyBasicMetadata(OdDbEntityPtr pEntity, const ProxyInfo& info, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	extractEntityProperties(pEntity, metadata);

	if (!info.originalClass.empty())
	{
		metadata["Proxy::Original Class"] = info.originalClass;
	}

	if (!info.originalDxfName.empty())
	{
		metadata["Proxy::Original DXF Name"] = info.originalDxfName;
	}

	if (!info.applicationDescription.empty())
	{
		metadata["Proxy::Application Description"] = info.applicationDescription;
	}

	auto appType = formatApplicationDisplayString(info);
	if (!appType.empty())
	{
		metadata["Proxy::Application"] = appType;
	}
}

void DataProcessorDwg::addProxyGeneralMetadata(OdDbEntityPtr pEntity, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	auto setIfMissing = [&](const std::string& key, const repo::lib::RepoVariant& value) {
		if (metadata.find(key) == metadata.end())
		{
			metadata[key] = value;
		}
	};

	auto colorToString = [](const OdCmColor& clr) {
		switch (clr.colorMethod())
		{
		case OdCmEntityColor::kByLayer:
			return std::string("ByLayer");
		case OdCmEntityColor::kByBlock:
			return std::string("ByBlock");
		case OdCmEntityColor::kByACI:
		case OdCmEntityColor::kByPen:
		case OdCmEntityColor::kByDgnIndex:
		{
			int aci = clr.colorIndex();
			switch (aci)
			{
			case 0: return std::string("ByBlock");
			case 1: return std::string("Red");
			case 2: return std::string("Yellow");
			case 3: return std::string("Green");
			case 4: return std::string("Cyan");
			case 5: return std::string("Blue");
			case 6: return std::string("Magenta");
			case 7: return std::string("White");
			case 256: return std::string("ByLayer");
			default: return std::string("Color ") + std::to_string(aci);
			}
		}
		case OdCmEntityColor::kByColor:
			return std::string("RGB(") + std::to_string(clr.red()) + "," +
				std::to_string(clr.green()) + "," + std::to_string(clr.blue()) + ")";
		case OdCmEntityColor::kForeground:
			return std::string("Foreground");
		case OdCmEntityColor::kNone:
			return std::string("None");
		default:
			return std::string("RGB(") + std::to_string(clr.red()) + "," +
				std::to_string(clr.green()) + "," + std::to_string(clr.blue()) + ")";
		}
	};

	auto lineWeightToString = [](OdDb::LineWeight lw) {
		switch (lw)
		{
		case OdDb::kLnWtByLayer:
			return std::string("ByLayer");
		case OdDb::kLnWtByBlock:
			return std::string("ByBlock");
		case OdDb::kLnWtByLwDefault:
			return std::string("Default");
		default:
			return std::to_string(static_cast<int>(lw) / 100.0) + " mm";
		}
	};

	setIfMissing("General::Layer", convertToStdString(toString(pEntity->layer())));
	setIfMissing("General::True Color", colorToString(pEntity->color()));
	setIfMissing("General::Linetype", convertToStdString(toString(pEntity->linetype())));
	setIfMissing("General::Linetype scale", pEntity->linetypeScale());
	setIfMissing("General::Lineweight", lineWeightToString(pEntity->lineWeight()));
	setIfMissing("General::Visibility", pEntity->visibility() == OdDb::kInvisible ? std::string("Invisible") : std::string("Visible"));
}

void DataProcessorDwg::addProxyGeometryMetadata(OdDbEntityPtr pEntity, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	try
	{
		OdGeExtents3d extents;
		if (pEntity->getGeomExtents(extents) == eOk)
		{
			auto min = extents.minPoint();
			auto max = extents.maxPoint();
			metadata["Geometry::Bounds Min"] = "(" + std::to_string(min.x) + ", " +
				std::to_string(min.y) + ", " + std::to_string(min.z) + ")";
			metadata["Geometry::Bounds Max"] = "(" + std::to_string(max.x) + ", " +
				std::to_string(max.y) + ", " + std::to_string(max.z) + ")";
		}
	}
	catch (...) {}
}

void DataProcessorDwg::addProxyXDataMetadata(const ProxyInfo& info, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	if (!info.xData.isNull())
	{
		extractXDataProperties(info.xData, metadata);
	}
}

void DataProcessorDwg::addProxyDictionaryMetadata(const ProxyInfo& info, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	if (info.extensionDictionary.isNull() || !info.matchedHandler) return;
	info.matchedHandler->addDictionaryMetadata(info.extensionDictionary, metadata);
}

void DataProcessorDwg::removeDuplicateGeneralMetadata(
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

void DataProcessorDwg::setEntityMetadata(
	const std::string& layerId,
	const std::string& handleMetaValue,
	OdDbEntityPtr pEntity,
	ProxyInfo& info,
	ProxyGeometryCapture* capturedGeometry)
{
	if (layerId.empty() || handleMetaValue.empty() || collector->hasMetadata(layerId)) return;

	std::unordered_map<std::string, repo::lib::RepoVariant> meta, metadata;
	meta["Entity Handle::Value"] = handleMetaValue;

	if (!pEntity.isNull())
	{
		if (info.isProxy())
		{
			metadata = getProxyEntityMetadata(pEntity, info);
		}
		else
		{
			extractEntityProperties(pEntity, metadata);
		}

		if (capturedGeometry && capturedGeometry->hasTriangles())
		{
			capturedGeometry->addComputedMetadata(pEntity, metadata);
		}
		removeDuplicateGeneralMetadata(metadata);

		for (const auto& [key, value] : metadata)
		{
			meta[key] = value;
		}
	}

	collector->setMetadata(layerId, meta);
}

void DataProcessorDwg::extractXDataProperties(OdResBufPtr pRb, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	std::string currentApp = "";
	std::string lastStringValue = "";

	for (; !pRb.isNull(); pRb = pRb->next())
	{
		int resType = pRb->restype();

		if (resType == OdResBuf::kDxfRegAppName)
		{
			currentApp = convertToStdString(pRb->getString());
		}
		else if (!currentApp.empty())
		{
			std::string key = "XData::" + currentApp + "::";

			switch (resType)
			{
			case OdResBuf::kDxfXdAsciiString:
				lastStringValue = convertToStdString(pRb->getString());
				// Check if it looks like a property name
				if (lastStringValue.find("=") == std::string::npos)
				{
					key += lastStringValue;
				}
				else
				{
					metadata[key + "Value"] = lastStringValue;
				}
				break;

			case OdResBuf::kDxfXdReal:
			case OdResBuf::kDxfXdDist:
				if (!lastStringValue.empty())
				{
					metadata["XData::" + currentApp + "::" + lastStringValue] = pRb->getDouble();
					lastStringValue = "";
				}
				else
				{
					metadata[key + "Real"] = pRb->getDouble();
				}
				break;

			case OdResBuf::kDxfXdInteger32:
				if (!lastStringValue.empty())
				{
					metadata["XData::" + currentApp + "::" + lastStringValue] = (int64_t)pRb->getInt32();
					lastStringValue = "";
				}
				else
				{
					metadata[key + "Integer"] = (int64_t)pRb->getInt32();
				}
				break;
			}
		}
	}
}

void DataProcessorDwg::extractTextPropertiesFromProxy(OdDbProxyEntityPtr proxyEntity, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	try {
		// Some proxy entities save property descriptions as text
		OdString appDesc = proxyEntity->applicationDescription();
		if (!appDesc.isEmpty())
		{
			std::string desc = convertToStdString(appDesc);

			// Parse common patterns like "Property=Value"
			size_t pos = 0;
			while ((pos = desc.find("=", pos)) != std::string::npos)
			{
				// Find property name (before =)
				size_t nameStart = desc.rfind(" ", pos);
				if (nameStart == std::string::npos) nameStart = 0;
				else nameStart++;

				std::string propName = desc.substr(nameStart, pos - nameStart);

				// Find property value (after =)
				size_t valueEnd = desc.find(",", pos);
				if (valueEnd == std::string::npos) valueEnd = desc.find(";", pos);
				if (valueEnd == std::string::npos) valueEnd = desc.length();

				std::string propValue = desc.substr(pos + 1, valueEnd - pos - 1);

				// Trim whitespace
				propName.erase(0, propName.find_first_not_of(" \t\n\r"));
				propName.erase(propName.find_last_not_of(" \t\n\r") + 1);
				propValue.erase(0, propValue.find_first_not_of(" \t\n\r"));
				propValue.erase(propValue.find_last_not_of(" \t\n\r") + 1);

				if (!propName.empty() && !propValue.empty())
				{
					metadata["Property::" + propName] = propValue;
				}

				pos = valueEnd + 1;
			}
		}
	}
	catch (...) {}
}

void DataProcessorDwg::extractEntityProperties(OdDbEntityPtr pEntity, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
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

bool DataProcessorDwg::doDraw(OdUInt32 i, const OdGiDrawable* pDrawable)
{
	std::unique_ptr<GeometryCollector::Context> ctx;

	// These three locals track where the geometry from ctx - if any - should go.

	Layer entityLayer;
	Layer parentLayer;
	std::string handleMetaValue;

	// GeoDataMarkers derive directly from drawables; they aren't entities and
	// don't have Ids. The current behaviour is to disable these by default
	// through pDb->setGEOMARKERVISIBILITY. If they are re-enabled, this snippet
	// ensures that the geometry goes into its own tree node.

	// dynamic_cast is a workaround for an ODA bug where OdDbGeoDataMarker has no ODA RTTI
	// https://forum.opendesign.com/showthread.php?24537-Cast-of-OdGiDrawable-to-OdDbGeoDataMarker-succeeding-for-OdDbEntity
	auto pGeoDataMarker = dynamic_cast<const OdDbGeoDataMarker*>(pDrawable);
	if (pGeoDataMarker)
	{
		entityLayer = { "GeoPositionMarker", "Geo Position Marker" };
		ctx = collector->makeNewDrawContext();
	}

	// A proxy entity is inspected exactly once here; every downstream
	// consumer in this function (diagnostics, TIN detection, stored-graphics
	// replay, metadata) reuses this same ProxyInfo instead of re-casting or
	// re-querying ODA for the same entity.
	ProxyInfo info;
	bool isProxy = false;

	OdDbEntityPtr pEntity = OdDbEntity::cast(pDrawable);
	if (!pEntity.isNull())
	{
		// ===== DIAGNOSTIC TRACKING =====
		stats.totalEntities++;
		auto className = convertToStdString(pEntity->isA()->name());
		stats.entityTypeCount[className]++;

		// Cheap inspection only: one cast, no XData or extension dictionary
		// read. Consumers that need those load them on demand.
		isProxy = getProxyInfo(pEntity, info);
		if (isProxy)
		{
			stats.proxyEntities++;
			stats.entityTypeCount["Proxy-" + info.originalClass]++;
			// isCivil3D()/isPlant3D() read a single ProxyAppType value, so an
			// entity can only ever land in one bucket, never both.
			if (info.isCivil3D())
			{
				stats.civil3dEntities++;
			}
			else if (info.isPlant3D())
			{
				stats.plant3dEntities++;
			}
		}

		// As soon as we get an actual entity, cache the active Layout Id. This
		// can be used to determine when we are back at the top level (out of a
		// block).

		if (context.layoutId.isNull())
		{
			auto layout = OdDbLayout::cast(pEntity->database()->currentLayoutId().safeOpenObject());
			auto layoutBlockId = layout->getBlockTableRecordId();
			context.layoutId = layoutBlockId;
		}

		// Get some common properties that will be used throughout

		auto layerId = convertToStdString(toString(pEntity->layerId().getHandle()));
		auto layerName = convertToStdString(toString(pEntity->layer()));

		Layer assignedLayer(layerId, layerName);

		// If a Block Entity has the default layer assigned, then it appears in
		// the Navisworks tree under the Block Reference's layer. If it has a non
		// default layer assigned, it appears under that Layer in the tree.
		// In both cases the Entity appears under the Block's name.

		// Layer "0" cannot be renamed, so this is a safe way to check if the
		// current entity has its layer over-ridden for not.

		auto isDefaultLayer = layerName == "0";

		const OdDbHandle& handle = pEntity->objectId().getHandle();
		auto sHandle = pEntity->isDBRO() ? toString(handle) : toString(L"non-DbResident");
		auto entityId = convertToStdString(toString(sHandle));
		auto entityName = getClassDisplayName(pEntity, info);

		// Check if this drawable is directly under a layer or in a block.

		// We do this by checking the blockId - Entities that are not in blocks
		// belong to the Layout's Block Record.

		// (An alternative is to check the OdDbBlockTableRecord's getName() for
		// the '*' character, since this prefixes the system block records and
		// is not allowed in user defined block names.)

		if (pEntity->blockId() == context.layoutId)
		{
			context.inBlock = false;
		}

		// OdDbBlockReference indicates we are entering a Block. This object
		// defines the handle and default layer of all subsequent entities.

		OdDbBlockReferencePtr pBlock = OdDbBlockReference::cast(pDrawable);
		if (!pBlock.isNull() && !context.inBlock) // We only consider the top level block when building the tree.
		{
			context.inBlock = true;
			context.currentBlockReferenceLayer = assignedLayer;

			// Information about the Block prototype itself is available in its
			// table record.

			auto record = OdDbBlockTableRecord::cast(pBlock->blockTableRecord().safeOpenObject());
			context.currentBlockReference = Layer(entityId, convertToStdString(record->getName()));
		}

		if (context.inBlock)
		{
			// When inside a block, entities should sit under the Block's
			// reference in the appropriate layer. A single Block Reference
			// therefore may appear multiple times, once under a variety of
			// different layers.

			// Create a unique node for the reference under each layer in which
			// it appears.

			entityLayer = {
				context.currentBlockReference.id + layerId,
				context.currentBlockReference.name
			};
			handleMetaValue = context.currentBlockReference.id;
			parentLayer = context.currentBlockReferenceLayer;

			if (!isDefaultLayer)
			{
				parentLayer = assignedLayer; // If the Block Entity layer has been overridden within the Block, take the absolute layer
			}

			ctx = collector->makeNewDrawContext();
		}
		else
		{
			// When not inside a block, each entity appears under its own tree
			// node, under the specified layer.

			entityLayer = { entityId, entityName };
			parentLayer = assignedLayer;
			handleMetaValue = entityId;

			ctx = collector->makeNewDrawContext();
		}
	}

	// thisEntityCapture: is THIS entity a Civil3D TIN surface, and if so, the
	// capture object for it. Kept separate from the activeGeometryCapture MEMBER
	// (below), which tracks what the geometry callbacks should route into for the
	// duration of the nested OdGsBaseMaterialView::doDraw() call only - the
	// underlying capture buffer is never swapped per-entity, only toggled.
	ProxyGeometryCapture* thisEntityCapture = (isProxy && isCivil3DTinSurface(info)) ? info.matchedHandler->geometryCapture() : nullptr;
	bool tinSurfaceProxy = thisEntityCapture != nullptr;
	bool replayStoredProxyGraphics = isProxy;

	collector->pushDrawContext(ctx.get());
	bool ret = false;
	if (replayStoredProxyGraphics)
	{
		ProxyGeometryCapture* previousCapture = activeGeometryCapture;
		if (tinSurfaceProxy)
		{
			thisEntityCapture->beginCapture();
			activeGeometryCapture = thisEntityCapture;
		}
		else
		{
			activeGeometryCapture = nullptr;
		}

		ret = drawStoredProxyGraphics(pEntity, info);
		if (!(ctx && ctx->hasMeshes()) && (!tinSurfaceProxy || !thisEntityCapture->hasTriangles()))
		{
			ret = OdGsBaseMaterialView::doDraw(i, pDrawable) || ret;
		}
		activeGeometryCapture = previousCapture;
	}
	else
	{
		ret = OdGsBaseMaterialView::doDraw(i, pDrawable);
	}
	collector->popDrawContext(ctx.get());

	const bool hasTinSurfaceFaces = tinSurfaceProxy && thisEntityCapture->hasTriangles();

	// ===== CHECK GEOMETRY EXTRACTION =====
	if (ctx && !pEntity.isNull())
	{
		if (ctx->hasMeshes() || hasTinSurfaceFaces)
		{
			stats.entitiesWithGeometry++;
		}
		else
		{
			stats.entitiesWithoutGeometry++;
			if (isProxy)
			{
				logProxyWithoutRenderableGeometry(pEntity, info, replayStoredProxyGraphics, ret);
			}

		}
	}

	if (ctx && (ctx->hasMeshes() || hasTinSurfaceFaces))
	{
		// This stack frame should create a layer with actual geometry

		if (parentLayer) {
			collector->createLayer(parentLayer.id, parentLayer.name, {}, {});
		}

		if (tinSurfaceProxy && hasTinSurfaceFaces)
		{
			if (ctx->hasMeshes())
			{
				auto meshes = ctx->extractMeshes(collector->getLayerTransform(parentLayer.id).inverse());
				collector->addMeshes(parentLayer.id, meshes);
			}

			thisEntityCapture->applyFaceLayers(collector, parentLayer.id, entityLayer.id);
			setEntityMetadata(parentLayer.id, handleMetaValue, pEntity, info, thisEntityCapture);
			thisEntityCapture->clearTriangles();
		}
		else
		{
			if (!collector->hasLayer(entityLayer.id)) {
				auto bounds = ctx->getBounds();
				auto m = repo::lib::RepoMatrix::translate(bounds.min());
				collector->createLayer(entityLayer.id, entityLayer.name, parentLayer.id, m);
			}

			if (ctx->hasMeshes())
			{
				auto meshes = ctx->extractMeshes(collector->getLayerTransform(entityLayer.id).inverse());
				collector->addMeshes(entityLayer.id, meshes);
			}

			setEntityMetadata(entityLayer.id, handleMetaValue, pEntity, info, nullptr);
		}
	}
	return ret;
}

void DataProcessorDwg::processTriangleOut(const OdInt32* p3Vertices, const OdGeVector3d* pNormal)
{
	if (!activeGeometryCapture)
	{
		DataProcessor::processTriangleOut(p3Vertices, pNormal);
		return;
	}

	const auto pVertexDataList = vertexDataList();
	activeGeometryCapture->addTriangle(
		collector,
		toRepoVector(pVertexDataList[p3Vertices[0]]),
		toRepoVector(pVertexDataList[p3Vertices[1]]),
		toRepoVector(pVertexDataList[p3Vertices[2]]));
}

void DataProcessorDwg::polygonOut(OdInt32 numPoints, const OdGePoint3d* vertexList, const OdGeVector3d* pNormal)
{
	if (!activeGeometryCapture)
	{
		OdGiGeometrySimplifier::polygonOut(numPoints, vertexList, pNormal);
		return;
	}

	if (numPoints < 3) return;

	for (OdInt32 i = 1; i + 1 < numPoints; ++i)
	{
		activeGeometryCapture->addTriangle(
			collector,
			toRepoVector(vertexList[0]),
			toRepoVector(vertexList[i]),
			toRepoVector(vertexList[i + 1]));
	}
}

void DataProcessorDwg::shellProc(
	OdInt32 numVertices,
	const OdGePoint3d* vertexList,
	OdInt32 faceListSize,
	const OdInt32* faceList,
	const OdGiEdgeData* pEdgeData,
	const OdGiFaceData* pFaceData,
	const OdGiVertexData* pVertexData)
{
	OdGiGeometrySimplifier::shellProc(
		numVertices,
		vertexList,
		faceListSize,
		faceList,
		pEdgeData,
		pFaceData,
		pVertexData);
}

void DataProcessorDwg::meshProc(
	OdInt32 numRows,
	OdInt32 numColumns,
	const OdGePoint3d* vertexList,
	const OdGiEdgeData* pEdgeData,
	const OdGiFaceData* pFaceData,
	const OdGiVertexData* pVertexData)
{
	OdGiGeometrySimplifier::meshProc(
		numRows,
		numColumns,
		vertexList,
		pEdgeData,
		pFaceData,
		pVertexData);
}

void DataProcessorDwg::tristripProc(
	OdInt32 numVertices,
	const OdGePoint3d* vertexList,
	OdInt32 stripListSize,
	const OdInt32* stripList,
	const OdGiEdgeData* pEdgeData,
	const OdGiVertexData* pVertexData)
{
	if (!activeGeometryCapture)
	{
		OdGiGeometrySimplifier::tristripProc(
			numVertices,
			vertexList,
			stripListSize,
			stripList,
			pEdgeData,
			pVertexData);
		return;
	}

	const OdInt32 count = stripList && stripListSize > 0 ? stripListSize : numVertices;
	if (count < 3) return;

	auto pointAt = [&](OdInt32 index) -> const OdGePoint3d& {
		return stripList && stripListSize > 0 ? vertexList[stripList[index]] : vertexList[index];
	};

	for (OdInt32 i = 0; i + 2 < count; ++i)
	{
		if ((i % 2) == 0)
		{
			activeGeometryCapture->addTriangle(
				collector,
				toRepoVector(pointAt(i)),
				toRepoVector(pointAt(i + 1)),
				toRepoVector(pointAt(i + 2)));
		}
		else
		{
			activeGeometryCapture->addTriangle(
				collector,
				toRepoVector(pointAt(i + 1)),
				toRepoVector(pointAt(i)),
				toRepoVector(pointAt(i + 2)));
		}
	}
}

void DataProcessorDwg::processPolylineOut(OdInt32 numPoints, const OdInt32* vertexIndexList)
{
	if (activeGeometryCapture && numPoints == 4)
	{
		std::vector<repo::lib::RepoVector3D64> points;
		points.reserve(numPoints);

		const auto pVertexDataList = vertexDataList();
		for (OdInt32 i = 0; i < numPoints; ++i)
		{
			points.push_back(toRepoVector(pVertexDataList[vertexIndexList[i]]));
		}

		if (activeGeometryCapture->addPolyline(collector, points)) return;
	}

	DataProcessor::processPolylineOut(numPoints, vertexIndexList);
}

void DataProcessorDwg::processPolylineOut(OdInt32 numPoints, const OdGePoint3d* vertexList)
{
	if (activeGeometryCapture && numPoints == 4)
	{
		std::vector<repo::lib::RepoVector3D64> points;
		points.reserve(numPoints);

		for (OdInt32 i = 0; i < numPoints; ++i)
		{
			points.push_back(toRepoVector(vertexList[i]));
		}

		if (activeGeometryCapture->addPolyline(collector, points)) return;
	}

	DataProcessor::processPolylineOut(numPoints, vertexList);
}

void DataProcessorDwg::convertTo3DRepoColor(OdCmEntityColor& color, repo::lib::repo_color3d_t& out)
{
	switch (color.colorMethod())
	{
	case OdCmEntityColor::ColorMethod::kByBlock:
	case OdCmEntityColor::ColorMethod::kByLayer:
	case OdCmEntityColor::ColorMethod::kByPen:
		// Currently no special handling is needed for these
		break;

	case OdCmEntityColor::ColorMethod::kByDgnIndex:
	case OdCmEntityColor::ColorMethod::kByACI:
		color.setTrueColor();
		break;

	case OdCmEntityColor::ColorMethod::kForeground:
		color = OdCmEntityColor(255, 255, 255);
		break;
	}

	out.r = color.red() / 255.0f;
	out.g = color.green() / 255.0f;
	out.b = color.blue() / 255.0f;
}


void DataProcessorDwg::convertTo3DRepoMaterial(
	OdGiMaterialItemPtr prevCache,
	OdDbStub* materialId,
	const OdGiMaterialTraitsData& materialData,
	MaterialColours& matColors,
	repo::lib::repo_material_t& material)
{
	DataProcessor::convertTo3DRepoMaterial(prevCache, materialId, materialData, matColors, material);

	// The Gs superclass supercedes colour data from the material, unless the
	// override flag is set.

	auto traits = effectiveTraits();
	auto deviceColor = traits.trueColor();

	convertTo3DRepoColor(matColors.colorDiffuseOverride ? matColors.colorDiffuse : deviceColor, material.diffuse);
	convertTo3DRepoColor(matColors.colorSpecularOverride ? matColors.colorSpecular : deviceColor, material.specular);

	// For DWGs, we don't set ambient or emissive properties of materials

	material.shininessStrength = 0;
}

void DataProcessorDwg::setMode(OdGsView::RenderMode mode)
{
	OdGsBaseVectorizeView::m_renderMode = kGouraudShaded;
	m_regenerationType = kOdGiRenderCommand;
	OdGiGeometrySimplifier::m_renderMode = OdGsBaseVectorizeView::m_renderMode;
}

std::string DataProcessorDwg::getClassDisplayName(OdDbEntityPtr entity, const ProxyInfo& info)
{
	// This method is used to get a user friendly version of the entity type to
	// display in the tree. For example, AcDb3dSolid -> 3D Solid.

	// ODA does not have inbuilt functionality for this, so we convert the class
	// name based on the potential inheritance,
	// https://docs.opendesign.com/td_api_cpp/OdDbEntity.html

	// Some of the entries below will never actually appear in the tree. For
	// example, the Block Start and Block End are database records that are
	// not geometric entities in their own right. AdDbCurve will always the name
	// of its subclass. Block References have their name overridden with the
	// Block's name in doDraw.
	// They are included here for completeness, to indicate they are not
	// 'missing'.

	auto className = convertToStdString(entity->isA()->name());
	const static std::unordered_map<std::string, std::string> classToDisplayName
	{
		{"AcDb3dSolid", "3D Solid"},
		{"AcDbArcAlignedText", "Arc-Aligned Text"},
		{"AcDbAssocProjectedEntityPersSubentIdHolder", "Entity Id Holder"},
		{"AcDbBlockBegin", "Block Begin"},
		{"AcDbBlockEnd", "Block End"},
		{"AcDbBlockReference", "Block Reference"},
		{"AcDbBody", "Body"},
		{"AcDbCamera", "Camera"},
		{"AcDbCurve", "Curve"},
		{"AcDb2dPolyline", "2D Polyline"},
		{"AcDb3dPolyline", "3D Polyline"},
		{"AcDbArc", "Arc"},
		{"AcDbCircle", "Circle"},
		{"AcDbEllipse", "Ellipse"},
		{"AcDbLeader", "Leader"},
		{"AcDbLine", "Line"},
		{"AcDbPolyline", "Polyline"},
		{"AcDbRay", "Ray"},
		{"AcDbSpline", "Spline"},
		{"AcDbXline", "XLine"}, // Infinity line
		{"AcDbDimension", "Dimension"},
		{"AcDb2LineAngularDimension", "2 Line Angular Dimension"},
		{"AcDb3PointAngularDimension", "3 Point Angular Dimension"},
		{"AcDbAlignedDimension", "Aligned Dimension"},
		{"AcDbArcDimension", "Arc Dimension"},
		{"AcDbDiametricDimension", "Diametric Dimension"},
		{"AcDbOrdinateDimension", "Ordinate Dimension"},
		{"AcDbRadialDimension", "Radial Dimension"},
		{"AcDbRadialDimensionLarge", "Large Radial Dimension"},
		{"AcDbRotatedDimension", "Rotated Dimension"},
		{"AcDbFace", "Face"},
		{"AcDbFaceRecord", "Face Record"},
		{"AcDbFcf", "Feature Control Frame"},
		{"AcDbFrame", "Frame"},
		{"AcDbGeoPositionMarker", "Geographic Location"},
		{"AcDbHatch", "Hatch"},
		{"AcDbImage", "Image"},
		{"AcDbRasterImage", "Raster Image"},
		{"AcDbLight", "Light"},
		{"AcDbMLeader", "Multileader"},
		{"AcDbMPolygon", "Polygon"},
		{"AcDbMText", "MText"},
		{"AcDbMline", "Line"},
		{"AcDbNavisworksReference", "Navisworks Reference"},
		{"AcDbPoint", "Point"},
		{"AcDbPointCloud", "Point Cloud"},
		{"AcDbPointCloudEx", "Point Cloud"},
		{"AcDbPolyFaceMesh", "Poly Face Mesh"},
		{"AcDbPolygonMesh", "Polygon Mesh"},
		{"AcDbProxyEntity", "Proxy"},
		{"AcDbRegion", "Region"},
		{"AcDbSection", "Section"},
		{"AcDbSequenceEnd", "Sequence End"},
		{"AcDbShape", "Shape"},
		{"AcDbSolid", "Solid"},
		{"AcDbSubDMesh", "Subdivision Mesh"},
		{"AcDbSurface", "Surface"},
		{"AcDbExtrudedSurface", "Extruded Surface"},
		{"AcDbLoftedSurface", "Lofted Surface"},
		{"AcDbNurbSurface", "Nurb Surface"},
		{"AcDbPlaneSurface", "Plane Surface"},
		{"AcDbRevolvedSurface", "Revolved Surface"},
		{"AcDbSweptSurface", "Swept Surface"},
		{"AcDbText", "Text"},
		{"AcDbAttribute", "Attribute"},
		{"AcDbAttributeDefinition", "Attribute Definition"},
		{"AcDbTrace", "Trace"},
		{"AcDbUnderlayReference", "Underlay Reference"},
		{"AcDbDgnReference", "DGN Reference"},
		{"AcDbDwfReference", "DWF Reference"},
		{"AcDbPdfReference", "PDF Reference"},
		{"AcDbVertex", "Vertex"},
		{"AcDb2dVertex", "2D Vertex"},
		{"AcDb3dPolylineVertex", "3D Polyline Vertex"},
		{"AcDbPolyFaceMeshVertex", "Poly Face Mesh Vertex"},
		{"AcDbPolygonMeshVertex", "Polygon Mesh Vertex"},
		{"AcDbViewBorder", "View Border"},
		{"AcDbViewRepImage", "View Image"},
		{"AcDbViewSymbol", "View Symbol"},
		{"AcDbDetailSymbol", "Detail Symbol"},
		{"AcDbSectionSymbol", "Section Symbol"},
		{"AcDbViewport", "Viewport"},
		{"RText", "RText"},
	};

	if (info.isProxy() && info.matchedHandler)
	{
		std::string name;
		if (info.matchedHandler->getDisplayName(info.originalClass, name))
			return name;
	}

	auto it = classToDisplayName.find(className);
	if (it != classToDisplayName.end())
	{
		return it->second;
	}
	else
	{
		return className;
	}
}
