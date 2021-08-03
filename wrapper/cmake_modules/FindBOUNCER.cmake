#  Copyright (C) 2017 3D Repo Ltd
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

#attempt to find BOUNCER library (renamed to avoid clashing with the assimp default one)
#if BOUNCER is found, BOUNCER_FOUND is set to true
#BOUNCER_INCLUDE_DIR will point to the include folder of the installation
#BOUNCER_LIBRARIES will point to the libraries


if(DEFINED ENV{REPOBOUNCER_ROOT})
	set(BOUNCER_ROOT $ENV{REPOBOUNCER_ROOT})
	message(STATUS "$BOUNCER_ROOT defined: ${BOUNCER_ROOT}")
	find_path(BOUNCER_INCLUDE_DIR repo
		${BOUNCER_ROOT}/include
		)    
	find_library(BOUNCER_LIBRARIES_RELEASE NAMES 3drepobouncer_4_17_1 3drepobouncer
		PATHS
		${BOUNCER_ROOT}/lib
	)	
	find_library(BOUNCER_LIBRARIES_DEBUG NAMES 3drepobouncer_4_17_1_d 3drepobouncer_4_17_1 3drepobouncer
		PATHS
		${BOUNCER_ROOT}/lib
	)
	set(BOUNCER_LIBRARIES
		debug ${BOUNCER_LIBRARIES_DEBUG}
		optimized ${BOUNCER_LIBRARIES_RELEASE}
	)
endif()

if(BOUNCER_INCLUDE_DIR AND BOUNCER_LIBRARIES)
	set(BOUNCER_FOUND TRUE)

else(BOUNCER_INCLUDE_DIR AND BOUNCER_LIBRARIES)
	find_path(BOUNCER_INCLUDE_DIR 3drepobouncer
		/usr/include
		/usr/local/include
		/opt/local/include
    )

	find_library(BOUNCER_LIBRARIES_RELEASE NAMES  3drepobouncer_1_13_0 3drepobouncer
    	PATHS
    	/usr/lib/
    	/usr/local/lib/
    	/opt/local/lib/
    )
	
	find_library(BOUNCER_LIBRARIES_DEBUG NAMES 3drepobouncer_4_17_1_d 3drepobouncer_4_17_1    	PATHS
    	/usr/lib/
    	/usr/local/lib/
    	/opt/local/lib/
    )

	set(BOUNCER_LIBRARIES
		debug ${BOUNCER_LIBRARIES_DEBUG}
		optimized ${BOUNCER_LIBRARIES_RELEASE}
		)
endif(BOUNCER_INCLUDE_DIR AND BOUNCER_LIBRARIES)



if(BOUNCER_INCLUDE_DIR AND BOUNCER_LIBRARIES)
	set(BOUNCERM_FOUND TRUE)
	message(STATUS "BOUNCER installation found.")
	message(STATUS "BOUNCER_INCLUDE_DIR: ${BOUNCER_INCLUDE_DIR}")
	message(STATUS "BOUNCER_LIBRARIES: ${BOUNCER_LIBRARIES}")
	
else(BOUNCER_INCLUDE_DIR AND BOUNCER_LIBRARIES)
#cannot find mongo anywhere!
	set(BOUNCERM_FOUND FALSE)
	message(STATUS "BOUNCER not found. Please set REPOBOUNCER_ROOT to your installation directory")
endif(BOUNCER_INCLUDE_DIR AND BOUNCER_LIBRARIES)
