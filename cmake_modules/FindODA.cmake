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

#attempt to find ODA library (renamed to avoid clashing with the assimp default one)
#if ODA is found, ODA_FOUND is set to true
#ODA_INCLUDE_DIR will point to the include folder of the installation
#ODA_LIBRARIES will point to the libraries

set(ODA_LIB_NAMES
	TD_ExamplesCommon TG_ExamplesCommon TB_Common TB_ExamplesCommon TB_Essential
	TG_Db TD_DbRoot TD_Gs TD_Gi TD_Ge TD_Root TD_Alloc TB_Loader
	TB_Database TB_Base TB_ModelerGeometry TD_Br TD_TfCore TB_Main
	TB_MEP TB_Analytical TB_Architecture TB_StairsRamp TB_Geometry TB_HostObj)


if(DEFINED ENV{ODA_LIB_DIR})
	set(ODA_LIB_DIR $ENV{ODA_LIB_DIR})
endif()

if(DEFINED ENV{ODA_ROOT})
	set(ODA_ROOT $ENV{ODA_ROOT})

	message(STATUS "ODA_ROOT: ${ODA_ROOT}")
	message(STATUS "ODA_LIB_DIR: ${ODA_LIB_DIR}")
	string(REPLACE "lib" "bin" ODA_BIN_DIR ${ODA_LIB_DIR})
	message(STATUS "ODA_BIN_DIR: ${ODA_BIN_DIR}")

	set(ODA_INCLUDE_DIR
			${ODA_ROOT}/ThirdParty/activation
			${ODA_ROOT}/Kernel/Include
			${ODA_ROOT}/KernelBase/Include
			${ODA_ROOT}/Drawing/include
			${ODA_ROOT}/Dgn/include
			${ODA_ROOT}/Dgn/Examples/Common
			${ODA_ROOT}/Dgn/Extensions/ExServices
			${ODA_ROOT}/Exchange
			${ODA_ROOT}/Kernel/Extensions/ExServices
			${ODA_ROOT}/BimRv/Include
			${ODA_ROOT}/BimRv/Extensions
			${ODA_ROOT}/BimRv/Extensions/ExServices
			${ODA_ROOT}/BimRv/Examples/Common
			${ODA_ROOT}/BimRv/Examples
			${ODA_ROOT}/Prc/Include
	)

	foreach(libName ${ODA_LIB_NAMES})
		find_library(libPathRelease${libName} NAMES ${libName}
			PATHS
			${ODA_ROOT}/lib
			${ODA_BIN_DIR}
			${ODA_LIB_DIR}
		)

		if(NOT libPathRelease${libName} AND UNIX)
			#check for .tx (unix)
			SET(CMAKE_FIND_LIBRARY_PREFIXES "")
			SET(CMAKE_FIND_LIBRARY_SUFFIXES ".tx")
			find_library(libPathRelease${libName} NAMES ${libName}
				PATHS
				${ODA_ROOT}/lib
				${ODA_BIN_DIR}
				${ODA_LIB_DIR}
			)

			SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
			SET(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".a")
		endif()

		if(NOT libPathRelease${libName} AND APPLE)
			#check for .tx
			SET(CMAKE_FIND_LIBRARY_PREFIXES "")
			SET(CMAKE_FIND_LIBRARY_SUFFIXES ".tx")
			find_library(libPathRelease${libName} NAMES ${libName}
				PATHS
				${ODA_ROOT}/lib
				${ODA_BIN_DIR}
				${ODA_LIB_DIR}
			)

			SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
			SET(CMAKE_FIND_LIBRARY_SUFFIXES ".dylib" ".a")
		endif()

		set(ODA_LIBRARIES_RELEASE ${ODA_LIBRARIES_RELEASE} ${libPathRelease${libName}})

		#There's no debug library provided
		find_library(libPathDebug${libName} NAMES ${libName}
			PATHS
			${ODA_ROOT}/lib
			${ODA_BIN_DIR}
			${ODA_LIB_DIR}
		)

		if(NOT libPathDebug${libName} AND UNIX)
			#check for .tx
			SET(CMAKE_FIND_LIBRARY_PREFIXES "")
			SET(CMAKE_FIND_LIBRARY_SUFFIXES ".tx")
			find_library(libPathDebug${libName} NAMES ${libName}
				PATHS
				${ODA_ROOT}/lib
				${ODA_BIN_DIR}
				${ODA_LIB_DIR}
			)

			SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
			SET(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".a")
		endif()

		if(NOT libPathDebug${libName} AND APPLE)
			#check for .tx
			SET(CMAKE_FIND_LIBRARY_PREFIXES "")
			SET(CMAKE_FIND_LIBRARY_SUFFIXES ".tx")
			find_library(libPathDebug${libName} NAMES ${libName}
				PATHS
				${ODA_ROOT}/lib
				${ODA_BIN_DIR}
				${ODA_LIB_DIR}
			)

			SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
			SET(CMAKE_FIND_LIBRARY_SUFFIXES ".dylib" ".a")
		endif()
		set(ODA_LIBRARIES_DEBUG ${ODA_LIBRARIES_DEBUG} ${libPathDebug${libName}})
	endforeach()

	set(ODA_LIB
		debug ${ODA_LIBRARIES_DEBUG}
		optimized ${ODA_LIBRARIES_RELEASE}
	)
	message(STATUS "ODA_INCLUDE_DIR: ${ODA_INCLUDE_DIR}")
	message(STATUS "ODA_LIBRARIES: ${ODA_LIB}")
endif()


if(ODA_INCLUDE_DIR AND ODA_LIB)
	set(ODA_FOUND TRUE)
	message(STATUS "ODA installation found.")
	message(STATUS "ODA_INCLUDE_DIR: ${ODA_INCLUDE_DIR}")
	message(STATUS "ODA_LIBRARIES: ${ODA_LIB}")

else(ODA_INCLUDE_DIR AND ODA_LIB)
	set(ODA_FOUND FALSE)
	message(STATUS "ODA not found. Please set ODA_ROOT to your installation directory")
endif(ODA_INCLUDE_DIR AND ODA_LIB)
