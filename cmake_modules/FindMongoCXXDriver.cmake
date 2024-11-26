#  Copyright (C) 2024 3D Repo Ltd
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
#if MONGO CXX Driver is found, MONGO_CXX_DRIVER_FOUND is set to true
#MONGO_CXX_DRIVER_MONGO_INCLUDE_DIR will point to the include folder of the installation
#MONGO_CXX_DRIVER_BSON_INCLUDE_DIR will point to the include folder of the bson installation
#MONGO_CXX_DRIVER_LIBRARIES will point to the libraries


if(DEFINED ENV{MONGO_ROOT})
	set(MONGO_CXX_DRIVER_ROOT $ENV{MONGO_ROOT})
	message(STATUS "$MONGO_CXX_DRIVER_ROOT defined: ${MONGO_CXX_DRIVER_ROOT}")
	find_path(MONGO_CXX_DRIVER_MONGO_INCLUDE_DIR mongocxx/instance.hpp
		${MONGO_CXX_DRIVER_ROOT}/include/mongocxx/v_noabi
	)
	
	find_path(MONGO_CXX_DRIVER_BSON_INCLUDE_DIR bsoncxx/types.hpp
		${MONGO_CXX_DRIVER_ROOT}/include/bsoncxx/v_noabi
	)
	
	find_library(MONGO_CXX_DRIVER
		NAMES
			mongocxx
		PATHS
			${MONGO_CXX_DRIVER_ROOT}/lib
	)

	find_library(MONGO_CXX_BSON
		NAMES
			bsoncxx
		PATHS
			${MONGO_CXX_DRIVER_ROOT}/lib
	)

	set(MONGO_CXX_DRIVER_LIBRARIES
		${MONGO_CXX_DRIVER}
		${MONGO_CXX_BSON}
	)
endif()

if(MONGO_CXX_DRIVER_MONGO_INCLUDE_DIR AND MONGO_CXX_DRIVER_BSON_INCLUDE_DIR AND MONGO_CXX_DRIVER_LIBRARIES)
	set(MONGO_CXX_DRIVER_FOUND TRUE)

else(MONGO_CXX_DRIVER_MONGO_INCLUDE_DIR AND MONGO_CXX_DRIVER_BSON_INCLUDE_DIR AND MONGO_CXX_DRIVER_LIBRARIES)
	find_path(MONGO_CXX_DRIVER_MONGO_INCLUDE_DIR mongocxx/v_noabi/mongocxx/instance.hpp
		/usr/include
		/usr/local/include
		/opt/local/include
    	)

	find_path(MONGO_CXX_DRIVER_MONGO_INCLUDE_DIR bsoncxx/v_noabi/bsoncxx/types.hpp
		/usr/include
		/usr/local/include
		/opt/local/include
	)

	find_library(MONGO_CXX_DRIVER
		NAMES
			mongocxx
    		PATHS
			/usr/lib
			/usr/local/lib
			/opt/local/lib
			/usr/lib64
	)

	find_library(MONGO_CXX_BSON
		NAMES
			bsoncxx
		PATHS
			/usr/lib
			/usr/local/lib
			/opt/local/lib
			/usr/lib64
    	)

	set(MONGO_CXX_DRIVER_LIBRARIES
		${MONGO_CXX_DRIVER}
		${MONGO_CXX_BSON}
	)
endif(MONGO_CXX_DRIVER_MONGO_INCLUDE_DIR AND MONGO_CXX_DRIVER_BSON_INCLUDE_DIR AND MONGO_CXX_DRIVER_LIBRARIES)


if(MONGO_CXX_DRIVER_MONGO_INCLUDE_DIR AND MONGO_CXX_DRIVER_BSON_INCLUDE_DIR AND MONGO_CXX_DRIVER_LIBRARIES)
	set(MONGO_CXX_DRIVER_FOUND TRUE)
	message(STATUS "MONGO CXX DRIVER installation found.")
	message(STATUS "MONGO_CXX_DRIVER_MONGO_INCLUDE_DIR: ${MONGO_CXX_DRIVER_MONGO_INCLUDE_DIR}")
	message(STATUS "MONGO_CXX_DRIVER_BSON_INCLUDE_DIR: ${MONGO_CXX_DRIVER_BSON_INCLUDE_DIR}")
	message(STATUS "MONGO_CXX_DRIVER_LIBRARIES: ${MONGO_CXX_DRIVER_LIBRARIES}")

else(MONGO_CXX_DRIVER_MONGO_INCLUDE_DIR AND MONGO_CXX_DRIVER_BSON_INCLUDE_DIR AND MONGO_CXX_DRIVER_LIBRARIES)
#cannot find mongo anywhere!
	set(MONGO_CXX_DRIVER_FOUND FALSE)
	message(STATUS "MONGO CXX DRIVER not found. Please set MONGO_ROOT to your installation directory containing both C and CXX drivers")
endif(MONGO_CXX_DRIVER_MONGO_INCLUDE_DIR AND MONGO_CXX_DRIVER_BSON_INCLUDE_DIR AND MONGO_CXX_DRIVER_LIBRARIES)
