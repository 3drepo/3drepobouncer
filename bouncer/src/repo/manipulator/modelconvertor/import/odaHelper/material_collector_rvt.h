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

#pragma once

#include "../../../../core/model/bson/repo_node_mesh.h"
#include "geometry_collector.h"
#include <OdaCommon.h>
#include <Gs/GsBaseInclude.h>
#include <RxObjectImpl.h>
#include <vector>
#include <string>

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
#include "Main\Entities\BmElemRecPointer.h"
#include "Database\Entities\BmElemRec.h"
#include "Geometry\Entities\BmGeometry.h"
#include "Database/BmElement.h"
#include "Geometry\Entities\BmGElement.h"
#include "Main\Entities\BmSysPanelFamSym.h"
#include "Main\Entities\BmFilledRegion.h"
#include "Main\Entities\BmShaftOpening.h"
#include "StairsRamp\Entities\BmBaseRailing.h"
#include "StairsRamp\Entities\BmRoomElem.h"
#include "StairsRamp\Entities\BmBaseRailingSym.h"
#include "Database\BmAssetHelpers.h"
#include "Database\Entities\BmCurtainGrid.h"
#include "Database\Entities\BmFamilyInstance.h"
#include "Database\Entities\BmInstanceInfo.h"
#include "HostObj\Entities\BmWallCGDriver.h"
#include "Geometry\Entities\BmGInstance.h"
#include "Geometry\Entities\BmGFilter.h"
#include "Geometry\Entities\BmGConditionBase.h"
#include "Geometry\Entities\BmGConditionInt.h"
#include "HostObj\Entities\BmArcWall.h"
#include "Geometry\Entities\BmMaterial.h"
#include "Database\Entities\BmMaterialElem.h"
#include "Tf\Tf.h"
#include "Br\BrEnums.h"
#include "Geometry\Entities\BmFace.h"
#include "Database\Entities\BmElemTable.h"
#include "Database\Entities\BmImageHolder.h"
#include "Database\Entities\BmARasterImage.h"
#include "Database\Entities\BmImageSymbol.h"
#include "Gi\GiRasterWrappers.h"
#include "Gi\GiRasterImage.h"
#include "Gi\GiRasterImageArray.h"
#include "RxRasterServices.h"
#include "HostObj/Entities/BmSWall.h"
#include "Database/Entities/BmWallType.h"

class MaterialCollectorRvt
{
public:
	//for materials
	bool convertMaterialData(OdBmObjectId matId, repo_material_t* TMaterial);
	int getGeometries(const OdGiDrawable* elemRec, OdBmGeometryPtrArray& geoms);
	bool getMaterial(const OdGiDrawable* pDrawable, repo_material_t& mat);
	OdBmObjectIdArray getFacesMaterialId(const OdGiDrawable* elemRec);
	OdBmGeometryPtrArray getGeometriesFromNode(OdBmGNodePtr node);
private:
	//for textures
	bool isFileExist(const std::string& fileName);

	OdArray<OdBmFacePtr> faces;
};

