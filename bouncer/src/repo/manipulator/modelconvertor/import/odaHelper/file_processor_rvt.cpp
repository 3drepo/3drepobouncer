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

//3d repo bouncer
#include "file_processor_rvt.h"

//help
#include "vectorise_device_rvt.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

class RepoRvtServices : public ExSystemServices, public OdExBimHostAppServices
{
protected:
    ODRX_USING_HEAP_OPERATORS(ExSystemServices);
};

OdString Get3DLayout(OdDbBaseDatabasePEPtr baseDatabase, OdBmDatabasePtr bimDatabase)
{
	OdRxIteratorPtr layouts = baseDatabase->layouts(bimDatabase);

	for (; !layouts->done(); layouts->next())
	{
		OdDbBaseLayoutPEPtr pLayout(layouts->object());
		OdString layoutName = pLayout->name(layouts->object());
		if (layoutName.find(L"3D") != -1)
			return layoutName;
	}

	return OdString();
}

repo::manipulator::modelconvertor::odaHelper::FileProcessorRvt::~FileProcessorRvt()
{

}

int FileProcessorRvt::readFile()
{
	int nRes = 1;
	OdStaticRxObject<RepoRvtServices> svcs;
	odrxInitialize(&svcs);
	OdRxModule* pModule = ::odrxDynamicLinker()->loadModule(OdBmLoaderModuleName, false);
	try
	{
		odgsInitialize();
		OdBmDatabasePtr pDb = svcs.readFile(OdString(file.c_str()));
		if (!pDb.isNull())
		{
			OdDbBaseDatabasePEPtr pDbPE(pDb);
			OdString layout = Get3DLayout(pDbPE, pDb);
			if (!layout.isEmpty())
				pDbPE->setCurrentLayout(pDb, layout);

			OdGiContextForBmDatabasePtr pBimContext = OdGiContextForBmDatabase::createObject();

			OdGsDevicePtr pDevice = OdRxObjectImpl<VectoriseDeviceRvt, OdGsDevice>::createObject();
			(static_cast<VectoriseDeviceRvt*>(pDevice.get()))->init(collector);

			pBimContext->setDatabase(pDb);
			pDevice = pDbPE->setupActiveLayoutViews(pDevice, pBimContext);
			OdGsDCRect screenRect(OdGsDCPoint(0, 0), OdGsDCPoint(1000, 1000)); //Set the screen space to the borders of the scene
			pDevice->onSize(screenRect);
			pDevice->update();
		}
		nRes = 0;
	}
	catch (OdError& e)
	{
		nRes = e.code();
		repoError << e.description().c_str();
	}
	catch (...)
	{
		nRes = 1;
	}
	return nRes;
}

