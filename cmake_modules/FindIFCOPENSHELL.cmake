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


if(DEFINED ENV{IFCOPENSHELL_ROOT})
	set(IFCOPENSHELL_ROOT $ENV{IFCOPENSHELL_ROOT})
	find_path(IFCOPENSHELL_INCLUDE_DIR ifcparse ifcgeom
		${IFCOPENSHELL_ROOT}/include
		)    
	find_library(IFCOPENSHELL_PARSE_LIBRARIES_RELEASE NAMES IfcParse
		PATHS
		${IFCOPENSHELL_ROOT}/lib
	)	
	find_library(IFCOPENSHELL_GEOM_LIBRARIES_RELEASE NAMES IfcGeom
		PATHS
		${IFCOPENSHELL_ROOT}/lib
	)	
	find_library(IFCOPENSHELL_PARSE_LIBRARIES_DEBUG NAMES IfcParse_d
		PATHS
		${IFCOPENSHELL_ROOT}/lib
	)
	find_library(IFCOPENSHELL_GEOM_LIBRARIES_DEBUG NAMES IfcGeom_d
		PATHS
		${IFCOPENSHELL_ROOT}/lib
	)
	set(IFCOPENSHELL_LIBRARIES
		debug ${IFCOPENSHELL_PARSE_LIBRARIES_DEBUG} ${IFCOPENSHELL_GEOM_LIBRARIES_DEBUG}
		optimized ${IFCOPENSHELL_PARSE_LIBRARIES_RELEASE} ${IFCOPENSHELL_GEOM_LIBRARIES_RELEASE}
	)
endif()

if(IFCOPENSHELL_INCLUDE_DIR AND IFCOPENSHELL_LIBRARIES)
	set(IFCOPENSHELL_FOUND TRUE)

else(IFCOPENSHELL_INCLUDE_DIR AND IFCOPENSHELL_LIBRARIES)
	find_path(IFCOPENSHELL_INCLUDE_DIR ifcparse ifcgeom
		/usr/include
		/usr/local/include
		/opt/local/include
    )

	find_library(IFCOPENSHELL_PARSE_LIBRARIES_RELEASE NAMES IfcParse
    	PATHS
    	/usr/lib/
    	/usr/local/lib/
    	/opt/local/lib/
    )
	
	find_library(IFCOPENSHELL_GEOM_LIBRARIES_RELEASE NAMES IfcGeom
    	PATHS
    	/usr/lib/
    	/usr/local/lib/
    	/opt/local/lib/
    )
	
	find_library(IFCOPENSHELL_PARSE_LIBRARIES_DEBUG NAMES IfcParse_d IfcParse
    	PATHS
    	/usr/lib/
    	/usr/local/lib/
    	/opt/local/lib/
    )
	
	find_library(IFCOPENSHELL_GEOM_LIBRARIES_DEBUG NAMES IfcGeom_d IfcGeom
    	PATHS
    	/usr/lib/
    	/usr/local/lib/
    	/opt/local/lib/
    )

	set(IFCOPENSHELL_LIBRARIES
		debug ${IFCOPENSHELL_PARSE_LIBRARIES_DEBUG} ${IFCOPENSHELL_GEOM_LIBRARIES_DEBUG}
		optimized ${IFCOPENSHELL_PARSE_LIBRARIES_RELEASE}  ${IFCOPENSHELL_GEOM_LIBRARIES_RELEASE}
		)
endif(IFCOPENSHELL_INCLUDE_DIR AND IFCOPENSHELL_LIBRARIES)



if(IFCOPENSHELL_INCLUDE_DIR AND IFCOPENSHELL_LIBRARIES)
	set(IFCOPENSHELLM_FOUND TRUE)
	message(STATUS "IFCOPENSHELL installation found.")
	message(STATUS "IFCOPENSHELL_INCLUDE_DIR: ${IFCOPENSHELL_INCLUDE_DIR}")
	message(STATUS "IFCOPENSHELL_LIBRARIES: ${IFCOPENSHELL_LIBRARIES}")
	
else(IFCOPENSHELL_INCLUDE_DIR AND IFCOPENSHELL_LIBRARIES)
	set(IFCOPENSHELL_FOUND FALSE)
	message(STATUS "IFCOPENSHELL not found. Please set IFCOPENSHELL_ROOT to your installation directory")
endif(IFCOPENSHELL_INCLUDE_DIR AND IFCOPENSHELL_LIBRARIES)
