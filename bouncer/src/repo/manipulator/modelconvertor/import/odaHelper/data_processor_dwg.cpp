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
#include <DbDatabase.h>
#include <CmColorBase.h>
#include "helper_functions.h"
#include "data_processor_dwg.h"
#include <repo_log.h>

using namespace repo::manipulator::modelconvertor::odaHelper;

// All proxy entity inspection, classification, metadata extraction and
// diagnostics now live on the file-scoped Proxy class (proxy.h/.cpp), shared
// by every DataProcessorDwg instance created while vectorising one file.
// Civil3D/Plant3D-specific data and logic live in civil3d_proxy_handler.h/.cpp
// and plant3d_proxy_handler.h/.cpp.

DataProcessorDwg::~DataProcessorDwg()
{
	// This exists so we can use unique_ptr with a forward declaration of DwgDrawContext
	printDiagnostics();
}

void DataProcessorDwg::printDiagnostics() const
{
	if (stats.totalEntities == 0) return;

	repoInfo << "DWG import: " << stats.totalEntities << " entities, "
		<< stats.entitiesWithGeometry << " with geometry, "
		<< stats.entitiesWithoutGeometry << " without";

	if (proxy) proxy->printDiagnostics(stats.entityTypeCount);
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
			metadata = proxy->getProxyEntityMetadata(pEntity, info);
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

	// A proxy entity is inspected exactly once here, via the shared Proxy; every
	// downstream consumer in this function (diagnostics, TIN detection,
	// stored-graphics replay, metadata) reuses this same ProxyInfo instead of
	// re-casting or re-querying ODA for the same entity.
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
		isProxy = proxy->getProxyInfo(pEntity, info);
		if (isProxy)
		{
			stats.entityTypeCount["Proxy-" + info.originalClass]++;
		}
		proxy->recordEntitySeen(isProxy, info);

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
	// capture object for it. Kept separate from the Proxy's activeCapture()
	// (below), which tracks what the geometry callbacks should route into for
	// the duration of the nested OdGsBaseMaterialView::doDraw() call only - the
	// underlying capture buffer is never swapped per-entity, only toggled.
	ProxyGeometryCapture* thisEntityCapture = (isProxy && proxy->isSpecialGeometryClass(info)) ? info.matchedHandler->geometryCapture() : nullptr;
	bool tinSurfaceProxy = thisEntityCapture != nullptr;
	bool replayStoredProxyGraphics = isProxy;

	collector->pushDrawContext(ctx.get());
	bool ret = false;
	if (replayStoredProxyGraphics)
	{
		ProxyGeometryCapture* previousCapture = proxy->activeCapture();
		if (tinSurfaceProxy)
		{
			thisEntityCapture->beginCapture();
			proxy->setActiveCapture(thisEntityCapture);
		}
		else
		{
			proxy->setActiveCapture(nullptr);
		}

		ret = proxy->drawStoredProxyGraphics(pEntity, info, this);
		if (!(ctx && ctx->hasMeshes()) && (!tinSurfaceProxy || !thisEntityCapture->hasTriangles()))
		{
			ret = OdGsBaseMaterialView::doDraw(i, pDrawable) || ret;
		}
		proxy->setActiveCapture(previousCapture);
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
				proxy->logProxyWithoutRenderableGeometry(pEntity, info, replayStoredProxyGraphics, ret);
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
	auto capture = proxy->activeCapture();
	if (!capture)
	{
		DataProcessor::processTriangleOut(p3Vertices, pNormal);
		return;
	}

	const auto pVertexDataList = vertexDataList();
	capture->addTriangle(
		collector,
		toRepoVector(pVertexDataList[p3Vertices[0]]),
		toRepoVector(pVertexDataList[p3Vertices[1]]),
		toRepoVector(pVertexDataList[p3Vertices[2]]));
}

void DataProcessorDwg::polygonOut(OdInt32 numPoints, const OdGePoint3d* vertexList, const OdGeVector3d* pNormal)
{
	auto capture = proxy->activeCapture();
	if (!capture)
	{
		OdGiGeometrySimplifier::polygonOut(numPoints, vertexList, pNormal);
		return;
	}

	if (numPoints < 3) return;

	for (OdInt32 i = 1; i + 1 < numPoints; ++i)
	{
		capture->addTriangle(
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
	auto capture = proxy->activeCapture();
	if (!capture)
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
			capture->addTriangle(
				collector,
				toRepoVector(pointAt(i)),
				toRepoVector(pointAt(i + 1)),
				toRepoVector(pointAt(i + 2)));
		}
		else
		{
			capture->addTriangle(
				collector,
				toRepoVector(pointAt(i + 1)),
				toRepoVector(pointAt(i)),
				toRepoVector(pointAt(i + 2)));
		}
	}
}

void DataProcessorDwg::processPolylineOut(OdInt32 numPoints, const OdInt32* vertexIndexList)
{
	auto capture = proxy->activeCapture();
	if (capture && numPoints == 4)
	{
		std::vector<repo::lib::RepoVector3D64> points;
		points.reserve(numPoints);

		const auto pVertexDataList = vertexDataList();
		for (OdInt32 i = 0; i < numPoints; ++i)
		{
			points.push_back(toRepoVector(pVertexDataList[vertexIndexList[i]]));
		}

		if (capture->addPolyline(collector, points)) return;
	}

	DataProcessor::processPolylineOut(numPoints, vertexIndexList);
}

void DataProcessorDwg::processPolylineOut(OdInt32 numPoints, const OdGePoint3d* vertexList)
{
	auto capture = proxy->activeCapture();
	if (capture && numPoints == 4)
	{
		std::vector<repo::lib::RepoVector3D64> points;
		points.reserve(numPoints);

		for (OdInt32 i = 0; i < numPoints; ++i)
		{
			points.push_back(toRepoVector(vertexList[i]));
		}

		if (capture->addPolyline(collector, points)) return;
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
