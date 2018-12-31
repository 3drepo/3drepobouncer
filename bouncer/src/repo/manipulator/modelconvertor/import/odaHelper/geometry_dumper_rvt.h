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

#include "SharedPtr.h"
#include "Gi/GiGeometrySimplifier.h"

#include <vector>

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class GeometryDumperRvt : public OdGiGeometrySimplifier
				{
					GeometryCollector* collector;
					uint64_t meshesCount;
				public:
					GeometryDumperRvt() {}

					void init(GeometryCollector *const geoCollector);

					virtual void triangleOut(const OdInt32* vertices,
						const OdGeVector3d* pNormal);

				};
				typedef OdSharedPtr<GeometryDumperRvt> GeometryRvtDumperPtr;
			}
		}
	}
}

