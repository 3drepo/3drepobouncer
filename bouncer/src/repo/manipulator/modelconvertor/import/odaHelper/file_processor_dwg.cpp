#include "file_processor_dwg.h"

#include <StaticRxObject.h>
#include <RxInit.h>
#include <RxDynamicModule.h>
#include <DynamicLinker.h>
#include <DgDatabase.h>
#include <RxDynamicModule.h>
#include <Exports/DgnExport/DgnExport.h>
#include <ExHostAppServices.h>
#include "../../../../lib/repo_exception.h"

using namespace repo::manipulator::modelconvertor::odaHelper;
using namespace TD_DGN_EXPORT;

OdDgDatabasePtr FileProcessorDwg::initialiseOdDatabase() {
	OdString fileSource = file.c_str();
	OdDgDatabasePtr pDb;
	// Register ODA Drawings API for DGN
	::odrxDynamicLinker()->loadModule(OdDbModuleName, false);
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
		std::stringstream ss;

		ss << "Failed to export dwg/dxf into a dgn: ";
		switch (res)
		{
		case OdDgnExport::bad_database:
			ss << "Bad database";
			break;
		case OdDgnExport::bad_file:
			ss << "DGN export";
			break;
		case OdDgnExport::encrypted_file:
		case OdDgnExport::bad_password:
			ss << "The file is encrypted";
			break;

		case OdDgnExport::fail:
			ss << "Unknown import error";
			break;
		}
		throw new repo::lib::RepoException(ss.str());
	}

	pExporter.release();
	pModule.release();

	return pDb;
}