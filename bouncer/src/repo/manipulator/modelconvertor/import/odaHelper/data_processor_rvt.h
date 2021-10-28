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
#include <SharedPtr.h>
#include <Gs/GsBaseInclude.h>
#include <Gs/GsBaseMaterialView.h>
#include <Gi/GiGeometrySimplifier.h>
#include <Gs/GsBaseInclude.h>
#include <Gs/GsBaseMaterialView.h>

#include <OdPlatformSettings.h>
#include <BimCommon.h>
#include <Database/BmElement.h>
#include <RxObjectImpl.h>
#include <ColorMapping.h>
#include <toString.h>
#include <Common/BuiltIns/BmBuiltInParameter.h>
#include <Database/Entities/BmMaterialElem.h>
#include <Geometry/Entities/BmMaterial.h>
#include <Database/BmAssetHelpers.h>
#include <Database/Entities/BmParamElem.h>

#include <Main/Entities/BmDirectShape.h>
#include <Main/Entities/BmDirectShapeCell.h>
#include <Main/Entities/BmDirectShapeDataCell.h>

#include <Database/Entities/BmElementHeader.h>

#include <Essential/Entities/BmLevel.h>

#include <Database/BmElement.h>
#include <Database/PE/BmLabelUtilsPE.h>
#include <Database/Entities/BmAUnits.h>
#include <Database/BmUnitUtils.h>
#include <Database/Managers/BmUnitsTracking.h>
#include <Database/Managers/BmElementTrackingData.h>
#include <Database/Entities/BmUnitsElem.h>
#include <Database/Entities/BmViewport.h>
#include <Database/Entities/BmHiddenElementsViewSettings.h>
#include <Database/Entities/BmBasePoint.h>
#include <Database/Entities/BmGeoLocation.h>

#include <TB_ExLabelUtils/BmSampleLabelUtilsPE.h>

#include "../../../../lib/datastructure/repo_structs.h"
#include "../../../../core/model/bson/repo_bson_builder.h"
#include "geometry_collector.h"
#include "data_processor.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class VectoriseDeviceRvt;

				class DataProcessorRvt : public DataProcessor
				{
					//Environment variable name for Revit textures
					const char* RVT_TEXTURES_ENV_VARIABLE = "REPO_RVT_TEXTURES";

				public:
					DataProcessorRvt();

					VectoriseDeviceRvt* device();

					void init(GeometryCollector* geoColl, OdBmDatabasePtr database);

				protected:
					void draw(const OdGiDrawable*) override;

					void beginViewVectorization() override;

					void convertTo3DRepoMaterial(
						OdGiMaterialItemPtr prevCache,
						OdDbStub* materialId,
						const OdGiMaterialTraitsData & materialData,
						MaterialColours& matColors,
						repo_material_t& material,
						bool& missingTexture) override;

					void convertTo3DRepoTriangle(
						const OdInt32* p3Vertices,
						std::vector<repo::lib::RepoVector3D64>& verticesOut,
						repo::lib::RepoVector3D64& normalOut,
						std::vector<repo::lib::RepoVector2D>& uvOut) override;

				private:
					void hiddenElementsViewRejection(OdBmDBViewPtr pDBView);
					void getCameras(OdBmDatabasePtr database);
					camera_t convertCamera(OdBmDBViewPtr view);
					std::string determineTexturePath(const std::string& inputPath);
					void establishProjectTranslation(OdBmDatabase* pDb);
					void fillTexture(OdBmMaterialElemPtr materialPtr, repo_material_t& material, bool& missingTexture);
					void fillMaterial(OdBmMaterialElemPtr materialPtr, const MaterialColours& matColors, repo_material_t& material);
					void fillMeshData(const OdGiDrawable* element);

					void fillMetadataById(
						OdBmObjectId id,
						std::unordered_map<std::string, std::string>& metadata);

					void fillMetadataByElemPtr(
						OdBmElementPtr element,
						std::unordered_map<std::string, std::string>& metadata);

					std::unordered_map<std::string, std::string> fillMetadata(OdBmElementPtr element);
					std::string getLevel(OdBmElementPtr element, const std::string& name);
					std::string getElementName(OdBmElementPtr element);

					void initLabelUtils();

					OdBm::DisplayUnitType::Enum getUnits(OdBmDatabasePtr database);

					bool ignoreParam(const std::string& param);

					std::string translateMetadataValue(
						const OdTfVariant& val,
						OdBmLabelUtilsPEPtr labelUtils,
						OdBmParamDefPtr paramDef,
						OdBmDatabase* database,
						OdBm::BuiltInParameterDefinition::Enum param);

					std::string unitsToString(const OdBm::DisplayUnitType::Enum &units);

					OdBmDatabasePtr database;
					OdBmSampleLabelUtilsPE* labelUtils = nullptr;
				};
			}
		}
	}
}
