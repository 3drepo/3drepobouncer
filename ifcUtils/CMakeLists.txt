#  Copyright (C) 2025 3D Repo Ltd
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

# Ifc Utils is built around IfcOpenShell; find IFCOS but also
# any dependencies that are not in its cmake find module (e.g. OCCT).

find_package(OCCT REQUIRED)
if(NOT ${OCCT_FOUND})
	message(FATAL_ERROR "Cannot find OCCT (a dependency of IfcOpenShell)")
endif()

find_package(EIGEN REQUIRED)
if(NOT ${EIGEN_FOUND})
	message(FATAL_ERROR "Cannot find Eigen (a dependency of IfcOpenShell)")
endif()

find_package(GMP REQUIRED)
if(NOT ${GMP_FOUND})
	message(FATAL_ERROR "Cannot find GMP (a dependency of IfcOpenShell)")
endif()

find_package(MPFR REQUIRED)
if(NOT ${MPFR_FOUND})
	message(FATAL_ERROR "Cannot find MPFR (a dependency of IfcOpenShell)")
endif()

find_package(IFCOPENSHELL REQUIRED)
if(NOT ${IFCOPENSHELL_FOUND})
	message(FATAL_ERROR "Cannot find IfcOpenShell")
endif()

include_directories(
	${Boost_INCLUDE_DIR}
	${OCCT_INCLUDE_DIR}
	${EIGEN_INCLUDE_DIR}
	${IFCOPENSHELL_INCLUDE_DIR}
	../bouncer/src
	../log
)

if(NOT SCHEMA_VERSIONS)
	set(SCHEMA_VERSIONS "2x3" "4" "4x1" "4x2" "4x3_add2")
endif()

# On GCC the static libraries must be compiled with Position Independent
# Code because they will be linked into the 3drepobouncer shared library,
# which will have its addresses changed by the dynamic loader.
	
set(STATIC_COMPILE_FLAGS "")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	set(STATIC_COMPILE_FLAGS "-fPIC")
endif()

foreach(schema ${SCHEMA_VERSIONS})
	add_library("ifcSerialiser_${schema}" STATIC
		./repo_ifc_serialiser.cpp
	)
	
	set_target_properties("ifcSerialiser_${schema}" PROPERTIES
		COMPILE_FLAGS "-DREPO_API_LIBRARY -DIfcSchema=Ifc${schema} -DDEFINED_Ifc${schema} ${STATIC_COMPILE_FLAGS}"
	)	
	
	target_link_libraries("ifcSerialiser_${schema}"
		${Boost_LIBRARIES}
		${OCCT_LIBRARIES}
		${GMP_LIBRARIES}
		${MPFR_LIBRARIES}
		${IFCOPENSHELL_LIBRARIES}
		log
	)
		
	set(IFCSERIALISER_LIBARIES ${IFCSERIALISER_LIBARIES} "ifcSerialiser_${schema}")
endforeach()

# This library is the entry point for the AbstractImporter - it will figure out
# which schema to use and build the appropriate serialiser.

add_library("ifcUtils" STATIC
	./repo_ifc_utils.cpp
)

set_target_properties("ifcUtils" PROPERTIES COMPILE_FLAGS "${STATIC_COMPILE_FLAGS}")

target_link_libraries("ifcUtils"
	${IFCSERIALISER_LIBARIES}
)

set(IFCUTILS_LIBRARIES
		ifcUtils
	PARENT_SCOPE
)
set(IFCUTILS_INCLUDE_DIR
		${CMAKE_CURRENT_SOURCE_DIR}
	PARENT_SCOPE
)