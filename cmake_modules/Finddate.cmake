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

if(DEFINED ENV{DATE_ROOT})
	set(DATE_ROOT $ENV{DATE_ROOT})
	message(STATUS "$DATE_ROOT defined: ${DATE_ROOT}")
	find_path(DATE_INCLUDE_DIR date/date.h
		${DATE_ROOT}/include
	)  
	find_library(DATE_LIBRARIES_RELEASE 
		NAMES date-tz
		PATHS
		${DATE_ROOT}/lib/Release
		${DATE_ROOT}/lib
	)	
	find_library(DATE_LIBRARIES_DEBUG 
		NAMES date-tz
		PATHS
		${DATE_ROOT}/lib/Debug
		${DATE_ROOT}/lib
	)
	set(DATE_LIBRARIES
		debug ${DATE_LIBRARIES_DEBUG}
		optimized ${DATE_LIBRARIES_RELEASE}
		)
endif()

# attempt to find DATE C++ driver via common install dirs
if(DATE_INCLUDE_DIR AND DATE_LIBRARIES)
	set(DATE_FOUND TRUE)
else(DATE_INCLUDE_DIR AND DATE_LIBRARIES)
	find_path(DATE_INCLUDE_DIR DATE/core.hpp
		/usr/include
		/usr/local/include
		/opt/local/include
    )
	find_library(DATE_LIBRARIES_RELEASE NAMES DATE
    	PATHS
    	/usr/lib
    	/usr/local/lib
    	/opt/local/lib
		/usr/lib64
    )
	set(DATE_LIBRARIES
		debug ${DATE_LIBRARIES_RELEASE}
		optimized ${DATE_LIBRARIES_RELEASE}
		)
endif(DATE_INCLUDE_DIR AND DATE_LIBRARIES)

# exit if we found libs/includes
if(DATE_INCLUDE_DIR AND DATE_LIBRARIES)
	set(DATE_FOUND TRUE)
	message(STATUS "DATE installation found.")
	message(STATUS "DATE_INCLUDE_DIR: ${DATE_INCLUDE_DIR}")
	message(STATUS "DATE_LIBRARIES: ${DATE_LIBRARIES}")
# cannot find DATE anywhere!
else(DATE_INCLUDE_DIR AND DATE_LIBRARIES)
	set(DATE_FOUND FALSE)
	message(STATUS "DATE not found. Please set DATE_ROOT to your installation directory")
endif(DATE_INCLUDE_DIR AND DATE_LIBRARIES)
