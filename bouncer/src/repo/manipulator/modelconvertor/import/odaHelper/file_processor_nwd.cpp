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

// 3D repo
#include "file_processor_nwd.h"
#include "data_processor_nwd.h"

#include "../repo_model_units.h"

// ODA
#include <OdaCommon.h>
#include <RxDynamicModule.h>
#include <DynamicLinker.h>
#include <StaticRxObject.h>
#include <ExSystemServices.h>
#include <NwHostAppServices.h>
#include <NwDatabase.h>

using namespace repo::manipulator::modelconvertor::odaHelper;
using ModelUnits = repo::manipulator::modelconvertor::ModelUnits;

static OdString sNwDbModuleName = L"TNW_Db";

class OdExNwSystemServices : public ExSystemServices
{
public:
	OdExNwSystemServices() {}
};

class RepoNwServices : public OdExNwSystemServices, public OdNwHostAppServices
{
protected:
	ODRX_USING_HEAP_OPERATORS(OdExNwSystemServices);
};

repo::manipulator::modelconvertor::odaHelper::FileProcessorNwd::~FileProcessorNwd()
{
}

uint8_t FileProcessorNwd::readFile()
{
	int nRes = REPOERR_OK;

	OdStaticRxObject<RepoNwServices> svcs;
	odrxInitialize(&svcs);
	OdRxModule* pModule = ::odrxDynamicLinker()->loadModule(sNwDbModuleName, false);
	try
	{
		OdNwDatabasePtr pNwDb = svcs.readFile(OdString(file.c_str()));

		if (pNwDb.isNull())
		{
			throw new OdError("Failed to open file");
		}

		if (pNwDb->isComposite())
		{
			throw new OdError("Navisworks Composite/Index files (.nwf) are not supported. Files must contain embedded geometry (.nwd)");
		}
		auto fileUnits = pNwDb->getUnits();
		switch (fileUnits) {
		case  NwModelUnits::UNITS_METERS:
			collector->units = ModelUnits::METRES;
			break;
		case  NwModelUnits::UNITS_CENTIMETERS:
			collector->units = ModelUnits::CENTIMETRES;
			break;
		case  NwModelUnits::UNITS_MILLIMETERS:
			collector->units = ModelUnits::MILLIMETRES;
			break;
		case  NwModelUnits::UNITS_FEET:
			collector->units = ModelUnits::FEET;
			break;
		case  NwModelUnits::UNITS_INCHES:
			collector->units = ModelUnits::INCHES;
			break;
		default:
			repoWarning << "Unsupported project units: " << (int)fileUnits;
			collector->units = ModelUnits::UNKNOWN;
		}

		DataProcessorNwd dataProcessor(collector);
		dataProcessor.process(pNwDb);
	}
	catch (OdError& e)
	{
		repoError << convertToStdString(e.description()) << ", code: " << e.code();
		switch (e.code())
		{
		case OdResult::eUnsupportedFileFormat:
			nRes = REPOERR_UNSUPPORTED_VERSION;
			break;
		case OdResult::eDecryptionError:
			nRes = REPOERR_FILE_IS_ENCRYPTED;
				break;
		default:
			nRes = REPOERR_LOAD_SCENE_FAIL;
		}
	}
	odrxUninitialize();

	return nRes;
}