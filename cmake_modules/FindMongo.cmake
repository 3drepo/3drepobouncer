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

#attempt to find MONGO C++ driver 
#if MONGO is found, MONGO_FOUND is set to true
#MONGO_INCLUDE_DIR will point to the include folder of the installation
#MONGO_LIBRARIES will point to the libraries


if(DEFINED ENV{MONGO_ROOT})
	set(MONGO_ROOT $ENV{MONGO_ROOT})
	message(STATUS "$MONGO_ROOT defined: ${MONGO_ROOT}")
	find_path(MONGO_INCLUDE_DIR mongo/client/dbclient.h
		${MONGO_ROOT}/include
		)    
	find_library(MONGO_LIBRARIES_RELEASE NAMES mongoclient
		PATHS
		${MONGO_ROOT}/lib
	)	
	
#	find_library(MONGO_LIBRARIES_DEBUG NAMES mongoclient-gd
#		PATHS
#		${MONGO_ROOT}/lib
#	)

	set(MONGO_LIBRARIES
		#		debug ${MONGO_LIBRARIES_DEBUG}
		optimized ${MONGO_LIBRARIES_RELEASE}
		)
endif()

if(MONGO_INCLUDE_DIR AND MONGO_LIBRARIES)
	set(MONGO_FOUND TRUE)

else(MONGO_INCLUDE_DIR AND MONGO_LIBRARIES)
	find_path(MONGO_INCLUDE_DIR mongo/client/dbclient.h
		/usr/include
		/usr/local/include
		/opt/local/include
    )

	find_library(MONGO_LIBRARIES_RELEASE NAMES mongoclient
    	PATHS
    	/usr/lib
    	/usr/local/lib
    	/opt/local/lib
		/usr/lib64
    )

	set(MONGO_LIBRARIES
		debug ${MONGO_LIBRARIES_RELEASE}
		optimized ${MONGO_LIBRARIES_RELEASE}
		)
endif(MONGO_INCLUDE_DIR AND MONGO_LIBRARIES)



if(MONGO_INCLUDE_DIR AND MONGO_LIBRARIES)
	set(MONGO_FOUND TRUE)
	message(STATUS "MONGO installation found.")
	message(STATUS "MONGO_INCLUDE_DIR: ${MONGO_INCLUDE_DIR}")
	message(STATUS "MONGO_LIBRARIES: ${MONGO_LIBRARIES}")
	
else(MONGO_INCLUDE_DIR AND MONGO_LIBRARIES)
#cannot find mongo anywhere!
	set(MONGO_FOUND FALSE)
	message(STATUS "MONGO not found. Please set MONGO_ROOT to your installation directory")
endif(MONGO_INCLUDE_DIR AND MONGO_LIBRARIES)
