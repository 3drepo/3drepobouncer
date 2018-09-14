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

#include <DgGiContext.h>
#include <DgGsManager.h>

#include "oda_file_processor.h"
#include "oda_gi_dumper.h"
#include "oda_gi_geo_dumper.h"
#include "oda_vectorise_device.h"

#include <DgLine.h>      // This file puts OdDgLine3d in the output file
using namespace repo::manipulator::modelconvertor::odaHelper;

OdaFileProcessor::OdaFileProcessor(const std::string &inputFile, OdaGeometryCollector *geoCollector)
	: file(inputFile),
	  collector(geoCollector)
{
}


OdaFileProcessor::~OdaFileProcessor()
{
}

class StubDeviceModuleText : public OdGsBaseModule
{
private:
	OdaGeometryCollector *collector;
	OdGeMatrix3d        m_matTransform;
	const std::map<OdDbStub*, double>* m_pMapDeviations;

public:
	void init(OdaGeometryCollector* const collector)
	{
		this->collector = collector;
	}
protected:
	OdSmartPtr<OdGsBaseVectorizeDevice> createDeviceObject()
	{
		return OdRxObjectImpl<OdaVectoriseDevice, OdGsBaseVectorizeDevice>::createObject();
	}
	OdSmartPtr<OdGsViewImpl> createViewObject()
	{
		OdSmartPtr<OdGsViewImpl> pP = OdRxObjectImpl<OdGiConveyorGeometryDumper, OdGsViewImpl>::createObject();
		((OdGiConveyorGeometryDumper*)pP.get())->init(collector);
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
ODRX_DEFINE_PSEUDO_STATIC_MODULE(StubDeviceModuleText);

class RepoDgnServices : public OdExDgnSystemServices, public OdExDgnHostAppServices
{
protected:
	ODRX_USING_HEAP_OPERATORS(OdExDgnSystemServices);
};

int OdaFileProcessor::readFile() {

	int   nRes = 0;               // Return value for the function
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

		OdDgDatabasePtr pDb = svcs.readFile(fileSource);

		if (!pDb.isNull())
		{

			const ODCOLORREF* refColors = OdDgColorTable::currentPalette(pDb);
			ODGSPALETTE pPalCpy;
			pPalCpy.insert(pPalCpy.begin(), refColors, refColors + 256);


			OdDgElementId elementId = pDb->getActiveModelId();
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
				pModel->getGeomExtents(vectorizedViewId, extModel);
				// Color with #255 always defines backround. The background of the active model must be considered in the device palette.
				pPalCpy[255] = background;
				// Note: This method should be called to resolve "white background issue" before setting device palette
				bool bCorrected = OdDgColorTable::correctPaletteForWhiteBackground(pPalCpy.asArrayPtr());

				// Calculate deviations for dgn elements

				std::map<OdDbStub*, double > mapDeviations;

				OdDgElementIteratorPtr pIter = pModel->createGraphicsElementsIterator();

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
				}

				importDgn(pDb, pPalCpy.asArrayPtr(), 256);			
			}
		}
	}
	catch (std::exception &e)
	{
		repoError << "Failed: " << e.what();
	}
	return nRes;
}

int OdaFileProcessor::importDgn(OdDbBaseDatabase *pDb,
	const ODCOLORREF* pPallete,
	int numColors,
	const OdGiDrawable* pEntity,
	const OdGeMatrix3d& matTransform,
	const std::map<OdDbStub*, double>* pMapDeviations)
{

	int ret = 0;
	OdUInt32 iCurEntData = 0;
	try
	{
		odgsInitialize();
		
		OdGsModulePtr pGsModule = ODRX_STATIC_MODULE_ENTRY_POINT(StubDeviceModuleText)(OD_T("StubDeviceModuleText"));

		((StubDeviceModuleText*)pGsModule.get())->init(collector);


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
		repoError << e.description().c_str();
	}
	catch (...)
	{
		ret = 1;
	}
	return ret;
}
