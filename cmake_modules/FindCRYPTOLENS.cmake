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


# attempt to find CRYPTOLENS C++ driver 
# if CRYPTOLENS is found, CRYPTOLENS_FOUND is set to true
# CRYPTOLENS_INCLUDE_DIR will point to the include folder of the installation
# CRYPTOLENS_LIBRARIES will point to the libraries
#
# Assuming the folder structure is:
# cryptolens_root
#   - include
#   - lib
#     - Debug
#     - Release
#
# start the search by looking for the lib in the 
# root DIR of of CRYPTOLENS
if(DEFINED ENV{CRYPTOLENS_ROOT})
	set(CRYPTOLENS_ROOT $ENV{CRYPTOLENS_ROOT})
	message(STATUS "$CRYPTOLENS_ROOT defined: ${CRYPTOLENS_ROOT}")
	find_path(CRYPTOLENS_INCLUDE_DIR cryptolens/core.hpp
		${CRYPTOLENS_ROOT}/include
		)    
	find_library(CRYPTOLENS_LIBRARIES_RELEASE 
		NAMES cryptolens
		PATHS
		${CRYPTOLENS_ROOT}/lib/Release
	)	
	find_library(CRYPTOLENS_LIBRARIES_DEBUG 
		NAMES cryptolens
		PATHS
		${CRYPTOLENS_ROOT}/lib/Debug
	)
	set(CRYPTOLENS_LIBRARIES
		debug ${CRYPTOLENS_LIBRARIES_DEBUG}
		optimized ${CRYPTOLENS_LIBRARIES_RELEASE}
		)
endif()

# exit if we found libs/includes
if(CRYPTOLENS_INCLUDE_DIR AND CRYPTOLENS_LIBRARIES)
	set(CRYPTOLENS_FOUND TRUE)
	message(STATUS "CRYPTOLENS installation found.")
	message(STATUS "CRYPTOLENS_INCLUDE_DIR: ${CRYPTOLENS_INCLUDE_DIR}")
	message(STATUS "CRYPTOLENS_LIBRARIES: ${CRYPTOLENS_LIBRARIES}")
	
# cannot find CRYPTOLENS anywhere!
else(CRYPTOLENS_INCLUDE_DIR AND CRYPTOLENS_LIBRARIES)
	set(CRYPTOLENS_FOUND FALSE)
	message(STATUS "CRYPTOLENS not found. Please set CRYPTOLENS_ROOT to your installation directory")
endif(CRYPTOLENS_INCLUDE_DIR AND CRYPTOLENS_LIBRARIES)
