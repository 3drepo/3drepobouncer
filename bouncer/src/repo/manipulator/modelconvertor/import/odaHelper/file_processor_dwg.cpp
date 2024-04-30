#include "file_processor_dwg.h"

#include <OdaCommon.h>
#include <StaticRxObject.h>
#include <RxInit.h>
#include <RxDynamicModule.h>
#include <DynamicLinker.h>
#include <RxDynamicModule.h>
#include <Gs/GsBaseInclude.h>
#include <DbGsManager.h>
#include <GiContextForDbDatabase.h>

#include "../../../../error_codes.h"
#include "../../../../lib/repo_exception.h"

#include <vector>

#include "data_processor_dwg.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

class DeviceModuleDwg : public OdGsBaseModule
{
public:
	void init(GeometryCollector* collector)
	{
		this->collector = collector;
	}

protected:
	OdSmartPtr<OdGsBaseVectorizeDevice> createDeviceObject()
	{
		return OdRxObjectImpl<VectoriseDeviceDgn, OdGsBaseVectorizeDevice>::createObject();
	}
	OdSmartPtr<OdGsViewImpl> createViewObject()
	{
		auto pP = OdRxObjectImpl<DataProcessorDwg, OdGsViewImpl>::createObject();
		((DataProcessorDwg*)pP.get())->init(collector);
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

private:
	GeometryCollector* collector;
};
ODRX_DEFINE_PSEUDO_STATIC_MODULE(DeviceModuleDwg);

void importDwg(OdDbDatabasePtr pDb, GeometryCollector* collector)
{
	// Create the vectorizer device that will render the DWG database. This will
	// use the GeometryCollector underneath.

	OdGsModulePtr pGsModule = ODRX_STATIC_MODULE_ENTRY_POINT(DeviceModuleDwg)(OD_T("DeviceModuleDwg"));
	auto deviceModule = (DeviceModuleDwg*)pGsModule.get();
	deviceModule->init(collector);
	auto pDevice = pGsModule->createDevice();

	// Set up the view to the default one in the file


	OdGiContextForDbDatabasePtr pDwgContext = OdGiContextForDbDatabase::createObject();
	pDwgContext->setDatabase(pDb);

	auto pHelperDevice = OdDbGsManager::setupActiveLayoutViews(pDevice, pDwgContext);

	OdGsDCRect screenRect(OdGsDCPoint(0, 1000), OdGsDCPoint(1000, 0));
	pHelperDevice->onSize(screenRect);
	pHelperDevice->update();

	pGsModule.release();
}

uint8_t FileProcessorDwg::readFile()
{
	uint8_t nRes = 0;               // Return value for the function
	try 
	{
		odInitialize(&svcs);
		odgsInitialize();

		::odrxDynamicLinker()->loadModule(L"TG_Db", false);
		::odrxDynamicLinker()->loadModule(DbCryptModuleName, false);
		::odrxDynamicLinker()->loadModule(OdExFieldEvaluatorModuleName, false);
		::odrxDynamicLinker()->loadModule(Od3DSolidHistoryTxModuleName, false);
		::odrxDynamicLinker()->loadModule(OdDynBlocksModuleName, false);
		::odrxDynamicLinker()->loadModule(RxPropertiesModuleName, false);
		::odrxDynamicLinker()->loadModule(DbPropertiesModuleName, false);
		::odrxDynamicLinker()->loadModule(RxCommonDataAccessModuleName, false);

		OdString f = file.c_str();
		OdDbDatabasePtr pDb = svcs.readFile(f);

		importDwg(pDb, collector);

		pDb.release();
		odgsUninitialize();
		odUninitialize();

		nRes = REPOERR_OK;
	}
	catch (OdError& e)
	{
		repoError << convertToStdString(e.description());
		nRes = REPOERR_MODEL_FILE_READ;
	}
	catch (std::exception& e)
	{
		repoError << "Failed: " << e.what();
		nRes = REPOERR_MODEL_FILE_READ;
	}
	return nRes;
}