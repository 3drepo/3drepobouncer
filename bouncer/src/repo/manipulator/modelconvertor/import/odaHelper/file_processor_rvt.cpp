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
#include "Common/BmBuildSettings.h"
#include <OdaCommon.h>
#include <StaticRxObject.h>
#include <RxInit.h>
#include <RxDynamicModule.h>
#include <DynamicLinker.h>
#include <RxDynamicModule.h>
#include <ExSystemServices.h>
#include "ExBimHostAppServices.h"
#include "BimCommon.h"
#include "RxObjectImpl.h"
#include "Database/BmDatabase.h"
#include "Database/GiContextForBmDatabase.h"
#include "Database/Entities/BmDBDrawing.h"
#include "ModelerGeometry/BmModelerModule.h"
#include "Wr/wrTriangulationParams.h"
#include "Database/Entities/BmUnitsElem.h"
#include "Database/BmUnitUtils.h"
#include "Database/Managers/BmUnitsTracking.h"

//3d repo bouncer
#include "file_processor_rvt.h"

//help
#include "vectorise_device_rvt.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

const double TRIANGULATION_EDGE_LENGTH = 1.0;
const double ROUNDING_ACCURACY = 0.000001;

class RepoRvtServices : public ExSystemServices, public OdExBimHostAppServices
{
protected:
    ODRX_USING_HEAP_OPERATORS(ExSystemServices);
};

OdString Get3DLayout(OdDbBaseDatabasePEPtr baseDatabase, OdBmDatabasePtr bimDatabase)
{
	OdString layoutName;
	OdRxIteratorPtr layouts = baseDatabase->layouts(bimDatabase);

	for (; !layouts->done(); layouts->next())
	{
		OdBmDBDrawingPtr pDBDrawing = layouts->object();
		OdDbBaseLayoutPEPtr pLayout(layouts->object());
		
		if (pDBDrawing->getBaseViewNameFormat() == OdBm::ViewType::_3d)
		{
			//.. NOTE: set first 3D view available
			if (layoutName.isEmpty())
				layoutName = pLayout->name(layouts->object());
			
			//.. NOTE: try to find 3D view with a valid default name
			if (pDBDrawing->getName().find(L"3D") != -1)
				return pLayout->name(layouts->object());
		}
	}

	return layoutName;
}

repo::manipulator::modelconvertor::odaHelper::FileProcessorRvt::~FileProcessorRvt()
{

}

void FileProcessorRvt::setMaxEdgeLength(double edgeLength)
{
	OdSmartPtr<BmModelerModule> pModModule;
	wrTriangulationParams TriangulationParams; //.. NOTE: This structure contains fields related to triangulation
	OdRxModulePtr pModule = odrxDynamicLinker()->loadModule(OdBmModelerModuleName);
	if (pModule.get())
	{
		pModModule = BmModelerModule::cast(pModule);
		pModModule->getTriangulationParams(TriangulationParams);
		//.. NOTE: We may adjust triangulation options. sometimes it creates a lot of triangles
		TriangulationParams.maxFacetEdgeLength = edgeLength; 
		pModModule->setTriangulationParams(TriangulationParams);
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

	OdBmFormatOptionsPtrArray formatOptionsArr;
	units->getFormatOptionsArr(formatOptionsArr);
	for (uint32_t i = 0; i < formatOptionsArr.size(); i++)
	{
		OdBmFormatOptions* formatOptions = formatOptionsArr[i];
		//.. NOTE: Here the format of units is configured
		formatOptions->setAccuracy(accuracy);
		formatOptions->setRoundingMethod(OdBm::RoundingMethod::Nearest);
	}
}

uint8_t FileProcessorRvt::readFile()
{
	int nRes = REPOERR_OK;
	OdStaticRxObject<RepoRvtServices> svcs;
	odrxInitialize(&svcs);
	OdRxModule* pModule = ::odrxDynamicLinker()->loadModule(OdBmLoaderModuleName, false);
	try
	{
		setMaxEdgeLength(TRIANGULATION_EDGE_LENGTH);

		odgsInitialize();
		OdBmDatabasePtr pDb = svcs.readFile(OdString(file.c_str()));
		if (!pDb.isNull())
		{
			OdDbBaseDatabasePEPtr pDbPE(pDb);
			OdString layout = Get3DLayout(pDbPE, pDb);
			if (layout.isEmpty())
				return REPOERR_VALID_3D_VIEW_NOT_FOUND;

			pDbPE->setCurrentLayout(pDb, layout);

			OdGiContextForBmDatabasePtr pBimContext = OdGiContextForBmDatabase::createObject();

			OdGsDevicePtr pDevice = OdRxObjectImpl<VectoriseDeviceRvt, OdGsDevice>::createObject();
			(static_cast<VectoriseDeviceRvt*>(pDevice.get()))->init(collector, pDb);

			pBimContext->setDatabase(pDb);
			pDevice = pDbPE->setupActiveLayoutViews(pDevice, pBimContext);
			OdGsDCRect screenRect(OdGsDCPoint(0, 0), OdGsDCPoint(1000, 1000)); //Set the screen space to the borders of the scene
			pDevice->onSize(screenRect);
			setupUnitsFormat(pDb, ROUNDING_ACCURACY);
			pDevice->update();
		}
	}
	catch (OdError& e)
	{
		nRes = REPOERR_LOAD_SCENE_FAIL;
		repoError << e.description().c_str();
	}

	return nRes;
}

