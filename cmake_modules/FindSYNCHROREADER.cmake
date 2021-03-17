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

#attempt to find SYNCHRO_READER library (renamed to avoid clashing with the assimp default one)
#if SYNCHRO_READER is found, SYNCHRO_READER_FOUND is set to true
#SYNCHRO_READER_INCLUDE_DIR will point to the include folder of the installation
#SYNCHRO_READER_LIBRARIES will point to the libraries


if(DEFINED ENV{SYNCHRO_READER_ROOT})
	set(SYNCHRO_READER_ROOT $ENV{SYNCHRO_READER_ROOT})
	message(STATUS "$SYNCHRO_READER_ROOT defined: ${SYNCHRO_READER_ROOT}")
	find_path(SYNCHRO_READER_INCLUDE_DIR synchro_reader
		${SYNCHRO_READER_ROOT}/include
		)
	find_library(SYNCHRO_READER_LIBRARIES_RELEASE NAMES 3drepoSynchroReader_2_0_0
		PATHS
		${SYNCHRO_READER_ROOT}/lib
	)
find_library(SYNCHRO_READER_LIBRARIES_DEBUG NAMES 3drepoSynchroReader_2_0_0d 3drepoSynchroReader_2_0_0
		PATHS
		${SYNCHRO_READER_ROOT}/lib
	)
	set(SYNCHRO_READER_LIBRARIES
		debug ${SYNCHRO_READER_LIBRARIES_DEBUG}
		optimized ${SYNCHRO_READER_LIBRARIES_RELEASE}
	)
endif()

if(SYNCHRO_READER_INCLUDE_DIR AND SYNCHRO_READER_LIBRARIES)
	set(SYNCHRO_READER_FOUND TRUE)

else(SYNCHRO_READER_INCLUDE_DIR AND SYNCHRO_READER_LIBRARIES)
	find_path(_SYNCHRO_READER_INCLUDE_DIR synchro_reader
		/usr/include
		/usr/local/include
		/opt/local/include
    )

	find_library(SYNCHRO_READER_LIBRARIES_RELEASE NAMES 3drepoSynchroReader_2_0_0
    	PATHS
    	/usr/lib/
    	/usr/local/lib/
    	/opt/local/lib/
    )

	find_library(SYNCHRO_READER_LIBRARIES_DEBUG NAMES 3drepoSynchroReader_2_0_0d 3drepoSynchroReader_2_0_0
    	PATHS
    	/usr/lib/
    	/usr/local/lib/
    	/opt/local/lib/
    )

	set(SYNCHRO_READER_LIBRARIES
		debug ${SYNCHRO_READER_LIBRARIES_DEBUG}
		optimized ${SYNCHRO_READER_LIBRARIES_RELEASE}
		)
endif(SYNCHRO_READER_INCLUDE_DIR AND SYNCHRO_READER_LIBRARIES)



if(SYNCHRO_READER_INCLUDE_DIR AND SYNCHRO_READER_LIBRARIES)
	set(SYNCHRO_READERM_FOUND TRUE)
	message(STATUS "SYNCHRO_READER installation found.")
	message(STATUS "SYNCHRO_READER_INCLUDE_DIR: ${SYNCHRO_READER_INCLUDE_DIR}")
	message(STATUS "SYNCHRO_READER_LIBRARIES: ${SYNCHRO_READER_LIBRARIES}")

else(SYNCHRO_READER_INCLUDE_DIR AND SYNCHRO_READER_LIBRARIES)
	set(SYNCHRO_READERM_FOUND FALSE)
	message(STATUS "SYNCHRO_READER not found. Please set SYNCHRO_READER_ROOT to your installation directory")
endif(SYNCHRO_READER_INCLUDE_DIR AND SYNCHRO_READER_LIBRARIES)
