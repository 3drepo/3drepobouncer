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

#include <DgGiContext.h>
#include <DgGsManager.h>

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
	uint8_t nRes = 0;               // Return value for the function

	OdString strStlFilename = OdString::kEmpty;

	/**********************************************************************/
	/* Initialize Runtime Extension environment                           */
	/**********************************************************************/
	odrxInitialize(&svcs);
	odgsInitialize();

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

		collector->units = determineModelUnits(pModel->getMasterUnit());

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
		collector->setOrigin(origin.x, origin.y, origin.z);
		// Color with #255 always defines backround. The background of the active model must be considered in the device palette.
		pPalCpy[255] = background;
		// Note: This method should be called to resolve "white background issue" before setting device palette
		bool bCorrected = OdDgColorTable::correctPaletteForWhiteBackground(pPalCpy.asArrayPtr());

		// Calculate deviations for dgn elements

		std::map<OdDbStub*, double > mapDeviations;

		/*OdDgElementIteratorPtr pIter = pModel->createGraphicsElementsIterator();

		for (; !pIter->done(); pIter->step())
		{
			OdGeExtents3d extElm;
			OdDgElementPtr pItem = pIter->item().openObject(OdDg::kForRead);

			if (pItem->isKindOf(OdDgGraphicsElement::desc()))
			{
				pItem->getGeomExtents(extElm);

				if (extElm.isValidExtents())
					mapDeviations[pItem->elementId()] = extElm.maxPoint().distanceTo(extElm.minPoint()) / 1e4;
			}
		}*/

		importDgn(pDb, pPalCpy.asArrayPtr(), 256, extModel);
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

void FileProcessorDgn::importDgn(OdDbBaseDatabase *pDb,
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