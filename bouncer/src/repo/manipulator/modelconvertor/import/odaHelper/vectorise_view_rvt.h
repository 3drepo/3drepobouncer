#pragma once

#pragma once

#include "SharedPtr.h"
#include "Gs/GsBaseInclude.h"
#include <Gs/GsBaseMaterialView.h>
#include "Gi/GiGeometrySimplifier.h"
#include "Gs/GsBaseInclude.h"
#include <Gs/GsBaseMaterialView.h>

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

					GeometryCollector* geoColl;
					uint64_t meshesCount;
				};
			}
		}
	}
}

