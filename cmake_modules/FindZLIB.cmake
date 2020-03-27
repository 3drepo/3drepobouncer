#  Copyright (C) 2019 3D Repo Ltd
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

#attempt to find ZLIB library (renamed to avoid clashing with the assimp default one)
#if ZLIB is found, ZLIB_FOUND is set to true
#ZLIB_INCLUDE_DIR will point to the include folder of the installation
#ZLIB_LIBRARIES will point to the libraries


if(DEFINED ENV{ZLIB_ROOT})
	set(ZLIB_ROOT $ENV{ZLIB_ROOT})
	message(STATUS "$ZLIB_ROOT defined: ${ZLIB_ROOT}")
	find_path(ZLIB_INCLUDE_DIR zlib.h
		${ZLIB_ROOT}/include
		${ZLIB_ROOT}
		)
	find_library(ZLIB_LIBRARIES_RELEASE NAMES zlibwapi z
		PATHS
		${ZLIB_ROOT}/lib
	)
	find_library(ZLIB_LIBRARIES_DEBUG NAMES zlibwapid zlibwapi z
		PATHS
		${ZLIB_ROOT}/lib
	)
	set(ZLIB_LIBRARIES
		debug ${ZLIB_LIBRARIES_DEBUG}
		optimized ${ZLIB_LIBRARIES_RELEASE}
	)
endif()

if(ZLIB_INCLUDE_DIR AND ZLIB_LIBRARIES)
	set(ZLIB_FOUND TRUE)

else(ZLIB_INCLUDE_DIR AND ZLIB_LIBRARIES)
	find_path(ZLIB_INCLUDE_DIR zlib.h
		/usr/include
		/usr/local/include
		/opt/local/include
    )

	find_library(ZLIB_LIBRARIES_RELEASE NAMES zlibwapi z
    	PATHS
    	/usr/lib/
    	/usr/local/lib/
    	/opt/local/lib/
    )

	find_library(ZLIB_LIBRARIES_DEBUG NAMES zlibwapid zlibwapi z
    	PATHS
    	/usr/lib/
    	/usr/local/lib/
    	/opt/local/lib/
    )

	set(ZLIB_LIBRARIES
		debug ${ZLIB_LIBRARIES_DEBUG}
		optimized ${ZLIB_LIBRARIES_RELEASE}
		)
endif(ZLIB_INCLUDE_DIR AND ZLIB_LIBRARIES)



if(ZLIB_INCLUDE_DIR AND ZLIB_LIBRARIES)
	set(ZLIBM_FOUND TRUE)
	message(STATUS "ZLIB installation found.")
	message(STATUS "ZLIB_INCLUDE_DIR: ${ZLIB_INCLUDE_DIR}")
	message(STATUS "ZLIB_LIBRARIES: ${ZLIB_LIBRARIES}")

else(ZLIB_INCLUDE_DIR AND ZLIB_LIBRARIES)
	set(ZLIBM_FOUND FALSE)
	message(STATUS "ZLIB not found. Please set ZLIB_ROOT to your installation directory")
endif(ZLIB_INCLUDE_DIR AND ZLIB_LIBRARIES)
