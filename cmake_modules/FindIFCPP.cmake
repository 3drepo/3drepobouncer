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

#attempt to find IFCPP library (renamed to avoid clashing with the assimp default one)
#if IFCPP is found, IFCPP_FOUND is set to true
#IFCPP_INCLUDE_DIR will point to the include folder of the installation
#IFCPP_LIBRARIES will point to the libraries


if(DEFINED ENV{IFCPP_ROOT})
	set(IFCPP_ROOT $ENV{IFCPP_ROOT})
	find_path(IFCPP_INCLUDE_DIR ifcpp carve earcut.hpp
		${IFCPP_ROOT}/include
		)
	find_library(IFCPP_MAIN_LIBRARIES_RELEASE NAMES IfcPlusPlus
		PATHS
		${IFCPP_ROOT}/lib
		${IFCPP_ROOT}/bin
	)
	find_library(IFCPP_CARVE_LIBRARIES_RELEASE NAMES carve
		PATHS
		${IFCPP_ROOT}/lib
		${IFCPP_ROOT}/bin
	)
	set(IFCPP_MAINLIB
		debug ${IFCPP_MAIN_LIBRARIES_DEBUG}
		optimized ${IFCPP_MAIN_LIBRARIES_RELEASE}
	)
	set(IFCPP_CARVELIB
		debug  ${IFCPP_CARVE_LIBRARIES_DEBUG}
		optimized  ${IFCPP_CARVE_LIBRARIES_RELEASE}
	)
endif()

if(IFCPP_INCLUDE_DIR AND IFCPP_MAINLIB AND IFCPP_CARVELIB)
	set(IFCPP_FOUND TRUE)

else(IFCPP_INCLUDE_DIR AND IFCPP_MAINLIB AND IFCPP_CARVELIB)
	find_path(IFCPP_INCLUDE_DIR ifcpp carve earcut.hpp
		/usr/include
		/usr/local/include
		/opt/local/include
    )

	find_library(IFCPP_MAIN_LIBRARIES_RELEASE NAMES IfcPlusPlus
    	PATHS
    	/usr/lib/
    	/usr/local/lib/
    	/opt/local/lib/
    )

	find_library(IFCPP_CARVE_LIBRARIES_RELEASE NAMES carve
    	PATHS
    	/usr/lib/
    	/usr/local/lib/
    	/opt/local/lib/
    )

	set(IFCPP_MAINLIB
		debug ${IFCPP_MAIN_LIBRARIES_DEBUG}
		optimized ${IFCPP_MAIN_LIBRARIES_RELEASE}
		)
	set(IFCPP_CARVELIB
		debug ${IFCPP_CARVE_LIBRARIES_DEBUG}
		optimized  ${IFCPP_CARVE_LIBRARIES_RELEASE}
	)
endif(IFCPP_INCLUDE_DIR AND IFCPP_MAINLIB AND IFCPP_CARVELIB)



if(IFCPP_INCLUDE_DIR AND IFCPP_MAINLIB AND IFCPP_CARVELIB)
	set(IFCPP_FOUND TRUE)
	message(STATUS "IFCPP installation found.")
	message(STATUS "IFCPP_INCLUDE_DIR: ${IFCPP_INCLUDE_DIR}")
	message(STATUS "IFCPP_LIBRARIES: ${IFCPP_MAINLIB} ${IFCPP_CARVELIB}")

else(IFCPP_INCLUDE_DIR AND IFCPP_MAINLIB AND IFCPP_CARVELIB)
	set(IFCPP_FOUND FALSE)
	message(STATUS "IFCPP not found. Please set IFCPP_ROOT to your installation directory")
endif(IFCPP_INCLUDE_DIR AND IFCPP_MAINLIB AND IFCPP_CARVELIB)
