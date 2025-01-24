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



#include "repo/lib/repo_utils.h"

//3d repo bouncer
#include "file_processor_rvt.h"
#include "data_processor_rvt.h"
#include "repo_system_services.h"
#include "helper_functions.h"

//help
#include "vectorise_device_rvt.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

const bool USE_NEW_TESSELLATION = true;
const double TRIANGULATION_EDGE_LENGTH = 1;
const double ROUNDING_ACCURACY = 0.000001;

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
	GeometryCollector *collector;

public:
	void init(GeometryCollector* const collector, OdBmDatabasePtr database, OdBmDBViewPtr view)
	{
		this->collector = collector;
		this->database = database;
		this->view = view;
	}
protected:
	OdSmartPtr<OdGsBaseVectorizeDevice> createDeviceObject()
	{
		return OdRxObjectImpl<VectoriseDeviceRvt, OdGsBaseVectorizeDevice>::createObject();
	}
	OdSmartPtr<OdGsViewImpl> createViewObject()
	{
		OdSmartPtr<OdGsViewImpl> pP = OdRxObjectImpl<DataProcessorRvt, OdGsViewImpl>::createObject();
		((DataProcessorRvt*)pP.get())->initialise(collector, database, view);
		return pP;
	}
};
ODRX_DEFINE_PSEUDO_STATIC_MODULE(StubDeviceModuleRvt);

class RepoRvtServices : public RepoSystemServices, public OdExBimHostAppServices
{
protected:
	ODRX_USING_HEAP_OPERATORS(RepoSystemServices);
};

#pragma optimize("",off)

OdBmDBDrawingPtr Get3DDrawing(OdDbBaseDatabasePEPtr baseDatabase, OdBmDatabasePtr bimDatabase)
{
	// Layouts correspond to the Views listed in the Project Browser in Revit.

	OdRxIteratorPtr layouts = baseDatabase->layouts(bimDatabase);
	bool navisNameFound = false;

	OdBmDBDrawingPtr currentpDBDrawing;
	for (; !layouts->done(); layouts->next())
	{
		OdBmDBDrawingPtr pDBDrawing = layouts->object();
		if (pDBDrawing->getBaseViewType() == OdBm::ViewType::ThreeD)
		{
			OdDbBaseLayoutPEPtr pLayout(pDBDrawing);
			auto viewNameStr = convertToStdString(pDBDrawing->getTitle());

			//set first 3D view available
			if (!currentpDBDrawing) {
				currentpDBDrawing = pDBDrawing;
				repoInfo << "First layout name is " << viewNameStr;
			}

			// Make lowercase for the comparisons below
			repo::lib::toLower(viewNameStr);

			//3D view called "3D Repo" should return straight away
			if (viewNameStr.find("3drepo") != std::string::npos || viewNameStr.find("3d repo") != std::string::npos) {
				repoInfo << "Found 3D view named 3D Repo.";
				return pDBDrawing;
			}

			if (!navisNameFound) {
				navisNameFound = viewNameStr.find("navis") != std::string::npos;
				if (navisNameFound) {
					//3D View called "navis" has precedence over default 3D view
					repoInfo << "Found view containing \"navis\".";
					currentpDBDrawing = pDBDrawing;
				}
				else if (pDBDrawing->getName().iCompare("{3D}") == 0) { // For checking default 3D view, we use the drawings true name for an exact match
					repoInfo << "Found default named 3D view.";
					currentpDBDrawing = pDBDrawing;
				}
			}
		}
	}

	return currentpDBDrawing;
}

FileProcessorRvt::FileProcessorRvt(const std::string& inputFile,
	modelutility::RepoSceneBuilder* builder,
	const ModelImportConfig& config) :FileProcessor(inputFile, builder, config)
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

uint8_t FileProcessorRvt::readFile()
{
	int nRes = REPOERR_OK;
	OdStaticRxObject<RepoRvtServices> svcs;
	odrxInitialize(&svcs);
	OdRxModule* pModule = ::odrxDynamicLinker()->loadModule(OdBmLoaderModuleName, false);
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

		OdBmDatabasePtr pDb = svcs.readFile(OdString(file.c_str()));
		if (!pDb.isNull())
		{
			OdDbBaseDatabasePEPtr pDbPE(pDb);
			auto drawing = Get3DDrawing(pDbPE, pDb);

			if (!drawing) {
				throw repo::lib::RepoSceneProcessingException("Could not find a suitable view.", REPOERR_VALID_3D_VIEW_NOT_FOUND);
			}
			repoInfo << "Using 3D View: " << convertToStdString(drawing->getShortDescriptiveName());

			OdGiDefaultContextPtr pBimContext = pDbPE->createGiContext(pDb);
			OdGsModulePtr pGsModule = ODRX_STATIC_MODULE_ENTRY_POINT(StubDeviceModuleRvt)(OD_T("StubDeviceModuleRvt"));

			// The view is the parameterised Revit Type instance belonging to the named drawing

			OdBmDBViewPtr view = drawing->getBaseDBViewId().safeOpenObject();	

			GeometryCollector collector(repoSceneBuilder);
			((StubDeviceModuleRvt*)pGsModule.get())->init(&collector, pDb, view);
			OdGsDevicePtr pDevice = pGsModule->createDevice();

			pDbPE->setupLayoutView(pDevice, pBimContext, drawing->objectId());

			OdBmObjectIdArray elements;
			view->getVisibleDrawableElements(elements);

			repo::lib::RepoBounds bounds;
			for (auto& e : elements)
			{
				OdGeExtents3d extents;
				OdBmElementPtr element = e.safeOpenObject();
				element->getGeomExtents(extents); // These are in model space, in the file's native units
				auto min = extents.minPoint();
				bounds.encapsulate(repo::lib::RepoVector3D64(min.x, min.y, min.z));
			}

			repoInfo << bounds.min().x << " " << bounds.min().y << " " << bounds.min().z;

			// NOTE: Render mode can be kFlatShaded, kGouraudShaded, kFlatShadedWithWireframe, kGouraudShadedWithWireframe
			// kHiddenLine mode prevents materails from being uploaded
			// Uncomment the setupRenderMode function call to change render mode
			//setupRenderMode(pDb, pDevice, pBimContext, OdGsView::kFlatShaded);

			OdGsDCRect screenRect(OdGsDCPoint(0, 0), OdGsDCPoint(1000, 1000)); //Set the screen space to the borders of the scene
			pDevice->onSize(screenRect);
			setupUnitsFormat(pDb, ROUNDING_ACCURACY);
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
	odgsUninitialize();
	odrxUninitialize();

	return nRes;
}