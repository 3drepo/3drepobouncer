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
#include <DbDimStyleTableRecord.h>

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
		dev->properties()->putAt(L"MinimalWidth", OdRxVariantValue(0.01));
		pDbGiContext->setPaletteBackground(ODRGB(255, 255, 255));

		OdDbBaseDatabasePEPtr pBaseDatabase(pDb);
		pBaseDatabase->loadPlotstyleTableForActiveLayout(pDbGiContext, pDb);

		TD_2D_EXPORT::Od2dExportDevice* pDeviceSvg = (TD_2D_EXPORT::Od2dExportDevice*)dev.get();
		auto pHelperDevice = OdDbGsManager::setupActiveLayoutViews(pDeviceSvg, pDbGiContext);

		pDb->setGEOMARKERVISIBILITY(0); // This turns the OdDbGeoDataMarker 3D geometry off.

		pHelperDevice->onSize(OdGsDCRect(0, 1024, 768, 0));

		// This section extracts the view information which can be used to map
		// betweeen the SVG and world coordinate systems.
		// The graphics system (Gs) view https://docs.opendesign.com/tv/gs_OdGsView.html
		// is used to derive points that map between the WCS of the drawing and
		// the svg file.

		const OdGsView* pGsView = pDeviceSvg->viewAt(0);

		auto worldToDeviceMatrix = pGsView->worldToDeviceMatrix();
		auto objectToDeviceMatrix = pGsView->objectToDeviceMatrix();
		
		// Pick three points (two vectors) to describe the map. The transform
		// can be computed from these each time from then on.

		OdGePoint3d a(0, 0, 0);
		OdGePoint3d b(1, 0, 0);
		OdGePoint3d c(0, 1, 0);

		// The following vector contains the points in the SVG file corresponding
		// to the 3D coordinates above. Add these to the appropriate schema when
		// it is ready...

		//std::vector<OdGePoint3d> points;
		//points.push_back(worldToDeviceMatrix * a);
		//points.push_back(worldToDeviceMatrix * b);
		//points.push_back(worldToDeviceMatrix * c);

		// Calculate points in SVG space
		OdGePoint3d aS = worldToDeviceMatrix * a;
		OdGePoint3d bS = worldToDeviceMatrix * b;

		// Get pixel density to apply it to SVG coordinates for converting from pixel to unit values
		OdGePoint2d pixelDensity;
		pGsView->getNumPixelsInUnitSquare(OdGePoint3d::kOrigin, pixelDensity, false);

		// Convert to 2D by dropping z component and applying the density values
		// (note: have not thought about it. Just conceptually).
		repo::lib::RepoVector2D aS2d = repo::lib::RepoVector2D(aS.x / pixelDensity.x, aS.y / pixelDensity.y);
		repo::lib::RepoVector2D bS2d = repo::lib::RepoVector2D(bS.x / pixelDensity.x, bS.y / pixelDensity.y);

		// Convert 3d vectors from ODA format to 3d repo format
		repo::lib::RepoVector3D a3d = repo::lib::RepoVector3D(a.x, a.y, a.z);
		repo::lib::RepoVector3D b3d = repo::lib::RepoVector3D(b.x, b.y, b.z);

		// Assemble calibration outcome
		std::vector<repo::lib::RepoVector3D> horizontal3d;
		horizontal3d.push_back(a3d);
		horizontal3d.push_back(b3d);

		std::vector<repo::lib::RepoVector2D> horizontal2d;
		horizontal2d.push_back(aS2d);
		horizontal2d.push_back(bS2d);

		repo::manipulator::modelutility::DrawingCalibration calibration;
		calibration.horizontalCalibration3d = horizontal3d;
		calibration.horizontalCalibration2d = horizontal2d;

		calibration.verticalRange = { 0, 10 }; // TODO: how do I calculate that?

		repo::manipulator::modelconvertor::ModelUnits units = determineModelUnits(pDb->getINSUNITS());
		calibration.units = repo::manipulator::modelconvertor::toUnitsString(units);

		// Pass calibration outcome to collector
		drawingCollector->drawingCalibration = calibration;
		
		
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