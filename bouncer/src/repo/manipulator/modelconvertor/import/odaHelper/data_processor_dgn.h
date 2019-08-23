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

#include "geometry_collector.h"

#include <SharedPtr.h>
#include <Gi/GiGeometrySimplifier.h>
#include <Gs/GsBaseMaterialView.h>
#include <string>
#include <fstream>

#include "vectorise_device_dgn.h"
#include "../../../../core/model/bson/repo_node_mesh.h"

#include "data_processor.h"

#include <vector>

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class DataProcessorDgn : public DataProcessor
				{
				public:
					DataProcessorDgn() {}

					VectoriseDeviceDgn* device();

					bool doDraw(OdUInt32 i,	const OdGiDrawable* pDrawable) override;
					void init(
						GeometryCollector *const geoCollector, const OdGeExtents3d &extModel);

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
					OdCmEntityColor fixByACI(const ODCOLORREF *ids, const OdCmEntityColor &color);
					double deviationValue = 0;

					std::unordered_map<std::string, std::string> extractXMLLinkages(OdDgElementPtr pElm);

				};
				typedef OdSharedPtr<DataProcessorDgn> OdGiConveyorGeometryDgnDumperPtr;
			}
		}
	}
}
