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
#include "repo/lib/datastructure/repo_variant.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class DataProcessorDwg : public DataProcessor
				{
				public:
					bool doDraw(OdUInt32 i,	const OdGiDrawable* pDrawable) override;
					void setMode(OdGsView::RenderMode mode);
					~DataProcessorDwg();

				protected:

					void convertTo3DRepoMaterial(
						OdGiMaterialItemPtr prevCache,
						OdDbStub* materialId,
						const OdGiMaterialTraitsData & materialData,
						MaterialColours& matColors,
						repo::lib::repo_material_t& material) override;

				private:
					void convertTo3DRepoColor(OdCmEntityColor& color, repo::lib::repo_color3d_t& out);

					class Layer
					{
					public:
						std::string id;
						std::string name;

						Layer(std::string id, std::string name):
							id(id),
							name(name)
						{
						}

						Layer() 
						{
						}

						operator bool() const {
							return !id.empty() && !name.empty();
						}
					};

					// Some properties to be held between invocations of doDraw()
					class Context
					{
					public:
						bool inBlock = false;
						Layer currentBlockReferenceLayer;
						Layer currentBlockReference;
						OdDbObjectId layoutId;
					};

					class DwgDrawContext;
					std::unique_ptr<DwgDrawContext> makeDrawContext(const Layer& entity, const Layer& parent, const std::string& handle);

					Context context;

					std::string getClassDisplayName(OdDbEntityPtr entity);
				};

				typedef OdSharedPtr<DataProcessorDwg> DataProcessorDwgPtr;
			}
		}
	}
}
