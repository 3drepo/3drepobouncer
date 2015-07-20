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

#attempt to find Mongo C++ driver 
#if Mongo is found, Mongo_FOUND is set to true
#Mongo_INCLUDE_DIR will point to the include folder of the installation
#Mongo_LIBRARIES will point to the libraries


if(DEFINED ENV{MONGO_ROOT})
	set(Mongo_ROOT $ENV{MONGO_ROOT})
	message(STATUS "$MONGO_ROOT defined: ${Mongo_ROOT}")
	find_path(Mongo_INCLUDE_DIR mongo/client/dbclient.h
		${Mongo_ROOT}/include
		)    
	find_library(Mongo_LIBRARIES_RELEASE NAMES mongoclient
		PATHS
		${Mongo_ROOT}/lib
	)	
	
	find_library(Mongo_LIBRARIES_DEBUG NAMES mongoclient-gd
		PATHS
		${Mongo_ROOT}/lib
	)

	set(Mongo_LIBRARIES
		debug ${Mongo_LIBRARIES_DEBUG}
		optimized ${Mongo_LIBRARIES_RELEASE}
		)
endif()

if(Mongo_INCLUDE_DIR AND Mongo_LIBRARIES)
	set(Mongo_FOUND TRUE)

else(Mongo_INCLUDE_DIR AND Mongo_LIBRARIES)
	find_path(MONGO_INCLUDE_DIR mongo/client/dbclient.h
		/usr/include
		/usr/local/include
		/opt/local/include
    )

	find_library(Mongo_LIBRARIES_RELEASE NAMES mongoclient
    	PATHS
    	/usr/lib
    	/usr/local/lib
    	/opt/local/lib
    )

	find_library(Mongo_LIBRARIES_DEBUG NAMES mongoclient-gd
    	PATHS
    	/usr/lib
    	/usr/local/lib
    	/opt/local/lib
    )

	set(Mongo_LIBRARIES
		debug ${Mongo_LIBRARIES_DEBUG}
		optimized ${Mongo_LIBRARIES_RELEASE}
		)
endif(Mongo_INCLUDE_DIR AND Mongo_LIBRARIES)



if(Mongo_INCLUDE_DIR AND Mongo_LIBRARIES)
	set(Mongo_FOUND TRUE)
	message(STATUS "Mongo installation found.")
	message(STATUS "MONGO_INCLUDE_DIR: ${Mongo_INCLUDE_DIR}")
	message(STATUS "Mongo_LIBRARIES: ${Mongo_LIBRARIES}")
	
else(Mongo_INCLUDE_DIR AND Mongo_LIBRARIES)
#cannot find mongo anywhere!
	set(Mongo_FOUND FALSE)
	message(STATUS "Mongo not found. Please set MONGO_ROOT to your installation directory")
endif(Mongo_INCLUDE_DIR AND Mongo_LIBRARIES)
