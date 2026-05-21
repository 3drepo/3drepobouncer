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
#include <DbStubPtrArray.h>
#include <DbBlockReference.h>
#include <OdString.h>
#include <DgCmColor.h>
#include <toString.h>
#include <DbProxyEntity.h>
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
#include "repo/lib/datastructure/repo_variant_utils.h"
#include <boost/variant/apply_visitor.hpp>
#include <repo_log.h>

using namespace repo::manipulator::modelconvertor::odaHelper;

DataProcessorDwg::~DataProcessorDwg()
{
	// This exists so we can use unique_ptr with a forward declaration of DwgDrawContext
	printDiagnostics();
}

// Add this helper method to DataProcessorDwg class
bool DataProcessorDwg::isCivil3DEntity(OdDbEntityPtr pEntity)
{
	if (pEntity.isNull()) return false;

	auto className = convertToStdString(pEntity->isA()->name());

	// Check if class name starts with Civil3D prefixes
	return className.rfind("Aecc", 0) == 0 ||  // Civil3D entities start with "Aecc"
		className.rfind("AeccDb", 0) == 0;
}
bool DataProcessorDwg::isPlant3DEntity(OdDbEntityPtr pEntity)
{
	if (pEntity.isNull()) return false;

	auto className = convertToStdString(pEntity->isA()->name());

	// Check if class name starts with Plant3D prefixes
	return className.rfind("AcPp", 0) == 0 ||      // Plant3D entities start with "AcPp"
		className.rfind("AcPpDb", 0) == 0;
}
bool DataProcessorDwg::isProxyEntity(OdDbEntityPtr pEntity)
{
	if (pEntity.isNull()) return false;
	return convertToStdString(pEntity->isA()->name()) == "AcDbProxyEntity";
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

				// Proxy flags
				int flags = proxyEntity->proxyFlags();
				metadata["ProxyFlags"] = std::to_string(flags);
				metadata["HasProxyGraphics"] = (flags & 1) ? "true" : "false";
				metadata["IsR13Format"] = (flags & 2) ? "true" : "false";
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
void DataProcessorDwg::inspectProxyEntity(OdDbEntityPtr pEntity)
{
	if (pEntity.isNull()) return;

	auto handle = convertToStdString(toString(pEntity->objectId().getHandle()));
	repoWarning << "\n===============================================================";
	repoWarning << "||  PROXY ENTITY INSPECTION [Handle: " << handle << "]";
	repoWarning << "===============================================================";

	auto className = convertToStdString(pEntity->isA()->name());
	repoWarning << "  Class Name:           " << className;

	std::string originalClassName = getProxyOriginalClassName(pEntity);
	repoWarning << "  Original Class:       " << originalClassName;

	if (originalClassName.find("Aecc") != std::string::npos)
	{
		repoWarning << " ** IDENTIFIED AS:      CIVIL3D ENTITY";
		stats.civil3dEntities++;
	}
	else if (originalClassName.find("AcPp") != std::string::npos)
	{
		repoWarning << " ** IDENTIFIED AS:      PLANT3D ENTITY";
		stats.plant3dEntities++;
	}
	// Get all metadata
	auto metadata = getProxyMetadata(pEntity);

	// Display metadata
	for (const auto& [key, value] : metadata)
	{
		repoWarning << "  " << key << ": " << value;
	}
	auto xdataApps = getProxyXDataApps(pEntity);
	if (!xdataApps.empty())
	{
		repoWarning << "  XData Applications:";
		for (const auto& app : xdataApps)
		{
			repoWarning << "    - " << app;
		}
	}
	else
	{
		repoWarning << "  XData Applications:   None";
	}

	OdDbProxyEntityPtr proxyEntity = OdDbProxyEntity::cast(pEntity);
	if (!proxyEntity.isNull())
	{
		try {
			int proxyFlags = proxyEntity->proxyFlags();
			bool hasProxyGraphics = (proxyFlags & 1);
			bool isR13Entity = (proxyFlags & 2);

			repoWarning << "  Proxy Flags:          " << proxyFlags;
			repoWarning << "  Has Proxy Graphics:   " << (hasProxyGraphics ? "YES" : "NO");
			repoWarning << "  Is R13 Format:        " << (isR13Entity ? "YES" : "NO");

			if (!hasProxyGraphics)
			{
				repoError << " PROBLEM: No proxy graphics embedded!";
				repoError << " Entity REQUIRES native modules to render";
			}
		}
		catch (OdError& e) {
			repoWarning << "  Proxy flags error: " << convertToStdString(e.description());
		}
	}

	try {
		OdGeExtents3d extents;
		if (pEntity->getGeomExtents(extents) == eOk)
		{
			auto min = extents.minPoint();
			auto max = extents.maxPoint();
			repoWarning << "  Bounding Box:         Min(" << min.x << ", " << min.y << ", " << min.z << ")";
			repoWarning << "                        Max(" << max.x << ", " << max.y << ", " << max.z << ")";
		}
		else
		{
			repoError << " Cannot get bounding box";
		}
	}
	catch (OdError& e) {
		repoError << " Extents error: " << convertToStdString(e.description());
	}

	try {
		auto layerName = convertToStdString(toString(pEntity->layer()));
		repoWarning << "  Layer:                " << layerName;
	}
	catch (...) {}

	repoWarning << "===============================================================\n";
}
bool DataProcessorDwg::tryExplodeEntity(OdDbEntityPtr pEntity, std::vector<OdDbEntityPtr>& explodedEntities)
{
	if (pEntity.isNull()) return false;

	try {
		OdRxObjectPtrArray entitySet;

		// Try to explode the entity into simpler primitives
		OdResult res = pEntity->explode(entitySet);

		if (res == eOk && entitySet.size() > 0)
		{
			repoInfo << "Successfully exploded " << convertToStdString(pEntity->isA()->name())
				<< " into " << entitySet.size() << " simple entities";

			for (size_t i = 0; i < entitySet.size(); ++i)
			{
				OdDbEntityPtr exploded = OdDbEntity::cast(entitySet[i]);
				if (!exploded.isNull())
				{
					explodedEntities.push_back(exploded);
				}
			}
			return true;
		}
	}
	catch (OdError& e) {
		repoTrace << "Failed to explode entity: " << convertToStdString(e.description());
	}

	return false;
}
void DataProcessorDwg::extractBoundingBoxAsMesh(OdDbEntityPtr pEntity)
{
	try {
		OdGeExtents3d extents;
		if (pEntity->getGeomExtents(extents) == eOk)
		{
			// Create a simple box mesh from the extents
			auto min = toRepoVector(extents.minPoint());
			auto max = toRepoVector(extents.maxPoint());

			// Create 12 lines representing the bounding box edges
			std::vector<std::pair<repo::lib::RepoVector3D64, repo::lib::RepoVector3D64>> boxEdges = {
				// Bottom face
				{{min.x, min.y, min.z}, {max.x, min.y, min.z}},
				{{max.x, min.y, min.z}, {max.x, max.y, min.z}},
				{{max.x, max.y, min.z}, {min.x, max.y, min.z}},
				{{min.x, max.y, min.z}, {min.x, min.y, min.z}},
				// Top face
				{{min.x, min.y, max.z}, {max.x, min.y, max.z}},
				{{max.x, min.y, max.z}, {max.x, max.y, max.z}},
				{{max.x, max.y, max.z}, {min.x, max.y, max.z}},
				{{min.x, max.y, max.z}, {min.x, min.y, max.z}},
				// Vertical edges
				{{min.x, min.y, min.z}, {min.x, min.y, max.z}},
				{{max.x, min.y, min.z}, {max.x, min.y, max.z}},
				{{max.x, max.y, min.z}, {max.x, max.y, max.z}},
				{{min.x, max.y, min.z}, {min.x, max.y, max.z}},
			};

			for (const auto& edge : boxEdges) {
				collector->addFace({ edge.first, edge.second });
			}

			repoInfo << "Extracted bounding box for custom entity: "
				<< convertToStdString(pEntity->isA()->name());
		}
	}
	catch (OdError& e) {
		repoTrace << "Failed to extract bounding box: " << convertToStdString(e.description());
	}
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
	/*repoInfo << "===============================================================";
	repoInfo << "  Exploded successfully:          " << stats.explodedSuccessfully;
	repoInfo << "  Explode failed:                 " << stats.explodedFailed;
	repoInfo << "  Bounding box fallbacks:         " << stats.boundingBoxFallbacks;*/

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

std::unordered_map<std::string, repo::lib::RepoVariant> DataProcessorDwg::getCivil3DPlant3DMetadata(OdDbEntityPtr pEntity)
{
	std::unordered_map<std::string, repo::lib::RepoVariant> metadata;

	if (!isProxyEntity(pEntity)) return metadata;

	stats.proxyEntities++;
	OdDbProxyEntityPtr proxyEntity = OdDbProxyEntity::cast(pEntity);
	if (proxyEntity.isNull()) return metadata;

	try {
		// 1. Get original class name
		std::string originalClass = convertToStdString(proxyEntity->originalClassName());
		stats.entityTypeCount["Proxy-"+originalClass]++;
		if (originalClass.rfind("Aecc", 0) == 0)
			stats.civil3dEntities++;
		if (originalClass.rfind("AcPp", 0) == 0)
			stats.plant3dEntities++;
		//metadata["OriginalClassName"] = originalClass;


		//metadata["Application"] = detectApplicationType(pEntity);
		
		// 2. Check if we can render it
		int flags = proxyEntity->proxyFlags();
		/*metadata["HasGeometry"] = (bool)(flags & 1);
		metadata["CanExplode"] = (bool)(flags & 4);
		
		
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
		catch (...) {}*/

		// =====================================================================
		// 3. READ ALL PROPERTIES VIA ODA Common Data Access (CDA) / RxProperties
		// This is equivalent to the AutoCAD Properties panel (General, Pattern,
		// Geometry sections, etc.)
		// ODA SDK Reference: https://docs.opendesign.com/tkernel/OdRxProperty.html
		// Requires: RxPropertiesModule, DbPropertiesModule, RxCommonDataAccessModule
		// (already loaded in file_processor_dwg.cpp readFile())
		// =====================================================================
		extractEntityProperties(pEntity, metadata);

		// 3. Try to get stored properties from extension dictionary
		OdDbObjectId extDictId = pEntity->extensionDictionary();
		if (!extDictId.isNull())
		{
			OdDbDictionaryPtr pExtDict = extDictId.safeOpenObject();
			if (!pExtDict.isNull())
			{
				// Civil3D-specific dictionaries
				if (originalClass.find("Aecc") != std::string::npos)
				{
					extractCivil3DStoredProperties(pExtDict, metadata);
				}

				// Plant3D-specific dictionaries
				if (originalClass.find("AcPp") != std::string::npos)
				{
					extractPlant3DStoredProperties(pExtDict, metadata);
				}
			}
		}

		// 4. Get XData properties
		OdResBufPtr pRb = pEntity->xData();
		if (!pRb.isNull())
		{
			extractXDataProperties(pRb, metadata);
		}

		// 5. Get any properties that were saved as plain text
		extractTextPropertiesFromProxy(proxyEntity, metadata);

	}
	catch (OdError& e) {
		metadata["_Error"] = convertToStdString(e.description());
	}

	return metadata;
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
		/*
		// ===== IDENTIFY AND INSPECT PROXY ENTITIES =====
		if (isProxyEntity(pEntity))
		{
			stats.proxyEntities++;
			//inspectProxyEntity(pEntity);
		}
		else if (isCivil3DEntity(pEntity))
		{
			stats.civil3dEntities++;
			repoWarning << "[CIVIL3D] " << className << " | Handle: "
				<< convertToStdString(toString(pEntity->objectId().getHandle()));
		}
		else if (isPlant3DEntity(pEntity))
		{
			stats.plant3dEntities++;
			repoWarning << "[PLANT3D] " << className << " | Handle: "
				<< convertToStdString(toString(pEntity->objectId().getHandle()));
		}*/

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

	collector->pushDrawContext(ctx.get());
	auto ret = OdGsBaseMaterialView::doDraw(i, pDrawable);
	collector->popDrawContext(ctx.get());

	// ===== CHECK GEOMETRY EXTRACTION =====
	if (ctx && !pEntity.isNull())
	{
		if (ctx->hasMeshes())
		{
			stats.entitiesWithGeometry++;
		}
		else
		{
			stats.entitiesWithoutGeometry++;

			// ===== HANDLE MISSING GEOMETRY FROM CUSTOM ENTITIES =====
			/*if (isProxyEntity(pEntity) || isCivil3DEntity(pEntity) || isPlant3DEntity(pEntity))
			{
				auto className = convertToStdString(pEntity->isA()->name());
				auto handle = convertToStdString(toString(pEntity->objectId().getHandle()));

				repoError << "\n[NO GEOMETRY] " << className << " | Handle: " << handle;
				repoError << "Attempting fallback strategies...";

				// FALLBACK 1: Try explode
				repoInfo << "[FALLBACK 1/2] Trying explode...";
				std::vector<OdDbEntityPtr> explodedEntities;
				if (tryExplodeEntity(pEntity, explodedEntities))
				{
					stats.explodedSuccessfully++;

					for (auto& exploded : explodedEntities)
					{
						auto explodedCtx = collector->makeNewDrawContext();
						collector->pushDrawContext(explodedCtx.get());
						OdGsBaseMaterialView::doDraw(i, exploded.get());
						collector->popDrawContext(explodedCtx.get());

						if (explodedCtx && explodedCtx->hasMeshes())
						{
							auto meshes = explodedCtx->extractMeshes(collector->getLayerTransform(entityLayer.id).inverse());
							collector->addMeshes(entityLayer.id, meshes);
							stats.entitiesWithGeometry++;
						}
					}
				}
				else
				{
					stats.explodedFailed++;

					// FALLBACK 2: Bounding box
					repoInfo << "[FALLBACK 2/2] Using bounding box...";
					extractBoundingBoxAsMesh(pEntity);
				}
			}*/
		}
	}

	if (ctx && ctx->hasMeshes()) 
	{
		// This stack frame should create a layer with actual geometry

		if (parentLayer) {
			collector->createLayer(parentLayer.id, parentLayer.name, {}, {});
		}

		if (!collector->hasLayer(entityLayer.id)) {
			auto bounds = ctx->getBounds();
			auto m = repo::lib::RepoMatrix::translate(bounds.min());
			collector->createLayer(entityLayer.id, entityLayer.name, parentLayer.id, m);
		}

		auto meshes = ctx->extractMeshes(collector->getLayerTransform(entityLayer.id).inverse());
		collector->addMeshes(entityLayer.id, meshes);

		if (!handleMetaValue.empty() && !collector->hasMetadata(entityLayer.id)) {
			std::unordered_map<std::string, repo::lib::RepoVariant> meta, metadata;
			meta["Entity Handle::Value"] = handleMetaValue;

			// ===== ENHANCED METADATA FOR CUSTOM ENTITIES =====
			if (!pEntity.isNull())
			{
				extractEntityProperties(pEntity, metadata);
				//auto metadata = getCivil3DPlant3DMetadata(pEntity);
				if (!metadata.empty())
				{
					repoInfo << "****************************Metadata****************************************";
					// Display metadata
					for (const auto& [key, value] : metadata)
					{
						meta[key] = value;
						repoInfo << "  " << key << ": " << boost::apply_visitor(repo::lib::StringConversionVisitor(), value);
					}
				}
			}

			collector->setMetadata(entityLayer.id, meta);
		}
	}

	return ret;
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