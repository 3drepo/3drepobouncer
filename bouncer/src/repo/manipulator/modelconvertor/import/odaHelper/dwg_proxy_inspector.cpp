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

#include "dwg_proxy_inspector.h"
#include "helper_functions.h"

#include <OdString.h>
#include <toString.h>
#include <repo_log.h>
#include <DbEntity.h>
#include <CmColorBase.h>

using namespace repo::manipulator::modelconvertor::odaHelper;

bool DwgProxyInspector::getProxyInfo(OdDbEntityPtr entity, ProxyInfo& info)
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
		repoWarning << "Failed to get proxy class: " << convertToStdString(e.description());
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

	return true;
}

void DwgProxyInspector::ensureProxyXData(ProxyInfo& info)
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
	}
	catch (OdError& e)
	{
		repoWarning << "XData access failed: " << convertToStdString(e.description());
	}
	catch (...)
	{
		repoWarning << "XData access failed with unknown exception";
	}
}

void DwgProxyInspector::ensureProxyExtensionDictionary(ProxyInfo& info)
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
		repoWarning << "Proxy extension dictionary access failed";
	}
}

bool DwgProxyInspector::drawStoredProxyGraphics(OdDbEntityPtr pEntity, const ProxyInfo& info, OdGiWorldDraw* worldDraw)
{
	if (pEntity.isNull() || !info.isProxy()) return false;

	if (!info.hasFullGraphics()) return false;
	if (info.graphicsPE.isNull()) return false;

	try
	{
		return info.graphicsPE->worldDraw(pEntity, worldDraw);
	}
	catch (OdError& e)
	{
		repoWarning << "Stored proxy graphics replay failed: " << convertToStdString(e.description());
	}
	catch (...)
	{
		repoWarning << "Stored proxy graphics replay failed with unknown error";
	}

	return false;
}

std::unordered_map<std::string, repo::lib::RepoVariant> DwgProxyInspector::getProxyEntityMetadata(OdDbEntityPtr pEntity, ProxyInfo& info)
{
	std::unordered_map<std::string, repo::lib::RepoVariant> metadata;
	if (!info.isProxy()) return metadata;

	// Metadata is the one path that genuinely needs both of the expensive reads.
	ensureProxyXData(info);
	ensureProxyExtensionDictionary(info);

	std::string handle = "Unknown";
	try { handle = convertToStdString(toString(pEntity->objectId().getHandle())); }
	catch (...) {}

	try
	{
		// First ask ODA Common Data Access for palette-style properties. When a
		// native object enabler is available this can include custom object data;
		// otherwise it normally returns the proxy/general entity properties.
		addProxyBasicMetadata(pEntity, info, metadata);
		addProxyGeneralMetadata(pEntity, metadata);
		addProxyGeometryMetadata(pEntity, metadata);
		addProxyXDataMetadata(info, metadata);
		extractTextPropertiesFromProxy(info.entity, metadata);
	}
	catch (OdError& e)
	{
		repoWarning << "Failed to read proxy metadata for entity [" << handle << "]: " << convertToStdString(e.description());
	}
	catch (...)
	{
		repoWarning << "Failed to read proxy metadata for entity [" << handle << "]: unknown error";
	}

	return metadata;
}

void DwgProxyInspector::addProxyBasicMetadata(OdDbEntityPtr pEntity, const ProxyInfo& info, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
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
}

void DwgProxyInspector::setMetadataIfMissing(
	std::unordered_map<std::string, repo::lib::RepoVariant>& metadata,
	const std::string& key,
	const repo::lib::RepoVariant& value)
{
	if (metadata.find(key) == metadata.end())
	{
		metadata[key] = value;
	}
}

void DwgProxyInspector::addProxyGeneralMetadata(OdDbEntityPtr pEntity, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
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

	setMetadataIfMissing(metadata, "General::Layer", convertToStdString(toString(pEntity->layer())));
	setMetadataIfMissing(metadata, "General::True Color", colorToString(pEntity->color()));
	setMetadataIfMissing(metadata, "General::Linetype", convertToStdString(toString(pEntity->linetype())));
	setMetadataIfMissing(metadata, "General::Linetype scale", pEntity->linetypeScale());
	setMetadataIfMissing(metadata, "General::Lineweight", lineWeightToString(pEntity->lineWeight()));
	setMetadataIfMissing(metadata, "General::Visibility", pEntity->visibility() == OdDb::kInvisible ? std::string("Invisible") : std::string("Visible"));
}

void DwgProxyInspector::addProxyGeometryMetadata(OdDbEntityPtr pEntity, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
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

void DwgProxyInspector::addProxyXDataMetadata(const ProxyInfo& info, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	if (!info.xData.isNull())
	{
		extractXDataProperties(info.xData, metadata);
	}
}

void DwgProxyInspector::extractXDataProperties(OdResBufPtr pRb, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
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

void DwgProxyInspector::extractTextPropertiesFromProxy(OdDbProxyEntityPtr proxyEntity, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
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
