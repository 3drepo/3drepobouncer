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

DataProcessorDwg::~DataProcessorDwg()
{
	// This exists so we can use unique_ptr with a forward declaration of DwgDrawContext
	printDiagnostics();
}


bool DataProcessorDwg::isProxyEntity(OdDbEntityPtr pEntity)
{
	if (pEntity.isNull()) return false;
	return convertToStdString(pEntity->isA()->name()) == "AcDbProxyEntity";
}
bool DataProcessorDwg::isTinSurfaceProxy(OdDbEntityPtr pEntity)
{
	if (!isProxyEntity(pEntity)) return false;

	auto originalClass = getProxyOriginalClassName(pEntity);
	return originalClass == "AeccDbSurfaceTin" ||
		originalClass == "AeccDbTinSurface" ||
		originalClass.find("SurfaceTin") != std::string::npos ||
		originalClass.find("TinSurface") != std::string::npos;
}
bool DataProcessorDwg::shouldReplayStoredProxyGraphics(OdDbEntityPtr pEntity)
{
	return isProxyEntity(pEntity);
}
std::string DataProcessorDwg::getProxyOriginalClassName(OdDbEntityPtr pEntity)
{
	if (pEntity.isNull() || !isProxyEntity(pEntity)) return "";

	try {
		OdDbProxyEntityPtr proxyEntity = OdDbProxyEntity::cast(pEntity);
		if (!proxyEntity.isNull())
		{
			OdString originalClassName = proxyEntity->originalClassName();
			if (!originalClassName.isEmpty())
			{
				return convertToStdString(originalClassName);
			}

			OdString appDescription = proxyEntity->applicationDescription();
			if (!appDescription.isEmpty())
			{
				return convertToStdString(appDescription);
			}
		}
	}
	catch (OdError& e) {
		repoTrace << "Failed to get proxy class: " << convertToStdString(e.description());
	}

	return "Unknown";
}
std::unordered_map<std::string, std::string> DataProcessorDwg::getProxyMetadata(OdDbEntityPtr pEntity)
{
	std::unordered_map<std::string, std::string> metadata;

	if (pEntity.isNull()) return metadata;

	try {
		// Basic entity information
		metadata["EntityType"] = convertToStdString(pEntity->isA()->name());
		metadata["Handle"] = convertToStdString(toString(pEntity->objectId().getHandle()));
		metadata["IsDBRO"] = pEntity->isDBRO() ? "true" : "false";

		// Proxy-specific information
		if (isProxyEntity(pEntity))
		{
			OdDbProxyEntityPtr proxyEntity = OdDbProxyEntity::cast(pEntity);
			if (!proxyEntity.isNull())
			{
				// Original class name
				OdString originalClass = proxyEntity->originalClassName();
				if (!originalClass.isEmpty())
				{
					metadata["Original Class"] = convertToStdString(originalClass);
				}
				// Application
				auto appType = detectApplicationType(pEntity);
				if (!appType.empty())
				{
					metadata["Application"] = appType;
				}
				// Application description
				OdString appDesc = proxyEntity->applicationDescription();
				if (!appDesc.isEmpty())
				{
					metadata["Application Description"] = convertToStdString(appDesc);
				}

				// Proxy flags and saved graphics state
				int flags = proxyEntity->proxyFlags();
				metadata["ProxyFlags"] = std::to_string(flags);
				auto graphicsType = proxyEntity->graphicsMetafileType();
				metadata["GraphicsMetafileType"] = graphicsType == OdDbProxyEntity::kFullGraphics ? "Full Graphics" :
					graphicsType == OdDbProxyEntity::kBoundingBox ? "Bounding Box" : "No Metafile";
				metadata["HasProxyGraphics"] = graphicsType == OdDbProxyEntity::kFullGraphics ? "true" : "false";
			}
		}

		// Layer information
		try {
			metadata["Layer"] = convertToStdString(toString(pEntity->layer()));
		}
		catch (...) {}

		// Color information
		try {
			OdCmColor color = pEntity->color();
			metadata["Color"] = std::to_string(color.colorIndex());
		}
		catch (...) {}

		// Linetype
		try {
			metadata["Linetype"] = convertToStdString(toString(pEntity->linetype()));
		}
		catch (...) {}

		// Lineweight
		try {
			metadata["Lineweight"] = convertToStdString(toString(pEntity->lineWeight()));
		}
		catch (...) {}

		// Bounding box
		try {
			OdGeExtents3d extents;
			if (pEntity->getGeomExtents(extents) == eOk)
			{
				auto min = extents.minPoint();
				auto max = extents.maxPoint();
				metadata["BoundsMin"] = "(" + std::to_string(min.x) + ", " +
					std::to_string(min.y) + ", " +
					std::to_string(min.z) + ")";
				metadata["BoundsMax"] = "(" + std::to_string(max.x) + ", " +
					std::to_string(max.y) + ", " +
					std::to_string(max.z) + ")";
			}
		}
		catch (...) {}

		// XData - application-specific metadata
		try {
			OdResBufPtr pRb = pEntity->xData();
			if (!pRb.isNull())
			{
				int xdataIndex = 0;
				std::string currentApp = "";

				for (; !pRb.isNull(); pRb = pRb->next())
				{
					int resType = pRb->restype();

					if (resType == OdResBuf::kDxfRegAppName)  // 1001
					{
						currentApp = convertToStdString(pRb->getString());
						metadata["XData_App_" + std::to_string(xdataIndex)] = currentApp;
					}
					else if (!currentApp.empty())
					{
						// Store XData values with app prefix
						std::string key = "XData_" + currentApp + "_" + std::to_string(resType);

						switch (resType)
						{
						case OdResBuf::kDxfXdAsciiString:  // 1000
							metadata[key] = convertToStdString(pRb->getString());
							break;
						case OdResBuf::kDxfXdInteger16:    // 1070
							metadata[key] = std::to_string(pRb->getInt16());
							break;
						case OdResBuf::kDxfXdInteger32:    // 1071
							metadata[key] = std::to_string(pRb->getInt32());
							break;
						case OdResBuf::kDxfXdReal:         // 1040
							metadata[key] = std::to_string(pRb->getDouble());
							break;
						case OdResBuf::kDxfXdDist:         // 1041
							metadata[key] = std::to_string(pRb->getDouble());
							break;
						default:
							metadata[key] = "Type:" + std::to_string(resType);
							break;
						}
					}
					xdataIndex++;
				}
			}
		}
		catch (OdError& e) {
			metadata["XData_Error"] = convertToStdString(e.description());
		}

		// Extended dictionary (if any)
		try {
			OdDbObjectId extDictId = pEntity->extensionDictionary();
			if (!extDictId.isNull())
			{
				metadata["HasExtensionDictionary"] = "true";

				// Open the extension dictionary
				OdDbDictionaryPtr pExtDict = extDictId.safeOpenObject();
				if (!pExtDict.isNull())
				{
					// Iterate through all entries in the dictionary
					OdDbDictionaryIteratorPtr pDictIter = pExtDict->newIterator();
					int dictEntryIndex = 0;

					for (; !pDictIter->done(); pDictIter->next(), dictEntryIndex++)
					{
						try {
							// Get the entry name (key)
							OdString entryName = pDictIter->name();
							std::string entryNameStr = convertToStdString(entryName);

							// Get the entry object ID
							OdDbObjectId entryId = pDictIter->objectId();

							// Store the entry name
							std::string entryKey = "ExtDict_Entry_" + std::to_string(dictEntryIndex) + "_Name";
							metadata[entryKey] = entryNameStr;

							// Try to open and read the entry object
							if (!entryId.isNull())
							{
								OdDbObjectPtr pObj = entryId.safeOpenObject();
								if (!pObj.isNull())
								{
									// Store object class name
									std::string classKey = "ExtDict_Entry_" + std::to_string(dictEntryIndex) + "_Class";
									metadata[classKey] = convertToStdString(pObj->isA()->name());

									// Try to cast to dictionary (nested dictionaries)
									OdDbDictionaryPtr pNestedDict = OdDbDictionary::cast(pObj);
									if (!pNestedDict.isNull())
									{
										std::string sizeKey = "ExtDict_Entry_" + std::to_string(dictEntryIndex) + "_Size";
										metadata[sizeKey] = std::to_string(pNestedDict->numEntries());
									}

									// Try to read as XRecord (common storage type)
									OdDbXrecordPtr pXRec = OdDbXrecord::cast(pObj);
									if (!pXRec.isNull())
									{
										OdResBufPtr pRb = pXRec->rbChain();
										int xrecIndex = 0;

										for (; !pRb.isNull() && xrecIndex < 50; pRb = pRb->next(), xrecIndex++)
										{
											int resType = pRb->restype();
											std::string valueKey = "ExtDict_Entry_" + std::to_string(dictEntryIndex) +
												"_XRec_" + std::to_string(xrecIndex);

											try {
												switch (resType)
												{
												case OdResBuf::kDxfText:
												case OdResBuf::kDxfXTextString:
													metadata[valueKey] = convertToStdString(pRb->getString());
													break;
												case OdResBuf::kDxfInt8:
													metadata[valueKey] = std::to_string(pRb->getInt8());
													break;
												case OdResBuf::kDxfInt16:
													metadata[valueKey] = std::to_string(pRb->getInt16());
													break;
												case OdResBuf::kDxfInt32:
													metadata[valueKey] = std::to_string(pRb->getInt32());
													break;
												//case OdResBuf::kDxfDouble:
												case OdResBuf::kDxfReal:
													metadata[valueKey] = std::to_string(pRb->getDouble());
													break;
												case OdResBuf::kDxfBool:
													metadata[valueKey] = pRb->getBool() ? "true" : "false";
													break;
												/*case OdResBuf::kDxfObjectId:
													metadata[valueKey] = "ObjectId:" +
														convertToStdString(toString(pRb->getObjectId(OdDb::kForRead).getHandle()));
													break;*/
												/*case OdResBuf::kDxf3dPoint:
												{
													OdGePoint3d pt = pRb->getPoint3d();
													metadata[valueKey] = "Point(" + std::to_string(pt.x) + ", " +
														std::to_string(pt.y) + ", " +
														std::to_string(pt.z) + ")";
													break;
												}*/
												default:
													metadata[valueKey] = "ResType:" + std::to_string(resType);
													break;
												}
											}
											catch (...) {
												metadata[valueKey] = "Error reading ResType:" + std::to_string(resType);
											}
										}

										if (!pRb.isNull())
										{
											metadata["ExtDict_Entry_" + std::to_string(dictEntryIndex) + "_XRec_Truncated"] = "true";
										}
									}
								}
							}
						}
						catch (OdError& e) {
							metadata["ExtDict_Entry_" + std::to_string(dictEntryIndex) + "_Error"] =
								convertToStdString(e.description());
						}
						catch (...) {
							metadata["ExtDict_Entry_" + std::to_string(dictEntryIndex) + "_Error"] =
								"Unknown error reading entry";
						}
					}

					metadata["ExtDict_TotalEntries"] = std::to_string(dictEntryIndex);
				}
			}
			else
			{
				metadata["HasExtensionDictionary"] = "false";
			}
		}
		catch (...) {}

	}
	catch (OdError& e) {
		metadata["Error"] = convertToStdString(e.description());
	}
	catch (...) {
		metadata["Error"] = "Unknown exception reading metadata";
	}

	return metadata;
}
std::vector<std::string> DataProcessorDwg::getProxyXDataApps(OdDbEntityPtr pEntity)
{
	std::vector<std::string> apps;
	if (pEntity.isNull()) return apps;

	try {
		// Use xData() with nullptr to get all registered application names
		OdResBufPtr pRb = pEntity->xData();

		if (!pRb.isNull())
		{
			// XData is stored as a linked list of result buffers
			// Each app starts with a kDxfRegAppName (1001) entry
			for (; !pRb.isNull(); pRb = pRb->next())
			{
				if (pRb->restype() == OdResBuf::kDxfRegAppName)  // 1001 = App name
				{
					OdString appName = pRb->getString();
					if (!appName.isEmpty())
					{
						apps.push_back(convertToStdString(appName));
					}
				}
			}
		}
		else
		{
			repoTrace << "xData() returned null - trying database query method";

			// Method 2: Alternative - Query the database for registered apps
			if (pEntity->database() != nullptr)
			{
				OdDbObjectId entityId = pEntity->objectId();
				if (!entityId.isNull())
				{
					OdDbRegAppTablePtr pAppTable = pEntity->database()->getRegAppTableId().safeOpenObject();
					if (!pAppTable.isNull())
					{
						repoTrace << "Checking registered application table...";
						OdDbSymbolTableIteratorPtr pIter = pAppTable->newIterator();

						for (; !pIter->done(); pIter->step())
						{
							OdDbRegAppTableRecordPtr pApp = pIter->getRecord();
							if (!pApp.isNull())
							{
								OdString appName = pApp->getName();
								std::string appStr = convertToStdString(appName);

								// Check if this entity actually has data for this app
								OdResBufPtr pAppData = pEntity->xData(appName);
								if (!pAppData.isNull())
								{
									repoTrace << "  Entity has XData for: " << appStr;
									apps.push_back(appStr);
								}
							}
						}
					}
				}
			}
		}
	}
	catch (OdError& e) {
		repoTrace << "XData access failed: " << convertToStdString(e.description());
	}
	catch (...) {
		repoTrace << "XData access failed with unknown exception";
	}

	return apps;
}
std::string DataProcessorDwg::detectApplicationType(OdDbEntityPtr pEntity)
{
	if (pEntity.isNull()) return "";

	if (isProxyEntity(pEntity))
	{
		std::string originalClass = getProxyOriginalClassName(pEntity);
		if (!originalClass.empty() && originalClass != "Unknown")
		{
			if (originalClass.find("Aecc") != std::string::npos)
				return "Civil3D (" + originalClass + ")";
			if (originalClass.find("AcPp") != std::string::npos)
				return "Plant3D (" + originalClass + ")";
			if (originalClass.find("Civil") != std::string::npos)
				return "Civil3D (" + originalClass + ")";
			if (originalClass.find("Plant") != std::string::npos)
				return "Plant3D (" + originalClass + ")";

			return "CustomApp (" + originalClass + ")";
		}
	}

	auto xdataApps = getProxyXDataApps(pEntity);
	for (const auto& app : xdataApps)
	{
		if (app.find("Aecc") != std::string::npos)
			return "Civil3D (XData)";
		if (app.find("AcPp") != std::string::npos)
			return "Plant3D (XData)";
	}

	return "";
}

bool DataProcessorDwg::drawStoredProxyGraphics(OdDbEntityPtr pEntity)
{
	if (pEntity.isNull() || !shouldReplayStoredProxyGraphics(pEntity)) return false;

	OdDbProxyEntityPtr proxyEntity = OdDbProxyEntity::cast(pEntity);
	if (proxyEntity.isNull()) return false;

	if (proxyEntity->graphicsMetafileType() != OdDbProxyEntity::kFullGraphics)
	{
		repoTrace << "Proxy has no full graphics metafile: "
			<< getProxyOriginalClassName(pEntity);
		return false;
	}

	OdDbEntityWithGrDataPEPtr graphics = OdDbEntityWithGrDataPE::cast(pEntity);
	if (graphics.isNull())
	{
		repoTrace << "No stored graphics protocol extension for proxy: "
			<< getProxyOriginalClassName(pEntity);
		return false;
	}

	try
	{
		repoInfo << "Replaying stored proxy graphics for "
			<< getProxyOriginalClassName(pEntity);
		return graphics->worldDraw(pEntity, this);
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

	repoInfo << "===============================================================";
	repoInfo << "||        DWG IMPORT DIAGNOSTIC REPORT                       ||";
	repoInfo << "===============================================================";
	repoInfo << "  Total entities:                " << stats.totalEntities;
	repoInfo << "    With geometry:               " << stats.entitiesWithGeometry;
	repoInfo << "    Without geometry:            " << stats.entitiesWithoutGeometry;
	repoInfo << "===============================================================";
	repoInfo << "  Civil3D entities:               " << stats.civil3dEntities;
	repoInfo << "  Plant3D entities:               " << stats.plant3dEntities;
	repoInfo << "  Proxy entities:                 " << stats.proxyEntities;

	if (!stats.entityTypeCount.empty() && (stats.proxyEntities > 0 || stats.civil3dEntities > 0 || stats.plant3dEntities > 0))
	{
		repoInfo << "===============================================================";
		repoInfo << "|| CUSTOM ENTITY TYPES FOUND                                 ||";
		repoInfo << "===============================================================";
		for (const auto& [type, count] : stats.entityTypeCount)
		{
			if (type.find("Aecc") != std::string::npos ||
				type.find("AcPp") != std::string::npos ||
				type == "AcDbProxyEntity")
			{
				repoInfo << "  " << type << ": " << count;
			}
		}
	}

	repoInfo << "===============================================================\n";
}

void DataProcessorDwg::logProxyWithoutRenderableGeometry(
	OdDbEntityPtr pEntity,
	bool replayedStoredProxyGraphics,
	bool replayReturnedGeometry)
{
	if (!isProxyEntity(pEntity)) return;

	std::string handle = "Unknown";
	try
	{
		handle = convertToStdString(toString(pEntity->objectId().getHandle()));
	}
	catch (...) {}

	if (!loggedProxyGeometryFailures.insert(handle).second) return;

	std::string layer = "Unknown";
	try
	{
		layer = convertToStdString(toString(pEntity->layer()));
	}
	catch (...) {}

	std::string originalClass = getProxyOriginalClassName(pEntity);
	std::string originalDxfName;
	std::string appDescription;
	std::string graphicsTypeName = "Unknown";
	std::string reason = "No geometry was produced by the vectorizer";
	bool hasGraphicsProtocolExtension = false;

	OdDbProxyEntityPtr proxyEntity = OdDbProxyEntity::cast(pEntity);
	if (!proxyEntity.isNull())
	{
		OdString dxfName = proxyEntity->originalDxfName();
		if (!dxfName.isEmpty()) originalDxfName = convertToStdString(dxfName);

		OdString description = proxyEntity->applicationDescription();
		if (!description.isEmpty()) appDescription = convertToStdString(description);

		auto graphicsType = proxyEntity->graphicsMetafileType();
		if (graphicsType == OdDbProxyEntity::kNoMetafile)
		{
			graphicsTypeName = "No Metafile";
			reason = "Proxy entity has no saved graphics metafile";
		}
		else if (graphicsType == OdDbProxyEntity::kBoundingBox)
		{
			graphicsTypeName = "Bounding Box";
			reason = "Proxy entity only has bounding-box proxy graphics";
		}
		else if (graphicsType == OdDbProxyEntity::kFullGraphics)
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
	}

	try
	{
		hasGraphicsProtocolExtension = !OdDbEntityWithGrDataPE::cast(pEntity).isNull();
	}
	catch (...) {}

	std::string xdataApps;
	try
	{
		auto apps = getProxyXDataApps(pEntity);
		for (size_t i = 0; i < apps.size(); ++i)
		{
			if (i > 0) xdataApps += ",";
			xdataApps += apps[i];
		}
	}
	catch (...) {}

	repoWarning << "[DWG_PROXY_NO_RENDERABLE_GEOMETRY] handle=" << handle
		<< " layer=\"" << layer << "\""
		<< " originalClass=\"" << originalClass << "\""
		<< " originalDxfName=\"" << originalDxfName << "\""
		<< " graphicsMetafile=\"" << graphicsTypeName << "\""
		<< " replayAttempted=" << (replayedStoredProxyGraphics ? "true" : "false")
		<< " replayReturned=" << (replayReturnedGeometry ? "true" : "false")
		<< " hasGraphicsPE=" << (hasGraphicsProtocolExtension ? "true" : "false")
		<< " reason=\"" << reason << "\"";

	if (!appDescription.empty())
	{
		repoWarning << "[DWG_PROXY_NO_RENDERABLE_GEOMETRY] handle=" << handle
			<< " applicationDescription=\"" << appDescription << "\"";
	}

	if (!xdataApps.empty())
	{
		repoWarning << "[DWG_PROXY_NO_RENDERABLE_GEOMETRY] handle=" << handle
			<< " xdataApps=\"" << xdataApps << "\"";
	}

	try
	{
		OdGeExtents3d extents;
		if (pEntity->getGeomExtents(extents) == eOk)
		{
			auto min = extents.minPoint();
			auto max = extents.maxPoint();
			repoWarning << "[DWG_PROXY_NO_RENDERABLE_GEOMETRY] handle=" << handle
				<< " boundsMin=(" << min.x << "," << min.y << "," << min.z << ")"
				<< " boundsMax=(" << max.x << "," << max.y << "," << max.z << ")";
		}
	}
	catch (...) {}
}
std::unordered_map<std::string, repo::lib::RepoVariant> DataProcessorDwg::getProxyEntityMetadata(OdDbEntityPtr pEntity)
{
	std::unordered_map<std::string, repo::lib::RepoVariant> metadata;
	if (!isProxyEntity(pEntity)) return metadata;

	auto setIfMissing = [&](const std::string& key, const repo::lib::RepoVariant& value) {
		if (metadata.find(key) == metadata.end())
		{
			metadata[key] = value;
		}
	};

	auto yesNo = [](bool value) {
		return value ? std::string("Yes") : std::string("No");
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

	try
	{
		OdDbProxyEntityPtr proxyEntity = OdDbProxyEntity::cast(pEntity);
		if (proxyEntity.isNull()) return metadata;

		// First ask ODA Common Data Access for palette-style properties. When a
		// native object enabler is available this can include custom object data;
		// otherwise it normally returns the proxy/general entity properties.
		extractEntityProperties(pEntity, metadata);

		std::string originalClass = convertToStdString(proxyEntity->originalClassName());
		if (originalClass.empty()) originalClass = getProxyOriginalClassName(pEntity);
		if (!originalClass.empty())
		{
			metadata["Proxy::Original Class"] = originalClass;
		}

		OdString originalDxfName = proxyEntity->originalDxfName();
		if (!originalDxfName.isEmpty())
		{
			metadata["Proxy::Original DXF Name"] = convertToStdString(originalDxfName);
		}

		OdString appDescription = proxyEntity->applicationDescription();
		if (!appDescription.isEmpty())
		{
			metadata["Proxy::Application Description"] = convertToStdString(appDescription);
		}

		auto appType = detectApplicationType(pEntity);
		if (!appType.empty())
		{
			metadata["Proxy::Application"] = appType;
		}

		auto graphicsType = proxyEntity->graphicsMetafileType();
		std::string graphicsTypeName = "No Metafile";
		if (graphicsType == OdDbProxyEntity::kBoundingBox)
		{
			graphicsTypeName = "Bounding Box";
		}
		else if (graphicsType == OdDbProxyEntity::kFullGraphics)
		{
			graphicsTypeName = "Full Graphics";
		}
		metadata["Proxy::Graphics Metafile Type"] = graphicsTypeName;
		metadata["Proxy::Has Full Graphics"] = yesNo(graphicsType == OdDbProxyEntity::kFullGraphics);

		int flags = proxyEntity->proxyFlags();
		metadata["Proxy::Flags"] = static_cast<int64_t>(flags);
		metadata["Proxy Flags::Erase Allowed"] = yesNo(proxyEntity->eraseAllowed());
		metadata["Proxy Flags::Transform Allowed"] = yesNo(proxyEntity->transformAllowed());
		metadata["Proxy Flags::Color Change Allowed"] = yesNo(proxyEntity->colorChangeAllowed());
		metadata["Proxy Flags::Layer Change Allowed"] = yesNo(proxyEntity->layerChangeAllowed());
		metadata["Proxy Flags::Linetype Change Allowed"] = yesNo(proxyEntity->linetypeChangeAllowed());
		metadata["Proxy Flags::Linetype Scale Change Allowed"] = yesNo(proxyEntity->linetypeScaleChangeAllowed());
		metadata["Proxy Flags::Visibility Change Allowed"] = yesNo(proxyEntity->visibilityChangeAllowed());
		metadata["Proxy Flags::Lineweight Change Allowed"] = yesNo(proxyEntity->lineWeightChangeAllowed());
		metadata["Proxy Flags::Material Change Allowed"] = yesNo(proxyEntity->materialChangeAllowed());

		setIfMissing("General::Layer", convertToStdString(toString(pEntity->layer())));
		setIfMissing("General::True Color", colorToString(pEntity->color()));
		setIfMissing("General::Linetype", convertToStdString(toString(pEntity->linetype())));
		setIfMissing("General::Linetype scale", pEntity->linetypeScale());
		setIfMissing("General::Lineweight", lineWeightToString(pEntity->lineWeight()));
		setIfMissing("General::Visibility", pEntity->visibility() == OdDb::kInvisible ? std::string("Invisible") : std::string("Visible"));

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

		OdResBufPtr pRb = pEntity->xData();
		if (!pRb.isNull())
		{
			extractXDataProperties(pRb, metadata);
		}

		OdDbObjectId extDictId = pEntity->extensionDictionary();
		if (!extDictId.isNull())
		{
			OdDbDictionaryPtr pExtDict = extDictId.safeOpenObject();
			if (!pExtDict.isNull())
			{
				extractCivil3DStoredProperties(pExtDict, metadata);
				extractPlant3DStoredProperties(pExtDict, metadata);
			}
		}

		extractTextPropertiesFromProxy(proxyEntity, metadata);

		// Preserve raw proxy storage details too. These are useful for custom
		// proxy classes where the native palette fields are not exposed by CDA.
		auto rawMetadata = getProxyMetadata(pEntity);
		for (const auto& item : rawMetadata)
		{
			if (!item.second.empty())
			{
				setIfMissing("Proxy Raw::" + item.first, item.second);
			}
		}
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
void DataProcessorDwg::addTinSurfaceComputedMetadata(
	OdDbEntityPtr pEntity,
	std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	if (!isTinSurfaceProxy(pEntity) || tinSurfaceTriangles.empty()) return;

	auto setIfMissing = [&](const std::string& key, const repo::lib::RepoVariant& value) {
		if (metadata.find(key) == metadata.end())
		{
			metadata[key] = value;
		}
	};

	std::unordered_set<std::string> uniquePoints;
	double minElevation = 0.0;
	double maxElevation = 0.0;
	bool hasElevation = false;

	auto pointKey = [](const repo::lib::RepoVector3D64& p) {
		const double keyScale = 1000000.0;
		return std::to_string(static_cast<long long>(p.x * keyScale)) + "," +
			std::to_string(static_cast<long long>(p.y * keyScale)) + "," +
			std::to_string(static_cast<long long>(p.z * keyScale));
	};

	for (const auto& triangle : tinSurfaceTriangles)
	{
		for (int i = 0; i < triangle.numVertices; ++i)
		{
			const auto& p = triangle.vertices[i];
			if (uniquePoints.insert(pointKey(p)).second)
			{
				if (!hasElevation)
				{
					minElevation = p.z;
					maxElevation = p.z;
					hasElevation = true;
				}
				else
				{
					minElevation = std::min(minElevation, p.z);
					maxElevation = std::max(maxElevation, p.z);
				}
			}
		}
	}

	if (!uniquePoints.empty() && metadata.find("Data::Number of Points") == metadata.end())
	{
		metadata["Data::Rendered Number of Points"] = static_cast<int64_t>(uniquePoints.size());
	}
	if (hasElevation)
	{
		auto roundElevation = [](double value) {
			return std::round(value * 1000.0) / 1000.0;
		};
		setIfMissing("Data::Minimum Elevation", roundElevation(minElevation));
		setIfMissing("Data::Maximum Elevation", roundElevation(maxElevation));
	}

	try
	{
		auto layerName = convertToStdString(toString(pEntity->layer()));
		auto name = layerName;
		auto separator = layerName.find_last_of('-');
		if (separator != std::string::npos && separator + 1 < layerName.size())
		{
			name = layerName.substr(separator + 1);
		}
		setIfMissing("Information::Name", name);
	}
	catch (...) {}

	setIfMissing("Information::Style", std::string("Contours and Triangles"));
	setIfMissing("Information::Material", std::string("ByLayer"));
	setIfMissing("Information::Show Tooltips", std::string("Yes"));
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
	OdDbEntityPtr pEntity)
{
	if (layerId.empty() || handleMetaValue.empty() || collector->hasMetadata(layerId)) return;

	std::unordered_map<std::string, repo::lib::RepoVariant> meta, metadata;
	meta["Entity Handle::Value"] = handleMetaValue;

	if (!pEntity.isNull())
	{
		if (isProxyEntity(pEntity))
		{
			metadata = getProxyEntityMetadata(pEntity);
		}
		else
		{
			extractEntityProperties(pEntity, metadata);
		}

		addTinSurfaceComputedMetadata(pEntity, metadata);
		removeDuplicateGeneralMetadata(metadata);

		for (const auto& [key, value] : metadata)
		{
			meta[key] = value;
		}
	}

	collector->setMetadata(layerId, meta);
}
void DataProcessorDwg::extractCivil3DStoredProperties(OdDbDictionaryPtr pDict, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	// Known Civil3D dictionary entries that might contain data
	std::vector<std::string> civil3dDicts = {
		"ACAD_XREC_ROUNDTRIP",  // Roundtrip data often has useful info
		"AeccDbSurfaceTin",
		"AeccDbAssembly",
		"AeccDbSubassembly",
		"AeccDbAlignment",
		"AeccDbAlignmentStationLabeling",
		"AeccDbProfileDataBandLabeling",
		"AeccDbAlignmentMinorStationLabeling",
		"AeccDbVAlignment",
		"AeccDbCorridor",
		"AeccDbSurface",
		"AeccDbFace",
		"AeccDbLotLine",
		"AeccDbGraphProfile",
		"AeccDbProfile"
	};

	for (const auto& dictName : civil3dDicts)
	{
		OdResult pStatus;
		OdDbObjectId entryId = pDict->getAt(OdString(dictName.c_str()), &pStatus);
		if (pStatus == eOk && !entryId.isNull())
		{
			OdDbXrecordPtr pXRec = OdDbXrecord::cast(entryId.safeOpenObject());
			if (!pXRec.isNull())
			{
				OdResBufPtr pRb = pXRec->rbChain();

				// Parse known Civil3D property patterns
				std::string propName = "";
				for (; !pRb.isNull(); pRb = pRb->next())
				{
					int resType = pRb->restype();

					// Civil3D often stores property names as strings followed by values
					if (resType == OdResBuf::kDxfText || resType == OdResBuf::kDxfXTextString)
					{
						std::string text = convertToStdString(pRb->getString());

						// Check if it's a property name
						if (text.find("Station") != std::string::npos ||
							text.find("Offset") != std::string::npos ||
							text.find("Elevation") != std::string::npos ||
							text.find("Grade") != std::string::npos)
						{
							propName = "Civil3D::" + text;
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
					else if (!propName.empty() && resType == OdResBuf::kDxfInt32)
					{
						metadata[propName] = (int64_t)pRb->getInt32();
						propName = "";
					}
				}
			}
		}
	}
}
void DataProcessorDwg::extractPlant3DStoredProperties(OdDbDictionaryPtr pDict, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	// Known Plant3D dictionary entries
	std::vector<std::string> plant3dDicts = {
		"ACAD_XREC_ROUNDTRIP",
		"AcPpDb3dPart",
		"AcPpDb3dSpecPart",
		"PartSizeProperties"
	};

	for (const auto& dictName : plant3dDicts)
	{
		OdResult pStatus;
		OdDbObjectId entryId = pDict->getAt(OdString(dictName.c_str()), &pStatus);
		if (pStatus == eOk && !entryId.isNull())
		{
			OdDbXrecordPtr pXRec = OdDbXrecord::cast(entryId.safeOpenObject());
			if (!pXRec.isNull())
			{
				OdResBufPtr pRb = pXRec->rbChain();

				// Parse known Plant3D property patterns
				std::string propName = "";
				for (; !pRb.isNull(); pRb = pRb->next())
				{
					int resType = pRb->restype();

					// Plant3D often stores: Tag, Service, Size, Spec
					if (resType == OdResBuf::kDxfText || resType == OdResBuf::kDxfXTextString)
					{
						std::string text = convertToStdString(pRb->getString());

						// Common Plant3D properties
						if (text.find("Tag") != std::string::npos ||
							text.find("Service") != std::string::npos ||
							text.find("Size") != std::string::npos ||
							text.find("NominalDiameter") != std::string::npos ||
							text.find("Spec") != std::string::npos ||
							text.find("PartFamily") != std::string::npos)
						{
							propName = "Plant3D::" + text;
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
				}
			}
		}
	}
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

	OdDbEntityPtr pEntity = OdDbEntity::cast(pDrawable);
	if (!pEntity.isNull())
	{
		// ===== DIAGNOSTIC TRACKING =====
		stats.totalEntities++;
		auto className = convertToStdString(pEntity->isA()->name());
		stats.entityTypeCount[className]++;
		if (isProxyEntity(pEntity))
		{
			stats.proxyEntities++;
			auto originalClass = getProxyOriginalClassName(pEntity);
			stats.entityTypeCount["Proxy-" + originalClass]++;
			if (originalClass.find("Aecc") != std::string::npos || originalClass.find("Civil") != std::string::npos)
			{
				stats.civil3dEntities++;
			}
			if (originalClass.find("AcPp") != std::string::npos || originalClass.find("Plant") != std::string::npos)
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
		auto entityName = getClassDisplayName(pEntity);

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

	bool tinSurfaceProxy = !pEntity.isNull() && isTinSurfaceProxy(pEntity);
	bool replayStoredProxyGraphics = !pEntity.isNull() && shouldReplayStoredProxyGraphics(pEntity);

	collector->pushDrawContext(ctx.get());
	bool ret = false;
	if (replayStoredProxyGraphics)
	{
		bool previousTinSurfaceProxyCapture = capturingTinSurfaceProxy;
		capturingTinSurfaceProxy = tinSurfaceProxy;
		if (tinSurfaceProxy)
		{
			tinSurfaceEdgeKeys.clear();
			tinSurfaceTriangles.clear();
			hasTinSurfaceFaceMaterial = false;
		}

		ret = drawStoredProxyGraphics(pEntity);
		if (!(ctx && ctx->hasMeshes()) && (!tinSurfaceProxy || tinSurfaceTriangles.empty()))
		{
			ret = OdGsBaseMaterialView::doDraw(i, pDrawable) || ret;
		}
		capturingTinSurfaceProxy = previousTinSurfaceProxyCapture;
	}
	else
	{
		ret = OdGsBaseMaterialView::doDraw(i, pDrawable);
	}
	collector->popDrawContext(ctx.get());

	const bool hasTinSurfaceFaces = tinSurfaceProxy && !tinSurfaceTriangles.empty();

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
			if (isProxyEntity(pEntity))
			{
				logProxyWithoutRenderableGeometry(pEntity, replayStoredProxyGraphics, ret);
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

			addTinSurfaceFaceLayers(parentLayer.id, entityLayer.id);
			setEntityMetadata(parentLayer.id, handleMetaValue, pEntity);
			tinSurfaceTriangles.clear();
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

			setEntityMetadata(entityLayer.id, handleMetaValue, pEntity);
		}
	}
	return ret;
}

bool DataProcessorDwg::addTinSurfaceTriangle(
	const repo::lib::RepoVector3D64& p0,
	const repo::lib::RepoVector3D64& p1,
	const repo::lib::RepoVector3D64& p2)
{
	if (!capturingTinSurfaceProxy) return false;

	auto samePoint = [](const repo::lib::RepoVector3D64& a, const repo::lib::RepoVector3D64& b) {
		return compare(a.x, b.x) == 0 &&
			compare(a.y, b.y) == 0 &&
			compare(a.z, b.z) == 0;
	};

	if (samePoint(p0, p1) || samePoint(p1, p2) || samePoint(p2, p0)) return false;

	GeometryCollector::Face triangle;
	triangle.push_back(p0);
	triangle.push_back(p1);
	triangle.push_back(p2);
	tinSurfaceTriangles.push_back(triangle);
	if (!hasTinSurfaceFaceMaterial)
	{
		tinSurfaceFaceMaterial = collector->getLastMaterial();
		hasTinSurfaceFaceMaterial = true;
	}

	addTinSurfaceEdge(p0, p1);
	addTinSurfaceEdge(p1, p2);
	addTinSurfaceEdge(p2, p0);
	return true;
}

bool DataProcessorDwg::addTinSurfaceEdge(
	const repo::lib::RepoVector3D64& p0,
	const repo::lib::RepoVector3D64& p1)
{
	if (!capturingTinSurfaceProxy) return false;

	auto samePoint = [](const repo::lib::RepoVector3D64& a, const repo::lib::RepoVector3D64& b) {
		return compare(a.x, b.x) == 0 &&
			compare(a.y, b.y) == 0 &&
			compare(a.z, b.z) == 0;
	};

	if (samePoint(p0, p1)) return false;

	auto pointKey = [](const repo::lib::RepoVector3D64& p) {
		const double keyScale = 1000000.0;
		return std::to_string(static_cast<long long>(p.x * keyScale)) + "," +
			std::to_string(static_cast<long long>(p.y * keyScale)) + "," +
			std::to_string(static_cast<long long>(p.z * keyScale));
	};

	auto key0 = pointKey(p0);
	auto key1 = pointKey(p1);
	auto edgeKey = key0 < key1 ? key0 + "|" + key1 : key1 + "|" + key0;
	if (!tinSurfaceEdgeKeys.insert(edgeKey).second) return false;

	auto edgeMaterial = repo::lib::repo_material_t::DefaultMaterial();
	edgeMaterial.diffuse = { 0.03f, 0.03f, 0.03f };
	edgeMaterial.ambient = edgeMaterial.diffuse;
	edgeMaterial.emissive = edgeMaterial.diffuse;
	edgeMaterial.lineWeight = 1.0f;
	edgeMaterial.isWireframe = true;
	collector->setMaterial(edgeMaterial);
	collector->addFace({ p0, p1 });
	return true;
}

void DataProcessorDwg::processTriangleOut(const OdInt32* p3Vertices, const OdGeVector3d* pNormal)
{
	if (!capturingTinSurfaceProxy)
	{
		DataProcessor::processTriangleOut(p3Vertices, pNormal);
		return;
	}

	const auto pVertexDataList = vertexDataList();
	addTinSurfaceTriangle(
		toRepoVector(pVertexDataList[p3Vertices[0]]),
		toRepoVector(pVertexDataList[p3Vertices[1]]),
		toRepoVector(pVertexDataList[p3Vertices[2]]));
}

void DataProcessorDwg::polygonOut(OdInt32 numPoints, const OdGePoint3d* vertexList, const OdGeVector3d* pNormal)
{
	if (!capturingTinSurfaceProxy)
	{
		OdGiGeometrySimplifier::polygonOut(numPoints, vertexList, pNormal);
		return;
	}

	if (numPoints < 3) return;

	for (OdInt32 i = 1; i + 1 < numPoints; ++i)
	{
		addTinSurfaceTriangle(
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
	if (capturingTinSurfaceProxy)
	{
		repoTrace << "TIN surface proxy shellProc vertices=" << numVertices
			<< " faceListSize=" << faceListSize;
	}

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
	if (capturingTinSurfaceProxy)
	{
		repoTrace << "TIN surface proxy meshProc rows=" << numRows
			<< " columns=" << numColumns;
	}

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
	if (!capturingTinSurfaceProxy)
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
			addTinSurfaceTriangle(
				toRepoVector(pointAt(i)),
				toRepoVector(pointAt(i + 1)),
				toRepoVector(pointAt(i + 2)));
		}
		else
		{
			addTinSurfaceTriangle(
				toRepoVector(pointAt(i + 1)),
				toRepoVector(pointAt(i)),
				toRepoVector(pointAt(i + 2)));
		}
	}
}

void DataProcessorDwg::addTinSurfaceFaceLayers(
	const std::string& parentLayerId,
	const std::string& sourceEntityId)
{
	for (size_t i = 0; i < tinSurfaceTriangles.size(); ++i)
	{
		auto faceLayerId = sourceEntityId + ":TIN_FACE:" + std::to_string(i);

		auto faceContext = collector->makeNewDrawContext();
		collector->pushDrawContext(faceContext.get());
		if (hasTinSurfaceFaceMaterial)
		{
			collector->setMaterial(tinSurfaceFaceMaterial);
		}
		collector->addFace(tinSurfaceTriangles[i]);
		collector->popDrawContext(faceContext.get());

		if (faceContext->hasMeshes())
		{
			auto bounds = faceContext->getBounds();
			auto m = repo::lib::RepoMatrix::translate(bounds.min());
			collector->createLayer(faceLayerId, "Face", parentLayerId, m);

			auto meshes = faceContext->extractMeshes(m.inverse());
			collector->addMeshes(faceLayerId, meshes);
		}
	}
}

bool DataProcessorDwg::addTinSurfaceTrianglePolyline(const std::vector<repo::lib::RepoVector3D64>& points)
{
	if (!capturingTinSurfaceProxy || points.size() != 4) return false;

	auto samePoint = [](const repo::lib::RepoVector3D64& a, const repo::lib::RepoVector3D64& b) {
		return compare(a.x, b.x) == 0 &&
			compare(a.y, b.y) == 0 &&
			compare(a.z, b.z) == 0;
	};

	if (!samePoint(points.front(), points.back())) return false;
	return addTinSurfaceTriangle(points[0], points[1], points[2]);
}

void DataProcessorDwg::processPolylineOut(OdInt32 numPoints, const OdInt32* vertexIndexList)
{
	if (capturingTinSurfaceProxy && numPoints == 4)
	{
		std::vector<repo::lib::RepoVector3D64> points;
		points.reserve(numPoints);

		const auto pVertexDataList = vertexDataList();
		for (OdInt32 i = 0; i < numPoints; ++i)
		{
			points.push_back(toRepoVector(pVertexDataList[vertexIndexList[i]]));
		}

		if (addTinSurfaceTrianglePolyline(points)) return;
	}

	DataProcessor::processPolylineOut(numPoints, vertexIndexList);
}

void DataProcessorDwg::processPolylineOut(OdInt32 numPoints, const OdGePoint3d* vertexList)
{
	if (capturingTinSurfaceProxy && numPoints == 4)
	{
		std::vector<repo::lib::RepoVector3D64> points;
		points.reserve(numPoints);

		for (OdInt32 i = 0; i < numPoints; ++i)
		{
			points.push_back(toRepoVector(vertexList[i]));
		}

		if (addTinSurfaceTrianglePolyline(points)) return;
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

std::string DataProcessorDwg::getClassDisplayName(OdDbEntityPtr entity)
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

		// =========================================================================
		// CIVIL 3D ENTITIES - Complete mapping (Civil 3D 2021–2027)
		// Source: Autodesk.Civil.DatabaseServices namespace
		// https://help.autodesk.com/view/CIV3D/2025/ENU/?guid=89ffd413-aada-d770-e322-89dfa7b99369
		// DWG class name pattern: AeccDb<X> -> .NET: Autodesk.Civil.DatabaseServices.<X>
		// =========================================================================

		// =========================================================================
		// BASE ENTITY CLASSES
		// .NET: Entity, CivilObject
		// =========================================================================
		{ "AeccDbEntity", "Civil Entity" },
		{ "AeccDbCivilObject", "Civil Object" },

		// =========================================================================
		// SURFACES
		// .NET: Surface, TinSurface, GridSurface, TinVolumeSurface, GridVolumeSurface
		// =========================================================================
		{ "AeccDbSurface", "Surface" },
		{ "AeccDbSurfaceTin", "TIN Surface" },
		{ "AeccDbTinSurface", "TIN Surface" },
		{ "AeccDbSurfaceGrid", "Grid Surface" },
		{ "AeccDbGridSurface", "Grid Surface" },
		{ "AeccDbVolumeSurface", "Volume Surface" },
		{ "AeccDbTinVolumeSurface", "TIN Volume Surface" },
		{ "AeccDbGridVolumeSurface", "Grid Volume Surface" },
		{ "AeccDbSurfaceBoundary", "Surface Boundary" },
		{ "AeccDbSurfaceContour", "Surface Contour" },
		{ "AeccDbSurfaceWatershed", "Surface Watershed" },
		{ "AeccDbSurfaceDirection", "Surface Direction" },
		{ "AeccDbSurfaceSlope", "Surface Slope" },
		{ "AeccDbSurfaceDefinition", "Surface Definition" },
		{ "AeccDbSurfaceOperationAdd", "Surface Add Operation" },
		{ "AeccDbSurfaceOperationDelete", "Surface Delete Operation" },
		{ "AeccDbSurfaceOperationModify", "Surface Modify Operation" },
		{ "AeccDbSurfaceOperationSmooth", "Surface Smooth Operation" },
		{ "AeccDbSurfaceMask", "Surface Mask" },
		{ "AeccDbSurfaceAnalysis", "Surface Analysis" },
		{ "AeccDbSurfaceSimplify", "Surface Simplify" },             // 2023+
		{ "AeccDbWatershed", "Watershed" },
		{ "AeccDbFace", "TIN Face" },
		{ "AeccDbTinLine", "TIN Line" },
		{ "AeccDbBreakline", "Breakline" },
		{ "AeccDbDEMFile", "DEM File" },
		{ "AeccDbSubgradeSurface", "Subgrade Surface" },             // 2025+

		// =========================================================================
		// SURFACE LABELS
		// .NET: SurfaceElevationLabel, SurfaceSlopeLabel, SurfaceContourLabel, etc.
		// =========================================================================
		{ "AeccDbSurfaceElevationLabel", "Surface Elevation Label" },
		{ "AeccDbSurfaceSpotElevationLabel", "Spot Elevation Label" },
		{ "AeccDbSurfaceSlopeLabel", "Surface Slope Label" },
		{ "AeccDbSurfaceContourLabel", "Surface Contour Label" },    // 2022+
		{ "AeccDbContourLabel", "Contour Label" },

		// =========================================================================
		// ALIGNMENTS
		// .NET: Alignment, AlignmentSubEntity (Line, Arc, Spiral, SCS, SSS, etc.)
		// =========================================================================
		{ "AeccDbAlignment", "Alignment" },
		{ "AeccDbAlignmentEntity", "Alignment Entity" },
		{ "AeccDbAlignmentLine", "Alignment Line" },
		{ "AeccDbAlignmentArc", "Alignment Arc" },
		{ "AeccDbAlignmentCurve", "Alignment Curve" },
		{ "AeccDbAlignmentSpiral", "Alignment Spiral" },
		{ "AeccDbAlignmentTangent", "Alignment Tangent" },
		{ "AeccDbAlignmentSCS", "Alignment SCS" },
		{ "AeccDbAlignmentSSS", "Alignment SSS" },
		{ "AeccDbAlignmentSTS", "Alignment STS" },
		{ "AeccDbAlignmentCSC", "Alignment CSC" },                   // 2022+
		{ "AeccDbAlignmentCRC", "Alignment CRC" },
		{ "AeccDbAlignmentSSCSS", "Alignment SSCSS" },
		{ "AeccDbAlignmentCTS", "Alignment CTS" },
		{ "AeccDbAlignmentSS", "Alignment SS" },
		{ "AeccDbAlignmentMultiTransitionElement", "Multi-Transition Element" },
		{ "AeccDbAlignmentPI", "Alignment PI" },
		{ "AeccDbAlignmentStationEquation", "Station Equation" },
		{ "AeccDbAlignmentDesignSpeed", "Design Speed" },
		{ "AeccDbAlignmentCriteria", "Alignment Criteria" },
		{ "AeccDbOffsetAlignment", "Offset Alignment" },
		{ "AeccDbConnectedAlignment", "Connected Alignment" },
		{ "AeccDbCurbReturnAlignment", "Curb Return Alignment" },
		{ "AeccDbWidening", "Widening" },
		{ "AeccDbAlignmentRegion", "Alignment Region" },             // 2024+

		// =========================================================================
		// ALIGNMENT LABELS
		// .NET: AlignmentLabelGroup, AlignmentStationLabel, etc.
		// =========================================================================
		{ "AeccDbAlignmentLabel", "Alignment Label" },
		{ "AeccDbAlignmentLabeling", "Alignment Labeling" },
		{ "AeccDbAlignmentStationLabeling", "Station Labels" },
		{ "AeccDbAlignmentMajorStationLabeling", "Major Station Labels" },
		{ "AeccDbAlignmentMinorStationLabeling", "Minor Station Labels" },
		{ "AeccDbAlignmentGeometryPointLabeling", "Geometry Point Labels" },
		{ "AeccDbAlignmentSegmentLabeling", "Segment Labels" },
		{ "AeccDbAlignmentCurveLabeling", "Curve Labels" },
		{ "AeccDbAlignmentSpiralLabeling", "Spiral Labels" },
		{ "AeccDbAlignmentTangentIntersectionLabeling", "Tangent Intersection Labels" },
		{ "AeccDbAlignmentDesignSpeedLabeling", "Design Speed Labels" },
		{ "AeccDbAlignmentStationEquationLabeling", "Station Equation Labels" },
		{ "AeccDbAlignmentPILabeling", "PI Labels" },

		// =========================================================================
		// PROFILES
		// .NET: Profile, ProfileView, ProfileEntity (Line, Curve, etc.)
		// =========================================================================
		{ "AeccDbProfile", "Profile" },
		{ "AeccDbVAlignment", "Vertical Alignment" },
		{ "AeccDbOffsetProfile", "Offset Profile" },
		{ "AeccDbSuperelevationProfile", "Superelevation Profile" }, // 2021+
		{ "AeccDbProfileView", "Profile View" },
		{ "AeccDbGraphProfile", "Profile Graph" },
		{ "AeccDbProfileEntity", "Profile Entity" },
		{ "AeccDbProfilePVI", "Profile PVI" },
		{ "AeccDbProfilePVICurve", "PVI Curve" },
		{ "AeccDbProfileLine", "Profile Line" },
		{ "AeccDbProfileCurve", "Profile Curve" },
		{ "AeccDbProfileTangent", "Profile Tangent" },
		{ "AeccDbProfileCrestCurve", "Crest Curve" },
		{ "AeccDbProfileSagCurve", "Sag Curve" },
		{ "AeccDbProfileCircularCurve", "Profile Circular Curve" },
		{ "AeccDbProfileParabolicCurve", "Profile Parabolic Curve" },
		{ "AeccDbProfileAsymmetricParabolicCurve", "Asymmetric Parabolic Curve" },
		{ "AeccDbProfileGrade", "Profile Grade" },

		// =========================================================================
		// PROFILE LABELS
		// .NET: ProfileLabelGroup, ProfileBandSet
		// =========================================================================
		{ "AeccDbProfileLabel", "Profile Label" },
		{ "AeccDbProfileLabeling", "Profile Labeling" },
		{ "AeccDbProfileDataBandLabeling", "Profile Data Band Labels" },
		{ "AeccDbProfileHorizontalGeometryLabeling", "Horizontal Geometry Labels" },
		{ "AeccDbProfileStationLabeling", "Profile Station Labels" },
		{ "AeccDbProfileGradeBreakLabeling", "Grade Break Labels" },
		{ "AeccDbProfileCurveLabeling", "Profile Curve Labels" },
		{ "AeccDbProfileTangentLabeling", "Profile Tangent Labels" },
		{ "AeccDbProfileBandSet", "Profile Band Set" },
		{ "AeccDbProfileBand", "Profile Band" },
		{ "AeccDbProfileViewBandLabel", "Profile View Band Label" }, // 2022+

		// =========================================================================
		// CORRIDORS
		// .NET: Corridor, Baseline, BaselineRegion, AppliedAssembly,
		//       CorridorFeatureLine, CorridorSurface
		// =========================================================================
		{ "AeccDbCorridor", "Corridor" },
		{ "AeccDbBaseline", "Baseline" },
		{ "AeccDbBaselineRegion", "Baseline Region" },
		{ "AeccDbRegionCorridor", "Corridor Region" },
		{ "AeccDbCorridorBaseline", "Corridor Baseline" },
		{ "AeccDbCorridorRegion", "Corridor Region" },
		{ "AeccDbCorridorFeatureLine", "Corridor Feature Line" },
		{ "AeccDbCorridorSurface", "Corridor Surface" },
		{ "AeccDbCorridorSection", "Corridor Section" },
		{ "AeccDbCorridorCode", "Corridor Code" },
		{ "AeccDbCorridorLink", "Corridor Link" },
		{ "AeccDbCorridorPoint", "Corridor Point" },
		{ "AeccDbCorridorShape", "Corridor Shape" },
		{ "AeccDbCorridorTarget", "Corridor Target" },
		{ "AeccDbCorridorFrequency", "Corridor Frequency" },
		{ "AeccDbDaylightLine", "Daylight Line" },

		// =========================================================================
		// ASSEMBLIES / SUBASSEMBLIES
		// .NET: Assembly, Subassembly, AppliedSubassembly
		// =========================================================================
		{ "AeccDbAssembly", "Assembly" },
		{ "AeccDbAssemblyGroup", "Assembly Group" },
		{ "AeccDbAssemblyOffset", "Assembly Offset" },               // 2022+
		{ "AeccDbSubassembly", "Subassembly" },
		{ "AeccDbAppliedAssembly", "Applied Assembly" },
		{ "AeccDbAppliedSubassembly", "Applied Subassembly" },
		{ "AeccDbSubassemblyLane", "Lane Subassembly" },
		{ "AeccDbSubassemblyShoulder", "Shoulder Subassembly" },
		{ "AeccDbSubassemblyDitch", "Ditch Subassembly" },
		{ "AeccDbSubassemblyDaylight", "Daylight Subassembly" },
		{ "AeccDbSubassemblyBuffer", "Buffer Subassembly" },
		{ "AeccDbSubassemblyMedian", "Median Subassembly" },
		{ "AeccDbSubassemblyGenericLink", "Generic Link Subassembly" },
		{ "AeccDbSubassemblyMarkedPoint", "Marked Point Subassembly" },
		{ "AeccDbSubassemblyConditional", "Conditional Subassembly" },
		{ "AeccDbSubassemblyPKT", "PKT Subassembly" },              // 2021+

		// =========================================================================
		// FEATURE LINES / GRADING
		// .NET: FeatureLine, Grading, GradingGroup
		// =========================================================================
		{ "AeccDbFeatureLine", "Feature Line" },
		{ "AeccDbAutoFeatureLine", "Auto Feature Line" },
		{ "AeccDbGrading", "Grading" },
		{ "AeccDbGradingGroup", "Grading Group" },
		{ "AeccDbGradingFeatureLine", "Grading Feature Line" },
		{ "AeccDbGradingRule", "Grading Rule" },
		{ "AeccDbGradingCriteria", "Grading Criteria" },
		{ "AeccDbSteppedOffset", "Stepped Offset" },
		{ "AeccDbFeatureLinePoint", "Feature Line Point" },          // 2023+
		{ "AeccDbExtractionLine", "Extraction Line" },               // 2025+

		// =========================================================================
		// PARCELS
		// .NET: Parcel, ParcelSegment
		// =========================================================================
		{ "AeccDbParcel", "Parcel" },
		{ "AeccDbParcelSegment", "Parcel Segment" },
		{ "AeccDbParcelSegmentLine", "Parcel Segment Line" },
		{ "AeccDbParcelSegmentCurve", "Parcel Segment Curve" },
		{ "AeccDbParcelLoop", "Parcel Loop" },
		{ "AeccDbLotLine", "Lot Line" },
		{ "AeccDbROW", "Right Of Way" },
		{ "AeccDbParcelLabel", "Parcel Label" },
		{ "AeccDbParcelAreaLabel", "Parcel Area Label" },
		{ "AeccDbParcelLineLabel", "Parcel Line Label" },
		{ "AeccDbParcelCurveLabel", "Parcel Curve Label" },

		// =========================================================================
		// PIPE NETWORKS
		// .NET: Network, Part, Pipe, Structure, Connector
		// Hierarchy: Network -> Part -> {Pipe, Structure}
		//            Part -> ConnectorCollection -> Connector
		// =========================================================================
		{ "AeccDbNetwork", "Network" },
		{ "AeccDbNetworkPart", "Network Part" },
		{ "AeccDbNetworkPartConnector", "Network Part Connector" },
		{ "AeccDbPipeNetwork", "Pipe Network" },
		{ "AeccDbPipe", "Pipe" },
		{ "AeccDbStructure", "Structure" },
		{ "AeccDbPipeRun", "Pipe Run" },
		{ "AeccDbGravityPipe", "Gravity Pipe" },
		{ "AeccDbGravityStructure", "Gravity Structure" },
		{ "AeccDbPipeLabel", "Pipe Label" },
		{ "AeccDbStructureLabel", "Structure Label" },
		{ "AeccDbSpanningPipeLabel", "Spanning Pipe Label" },
		{ "AeccDbCrossingPipeLabel", "Crossing Pipe Label" },
		{ "AeccDbPartsList", "Parts List" },
		{ "AeccDbPartsListPipe", "Parts List Pipe" },
		{ "AeccDbPartsListStructure", "Parts List Structure" },
		{ "AeccDbPartFamily", "Part Family" },
		{ "AeccDbPartSize", "Part Size" },
		{ "AeccDbPartRule", "Part Rule" },
		{ "AeccDbPartData", "Part Data" },
		{ "AeccDbPipeNetworkLabel", "Pipe Network Label" },

		// =========================================================================
		// PRESSURE NETWORKS (2021+)
		// .NET: PressureNetwork, PressurePipe, PressureFitting,
		//       PressureAppurtenance, PressurePipeRun
		// =========================================================================
		{ "AeccDbPressureNetwork", "Pressure Network" },
		{ "AeccDbPressurePipe", "Pressure Pipe" },
		{ "AeccDbPressureFitting", "Pressure Fitting" },
		{ "AeccDbPressureAppurtenance", "Pressure Appurtenance" },
		{ "AeccDbPressurePipeRun", "Pressure Pipe Run" },
		{ "AeccDbPressurePartsList", "Pressure Parts List" },
		{ "AeccDbPressurePipeLabel", "Pressure Pipe Label" },
		{ "AeccDbPressureFittingLabel", "Pressure Fitting Label" },
		{ "AeccDbPressureNetworkLabel", "Pressure Network Label" },
		{ "AeccDbPressureNetworkPartConnector", "Pressure Network Connector" },

		// =========================================================================
		// SECTIONS / SAMPLE LINES
		// .NET: SampleLine, SampleLineGroup, SectionView, SectionViewGroup
		// =========================================================================
		{ "AeccDbSampleLine", "Sample Line" },
		{ "AeccDbSampleLineGroup", "Sample Line Group" },
		{ "AeccDbSampleLineLabeling", "Sample Line Labels" },
		{ "AeccDbSampleLineVertex", "Sample Line Vertex" },
		{ "AeccDbSection", "Section" },
		{ "AeccDbSectionCorridor", "Corridor Section" },
		{ "AeccDbSectionSurface", "Surface Section" },
		{ "AeccDbSectionPipe", "Pipe Section" },                     // 2022+
		{ "AeccDbSectionView", "Section View" },
		{ "AeccDbSectionViewGroup", "Section View Group" },
		{ "AeccDbMaterialSection", "Material Section" },
		{ "AeccDbSectionLabel", "Section Label" },
		{ "AeccDbSectionSegment", "Section Segment" },
		{ "AeccDbSectionBandSet", "Section Band Set" },
		{ "AeccDbSectionViewBandLabel", "Section View Band Label" },
		{ "AeccDbSectionSource", "Section Source" },

		// =========================================================================
		// SUPERELEVATION (2021+)
		// .NET: Superelevation, SuperelevationCriticalStation
		// =========================================================================
		{ "AeccDbSuperelevation", "Superelevation" },
		{ "AeccDbSuperelevationView", "Superelevation View" },
		{ "AeccDbSuperelevationCurve", "Superelevation Curve" },
		{ "AeccDbSuperelevationCriticalStation", "Superelevation Critical Station" },

		// =========================================================================
		// CANT / RAIL (2021+)
		// .NET: CantAlignment, RailAlignment
		// =========================================================================
		{ "AeccDbCantAlignment", "Cant Alignment" },
		{ "AeccDbCantView", "Cant View" },
		{ "AeccDbRailAlignment", "Rail Alignment" },

		// =========================================================================
		// MASS HAUL (2021+)
		// .NET: MassHaulDiagram, MassHaulView, MassHaulLine
		// =========================================================================
		{ "AeccDbMassHaulDiagram", "Mass Haul Diagram" },
		{ "AeccDbMassHaulView", "Mass Haul View" },
		{ "AeccDbMassHaulLine", "Mass Haul Line" },

		// =========================================================================
		// SURVEY
		// .NET: SurveyProject, SurveyNetwork, SurveyFigure, SurveyPoint
		// =========================================================================
		{ "AeccDbSurveyProject", "Survey Project" },
		{ "AeccDbSurveyNetwork", "Survey Network" },
		{ "AeccDbSurveyFigure", "Survey Figure" },
		{ "AeccDbSurveyFigureLabel", "Survey Figure Label" },
		{ "AeccDbSurveyPoint", "Survey Point" },
		{ "AeccDbSurveySetup", "Survey Setup" },
		{ "AeccDbSurveyObservation", "Survey Observation" },

		// =========================================================================
		// COGO POINTS
		// .NET: CogoPoint, PointGroup
		// =========================================================================
		{ "AeccDbCogoPoint", "COGO Point" },
		{ "AeccDbPointGroup", "Point Group" },
		{ "AeccDbPointLabel", "Point Label" },
		{ "AeccDbPointDescriptionKey", "Description Key" },
		{ "AeccDbPointCloud", "Point Cloud" },
		{ "AeccDbPointFile", "Point File" },

		// =========================================================================
		// SITES
		// .NET: Site
		// =========================================================================
		{ "AeccDbSite", "Site" },
		{ "AeccDbSiteParcel", "Site Parcel" },
		{ "AeccDbSiteAlignment", "Site Alignment" },
		{ "AeccDbSiteGrading", "Site Grading" },
		{ "AeccDbSiteFeatureLine", "Site Feature Line" },

		// =========================================================================
		// INTERSECTIONS (2021+)
		// .NET: Intersection
		// =========================================================================
		{ "AeccDbIntersection", "Intersection" },
		{ "AeccDbOffsetBaseline", "Offset Baseline" },
		{ "AeccDbConnectedAlignmentSet", "Connected Alignment Set" },

		// =========================================================================
		// DATA SHORTCUTS / REFERENCES
		// .NET: DataReference, SurfaceReference, AlignmentReference, etc.
		// =========================================================================
		{ "AeccDbDataReference", "Data Reference" },
		{ "AeccDbDataShortcut", "Data Shortcut" },
		{ "AeccDbDataShortcutNode", "Data Shortcut Node" },
		{ "AeccDbSurfaceReference", "Surface Reference" },
		{ "AeccDbAlignmentReference", "Alignment Reference" },
		{ "AeccDbProfileReference", "Profile Reference" },
		{ "AeccDbPipeNetworkReference", "Pipe Network Reference" },
		{ "AeccDbCorridorReference", "Corridor Reference" },
		{ "AeccDbViewFrameGroupReference", "View Frame Group Reference" },

		// =========================================================================
		// PLAN PRODUCTION / SHEETS
		// .NET: ViewFrame, ViewFrameGroup, MatchLine
		// =========================================================================
		{ "AeccDbViewFrame", "View Frame" },
		{ "AeccDbViewFrameGroup", "View Frame Group" },
		{ "AeccDbMatchLine", "Match Line" },
		{ "AeccDbSheet", "Sheet" },
		{ "AeccDbSheetSet", "Sheet Set" },
		{ "AeccDbViewFrameLabel", "View Frame Label" },
		{ "AeccDbMatchLineLabel", "Match Line Label" },

		// =========================================================================
		// QUANTITY TAKEOFF / MATERIALS
		// .NET: QuantityTakeoffCriteria, MaterialList
		// =========================================================================
		{ "AeccDbMaterial", "Material" },
		{ "AeccDbMaterialList", "Material List" },
		{ "AeccDbQuantityTakeoff", "Quantity Takeoff" },
		{ "AeccDbPayItem", "Pay Item" },
		{ "AeccDbPayItemCategory", "Pay Item Category" },
		{ "AeccDbComputeMaterials", "Compute Materials" },

		// =========================================================================
		// HYDRAULICS / CATCHMENTS (2021+)
		// .NET: Catchment, CatchmentGroup
		// =========================================================================
		{ "AeccDbCatchment", "Catchment" },
		{ "AeccDbCatchmentGroup", "Catchment Group" },
		{ "AeccDbFlowSegment", "Flow Segment" },
		{ "AeccDbHydraulicNetwork", "Hydraulic Network" },

		// =========================================================================
		// DRAINAGE (2021+)
		// These are Structure subtypes in the SDK
		// =========================================================================
		{ "AeccDbCatchBasin", "Catch Basin" },
		{ "AeccDbManhole", "Manhole" },
		{ "AeccDbInlet", "Inlet" },
		{ "AeccDbOutlet", "Outlet" },
		{ "AeccDbHeadwall", "Headwall" },

		// =========================================================================
		// INTERFERENCE / ANALYSIS
		// .NET: InterferenceCheck
		// =========================================================================
		{ "AeccDbInterferenceCheck", "Interference Check" },
		{ "AeccDbInterference", "Interference" },
		{ "AeccDbDepthCheck", "Depth Check" },

		// =========================================================================
		// MAP / GIS
		// =========================================================================
		{ "AeccDbCoordinateSystem", "Coordinate System" },
		{ "AeccDbMapFeature", "Map Feature" },
		{ "AeccDbGeoRaster", "Geo Raster" },

		// =========================================================================
		// ANALYSIS / VISUALIZATION
		// .NET: SlopeArrow, WaterDrop
		// =========================================================================
		{ "AeccDbSlopeArrow", "Slope Arrow" },
		{ "AeccDbWaterDrop", "Water Drop" },

		// =========================================================================
		// LABELS - GENERAL
		// .NET: Label, LabelGroup, GeneralLabelGroup
		// =========================================================================
		{ "AeccDbLabel", "Civil Label" },
		{ "AeccDbLabelGroup", "Label Group" },
		{ "AeccDbGeneralLabel", "General Label" },
		{ "AeccDbGeneralNoteLabel", "General Note Label" },
		{ "AeccDbTagLabel", "Tag Label" },
		{ "AeccDbReferenceText", "Reference Text" },
		{ "AeccDbLineLabel", "Line Label" },
		{ "AeccDbCurveLabel", "Curve Label" },
		{ "AeccDbNoteLabel", "Note Label" },

		// =========================================================================
		// TABLES
		// .NET: AlignmentTable, ParcelTable, PointTable, etc.
		// =========================================================================
		{ "AeccDbAlignmentTable", "Alignment Table" },
		{ "AeccDbParcelTable", "Parcel Table" },
		{ "AeccDbPointTable", "Point Table" },
		{ "AeccDbPipeTable", "Pipe Table" },
		{ "AeccDbStructureTable", "Structure Table" },
		{ "AeccDbSurfaceTable", "Surface Table" },
		{ "AeccDbVolumeTable", "Volume Table" },
		{ "AeccDbSegmentTable", "Segment Table" },
		{ "AeccDbProfileTable", "Profile Table" },
		{ "AeccDbSectionTable", "Section Table" },
		{ "AeccDbSurveyTable", "Survey Table" },

		// =========================================================================
		// PROJECTION OBJECTS
		// .NET: ProjectionFigure, ProjectionLabel
		// =========================================================================
		{ "AeccDbProjectionLabel", "Projection Label" },
		{ "AeccDbProjectionFigure", "Projection Figure" },

		// =========================================================================
		// STYLES (non-geometric but may appear as proxy originalClassName)
		// .NET: Style, LabelStyle, ObjectLabelStyle
		// =========================================================================
		{ "AeccDbStyle", "Civil Style" },
		{ "AeccDbStyleCollection", "Style Collection" },
		{ "AeccDbLabelStyle", "Label Style" },
		{ "AeccDbObjectLabelStyle", "Object Label Style" },
		{ "AeccDbAlignmentStyle", "Alignment Style" },
		{ "AeccDbProfileStyle", "Profile Style" },
		{ "AeccDbProfileViewStyle", "Profile View Style" },
		{ "AeccDbSurfaceStyle", "Surface Style" },
		{ "AeccDbCorridorStyle", "Corridor Style" },
		{ "AeccDbPipeStyle", "Pipe Style" },
		{ "AeccDbStructureStyle", "Structure Style" },
		{ "AeccDbSectionStyle", "Section Style" },
		{ "AeccDbSectionViewStyle", "Section View Style" },
		{ "AeccDbAssemblyStyle", "Assembly Style" },
		{ "AeccDbCodeSetStyle", "Code Set Style" },
		{ "AeccDbFeatureLineStyle", "Feature Line Style" },
		{ "AeccDbGradingStyle", "Grading Style" },
		{ "AeccDbParcelStyle", "Parcel Style" },
		{ "AeccDbPointStyle", "Point Style" },
		{ "AeccDbMarkerStyle", "Marker Style" },
		{ "AeccDbMatchLineStyle", "Match Line Style" },
		{ "AeccDbViewFrameStyle", "View Frame Style" },
		{ "AeccDbGroupPlotStyle", "Group Plot Style" },
		{ "AeccDbSheetStyle", "Sheet Style" },
		{ "AeccDbIntersectionStyle", "Intersection Style" },
		{ "AeccDbSampleLineStyle", "Sample Line Style" },
		{ "AeccDbMassHaulLineStyle", "Mass Haul Line Style" },       // 2021+
		{ "AeccDbMassHaulViewStyle", "Mass Haul View Style" },       // 2021+
		{ "AeccDbCatchmentStyle", "Catchment Style" },               // 2021+
		{ "AeccDbPressurePipeStyle", "Pressure Pipe Style" },        // 2021+
		{ "AeccDbPressureFittingStyle", "Pressure Fitting Style" },  // 2021+

		// =========================================================================
		// CONNECTED DESIGN (2024+)
		// .NET: Added in Civil 3D 2024
		// =========================================================================
		{ "AeccDbConnectedDesign", "Connected Design" },             // 2024+
		{ "AeccDbDesignCheck", "Design Check" }
	};

	if (isProxyEntity(entity))
	{
		auto proxyClassName = getProxyOriginalClassName(entity);
		auto it = classToDisplayName.find(proxyClassName);
		if (it != classToDisplayName.end())
			return it->second;
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