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
#include "ExBimServices.h"
#include "ExBimHostAppServices.h"
#include "BimCommon.h"
#include "RxObjectImpl.h"
#include "Database/BmDatabase.h"
#include "Database/GiContextForBmDatabase.h"
#include "DynamicLinker.h"
#include "RxInit.h"

//3d repo bouncer
#include "file_processor_rvt.h"
#include "vectorise_device.h"

//help
#include "BmGiDumperImpl.h"
#include "ExBmGsSimpleDevice.h"

#include <DgLine.h>      // This file puts OdDgLine3d in the output file
using namespace repo::manipulator::modelconvertor::odaHelper;
/*
class BimStubDeviceModuleText : public OdGsBaseModule
{
private:
    GeometryCollector *collector;
    OdGeMatrix3d        m_matTransform;
    const std::map<OdDbStub*, double>* m_pMapDeviations;

public:
    void init(GeometryCollector* const collector)
    {
        this->collector = collector;
    }
protected:
    OdSmartPtr<OdGsBaseVectorizeDevice> createDeviceObject()
    {
        return OdRxObjectImpl<VectoriseDevice, OdGsBaseVectorizeDevice>::createObject();
    }
    OdSmartPtr<OdGsViewImpl> createViewObject()
    {
        OdSmartPtr<OdGsViewImpl> pP = OdRxObjectImpl<GeometryDumper, OdGsViewImpl>::createObject();
        ((GeometryDumper*)pP.get())->init(collector);
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
ODRX_DEFINE_PSEUDO_STATIC_MODULE(BimStubDeviceModuleText);
*/

class MyServices : public OdExBimSystemServices, public OdExBimHostAppServices
{
protected:
    ODRX_USING_HEAP_OPERATORS(OdExBimSystemServices);
};

FileProcessorBim::FileProcessorBim(const std::string &inputFile, GeometryCollector *geoCollector) : file(inputFile), collector(geoCollector)
{}

FileProcessorBim::~FileProcessorBim()
{}

int FileProcessorBim::readFile()
{
    return importBIM();
}

int FileProcessorBim::importBIM()
{
    int nRes = 1;

    // Create a custom Services object.
    OdStaticRxObject<MyServices> svcs;

    // initialize Kernel
    odrxInitialize(&svcs);

    repoInfo << L"Developed using " << svcs.product().c_str() << " ver " << svcs.versionString().c_str() << '\n';

    OdRxModule* pModule = ::odrxDynamicLinker()->loadModule(OdBmLoaderModuleName, false);

    try
    {
        // initialize GS subsystem
        odgsInitialize();

        //OdGsModulePtr pGsModule = ODRX_STATIC_MODULE_ENTRY_POINT(BimStubDeviceModuleText)(OD_T("BimStubDeviceModuleText"));
        //((BimStubDeviceModuleText*)pGsModule.get())->init(collector);

        OdBmDatabasePtr pDb = svcs.readFile(OdString(file.c_str()));

        if (!pDb.isNull())
        {
            repoInfo << "DB Initialized...\n";

            //OdGiContextForBmDatabasePtr pBimContext = OdGiContextForBmDatabase::createObject();
            
            /*OdDbBaseDatabasePEPtr pDbPE(pDb);
            OdGiDefaultContextPtr pContext = pDbPE->createGiContext(pDb);

            OdGsDevicePtr pDevice = pGsModule->createDevice();
            OdGsDevicePtr pHlpDevice = pDbPE->setupActiveLayoutViews(pDevice, pContext);
            pDevice->setUserGiContext(pContext);

            OdGsDCRect screenRect(OdGsDCPoint(0, 1000), OdGsDCPoint(1000, 0));
            pDevice->onSize(screenRect);
            pDevice->update();*/

            /****************************************************************/
            /* Create the vectorization context.                            */
            /* This class defines the operations and properties that are    */
            /* used in the ODA vectorization of an OdBmDatabase.            */
            /****************************************************************/
            OdGiContextForBmDatabasePtr pBimContext = OdGiContextForBmDatabase::createObject();

            /****************************************************************/
            /* Create the custom rendering device, and set the output       */
            /* stream for the device.                                       */
            /* This class implements bitmapped GUI display windows.         */
            /****************************************************************/
            OdGsDevicePtr pDevice = ExGsSimpleDevice::createObject(ExGsSimpleDevice::k3dDevice, collector);

            /****************************************************************/
            /* Set the database to be vectorized.                           */
            /****************************************************************/
            pBimContext->setDatabase(pDb);

            /****************************************************************/
            /* Prepare the device to render the active layout in            */
            /* this database.                                               */
            /****************************************************************/
            OdDbBaseDatabasePEPtr pDbPE(pDb);
            pDevice = pDbPE->setupActiveLayoutViews(pDevice, pBimContext);

            /****************************************************************/
            /* Set the screen size for the generated (vectorized) geometry. */
            /****************************************************************/
            OdGsDCRect screenRect(OdGsDCPoint(0, 0), OdGsDCPoint(1000, 1000));
            pDevice->onSize(screenRect);

            /****************************************************************/
            /* Initiate Vectorization                                       */
            /****************************************************************/
            pDevice->update();
        }
        repoInfo << "\nDone.\n";
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

