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

#include "helper_functions.h"
#include "data_processor_dwg.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

DataProcessorDwg::~DataProcessorDwg()
{
	// This exists so we can use unique_ptr with a forward declaration of DwgDrawContext
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

	if (ctx) 
	{
		auto meshes = ctx->extractMeshes();
		if (meshes.size()) {
			// This stack frame should create a layer with actual geometry

			if (parentLayer) {
				collector->createLayer(parentLayer.id, parentLayer.name, {});
			}
			collector->createLayer(entityLayer.id, entityLayer.name, parentLayer.id);
			collector->addMeshes(entityLayer.id, meshes);

			if (!handleMetaValue.empty() && !collector->hasMetadata(entityLayer.id)) {
				std::unordered_map<std::string, repo::lib::RepoVariant> meta;
				meta["Entity Handle::Value"] = handleMetaValue;
				collector->setMetadata(entityLayer.id, meta);
			}
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
	};

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