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

#include "civil3d_proxy_handler.h"

#include <OdString.h>
#include <toString.h>
#include <DbXrecord.h>

#include "helper_functions.h"
#include "dwg_proxy_inspector.h"

#include <algorithm>
#include <cmath>

using namespace repo::manipulator::modelconvertor::odaHelper;

namespace {
	/* Known Civil3D extension dictionary entries that might contain stored property
	data, and the substrings used to spot a property name resbuf within them. Const
	at namespace scope, so these have internal linkage and are built once rather
	than per entity. */
	const std::vector<std::string> kCivil3DDicts = {
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
	const std::vector<std::string> kCivil3DTriggers = { "Station", "Offset", "Elevation", "Grade" };
}

bool Civil3DProxyHandler::matchesClassName(const std::string& originalClass)
{
	return originalClass.find("Aecc") != std::string::npos ||
		originalClass.find("Civil") != std::string::npos;
}

bool Civil3DProxyHandler::isTinSurfaceClass(const std::string& originalClass) const
{
	return originalClass == "AeccDbSurfaceTin" ||
		originalClass == "AeccDbTinSurface" ||
		originalClass.find("SurfaceTin") != std::string::npos ||
		originalClass.find("TinSurface") != std::string::npos;
}

void Civil3DProxyHandler::addDictionaryMetadata(
	OdDbDictionaryPtr dict,
	std::unordered_map<std::string, repo::lib::RepoVariant>& metadata) const
{
	extractProxyDictionaryProperties(dict, kCivil3DDicts, "Civil3D", kCivil3DTriggers, true, metadata);
}

bool Civil3DProxyHandler::getDisplayName(const std::string& originalClass, std::string& outName) const
{
	const static std::unordered_map<std::string, std::string> classToDisplayName
	{
		// =========================================================================
		// CIVIL 3D ENTITIES - Complete mapping (Civil 3D 2021-2027)
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

	auto it = classToDisplayName.find(originalClass);
	if (it == classToDisplayName.end()) return false;

	outName = it->second;
	return true;
}

void Civil3DProxyHandler::TinCapture::beginCapture()
{
	capturedEdgeKeys.clear();
	capturedTriangles.clear();
	hasFaceMaterial = false;
}

void Civil3DProxyHandler::TinCapture::captureMaterialIfNeeded(GeometryCollector* collector)
{
	if (!hasFaceMaterial)
	{
		faceMaterial = collector->getLastMaterial();
		hasFaceMaterial = true;
	}
}

bool Civil3DProxyHandler::TinCapture::addTriangle(
	GeometryCollector* collector,
	const repo::lib::RepoVector3D64& p0,
	const repo::lib::RepoVector3D64& p1,
	const repo::lib::RepoVector3D64& p2)
{
	if (samePoint(p0, p1) || samePoint(p1, p2) || samePoint(p2, p0)) return false;

	GeometryCollector::Face triangle;
	triangle.push_back(p0);
	triangle.push_back(p1);
	triangle.push_back(p2);
	capturedTriangles.push_back(triangle);
	captureMaterialIfNeeded(collector);

	addEdge(collector, p0, p1);
	addEdge(collector, p1, p2);
	addEdge(collector, p2, p0);
	return true;
}

bool Civil3DProxyHandler::TinCapture::addEdge(
	GeometryCollector* collector,
	const repo::lib::RepoVector3D64& p0,
	const repo::lib::RepoVector3D64& p1)
{
	if (samePoint(p0, p1)) return false;

	if (!capturedEdgeKeys.insert(edgeKey(p0, p1)).second) return false;

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

bool Civil3DProxyHandler::TinCapture::addPolyline(
	GeometryCollector* collector,
	const std::vector<repo::lib::RepoVector3D64>& points)
{
	if (points.size() != 4) return false;
	if (!samePoint(points.front(), points.back())) return false;
	return addTriangle(collector, points[0], points[1], points[2]);
}

void Civil3DProxyHandler::TinCapture::applyFaceLayers(
	GeometryCollector* collector,
	const std::string& parentLayerId,
	const std::string& sourceEntityId) const
{
	for (size_t i = 0; i < capturedTriangles.size(); ++i)
	{
		auto faceLayerId = sourceEntityId + ":TIN_FACE:" + std::to_string(i);

		auto faceContext = collector->makeNewDrawContext();
		collector->pushDrawContext(faceContext.get());
		if (hasFaceMaterial)
		{
			collector->setMaterial(faceMaterial);
		}
		collector->addFace(capturedTriangles[i]);
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

void Civil3DProxyHandler::TinCapture::addComputedMetadata(
	OdDbEntityPtr pEntity,
	std::unordered_map<std::string, repo::lib::RepoVariant>& metadata) const
{
	std::unordered_set<std::string> uniquePoints;
	double minElevation = 0.0;
	double maxElevation = 0.0;
	bool hasElevation = false;

	for (const auto& triangle : capturedTriangles)
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
		DwgProxyInspector::setMetadataIfMissing(metadata, "Data::Minimum Elevation", roundElevation(minElevation));
		DwgProxyInspector::setMetadataIfMissing(metadata, "Data::Maximum Elevation", roundElevation(maxElevation));
	}
}
