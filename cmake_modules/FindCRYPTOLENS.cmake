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


# attempt to find CRYPTOLENS C++ driver via CRYPTOLENS_ROOT
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
		${CRYPTOLENS_ROOT}/lib
	)
	find_library(CRYPTOLENS_LIBRARIES_DEBUG
		NAMES cryptolens
		PATHS
		${CRYPTOLENS_ROOT}/lib/Debug
		${CRYPTOLENS_ROOT}/lib
	)
	set(CRYPTOLENS_LIBRARIES
		debug ${CRYPTOLENS_LIBRARIES_DEBUG}
		optimized ${CRYPTOLENS_LIBRARIES_RELEASE}
		)
	if(WIN32)
	# required for cryptolens on windows
		set(CRYPTOLENS_LIBRARIES ${CRYPTOLENS_LIBRARIES} Winhttp.lib)
	endif()
endif()

# attempt to find CRYPTOLENS C++ driver via common install dirs
if(CRYPTOLENS_INCLUDE_DIR AND CRYPTOLENS_LIBRARIES)
	set(CRYPTOLENS_FOUND TRUE)
else(CRYPTOLENS_INCLUDE_DIR AND CRYPTOLENS_LIBRARIES)
	find_path(CRYPTOLENS_INCLUDE_DIR cryptolens/core.hpp
		/usr/include
		/usr/local/include
		/opt/local/include
    )
	find_library(CRYPTOLENS_LIBRARIES_RELEASE NAMES cryptolens
    	PATHS
    	/usr/lib
    	/usr/local/lib
    	/opt/local/lib
		/usr/lib64
    )
	set(CRYPTOLENS_LIBRARIES
		debug ${CRYPTOLENS_LIBRARIES_RELEASE}
		optimized ${CRYPTOLENS_LIBRARIES_RELEASE}
		)
endif(CRYPTOLENS_INCLUDE_DIR AND CRYPTOLENS_LIBRARIES)

# exit if we found libs/includes
if(CRYPTOLENS_INCLUDE_DIR AND CRYPTOLENS_LIBRARIES)
	# confirm structure of the folder is correct
	get_filename_component(THIRD_PARTY_FULLPATH "${CRYPTOLENS_INCLUDE_DIR}/../third_party" REALPATH)
    if (NOT EXISTS "${THIRD_PARTY_FULLPATH}")
		message(FATAL_ERROR "cryptolens third party folder not found")
    else()
		set(CRYPTOLENS_FOUND TRUE)
		message(STATUS "CRYPTOLENS installation found.")
		message(STATUS "CRYPTOLENS_INCLUDE_DIR: ${CRYPTOLENS_INCLUDE_DIR}")
		message(STATUS "CRYPTOLENS_LIBRARIES: ${CRYPTOLENS_LIBRARIES}")
	endif()
# cannot find CRYPTOLENS anywhere!
else(CRYPTOLENS_INCLUDE_DIR AND CRYPTOLENS_LIBRARIES)
	set(CRYPTOLENS_FOUND FALSE)
	message(STATUS "CRYPTOLENS not found. Please set CRYPTOLENS_ROOT to your installation directory")
endif(CRYPTOLENS_INCLUDE_DIR AND CRYPTOLENS_LIBRARIES)
