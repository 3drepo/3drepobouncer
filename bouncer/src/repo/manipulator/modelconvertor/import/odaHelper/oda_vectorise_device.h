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
#include <Gs/GsBaseInclude.h>
#include <RxObjectImpl.h>
#include <iostream>
#include <Gs/GsBaseMaterialView.h>
#include "oda_geometry_collector.h"


namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				/************************************************************************/
				/* OdGsBaseVectorizeDevice objects own, update, and refresh one or more */
				/* OdGsView objects.                                                    */
				/************************************************************************/
				class OdaVectoriseDevice :
					public OdGsBaseVectorizeDevice
				{
					OdaGeometryCollector * geoCollector;
				protected:
					ODRX_USING_HEAP_OPERATORS(OdGsBaseVectorizeDevice);
				public:

					OdaVectoriseDevice(){}
					~OdaVectoriseDevice(){}
				}; // end OdaVectoriseDevice			
			}
		}
	}
}