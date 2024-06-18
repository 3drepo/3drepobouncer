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
#include <OdaCommon.h>
#include <StaticRxObject.h>
#include <RxInit.h>
#include <RxDynamicModule.h>
#include <DynamicLinker.h>
#include <DgDatabase.h>
#include <RxDynamicModule.h>
#include <MemoryStream.h>

#include <DgGiContext.h>
#include <DgGsManager.h>
#include <DbGsManager.h> // Requires TD_DbCore
#include <GiContextForDbDatabase.h>
#include <../Exports/2dExport/Include/2dExportDevice.h>

#include "../../../../lib/repo_exception.h"
#include "../repo_model_units.h"
#include "file_processor_dgn.h"
#include "data_processor_dgn.h"
#include "vectorise_device_dgn.h"
#include "helper_functions.h"

#include <DgLine.h>      // This file puts OdDgLine3d in the output file
using namespace repo::manipulator::modelconvertor::odaHelper;

using ModelUnits = repo::manipulator::modelconvertor::ModelUnits;

class StubDeviceModuleDgn : public OdGsBaseModule
{
private:
	GeometryCollector *collector;
	OdGeMatrix3d        m_matTransform;
	OdGeExtents3d extModel;

public:
	void init(GeometryCollector* const collector, const OdGeExtents3d &extModel)
	{
		this->collector = collector;
		this->extModel = extModel;
	}
protected:
	OdSmartPtr<OdGsBaseVectorizeDevice> createDeviceObject()
	{
		return OdRxObjectImpl<VectoriseDeviceDgn, OdGsBaseVectorizeDevice>::createObject();
	}
	OdSmartPtr<OdGsViewImpl> createViewObject()
	{
		OdSmartPtr<OdGsViewImpl> pP = OdRxObjectImpl<DataProcessorDgn, OdGsViewImpl>::createObject();
		((DataProcessorDgn*)pP.get())->init(collector, extModel);
		return pP;
	}
	OdSmartPtr<OdGsBaseVectorizeDevice> createBitmapDeviceObject()
	{
		return OdSmartPtr<OdGsBaseVectorizeDevice>();
	}
	OdSmartPtr<OdGsViewImpl> createBitmapViewObject()
	{
		return OdSmartPtr<OdGsViewImpl>();
	}
};
ODRX_DEFINE_PSEUDO_STATIC_MODULE(StubDeviceModuleDgn);

repo::manipulator::modelconvertor::odaHelper::FileProcessorDgn::~FileProcessorDgn()
{
}

OdDgDatabasePtr FileProcessorDgn::initialiseOdDatabase() {
	OdString fileSource = file.c_str();
	return svcs.readFile(fileSource);
}

ModelUnits FileProcessorDgn::determineModelUnits(const OdDgModel::UnitMeasure &units) {
	switch (units) {
	case OdDgModel::UnitMeasure::kMeters: return ModelUnits::METRES;
	case OdDgModel::UnitMeasure::kDecimeters: return ModelUnits::DECIMETRES;
	case OdDgModel::UnitMeasure::kCentimeters: return ModelUnits::CENTIMETRES;
	case OdDgModel::UnitMeasure::kMillimeters: return ModelUnits::MILLIMETRES;
	case OdDgModel::UnitMeasure::kFeet: return ModelUnits::FEET;
	case OdDgModel::UnitMeasure::kInches: return ModelUnits::INCHES;
	default:
		repoWarning << "Unrecognised unit measure: " << (int)units;
		return ModelUnits::UNKNOWN;
	}
}

uint8_t FileProcessorDgn::readFile() {

	/**********************************************************************/
	/* Initialize Runtime Extension environment                           */
	/**********************************************************************/
	odrxInitialize(&svcs);
	odgsInitialize();

	uint8_t nRes = 0;               // Return value for the function
	try
	{
		/**********************************************************************/
		/* Initialize Teigha™ for .dgn files                                               */
		/**********************************************************************/
		::odrxDynamicLinker()->loadModule(L"TG_Db", false);

		OdDgDatabasePtr pDb = initialiseOdDatabase();

		if (pDb.isNull()) {
			throw new repo::lib::RepoException("Could not establish OdDgDatabasePtr from file");
		}

		const ODCOLORREF* refColors = OdDgColorTable::currentPalette(pDb);
		ODGSPALETTE pPalCpy;
		pPalCpy.insert(pPalCpy.begin(), refColors, refColors + 256);

		OdDgElementId elementId = pDb->getActiveModelId();

		if (elementId.isValid()) {
			OdDgModelPtr viewElement = elementId.openObject(OdDg::kForRead);
			auto activeViewName = convertToStdString(viewElement->getName());
			if (viewElement->getType() != OdDgModel::Type::kDesignModel || activeViewName == "Title")
			{
				OdDgElementIteratorPtr pModelIter = pDb->getModelTable()->createIterator();
				for (; !pModelIter->done(); pModelIter->step())
				{
					OdDgModelPtr elem = pModelIter->item().openObject();
					auto viewName = convertToStdString(elem->getName());
					if (!elem.isNull() && elem->getType() == OdDgModel::Type::kDesignModel &&
						viewName != "Title") {
						repoInfo << "Setting active view to: " << viewName;
						pDb->setActiveModelId(pModelIter->item());
						elementId = pModelIter->item();
						break;
					}
				}
			}
		}

		if (elementId.isNull())
		{
			elementId = pDb->getDefaultModelId();
			pDb->setActiveModelId(elementId);
		}
		OdDgElementId elementActId = pDb->getActiveModelId();
		OdDgModelPtr pModel = elementId.safeOpenObject();
		ODCOLORREF background = pModel->getBackground();

		if (collector) {
			collector->units = determineModelUnits(pModel->getMasterUnit());
		}

		OdDgElementId vectorizedViewId;
		OdDgViewGroupPtr pViewGroup = pDb->getActiveViewGroupId().openObject();
		if (pViewGroup.isNull())
		{
			//  Some files can have invalid id for View Group. Try to get & use a valid (recommended) View Group object.
			pViewGroup = pDb->recommendActiveViewGroupId().openObject();
			if (pViewGroup.isNull())
			{
				// Add View group
				pModel->createViewGroup();
				pModel->fitToView();
				pViewGroup = pDb->recommendActiveViewGroupId().openObject();
			}
		}
		if (!pViewGroup.isNull())
		{
			OdDgElementIteratorPtr pIt = pViewGroup->createIterator();
			for (; !pIt->done(); pIt->step())
			{
				OdDgViewPtr pView = OdDgView::cast(pIt->item().openObject());
				if (pView.get() && pView->getVisibleFlag())
				{
					vectorizedViewId = pIt->item();
					break;
				}
			}
		}

		if (vectorizedViewId.isNull() && !pViewGroup.isNull())
		{
			OdDgElementIteratorPtr pIt = pViewGroup->createIterator();

			if (!pIt->done())
			{
				OdDgViewPtr pView = OdDgView::cast(pIt->item().openObject(OdDg::kForWrite));

				if (pView.get())
				{
					pView->setVisibleFlag(true);
					vectorizedViewId = pIt->item();
				}
			}
		}

		if (vectorizedViewId.isNull())
		{
			throw new repo::lib::RepoException("Can not find an active view group or all its views are disabled");
		}

		repoInfo << "Importing view: " <<
			convertToStdString(OdDgView::cast(vectorizedViewId.openObject(OdDg::kForRead))->getName()) <<
			" (Group: '" << convertToStdString(pViewGroup->getName()) << "') " <<
			" (Model: '" << convertToStdString(OdDgModel::cast(pViewGroup->getModelId().openObject(OdDg::kForRead))->getName()) << "')";

		OdGeExtents3d extModel;
		//pModel->getGeomExtents(vectorizedViewId, extModel);
		auto origin = pModel->getGlobalOrigin();
		if (collector) {
			collector->setOrigin(origin.x, origin.y, origin.z);
		}
		// Color with #255 always defines backround. The background of the active model must be considered in the device palette.
		pPalCpy[255] = background;
		// Note: This method should be called to resolve "white background issue" before setting device palette
		bool bCorrected = OdDgColorTable::correctPaletteForWhiteBackground(pPalCpy.asArrayPtr());

		// Do the actual import
		if (drawingCollector)
		{
			importDrawing(pDb, pPalCpy.asArrayPtr(), 256, vectorizedViewId);
		}

		if (collector)
		{
			importModel(pDb, pPalCpy.asArrayPtr(), 256, extModel);
		}

		pDb.release();
		odgsUninitialize();
		odrxUninitialize();
	}
	catch (OdError & e)
	{
		repoError << convertToStdString(e.description());
		nRes = REPOERR_MODEL_FILE_READ;
	}
	catch (std::exception &e)
	{
		repoError << "Failed: " << e.what();
		nRes = REPOERR_MODEL_FILE_READ;
	}
	return nRes;
}

void FileProcessorDgn::importModel(OdDbBaseDatabase *pDb,
	const ODCOLORREF* pPallete,
	int numColors,
	const OdGeExtents3d &extModel,
	const OdGiDrawable* pEntity,
	const OdGeMatrix3d& matTransform,
	const std::map<OdDbStub*, double>* pMapDeviations)
{
	OdUInt32 iCurEntData = 0;

	OdGsModulePtr pGsModule = ODRX_STATIC_MODULE_ENTRY_POINT(StubDeviceModuleDgn)(OD_T("StubDeviceModuleDgn"));

	((StubDeviceModuleDgn*)pGsModule.get())->init(collector, extModel);

	OdDbBaseDatabasePEPtr pDbPE(pDb);
	OdGiDefaultContextPtr pContext = pDbPE->createGiContext(pDb);
	OdGsDevicePtr pDevice = pGsModule->createDevice();
	pDevice->setLogicalPalette(pPallete, numColors);
	OdGsDevicePtr pHlpDevice = pDbPE->setupActiveLayoutViews(pDevice, pContext);

	pDevice->setUserGiContext(pContext);

	OdGsDCRect screenRect(OdGsDCPoint(0, 1000), OdGsDCPoint(1000, 0));
	pDevice->onSize(screenRect);
	pDevice->update();

	pContext.release();
	pGsModule.release();
}

#pragma optimize("", off)

void FileProcessorDgn::importDrawing(OdDgDatabasePtr pDb, const ODCOLORREF* pPallete, int numColors, OdDgElementId view)
{
	OdGsModulePtr pModule = ::odrxDynamicLinker()->loadModule(OdSvgExportModuleName, false);
	OdGsDevicePtr dev = pModule->createDevice();
	if (!dev.isNull())
	{
		// This snippet sets the output of the device. Here is a memory stream
		// but it may also be a file created with ::odrxSystemServices().

		OdMemoryStreamPtr stream = OdMemoryStream::createNew();
		dev->properties()->putAt("Output", stream.get());

		// Prepare database context for device

		OdGiContextForDgDatabasePtr pDwgContext = OdGiContextForDgDatabase::createObject();

		pDwgContext->setDatabase(pDb);
		// do not render paper background and borders
		pDwgContext->setPlotGeneration(true);
		// Avoid tessellation if possible
		pDwgContext->setHatchAsPolygon(OdGiDefaultContext::kHatchPolygon);
		// Set minimal line width in SVG so that too thin line would not disappear
		dev->properties()->putAt(L"MinimalWidth", OdRxVariantValue(0.1));

		// load plotstyle tables (as set in layout settings)
		OdDbBaseDatabasePEPtr pBaseDatabase(pDb);
		pBaseDatabase->loadPlotstyleTableForActiveLayout(pDwgContext, pDb);

		// Prepare the device to render the active layout in this database.
		//OdGsDevicePtr wrapper = OdDbGsManager::setupActiveLayoutViews(dev, pDwgContext);

		// creating render device for rendering the shaded viewports

		// This has two parts. First we create the device that will render the
		// svg data. Then, we create the OdGsView from the OdDgView found
		// previously, and add the view to the device
		// https://docs.opendesign.com/tv/gs_OdGsView.html
		// This is done by the setupModelView method. This method returns a
		// wrapper with some overrides made.

		auto pView = OdDgView::cast(view.safeOpenObject());
		TD_2D_EXPORT::Od2dExportDevice* pDeviceSvg = (TD_2D_EXPORT::Od2dExportDevice*)dev.get();
		auto pDeviceForDg = OdGsDeviceForDgModel::setupModelView(pView->getModelId(), pView->elementId(), pDeviceSvg, pDwgContext);

		// Continue any final setup

		pDeviceForDg->setLogicalPalette(pPallete, numColors);

		// This section configures the device - all we do for now is set the
		// viewport size

		pDeviceForDg->onSize(OdGsDCRect(0, 1024, 768, 0));

		// This section extracts the view information which can be used to map
		// betweeen the SVG and world coordinate sytsems

		const OdGsView* pGsView = pDeviceSvg->viewAt(0);

		auto worldToDeviceMatrix = pGsView->worldToDeviceMatrix();
		auto objectToDeviceMatrix = pGsView->objectToDeviceMatrix();

		// Pick three points (two vectors) to describe the map. The transform
		// can be computed from these each time from then on.

		OdGePoint3d a(0, 0, 0);
		OdGePoint3d b(1, 0, 0);
		OdGePoint3d c(0, 1, 0);

		// The following vector contains the points in the SVG file corresponding
		// to the 3D coordinates above. Add these to the appropriate schema when
		// it is ready...

		std::vector<OdGePoint3d> points;
		points.push_back(worldToDeviceMatrix * a);
		points.push_back(worldToDeviceMatrix * b);
		points.push_back(worldToDeviceMatrix * c);

		// The call to update is what will create the svg in the memory stream

		pDeviceForDg->update();

		// Finally copy the contents of the stream to the collector's buffer;
		// getBytes advances OdMemoryStream, so we must first seek its start.

		drawingCollector->data.resize(stream->tell());
		stream->seek(0, OdDb::FilerSeekType::kSeekFromStart);
		stream->getBytes(drawingCollector->data.data(), stream->length());
	}
}