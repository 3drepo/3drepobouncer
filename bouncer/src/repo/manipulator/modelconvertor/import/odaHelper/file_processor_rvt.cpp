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

//ODA [ BIM BIM-EX RX DB ]
#include <BimCommon.h>
#include <Common/BmBuildSettings.h>
#include <Gs/Gs.h>
#include <Base/BmForgeTypeId.h>
#include <Base/PE/BmSystemServicesPE.h>
#include <Database/BmDatabase.h>
#include <Database/BmUnitUtils.h>
#include <Database/Entities/BmDBDrawing.h>
#include <Database/Entities/BmUnitsElem.h>
#include <Database/Managers/BmUnitsTracking.h>
#include <DynamicLinker.h>
#include <ExBimHostAppServices.h>
#include <ModelerGeometry/BmModelerModule.h>
#include <OdaCommon.h>
#include <RxDynamicModule.h>
#include <RxDynamicModule.h>
#include <RxInit.h>
#include <RxObjectImpl.h>
#include <StaticRxObject.h>
#include <Database/BmGsManager.h>
#include <Database/BmGsView.h>
#include <Database/Entities/BmBasicFileInfo.h>
#include <Database/Entities/BmOverrideGraphicSettings.h>
#include <Database/Managers/BmViewTable.h>
#include <Essential/Entities/BmDBView3d.h>
#include <Essential/Entities/BmBasePoint.h>
#include <Essential/Entities/BmGeoLocation.h>

#include "repo/lib/repo_utils.h"
#include "repo/lib/repo_exception.h"
#include "repo/error_codes.h"

//3d repo bouncer
#include "file_processor_rvt.h"
#include "data_processor_rvt.h"
#include "repo_system_services.h"
#include "helper_functions.h"

//help
#include "vectorise_device_rvt.h"
#include <Database/BmTransaction.h>

using namespace repo::manipulator::modelconvertor::odaHelper;

const bool USE_NEW_TESSELLATION = true;
const double TRIANGULATION_EDGE_LENGTH = 1;
const double ROUNDING_ACCURACY = 0.000001;

#define RVT_TEMPORARY_DIRECTORY_ENV_VARIABLE "REPO_RVT_TEMP_DIR"

// The following sets up the level of detail parameters for the FileProcessorRvt.
// The exact meaning of the LOD parameter provided in the config will depend on
// each processor's tessellation method. For Revit, these correspond to different
// tolerances in the BRep triangulator.

struct LevelOfDetailParams
{
	int normalTolerance;
	double surfaceTolerance;

	LevelOfDetailParams(int normalTolerance, double surfaceTolerance) {
		this->normalTolerance = normalTolerance;
		this->surfaceTolerance = surfaceTolerance;
	}
};

const std::vector<LevelOfDetailParams> LOD_PARAMETERS ({
	LevelOfDetailParams(15, 0), // Default matches the wrTriangulationParams default constructor when bNewTess is true
	LevelOfDetailParams(360, 10000),
	LevelOfDetailParams(40, 10000),
	LevelOfDetailParams(360, 0.1),
	LevelOfDetailParams(360, 0.01),
	LevelOfDetailParams(360, 0.005),
	LevelOfDetailParams(360, 0.0005),
});

class StubDeviceModuleRvt : public OdGsBaseModule
{
private:
	OdBmDatabasePtr database;
	OdBmDBViewPtr view;
	GeometryCollector* collector;
	OdGeMatrix3d modelToWorld;

public:
	void init(GeometryCollector* const collector, OdBmDatabasePtr database, OdBmDBViewPtr view, const OdGeMatrix3d& modelToWorld)
	{
		this->collector = collector;
		this->database = database;
		this->view = view;
		this->modelToWorld = modelToWorld;
	}
protected:
	OdSmartPtr<OdGsBaseVectorizeDevice> createDeviceObject()
	{
		return OdRxObjectImpl<VectoriseDeviceRvt, OdGsBaseVectorizeDevice>::createObject();
	}
	OdSmartPtr<OdGsViewImpl> createViewObject()
	{
		OdSmartPtr<OdGsViewImpl> pP = OdRxObjectImpl<DataProcessorRvt, OdGsViewImpl>::createObject();
		((DataProcessorRvt*)pP.get())->initialise(collector, database, view, modelToWorld);
		return pP;
	}
};
ODRX_DEFINE_PSEUDO_STATIC_MODULE(StubDeviceModuleRvt);

class RepoRvtServices : public RepoSystemServices, public OdExBimHostAppServices, public OdBmSystemServicesPE
{
public:
	/*
	* This methods implement the abstract OdBmSystemServicesPE class, in order to
	* turn on unloading to decrease memory usage at a cost of processing time.
	*/

	bool setUseUnload(bool enabled) 
	{
		this->useUnload = enabled;
	}

	virtual bool unloadEnabled() const override
	{
		return useUnload;
	}

	virtual bool useDisk() const override
	{
		return useUnload;
	}

	virtual double indexingRate() const override
	{
		return 0;
	}

	virtual OdString unloadFilePath() const override
	{
		return OdBmSystemServicesPE::unloadFilePath(); // We could change the file path using an environment variable here
	}

protected:
	ODRX_USING_HEAP_OPERATORS(RepoSystemServices);

	bool useUnload = false;
};

OdBmDBView3dPtr findView(OdBmDatabasePtr pDb, std::string name)
{
	OdBmViewTablePtr pViewTable = pDb->getAppInfo(OdBm::ManagerType::ViewTable);
	OdBmObjectId idView = pViewTable->findViewIdByName(name.c_str());
	try {
		OdBmDBView3dPtr pView = idView.safeOpenObject();
		repoInfo << "Using view " << convertToStdString(pView->getElementName());
		return pView;
	}
	catch (OdError& e) {
		switch (e.code()) {
		case OdResult::eNullObjectId:
			throw repo::lib::RepoSceneProcessingException("Cannot find view: " + name, REPOERR_VIEW_NOT_FOUND);
		case OdResult::eNotThatKindOfClass:
			throw repo::lib::RepoSceneProcessingException("View " + name + " is not a 3D view.", REPOERR_VIEW_NOT_3D);
		default:
			throw;
		}
	}
}

FileProcessorRvt::FileProcessorRvt(const std::string& inputFile,
	modelutility::RepoSceneBuilder* builder,
	const ModelImportConfig& config) :
	FileProcessor(inputFile, builder, config)
{
}

void FileProcessorRvt::setTessellationParams(wrTriangulationParams params)
{
	OdSmartPtr<BmModelerModule> pModModule;
	OdRxModulePtr pModule = odrxDynamicLinker()->loadModule(OdBmModelerModuleName);
	if (pModule.get())
	{
		pModModule = BmModelerModule::cast(pModule);
		pModModule->setTriangulationParams(params);
	}
}

void setupUnitsFormat(OdBmDatabasePtr pDb, double accuracy)
{
	OdBmUnitsTrackingPtr pUnitsTracking = pDb->getAppInfo(OdBm::ManagerType::UnitsTracking);
	OdBmUnitsElemPtr pUnitsElem = pUnitsTracking->getUnitsElemId().safeOpenObject();
	if (pUnitsElem.isNull())
		return;

	OdBmAUnitsPtr units = pUnitsElem->getUnits();
	if (units.isNull())
		return;

	OdBmMap<OdBmForgeTypeId, OdBmFormatOptionsPtr> aFormatOptions;

	for (const auto &entry : aFormatOptions)
	{
		auto formatOptions = entry.second;
		//.. NOTE: Here the format of units is configured
		formatOptions->setAccuracy(accuracy);
		formatOptions->setRoundingMethod(OdBm::RoundingMethod::Nearest);
	}
}

void setupRenderMode(OdBmDatabasePtr database, OdGsDevicePtr device, OdGiDefaultContextPtr bimContext, OdGsView::RenderMode renderMode)
{
	OdGsBmDBDrawingHelperPtr drawingHelper = OdGsBmDBDrawingHelper::setupDBDrawingViews(database->getActiveDBDrawingId(), device, bimContext);
	auto view = drawingHelper->activeView();
	view->setMode(renderMode);
}

/*
* Updates the view with some additional overrides, such as hiding section boxes.
*/
void setupViewOverrides(OdBmDBViewPtr pView)
{
	auto db = pView->database();

	ODBM_TRANSACTION_BEGIN(viewUpdate, db)
	viewUpdate.start();

	// This snippet uses the definitions in BmBuiltInCategory.h to collect all the
	// categories with the type Annotation, so we can turn them off.

	// Ideally, we would use the db->getCategoriesData() & getCategoryType()
	// methods, but getCategoriesData doesn't appear to have been exported in this
	// version of ODA.

	// The following macro invocations work by creating definitions for each
	// possible token of the KIND entry in the ODBM_ACTUAL_BUILTIN_CATEGORIES
	// def/table. For now, we are only interested in Annotations (which we add to a
	// list).
	// We undef the KIND tokens immediately after as they are common words that may
	// be used elsewhere.

#define Annotation(VALUE) ids.push_back(db->getObjectId(VALUE));
#define Invalid(VALUE)
#define Model(VALUE)
#define Internal(VALUE)
#define AnalyticalModel(VALUE)
#define ODBM_BUILTIN_ENUM_FN(NAME, VALUE, PARENT_CATEGORY, KIND, ...) KIND(VALUE)

	OdBmObjectIdArray ids;
	ODBM_BUILTIN_CATEGORIES(ODBM_BUILTIN_ENUM_FN);

#undef Annotation
#undef Invalid
#undef Model
#undef Internal
#undef AnalyticalModel

	// SunStudy is part of the Model category. We need to hide this explicitly.

	ids.push_back(db->getObjectId(OdBm::BuiltInCategory::OST_SunStudy));

	// As are a number of Lines

	ids.push_back(db->getObjectId(OdBm::BuiltInCategory::OST_Lines));

	// We also want to hide topography contours

	ids.push_back(db->getObjectId(OdBm::BuiltInCategory::OST_TopographyContours));
	ids.push_back(db->getObjectId(OdBm::BuiltInCategory::OST_SecondaryTopographyContours));
	ids.push_back(db->getObjectId(OdBm::BuiltInCategory::OST_ToposolidContours));
	ids.push_back(db->getObjectId(OdBm::BuiltInCategory::OST_ToposolidSecondaryContours));

	pView->setCategoryHidden(ids, true);

	pView->setActiveWorkPlaneVisibility(false);

	viewUpdate.commit();
	ODBM_TRANSACTION_END()
}

OdBmDBView3dPtr createDefault3DView(OdBmDatabasePtr pDb, std::string style)
{
	// (This implementation is based on the BmDBView3dCreateCmd.cpp TB_Commands
	// example in the BimRv samples.)

	OdBmDBView3dPtr pDBView3d; // (ODBM_TRANSACTION_BEGIN/END actually creates a try..catch block, so this must be defined outside for it to be returned.)

	ODBM_TRANSACTION_BEGIN(viewCreate, pDb)
	viewCreate.start();

	pDBView3d = OdBmDBView3d::createObject();
	pDb->addElement(pDBView3d); // element must be added to DB before setDefaultOrigin, createDefaultDrawingAndViewport, setPerspective

	if (style.empty()) {
		style = std::string("shaded");
	}

	if (style == "wireframe") {
		pDBView3d->setDisplayStyle(OdBm::ViewDisplayStyle::Wireframe);
	}
	else if (style == "hiddenline") {
		pDBView3d->setDisplayStyle(OdBm::ViewDisplayStyle::HiddenLine);
	}
	else if (style == "shaded") {
		pDBView3d->setDisplayStyle(OdBm::ViewDisplayStyle::Shaded);
	}
	else if (style == "shadedwithedges"){
		pDBView3d->setDisplayStyle(OdBm::ViewDisplayStyle::ShadedWithEdges);
	}
	else {
		throw repo::lib::RepoSceneProcessingException("Style: " + style + " is not a valid style. Must be wireframe, hiddenline, shadedwithedges or shaded");
	}

	pDBView3d->setDetailLevel(OdBm::ViewDetailLevel::Fine);
	pDBView3d->setViewDiscipline(OdBm::ViewDiscipline::Architectural); // Architectural shows all elements (search "About the View Discipline" in the Revit docs)
	pDBView3d->setParam(OdBm::BuiltInParameter::VIEW_PHASE_FILTER, OdBmObjectId::kNull); // Set the Phasing filter to None so no objects are filtered out
	pDBView3d->setDefaultOrigin();
	pDBView3d->setPerspective();

	// Shaded is the only style that does not draw lines - in this mode, we also
	// want to disable surface patterns, to be consistent with Revit's UI.
	// This is done by setting a Graphics Override for every Model category.

	if (style == "shaded") {
		OdBmOverrideGraphicSettingsPtr overrides = OdBmOverrideGraphicSettings::createObject();
		overrides->setProjForegroundPatternVisible(false);
		overrides->setSurfaceBackgroundPatternVisible(false);

		// See the comments in setupViewOverrides for how this macro works.

#define Annotation(NAME)
#define Invalid(NAME)
#define Model(NAME) pDBView3d->setCategoryOverrides(OdBm::BuiltInCategory::NAME, overrides);
#define Internal(NAME)
#define AnalyticalModel(NAME)
#define ODBM_BUILTIN_ENUM_FN(NAME, VALUE, PARENT_CATEGORY, KIND, ...) KIND(NAME)

		ODBM_BUILTIN_CATEGORIES(ODBM_BUILTIN_ENUM_FN);

#undef Annotation
#undef Invalid
#undef Model
#undef Internal
#undef AnalyticalModel
	}

	repoInfo << "Created view...";

	viewCreate.commit();
	ODBM_TRANSACTION_END();

	return pDBView3d;
}

OdGeExtents3d getModelBounds(OdBmDBViewPtr view)
{
	OdBmObjectIdArray elements;
	view->getVisibleDrawableElements(elements);

	repoInfo << "Getting bounds of " << elements.size() << " visible elements...";

	OdGeExtents3d model;
	for (auto& e : elements)
	{
		OdBmElementPtr element = e.safeOpenObject();
		OdGeExtents3d extents;
		model.addExt(element->getBBox()); // These are in model space, in Revits internal units
	}

	return model;
}

OdGeMatrix3d getModelToWorldMatrix(OdBmDatabasePtr pDb)
{
	OdBmElementTrackingDataPtr pElementTrackingDataMgr = pDb->getAppInfo(OdBm::ManagerType::ElementTrackingData);
	OdBmObjectIdArray aElements;
	OdResult res = pElementTrackingDataMgr->getElementsByType(
		pDb->getObjectId(OdBm::BuiltInCategory::OST_ProjectBasePoint),
		OdBm::TrackingElementType::Elements,
		aElements);

	if (!aElements.isEmpty())
	{
		OdBmBasePointPtr pThis = aElements.first().safeOpenObject();
		if (pThis->getLocationType() == 0)
		{
			if (OdBmGeoLocation::isGeoLocationAllowed(pThis->database()))
			{
				// Revit has three coordinate systems, defined relative to: Internal Origin,
				// Project Base Point and Survey Point.
				// The Survey Point is the one that defines the shared coordinate system of
				// a site; this is the coordinate system 3DRepo operates in, and is the one
				// shared in a Federation.

				// The Active Location is the Survey Point of the active Site. Revit can have
				// a number of Sites defined, each with exactly one Survey Point. 
				// This snippet initialises a matrix to convert from internal coordinates
				// (model space) to the shared coordinate system, including conversion to the
				// project's units. (Geometry still has to undergo the world offset applied to
				// a revision node.)

				OdBmGeoLocationPtr pActiveLocation = OdBmGeoLocation::getActiveLocationId(pThis->database()).safeOpenObject();
				OdGeMatrix3d activeTransform = pActiveLocation->getTransform();
				activeTransform.invert();

				return activeTransform;
			}
		}
	}
}

uint8_t FileProcessorRvt::readFile()
{
	int nRes = REPOERR_OK;
	OdStaticRxObject<RepoRvtServices> svcs;
	odrxInitialize(&svcs);

	::odrxDynamicLinker()->loadModule(OdBmLoaderModuleName, false);

	// Extensions should be initialised after the loader module, but before reading
	// a file
	OdRxSystemServices::desc()->addX(OdBmSystemServicesPE::desc(), static_cast<OdBmSystemServicesPE*>(&svcs));

	odgsInitialize();
	try
	{
		//.. change tessellation params here
		wrTriangulationParams triParams(USE_NEW_TESSELLATION);
		if (importConfig.getLevelOfDetail()) {
			triParams.bRecalculateSurfaceTolerance = false;
			triParams.surfaceTolerance = LOD_PARAMETERS[importConfig.getLevelOfDetail()].surfaceTolerance;
			triParams.normalTolerance = LOD_PARAMETERS[importConfig.getLevelOfDetail()].normalTolerance;
		}
		setTessellationParams(triParams);

		OdBmDatabasePtr pDb = svcs.readFile(getFilename());
		if (!pDb.isNull())
		{
			// The 'drawing' object corresponds to a named entry in the 'Views' list in
			// Revit, the drawing instance is a mostly empty container that points to an
			// instance of the View Family type, which is what holds the actual
			// properties.

			OdDbBaseDatabasePEPtr pDbPE(pDb);

			OdBmDBView3dPtr pView;
			if (!importConfig.getViewName().empty()) {
				pView = findView(pDb, importConfig.getViewName());
			}
			else {
				pView = createDefault3DView(pDb, importConfig.viewStyle);
			}

			setupViewOverrides(pView);
			auto modelToWorld = getModelToWorldMatrix(pDb); // World here is the shared or 'project' coordinate system			

			auto bounds = getModelBounds(pView);
			bounds.transformBy(modelToWorld);
			repoSceneBuilder->setWorldOffset(toRepoVector(bounds.minPoint()));

			OdGiDefaultContextPtr pBimContext = pDbPE->createGiContext(pDb);
			OdGsModulePtr pGsModule = ODRX_STATIC_MODULE_ENTRY_POINT(StubDeviceModuleRvt)(OD_T("StubDeviceModuleRvt"));

			GeometryCollector collector(repoSceneBuilder);
			((StubDeviceModuleRvt*)pGsModule.get())->init(&collector, pDb, pView, modelToWorld);
			OdGsDevicePtr pDevice = pGsModule->createDevice();

			pDbPE->setupLayoutView(pDevice, pBimContext, pView->getDbDrawingId());
			
			// NOTE: Render mode can be kFlatShaded, kGouraudShaded, kFlatShadedWithWireframe, kGouraudShadedWithWireframe
			// kHiddenLine mode prevents materails from being uploaded
			// Uncomment the setupRenderMode function call to change render mode
			//setupRenderMode(pDb, pDevice, pBimContext, OdGsView::kFlatShaded);

			OdGsDCRect screenRect(OdGsDCPoint(0, 0), OdGsDCPoint(1000, 1000)); //Set the screen space to the borders of the scene
			pDevice->onSize(screenRect);
			setupUnitsFormat(pDb, ROUNDING_ACCURACY);

			repoInfo << "Processing geometry...";

			pDevice->update();

			collector.finalise();
		}
	}
	catch (OdError& e)
	{
		repoError << convertToStdString(e.description()) << ", code: " << e.code();
		if (e.code() == OdResult::eUnsupportedFileFormat) {
			nRes = REPOERR_UNSUPPORTED_VERSION;
		}
		else {
			nRes = REPOERR_LOAD_SCENE_FAIL;
		}
	}
	catch (const std::exception& e)
	{
		// Rethrows the existing exception after attempting cleanup. Nested exceptions
		// are now the preferred way to report runtime issues. Try cleaning up ODA
		// though so that its destructors don't throw again & confuse the process exit.

		odgsUninitialize();
		odrxUninitialize();
		throw; 
	}

	odgsUninitialize();
	odrxUninitialize();

	return nRes;
}