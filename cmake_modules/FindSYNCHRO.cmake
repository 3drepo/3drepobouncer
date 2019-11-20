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

#attempt to find SYNCHRO library (renamed to avoid clashing with the assimp default one)
#if SYNCHRO is found, SYNCHRO_FOUND is set to true
#SYNCHRO_INCLUDE_DIR will point to the include folder of the installation
#SYNCHRO_LIBRARIES will point to the libraries


if(DEFINED ENV{SYNCHRO_ROOT})
	set(SYNCHRO_ROOT $ENV{SYNCHRO_ROOT})
	message(STATUS "$SYNCHRO_ROOT defined: ${SYNCHRO_ROOT}")
	find_path(_SYNCHRO_INCLUDE_DIR synchroAPI
		${SYNCHRO_ROOT}/include
		${SYNCHRO_ROOT}
		)
	find_library(SYNCHRO_LIBRARIES_RELEASE NAMES Synchro
		PATHS
		${SYNCHRO_ROOT}/lib
	)
find_library(SYNCHRO_LIBRARIES_DEBUG NAMES Synchro
		PATHS
		${SYNCHRO_ROOT}/lib
	)
	set(SYNCHRO_LIBRARIES
		debug ${SYNCHRO_LIBRARIES_DEBUG}
		optimized ${SYNCHRO_LIBRARIES_RELEASE}
	)
endif()

if(_SYNCHRO_INCLUDE_DIR AND SYNCHRO_LIBRARIES)
	set(SYNCHRO_FOUND TRUE)

else(_SYNCHRO_INCLUDE_DIR AND SYNCHRO_LIBRARIES)
	find_path(_SYNCHRO_INCLUDE_DIR synchroAPI
		/usr/include
		/usr/local/include
		/opt/local/include
    )

	find_library(SYNCHRO_LIBRARIES_RELEASE NAMES Synchro
    	PATHS
    	/usr/lib/
    	/usr/local/lib/
    	/opt/local/lib/
    )

	find_library(SYNCHRO_LIBRARIES_DEBUG NAMES Synchro
    	PATHS
    	/usr/lib/
    	/usr/local/lib/
    	/opt/local/lib/
    )

	set(SYNCHRO_LIBRARIES
		debug ${SYNCHRO_LIBRARIES_DEBUG}
		optimized ${SYNCHRO_LIBRARIES_RELEASE}
		)
endif(_SYNCHRO_INCLUDE_DIR AND SYNCHRO_LIBRARIES)



if(_SYNCHRO_INCLUDE_DIR AND SYNCHRO_LIBRARIES)
	set(SYNCHRO_INCLUDE_DIR
		${_SYNCHRO_INCLUDE_DIR}
		${_SYNCHRO_INCLUDE_DIR}/synchroAPI
	)
	set(SYNCHROM_FOUND TRUE)
	message(STATUS "SYNCHRO installation found.")
	message(STATUS "SYNCHRO_INCLUDE_DIR: ${SYNCHRO_INCLUDE_DIR}")
	message(STATUS "SYNCHRO_LIBRARIES: ${SYNCHRO_LIBRARIES}")

else(_SYNCHRO_INCLUDE_DIR AND SYNCHRO_LIBRARIES)
	set(SYNCHROM_FOUND FALSE)
	message(STATUS "SYNCHRO not found. Please set SYNCHRO_ROOT to your installation directory")
endif(_SYNCHRO_INCLUDE_DIR AND SYNCHRO_LIBRARIES)
