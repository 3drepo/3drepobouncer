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
#include "oda_vectorise_device.h"

#include <DgLine.h>      // This file puts OdDgLine3d in the output file


OdaFileProcessor::OdaFileProcessor(const std::string &inputFile, OdaGeometryCollector const *geoCollector)
	: file(inputFile),
	  collector(geoCollector)
{
}


OdaFileProcessor::~OdaFileProcessor()
{
}

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
			// Set device palette from dgn color table

			OdDgColorTablePtr clolortable = pDb->getColorTable();
			OdGsDevicePtr pDevice = OdaVectoriseDevice::createObject(OdaVectoriseDevice::k3dDevice, collector);

			const ODCOLORREF* refColors = OdDgColorTable::currentPalette(pDb);
			ODGSPALETTE pPalCpy;
			pPalCpy.insert(pPalCpy.begin(), refColors, refColors + 256);
			OdDgModelPtr pModel = pDb->getActiveModelId().safeOpenObject();
			ODCOLORREF background = pModel->getBackground();
			// Color with #255 always defines background. The background of the active model must be considered in the device palette.
			pPalCpy[255] = background;
			// Note: This method should be called to resolve "white background issue" before setting device palette
			bool bCorrected = OdDgColorTable::correctPaletteForWhiteBackground(pPalCpy.asArrayPtr());
			pDevice->setLogicalPalette(pPalCpy.asArrayPtr(), 256);
			pDevice->setBackgroundColor(background);

			// Find first active view.

			OdDgViewPtr pView;

			OdDgViewGroupPtr pViewGroup = pDb->getActiveViewGroupId().openObject(OdDg::kForRead);

			if (!pViewGroup.isNull())
			{
				OdDgElementIteratorPtr pIt = pViewGroup->createIterator();
				for (; !pIt->done(); pIt->step())
				{
					OdDgViewPtr pCurView = OdDgView::cast(pIt->item().openObject(OdDg::kForRead));

					if (pCurView.get() && pCurView->getVisibleFlag())
					{
						pView = pCurView;
						break;
					}
				}
			}

			if (!pView.isNull())
			{
				//create the context with OdDgView element given (to transmit some properties)

				OdGiContextForDgDatabasePtr pDgnContext = OdGiContextForDgDatabase::createObject(pDb, pView);
				pDgnContext->enableGsModel(true);

				OdDgElementId vectorizedViewId = pView->elementId();
				OdDgElementId vectorizedModelId = pView->getModelId();

				pDevice = OdGsDeviceForDgModel::setupModelView(vectorizedModelId, vectorizedViewId, pDevice, pDgnContext);

				for (int iii = 0; iii < pDevice->numViews(); iii++)
					pDevice->viewAt(iii)->setMode(OdGsView::kGouraudShaded);

				OdGsDCRect screenRect(OdGsDCPoint(0, 0), OdGsDCPoint(1000, 1000));
				pDevice->onSize(screenRect);

				pDevice->update();
			}

			OdaGiDumper::clearStlTriangles();
		}

		pDb = 0;
	}
	catch (OdError& e)
	{
		//FIXME: use repo log
		std::cerr << (L"\nTeigha(R) for .DGN Error: %ls\n", e.description().c_str());
	}
	catch (...)
	{
		/*STD(cout) << "\n\nUnexpected error.";*/
		nRes = -1;
		throw;
	}

	/**********************************************************************/
	/* Uninitialize Runtime Extension environment                         */
	/**********************************************************************/

	odgsUninitialize();
	::odrxUninitialize();

	return nRes;
}
