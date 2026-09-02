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
		activeProxyInfo = DwgProxyUtils::getProxyInfo(pEntity);
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
		auto entityName = getClassDisplayName(pEntity, activeProxyInfo);

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

	activeProxyInfo.currentSurfaceEdgeKeys.clear();
	activeProxyInfo.currentSurfacePointKeys.clear();

	collector->pushDrawContext(ctx.get());
	bool ret = false;
	if (activeProxyInfo.isProxy())
	{
		ret = DwgProxyUtils::drawStoredProxyGraphics(pEntity, activeProxyInfo, this);
		if (!(ctx && ctx->hasMeshes()))
		{
			ret = OdGsBaseMaterialView::doDraw(i, pDrawable) || ret;
		}
	}
	else
	{
		ret = OdGsBaseMaterialView::doDraw(i, pDrawable);
	}
	collector->popDrawContext(ctx.get());

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
			std::unordered_map<std::string, repo::lib::RepoVariant> meta;
			meta["Entity Handle::Value"] = handleMetaValue;
			if (activeProxyInfo.isProxy()) {
				DwgProxyUtils::addProxyMetadata(pEntity, activeProxyInfo, meta);
				meta["Entity Class::Value"] = activeProxyInfo.originalClass;
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

void DataProcessorDwg::triangleOut(const OdInt32* p3Vertices, const OdGeVector3d* pNormal)
{
	if (activeProxyInfo.isProxy() && activeProxyInfo.isCivil3DSurfaceClass())
	{
		const auto pVertexDataList = vertexDataList();
		auto p0 = toRepoVector(pVertexDataList[p3Vertices[0]]);
		auto p1 = toRepoVector(pVertexDataList[p3Vertices[1]]);
		auto p2 = toRepoVector(pVertexDataList[p3Vertices[2]]);
		if (samePoint(p0, p1) || samePoint(p1, p2) || samePoint(p2, p0))
			return;
		activeProxyInfo.currentSurfacePointKeys.insert(pointKey(p0));
		activeProxyInfo.currentSurfacePointKeys.insert(pointKey(p1));
		activeProxyInfo.currentSurfacePointKeys.insert(pointKey(p2));
		addSurfaceEdgeIfNeeded(p0, p1);
		addSurfaceEdgeIfNeeded(p1, p2);
		addSurfaceEdgeIfNeeded(p2, p0);
		return;
	}
	DataProcessor::triangleOut(p3Vertices, pNormal);	
}

void DataProcessorDwg::addSurfaceEdgeIfNeeded(const repo::lib::RepoVector3D64& p0, const repo::lib::RepoVector3D64& p1)
{
	if (samePoint(p0, p1)) return;
	if (!activeProxyInfo.currentSurfaceEdgeKeys.insert(edgeKey(p0, p1)).second) return;

	auto edgeMaterial = collector->getLastMaterial();
	edgeMaterial.diffuse = { 0.4f, 0.4f, 0.4f };
	collector->setMaterial(edgeMaterial);
	collector->addFace({ p0, p1 });
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
	if (info.isProxy())
	{
		// If this is a proxy, we want to display the original class name, not
		// the generic "Proxy" name. This is because the original class name
		// gives a better indication of what the entity actually is.
		className = info.originalClass;
	}

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
		{ "AeccDbDesignCheck", "Design Check" },
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