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

#include <DbObjectIterator.h>
#include <DbBlockTable.h>
#include <DbBlockTableRecord.h>

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

void FileProcessorDwg::importModel(OdDbDatabasePtr pDb)
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

void FileProcessorDwg::importDrawing(OdDbDatabasePtr pDb)
{
	// SvgExport is the name of the 3DRepo variant of the SVG exporter module.
	// The actual module name searched for will have an SDK and compiler version
	// suffix, depending on platform. This is handled by CMake.

	OdGsModulePtr pModule = ::odrxDynamicLinker()->loadModule("SvgExport", false);
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
		dev->properties()->putAt(L"MinimalWidth", OdRxVariantValue(0.08));
		dev->properties()->putAt(L"UseHLR", OdRxVariantValue(true));
		dev->properties()->putAt(L"ColorPolicy", OdRxVariantValue((OdInt32)3)); // kDarken
		pDbGiContext->setPaletteBackground(ODRGB(255, 255, 255));

		OdDbBaseDatabasePEPtr pBaseDatabase(pDb);
		pBaseDatabase->loadPlotstyleTableForActiveLayout(pDbGiContext, pDb);

		TD_2D_EXPORT::Od2dExportDevice* pDeviceSvg = (TD_2D_EXPORT::Od2dExportDevice*)dev.get();
		auto pHelperDevice = OdDbGsManager::setupActiveLayoutViews(pDeviceSvg, pDbGiContext);

		pDb->setGEOMARKERVISIBILITY(0); // This turns the OdDbGeoDataMarker 3D geometry off.

		pHelperDevice->onSize(OdGsDCRect(0, 1024, 768, 0));

		// This section extracts the view information which can be used to map
		// between the SVG and world coordinate systems. The graphics system (Gs) view
		// https://docs.opendesign.com/tv/gs_OdGsView.html is used to derive points
		// that map between the WCS of the drawing and the space the SVG primitives
		// are defined for the purpose of autocalibration.

		const OdGsView* pGsView = pDeviceSvg->viewAt(0);
		updateDrawingHorizontalCalibration(pGsView, drawingCollector->calibration);
		repo::manipulator::modelconvertor::ModelUnits units = determineModelUnits(pDb->getINSUNITS());
		drawingCollector->calibration.units = repo::manipulator::modelconvertor::toUnitsString(units);

		// Render the SVG

		pHelperDevice->update();

		// Copy the SVG contents into a string

		std::vector<char> buffer;
		buffer.resize(stream->tell());
		stream->seek(0, OdDb::FilerSeekType::kSeekFromStart);
		stream->getBytes(buffer.data(), stream->length());
		std::string svg(buffer.data(), buffer.size());

		// Perform any further necessary manipulations. In this case we add the width
		// and height attributes.

		svg.insert(61, "width=\"1024\" height=\"768\" "); // 61 is just after the svg tag. This offset is fixed for exporter version.

		// Provide the string to the collector as a vector

		std::copy(svg.c_str(), svg.c_str() + svg.length(), std::back_inserter(drawingCollector->data));
	}
}

repo::manipulator::modelconvertor::ModelUnits FileProcessorDwg::determineModelUnits(const OdDb::UnitsValue units){
	switch (units) {
		case OdDb::kUnitsMeters: return ModelUnits::METRES;
		case OdDb::kUnitsDecimeters: return ModelUnits::DECIMETRES;
		case OdDb::kUnitsCentimeters: return ModelUnits::CENTIMETRES;
		case OdDb::kUnitsMillimeters: return ModelUnits::MILLIMETRES;
		case OdDb::kUnitsFeet: return ModelUnits::FEET;
		case OdDb::kUnitsInches: return ModelUnits::INCHES;
		default:
			repoWarning << "Unrecognised unit measure: " << (int)units;
			return ModelUnits::UNKNOWN;
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
			importModel(pDb);
		}

		if (drawingCollector)
		{
			importDrawing(pDb);
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