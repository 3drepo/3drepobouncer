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

#attempt to find Assimp library 
#if Assimp is found, Assimp_FOUND is set to true
#Assimp_INCLUDE_DIR will point to the include folder of the installation
#Assimp_LIBRARIES will point to the libraries


if(DEFINED ENV{ASSIMP_ROOT})
	set(Assimp_ROOT $ENV{ASSIMP_ROOT})
	message(STATUS "$ASSIMP_ROOT defined: ${Assimp_ROOT}")
	find_path(Assimp_INCLUDE_DIR assimp
		${Assimp_ROOT}/include
		)    
	find_library(Assimp_LIBRARIES_RELEASE NAMES assimp
		PATHS
		${Assimp_ROOT}/lib
	)	
	find_library(Assimp_LIBRARIES_DEBUG NAMES assimpd
		PATHS
		${Assimp_ROOT}/lib
	)	
	set(Assimp_LIBRARIES
		debug ${Assimp_LIBRARIES_DEBUG}
		optimized ${Assimp_LIBRARIES_RELEASE}
		)
endif()

if(Assimp_INCLUDE_DIR AND Assimp_LIBRARIES)
	set(Assimp_FOUND TRUE)

else(Assimp_INCLUDE_DIR AND Assimp_LIBRARIES)
	find_path(ASSIMP_INCLUDE_DIR assimp
		/usr/include
		/usr/local/include
		/opt/local/include
    )

	find_library(Assimp_LIBRARIES_RELEASE NAMES assimp 
    	PATHS
    	/usr/lib
    	/usr/local/lib
    	/opt/local/lib
    )
	
	find_library(Assimp_LIBRARIES_DEBUG NAMES assimpd 
    	PATHS
    	/usr/lib
    	/usr/local/lib
    	/opt/local/lib
    )

	set(Assimp_LIBRARIES
		debug ${Assimp_LIBRARIES_DEBUG}
		optimized ${Assimp_LIBRARIES_RELEASE}
		)
endif(Assimp_INCLUDE_DIR AND Assimp_LIBRARIES)



if(Assimp_INCLUDE_DIR AND Assimp_LIBRARIES)
	set(Assimp_FOUND TRUE)
	message(STATUS "Assimp installation found.")
	message(STATUS "Assimp_INCLUDE_DIR: ${Assimp_INCLUDE_DIR}")
	message(STATUS "Assimp_LIBRARIES: ${Assimp_LIBRARIES}")
	
else(Assimp_INCLUDE_DIR AND Assimp_LIBRARIES)
#cannot find mongo anywhere!
	set(Assimp_FOUND FALSE)
	message(STATUS "Assimp not found. Please set ASSIMP_ROOT to your installation directory")
endif(Assimp_INCLUDE_DIR AND Assimp_LIBRARIES)
