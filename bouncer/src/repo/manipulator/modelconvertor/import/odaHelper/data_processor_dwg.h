/**
*  Copyright (C) 2024 3D Repo Ltd
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

#include "data_processor.h"
#include "geometry_collector.h"

#include <SharedPtr.h>
#include <Gi/GiGeometrySimplifier.h>
#include <Gs/GsBaseMaterialView.h>
#include <string>
#include <fstream>
#include <vector>

#include "vectorise_device_dgn.h"
#include "../../../../core/model/bson/repo_node_mesh.h"
#include "repo/lib/datastructure/repo_metadataVariant.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class DataProcessorDwg : public DataProcessor
				{
				public:
					DataProcessorDwg() {}

					bool doDraw(OdUInt32 i,	const OdGiDrawable* pDrawable) override;
					void init(GeometryCollector *const geoCollector);
					void setMode(OdGsView::RenderMode mode);
					void endViewVectorization();

				protected:

					void convertTo3DRepoMaterial(
						OdGiMaterialItemPtr prevCache,
						OdDbStub* materialId,
						const OdGiMaterialTraitsData & materialData,
						MaterialColours& matColors,
						repo_material_t& material,
						bool& missingTexture) override;

				private:
					void convertTo3DRepoColor(OdCmEntityColor& color, std::vector<float>& out);

					// Some properties to be held between invocations of doDraw()
					class Context
					{
					public:
						bool inBlock = false;
						std::string currentBlockReferenceLayerId;
						std::string currentBlockReferenceLayerName;
						std::string currentBlockReferenceId;
						std::string currentBlockReferenceName;
						OdDbObjectId layoutId;
					};

					Context context;

					std::string getClassDisplayName(OdDbEntityPtr entity);
				};

				typedef OdSharedPtr<DataProcessorDwg> DataProcessorDwgPtr;
			}
		}
	}
}
