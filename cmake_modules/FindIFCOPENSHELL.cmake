#  Copyright (C) 2015 3D Repo Ltd
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Affero General Public License as
#  published by the Free Software Foundation, either version 3 of the
#  License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Affero General Public License for more details.
#
#  You should have received a copy of the GNU Affero General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#attempt to find IFCOPENSHELL library (renamed to avoid clashing with the assimp default one)
#if IFCOPENSHELL is found, IFCOPENSHELL_FOUND is set to true
#IFCOPENSHELL_INCLUDE_DIR will point to the include folder of the installation
#IFCOPENSHELL_LIBRARIES will point to the libraries

# This cmake file encompasses the following IFCOS dependencies as well, which
# should be provided with the installation:
# mpfr
# mpir

SET(IFCOS_LIB_NAMES
	IfcGeom
	IfcParse
	geometry_kernel_cgal
	geometry_kernel_cgal_simple
	geometry_kernel_opencascade
	geometry_mapping_ifc2x3.lib
	geometry_mapping_ifc4.lib
	geometry_mapping_ifc4x1.lib
	geometry_mapping_ifc4x2.lib
	geometry_mapping_ifc4x3.lib
	geometry_mapping_ifc4x3_add1.lib
	geometry_mapping_ifc4x3_add2.lib
	geometry_mapping_ifc4x3_tc1.lib
	mpfr
	mpir
	mpirxx
)

set(IFCOPENSHELL_FOUND TRUE)

if(DEFINED ENV{IFCOPENSHELL_ROOT})
	set(IFCOPENSHELL_ROOT $ENV{IFCOPENSHELL_ROOT})
	foreach(libName ${IFCOS_LIB_NAMES})
		find_library(libPath${libName} NAMES ${libName}
			PATHS
			${IFCOPENSHELL_ROOT}/lib
			/usr/lib/
			/usr/local/lib/
			/opt/local/lib/
		)

		if(NOT libPath${libName})
			set(IFCOPENSHELL_FOUND FALSE)
			message("Could not find " ${libName})
		endif()

		set(IFCOPENSHELL_LIBRARIES ${IFCOPENSHELL_LIBRARIES} ${libPath${libName}})
	endforeach()
endif()

if(IFCOPENSHELL_INCLUDE_DIR AND IFCOPENSHELL_FOUND)
	set(IFCOPENSHELL_FOUND TRUE)
	message(STATUS "IFCOPENSHELL installation found.")
	message(STATUS "IFCOPENSHELL_INCLUDE_DIR: ${IFCOPENSHELL_INCLUDE_DIR}")
	message(STATUS "IFCOPENSHELL_LIBRARIES: ${IFCOPENSHELL_LIBRARIES}")
else()
	set(IFCOPENSHELL_FOUND FALSE)
	message(STATUS "IFCOPENSHELL not found. Please set IFCOPENSHELL_ROOT to your installation directory")
endif()
