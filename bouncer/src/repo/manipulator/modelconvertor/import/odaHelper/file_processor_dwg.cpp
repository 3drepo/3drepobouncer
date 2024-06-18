/**
*  Copyright (C) 2021 3D Repo Ltd
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

#include "file_processor_dwg.h"

#include <OdaCommon.h>
#include <StaticRxObject.h>
#include <RxInit.h>
#include <RxDynamicModule.h>
#include <DynamicLinker.h>
#include <MemoryStream.h>
#include <Gs/GsBaseInclude.h>
#include <DbGsManager.h>
#include <GiContextForDbDatabase.h>
#include <../Exports/2dExport/Include/2dExportDevice.h>

#include "../../../../error_codes.h"
#include "../../../../lib/repo_exception.h"

#include <vector>

#include "data_processor_dwg.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

class VectoriseDeviceDwg : public OdGsBaseVectorizeDevice
{
protected:
	ODRX_USING_HEAP_OPERATORS(OdGsBaseVectorizeDevice);

public:

	VectoriseDeviceDwg() {}
	~VectoriseDeviceDwg() {}
};

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
		return OdRxObjectImpl<VectoriseDeviceDwg, OdGsBaseVectorizeDevice>::createObject();
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

void importModel(OdDbDatabasePtr pDb, GeometryCollector* collector)
{
	// Create the vectorizer device that will render the DWG database. This will
	// use the GeometryCollector underneath.

	OdGsModulePtr pGsModule = ODRX_STATIC_MODULE_ENTRY_POINT(DeviceModuleDwg)(OD_T("DeviceModuleDwg"));
	auto deviceModule = (DeviceModuleDwg*)pGsModule.get();
	deviceModule->init(collector);
	auto pDevice = pGsModule->createDevice();

	OdGiContextForDbDatabasePtr pDbGiContext = OdGiContextForDbDatabase::createObject();
	pDbGiContext->setDatabase(pDb);

	auto pHelperDevice = OdDbGsManager::setupActiveLayoutViews(pDevice, pDbGiContext);

	pDb->setGEOMARKERVISIBILITY(0); // This turns the OdDbGeoDataMarker 3D geometry off.

	OdGsDCRect screenRect(OdGsDCPoint(0, 1000), OdGsDCPoint(1000, 0));
	pHelperDevice->onSize(screenRect);
	pHelperDevice->update();

	pGsModule.release();
}

void importDrawing(OdDbDatabasePtr pDb, repo::manipulator::drawingutility::DrawingImageInfo* collector)
{
	OdGsModulePtr pModule = ::odrxDynamicLinker()->loadModule(OdSvgExportModuleName, false);
	OdGsDevicePtr dev = pModule->createDevice();
	if (!dev.isNull())
	{
		auto stream = OdMemoryStream::createNew();
		dev->properties()->putAt("Output", stream.get());

		// The below follows the same logic as the Dgn Exporter, which itself
		// is based on the ODA samples. A Gi context - the interface between
		// a database and the graphics system (GS) - is created for the specific
		// database type. An SVG export device is created, and wrapped in an
		// object that connects the Gi to this device. When the device updates,
		// it will write an SVG with the contents of the database.

		auto pDbGiContext = OdGiContextForDbDatabase::createObject();
		pDbGiContext->setDatabase(pDb);

		pDbGiContext->setPlotGeneration(true);
		pDbGiContext->setHatchAsPolygon(OdGiDefaultContext::kHatchPolygon);
		dev->properties()->putAt(L"MinimalWidth", OdRxVariantValue(0.1));

		OdDbBaseDatabasePEPtr pBaseDatabase(pDb);
		pBaseDatabase->loadPlotstyleTableForActiveLayout(pDbGiContext, pDb);

		TD_2D_EXPORT::Od2dExportDevice* pDeviceSvg = (TD_2D_EXPORT::Od2dExportDevice*)dev.get();
		auto pHelperDevice = OdDbGsManager::setupActiveLayoutViews(pDeviceSvg, pDbGiContext);

		pDb->setGEOMARKERVISIBILITY(0); // This turns the OdDbGeoDataMarker 3D geometry off.

		pHelperDevice->onSize(OdGsDCRect(0, 1024, 768, 0));

		// Here we can extract the calibration in the same way as file_processor_dgn when ready...

		pHelperDevice->update();

		collector->data.resize(stream->tell());
		stream->seek(0, OdDb::FilerSeekType::kSeekFromStart);
		stream->getBytes(collector->data.data(), stream->length());
	}
}

uint8_t FileProcessorDwg::readFile()
{
	uint8_t nRes = 0; // Return value for the function
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

		if (collector)
		{
			importModel(pDb, collector);
		}

		if (drawingCollector)
		{
			importDrawing(pDb, drawingCollector);
		}

		pDb.release();

		nRes = REPOERR_OK;
	}
	catch (OdError& e)
	{
		repoError << convertToStdString(e.description());
		switch (e.code())
		{
		case eDwgFileIsEncrypted:
			nRes = REPOERR_FILE_IS_ENCRYPTED;
			break;
		default:
			nRes = REPOERR_MODEL_FILE_READ;
		}
	}
	catch (std::exception& e)
	{
		repoError << "Failed: " << e.what();
		nRes = REPOERR_MODEL_FILE_READ;
	}

	// These must be uninitialised even in the case of an error to allow the
	// process to exit cleanly

	odgsUninitialize();
	odUninitialize();

	return nRes;
}