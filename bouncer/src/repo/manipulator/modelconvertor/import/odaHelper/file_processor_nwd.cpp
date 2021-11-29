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

//ODA
#include <OdaCommon.h>
#include <RxDynamicModule.h>
#include <DynamicLinker.h>
#include <StaticRxObject.h>
#include <ExSystemServices.h>
#include <NwHostAppServices.h>
#include <NwDatabase.h>

#include <NwModelItem.h>
#include <NwMaterial.h>
#include <NwComponent.h>
#include <NwFragment.h>
#include <NwGeometryEllipticalShape.h>
#include <NwGeometryLineSet.h>
#include <NwGeometryMesh.h>
#include <NwGeometryPointSet.h>
#include <NwGeometryText.h>
#include <NwGeometryTube.h>
#include <NwTexture.h>
#include <NwColor.h>
#include <NwBackgroundElement.h>
#include <NwViewpoint.h>
#include <NwTextFontInfo.h>
#include <NwModel.h>
#include <NwProperty.h>
#include <NwPartition.h>

//3d repo bouncer
#include "file_processor_nwd.h"
//#include "data_processor_nwd.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

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
			throw new OdError("Navisworks Composite/Index files (NWF) are not supported. Files must be NWDs");
		}

		OdNwObjectId modelItemRootId = pNwDb->getModelItemRootId();
		if (!modelItemRootId.isNull())
		{
			OdNwModelItemPtr pModelItemRoot = OdNwModelItem::cast(modelItemRootId.safeOpenObject());

			// Our sample function traverses the scene graph recursively.
			// We may want to change to a non-recursive traversal.
			// (Can this also be multi-threaded? Since the Db is read-only...)
		}
	}
	catch (OdError& e)
	{
		repoError << convertToStdString(e.description()) << ", code: " << e.code();
		if (e.code() == OdResult::eUnsupportedFileFormat) {
			nRes = REPOERR_UNSUPPORTED_VERSION;
		}
		else {
			nRes = REPOERR_LOAD_SCENE_FAIL;
		}
	}
	odrxUninitialize();

	return nRes;
}