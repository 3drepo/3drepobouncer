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

#include <ExSystemServices.h>
#include <ExDgnServices.h>
#include <ExDgnHostAppServices.h>
#include <ExHostAppServices.h>

#include <DgGiContext.h>
#include <DgGsManager.h>
#include <Exports/DgnExport/DgnExport.h>

#include "../../../../lib/repo_exception.h"
#include "file_processor_dgn.h"
#include "data_processor_dgn.h"
#include "vectorise_device_dgn.h"
#include "helper_functions.h"

#include <DgLine.h>      // This file puts OdDgLine3d in the output file
using namespace repo::manipulator::modelconvertor::odaHelper;
using namespace TD_DGN_EXPORT;

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

class RepoDgnServices : public OdExDgnSystemServices, public OdExDgnHostAppServices
{
protected:
	ODRX_USING_HEAP_OPERATORS(OdExDgnSystemServices);
};

class RepoDwgServices : public  ExSystemServices, public ExHostAppServices
{
protected:
	ODRX_USING_HEAP_OPERATORS(ExSystemServices);
};

repo::manipulator::modelconvertor::odaHelper::FileProcessorDgn::~FileProcessorDgn()
{
}

uint8_t FileProcessorDgn::readFile() {
	uint8_t nRes = 0;               // Return value for the function
	OdStaticRxObject<RepoDgnServices> svcs;
	OdString fileSource = file.c_str();
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
		/*
		OdDgDatabasePtr pDb = svcs.readFile(fileSource);
		*/

		OdDgDatabasePtr pDb;
		// Register ODA Drawings API for DGN
		::odrxDynamicLinker()->loadModule(OdDbModuleName, false); // for instance, to draw .dwg file XRefs
		::odrxDynamicLinker()->loadModule(L"TG_DwgDb", false);
		// Dgn level table overrides for dwg reference attachments support
		::odrxDynamicLinker()->loadModule(L"ExDgnImportLineStyle");
		OdDgnExportModulePtr pModule = ::odrxDynamicLinker()->loadApp(OdDgnExportModuleName, false);
		OdDgnExportPtr pExporter = pModule->create();

		pExporter->properties()->putAt(L"DgnServices", static_cast<OdDgHostAppServices*>(&svcs));
		pExporter->properties()->putAt(L"DwgPath", OdRxVariantValue(OdString(fileSource)));

		OdDgnExport::ExportResult res = pExporter->exportDb();
		if (res == OdDgnExport::success)
			pDb = pExporter->properties()->getAt(L"DgnDatabase");
		else
		{
			switch (res)
			{
			case OdDgnExport::bad_database:
				throw new repo::lib::RepoException("Bad database");
			case OdDgnExport::bad_file:
				throw new repo::lib::RepoException("DGN export");

			case OdDgnExport::encrypted_file:
			case OdDgnExport::bad_password:
				throw new repo::lib::RepoException("The file is encrypted");

			case OdDgnExport::fail:
				throw new repo::lib::RepoException("Unknown import error");
			}
		}

		pExporter.release();

		if (!pDb.isNull())
		{
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
				repoError << "Can not find an active view group or all its views are disabled";
			}
			else
			{
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
			}
		}
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

int FileProcessorDgn::importDgn(OdDbBaseDatabase *pDb,
	const ODCOLORREF* pPallete,
	int numColors,
	const OdGeExtents3d &extModel,
	const OdGiDrawable* pEntity,
	const OdGeMatrix3d& matTransform,
	const std::map<OdDbStub*, double>* pMapDeviations)
{
	int ret = 0;
	OdUInt32 iCurEntData = 0;
	try
	{
		odgsInitialize();

		OdGsModulePtr pGsModule = ODRX_STATIC_MODULE_ENTRY_POINT(StubDeviceModuleDgn)(OD_T("StubDeviceModuleDgn"));

		((StubDeviceModuleDgn*)pGsModule.get())->init(collector, extModel);

		OdDbBaseDatabasePEPtr pDbPE(pDb);
		OdGiDefaultContextPtr pContext = pDbPE->createGiContext(pDb);
		{
			OdGsDevicePtr pDevice = pGsModule->createDevice();
			pDevice->setLogicalPalette(pPallete, numColors);

			if (pEntity)
			{
				//for one entity
				OdGsViewPtr pNewView = pDevice->createView();
				pNewView->setMode(OdGsView::kFlatShaded);
				pDevice->addView(pNewView);
				pNewView->add((OdGiDrawable*)pEntity, 0);
			}
			else
			{
				OdGsDevicePtr pHlpDevice = pDbPE->setupActiveLayoutViews(pDevice, pContext);
			}
			pDevice->setUserGiContext(pContext);

			OdGsDCRect screenRect(OdGsDCPoint(0, 1000), OdGsDCPoint(1000, 0));
			pDevice->onSize(screenRect);
			pDevice->update();
		}

		pContext.release();
		pGsModule.release();

		odgsUninitialize();
	}
	catch (OdError& e)
	{
		ret = e.code();
		repoError << convertToStdString(e.description());
	}
	catch (...)
	{
		ret = 1;
	}
	return ret;
}