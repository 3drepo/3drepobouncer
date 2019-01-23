#pragma once

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

#include "Main/Entities/BmDirectShape.h"
#include "Main/Entities/BmDirectShapeCell.h"
#include "Main/Entities/BmDirectShapeDataCell.h"

#include "repo/core/model/bson/repo_bson_builder.h"
#include "Database/Entities/BmElementHeader.h"

#include "HostObj/Entities/BmLevel.h"

#include <Database/BmElement.h>

#include "../../../../lib/datastructure/repo_structs.h"
#include "geometry_collector.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class VectoriseDeviceRvt;

				class VectorizeView : public OdGsBaseMaterialView, public OdGiGeometrySimplifier
				{
				public:
					VectorizeView();

					static OdGsViewPtr createObject(GeometryCollector* geoColl);

					virtual void beginViewVectorization();

					VectoriseDeviceRvt* device();

					void draw(const OdGiDrawable*);

				protected:
					
					OdGiMaterialItemPtr fillMaterialCache(
						OdGiMaterialItemPtr prevCache,
						OdDbStub* materialId,
						const OdGiMaterialTraitsData & materialData);

					void triangleOut(const OdInt32* vertices,
						const OdGeVector3d* pNormal);

				private:
					void fillTexture(OdDbStub* materialId, repo_material_t& material, bool& missingTexture);
					void fillMaterial(const OdGiMaterialTraitsData & materialData, repo_material_t& material);
					void fillMeshGroupAndLevel(const OdGiDrawable* element);
					std::string getLevel(OdBmElementPtr element, const std::string& name);

					GeometryCollector* geoColl;
					uint64_t meshesCount;
				};
			}
		}
	}
}

