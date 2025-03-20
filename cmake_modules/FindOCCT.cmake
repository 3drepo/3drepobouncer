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

#attempt to find OCCT library (renamed to avoid clashing with the assimp default one)
#if OCCT is found, OCCT_FOUND is set to true
#OCCT_INCLUDE_DIR will point to the include folder of the installation
#OCCT_LIBRARIES will point to the libraries


SET(OCCT_LIB_NAMES
	TKernel
	TKMath
	TKG2d
	TKG3d
	TKGeomBase
	TKBRep
	TKGeomAlgo
	TKTopAlgo
	TKPrim
	TKBO
	TKShHealing
	TKBool
	TKHLR
	TKFillet
	TKOffset
	TKFeat
	TKMesh
	TKXMesh
	TKService
	TKV3d
	TKOpenGl
	TKMeshVS
	TKCDF
	TKLCAF
	TKCAF
	TKBinL
	TKXmlL
	TKBin
	TKXml
	TKStdL
	TKStd
	TKTObj
	TKBinTObj
	TKXmlTObj
	TKVCAF
	TKXSBase
	TKXCAF
	TKRWMesh
	TKBinXCAF
	TKXmlXCAF
)

if(DEFINED ENV{OCCT_DEBUG_LIB_DIR})
	set(OCCT_DEBUG_LIB_DIR $ENV{OCCT_DEBUG_LIB_DIR})
	message(STATUS "$OCCT_DEBUG_LIB_DIR defined: ${OCCT_DEBUG_LIB_DIR}")
else(DEFINED ENV{OCCT_DEBUG_LIB_DIR})
	set(OCCT_DEBUG_LIB_DIR ${OCCT_LIB_DIR})
endif()

if(DEFINED ENV{OCCT_ROOT})
	set(OCCT_ROOT $ENV{OCCT_ROOT})
	message(STATUS "$OCCT_ROOT defined: ${OCCT_ROOT}")
	if(DEFINED ENV{OCCT_LIB_DIR})
		set(OCCT_LIB_DIR $ENV{OCCT_LIB_DIR})
		message(STATUS "$OCCT_LIB_DIR defined: ${OCCT_LIB_DIR}")
	endif()
	find_path(OCCT_INCLUDE_DIR  NAMES gp_Pnt.hxx
		PATHS
		${OCCT_ROOT}/include
		${OCCT_ROOT}/include/opencascade
		${OCCT_ROOT}/inc
	)
	
	foreach(libName ${OCCT_LIB_NAMES})
		find_library(libPathRelease${libName} NAMES ${libName}
			PATHS
			${OCCT_ROOT}/lib
			${OCCT_LIB_DIR}
		)
		set(OCCT_LIBRARIES_RELEASE ${OCCT_LIBRARIES_RELEASE} ${libPathRelease${libName}})
		find_library(libPathDebug${libName} NAMES ${libName}
			PATHS
			${OCCT_DEBUG_LIB_DIR}
			${OCCT_LIB_DIR}
			${OCCT_ROOT}/lib
		)
		set(OCCT_LIBRARIES_DEBUG ${OCCT_LIBRARIES_DEBUG} ${libPathDebug${libName}})
	endforeach()

	set(OCCT_LIBRARIES
		debug ${OCCT_LIBRARIES_DEBUG}
		optimized ${OCCT_LIBRARIES_RELEASE}
	)
endif()

if(OCCT_INCLUDE_DIR AND OCCT_LIBRARIES)
	set(OCCT_FOUND TRUE)
else(OCCT_INCLUDE_DIR AND OCCT_LIBRARIES)
	find_path(OCCT_INCLUDE_DIR gp_Pnt.hxx
		/usr/include/opencascade
		/usr/local/include/opencascade
		/opt/local/include/opencascade
	)

	foreach(libName ${OCCT_LIB_NAMES})
		find_library(libPathRelease${libName} NAMES ${libName}
			PATHS
			/usr/lib/
			/usr/local/lib/
			/opt/local/lib/
			/usr/lib/x86_64-linux-gnu/
		)
		set(OCCT_LIBRARIES_RELEASE ${OCCT_LIBRARIES_RELEASE} ${libPathRelease${libName}})
		find_library(libPathDebug${libName} NAMES ${libName}
			PATHS
			${OCCT_DEBUG_LIB_DIR}
			/usr/lib/
			/usr/local/lib/
			/opt/local/lib/
			/usr/lib/x86_64-linux-gnu/
		)
		set(OCCT_LIBRARIES_DEBUG ${OCCT_LIBRARIES_DEBUG} ${libPathDebug${libName}})
	endforeach()

	set(OCCT_LIBRARIES
		debug ${OCCT_LIBRARIES_DEBUG}
		optimized ${OCCT_LIBRARIES_RELEASE}
		)
endif(OCCT_INCLUDE_DIR AND OCCT_LIBRARIES)

# OCCT on Windows requires winsock
if(WIN32)
	set(OCCT_LIBRARIES ${OCCT_LIBRARIES} ws2_32)
endif()

message(STATUS "OCCT_INCLUDE_DIR: ${OCCT_INCLUDE_DIR}")
message(STATUS "OCCT_LIBRARIES: ${OCCT_LIBRARIES}")

if(OCCT_INCLUDE_DIR AND OCCT_LIBRARIES)
	set(OCCTM_FOUND TRUE)
	message(STATUS "OCCT installation found.")
	message(STATUS "OCCT_INCLUDE_DIR: ${OCCT_INCLUDE_DIR}")
	message(STATUS "OCCT_LIBRARIES: ${OCCT_LIBRARIES}")

else(OCCT_INCLUDE_DIR AND OCCT_LIBRARIES)
	set(OCCT_FOUND FALSE)
	message(STATUS "OCCT not found. Please set OCCT_ROOT to your installation directory")
endif(OCCT_INCLUDE_DIR AND OCCT_LIBRARIES)
