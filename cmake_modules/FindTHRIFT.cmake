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

#attempt to find THRIFT library (renamed to avoid clashing with the assimp default one)
#if THRIFT is found, THRIFT_FOUND is set to true
#THRIFT_INCLUDE_DIR will point to the include folder of the installation
#THRIFT_LIBRARIES will point to the libraries


if(DEFINED ENV{THRIFT_ROOT})
	set(THRIFT_ROOT $ENV{THRIFT_ROOT})
	message(STATUS "$THRIFT_ROOT defined: ${THRIFT_ROOT}")
	find_path(THRIFT_INCLUDE_DIR thrift
		${THRIFT_ROOT}/include
		${THRIFT_ROOT}/src
		)
	find_library(THRIFT_LIBRARIES_RELEASE NAMES libthrift
		PATHS
		${THRIFT_ROOT}/lib
	)
	find_library(THRIFT_LIBRARIES_DEBUG NAMES libthriftd libthrift
		PATHS
		${THRIFT_ROOT}/lib
	)
	set(THRIFT_LIBRARIES
		debug ${THRIFT_LIBRARIES_DEBUG}
		optimized ${THRIFT_LIBRARIES_RELEASE}
	)
endif()

if(THRIFT_INCLUDE_DIR AND THRIFT_LIBRARIES)
	set(THRIFT_FOUND TRUE)

else(THRIFT_INCLUDE_DIR AND THRIFT_LIBRARIES)
	find_path(THRIFT_INCLUDE_DIR thrift
		/usr/include
		/usr/local/include
		/opt/local/include
    )

	find_library(THRIFT_LIBRARIES_RELEASE NAMES libthrift
    	PATHS
    	/usr/lib/
    	/usr/local/lib/
    	/opt/local/lib/
    )

	find_library(THRIFT_LIBRARIES_DEBUG NAMES libthrift libthriftd
    	PATHS
    	/usr/lib/
    	/usr/local/lib/
    	/opt/local/lib/
    )

	set(THRIFT_LIBRARIES
		debug ${THRIFT_LIBRARIES_DEBUG}
		optimized ${THRIFT_LIBRARIES_RELEASE}
		)
endif(THRIFT_INCLUDE_DIR AND THRIFT_LIBRARIES)



if(THRIFT_INCLUDE_DIR AND THRIFT_LIBRARIES)
	set(THRIFTM_FOUND TRUE)
	message(STATUS "THRIFT installation found.")
	message(STATUS "THRIFT_INCLUDE_DIR: ${THRIFT_INCLUDE_DIR}")
	message(STATUS "THRIFT_LIBRARIES: ${THRIFT_LIBRARIES}")

else(THRIFT_INCLUDE_DIR AND THRIFT_LIBRARIES)
	set(THRIFTM_FOUND FALSE)
	message(STATUS "THRIFT not found. Please set THRIFT_ROOT to your installation directory")
endif(THRIFT_INCLUDE_DIR AND THRIFT_LIBRARIES)
