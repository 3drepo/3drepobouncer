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

#include "SharedPtr.h"
#include "Gs/GsBaseInclude.h"
#include <Gs/GsBaseMaterialView.h>
#include "Gi/GiGeometrySimplifier.h"
#include "Gs/GsBaseInclude.h"
#include <Gs/GsBaseMaterialView.h>

#include "OdPlatformSettings.h"
#include <BimCommon.h>
#include <Database/BmElement.h>
#include <RxObjectImpl.h>
#include <ColorMapping.h>
#include <toString.h>
#include "Database/Entities/BmMaterialElem.h"
#include "Geometry/Entities/BmMaterial.h"
#include "Database/BmAssetHelpers.h"
#include "Database/BmBuiltInParameter.h"
#include "Database/Entities/BmParamElem.h"

#include "Main/Entities/BmDirectShape.h"
#include "Main/Entities/BmDirectShapeCell.h"
#include "Main/Entities/BmDirectShapeDataCell.h"

#include "repo/core/model/bson/repo_bson_builder.h"
#include "Database/Entities/BmElementHeader.h"

#include "HostObj/Entities/BmLevel.h"

#include <Database/BmElement.h>
#include "Database/BmLabelUtilsPE.h"

#include "../../../../lib/datastructure/repo_structs.h"
#include "geometry_collector.h"
#include "data_collector_oda.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class VectoriseDeviceRvt;

				class VectorizeView : public DataCollectorOda
				{
				public:
					VectorizeView();

					static OdGsViewPtr createObject(GeometryCollector* geoColl);

					VectoriseDeviceRvt* device();

					void draw(const OdGiDrawable*);

				protected:
					
					void OnFillMaterialCache(
						OdGiMaterialItemPtr prevCache,
						OdDbStub* materialId,
						const OdGiMaterialTraitsData & materialData,
						const MaterialColors& matColors,
						repo_material_t& material) override;

					void OnTriangleOut(const std::vector<repo::lib::RepoVector3D64>& vertices) override;

				private:
					void fillTexture(OdDbStub* materialId, repo_material_t& material, bool& missingTexture);
					void fillMaterial(const MaterialColors& matColors, repo_material_t& material);
					void fillMeshData(const OdGiDrawable* element);

					std::pair<std::vector<std::string>, std::vector<std::string>> fillMetadata(OdBmElementPtr element);
					std::string getLevel(OdBmElementPtr element, const std::string& name);

					uint64_t meshesCount;
				};
			}
		}
	}
}

