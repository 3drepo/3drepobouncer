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

#attempt to find ASSIMP library (renamed to avoid clashing with the assimp default one)
#if ASSIMP is found, ASSIMP_FOUND is set to true
#ASSIMP_INCLUDE_DIR will point to the include folder of the installation
#ASSIMP_LIBRARIES will point to the libraries


if(DEFINED ENV{ASSIMP_ROOT})
	set(ASSIMP_ROOT $ENV{ASSIMP_ROOT})
	message(STATUS "$ASSIMP_ROOT defined: ${ASSIMP_ROOT}")
	find_path(ASSIMP_INCLUDE_DIR assimp
		${ASSIMP_ROOT}/include
	)
	find_library(ASSIMP_LIBRARIES
		NAMES
			assimp-vc140-mt
			assimp-vc142-mt
			assimp-vc143-mt
			assimp
		PATHS
			${ASSIMP_ROOT}/lib
	)
else()
	find_path(ASSIMP_INCLUDE_DIR assimp
		/usr/include
		/usr/local/include
		/opt/local/include
    )

	find_library(ASSIMP_LIBRARIES
		NAMES
			assimp
    	PATHS
			/usr/lib/
			/usr/local/lib/
			/opt/local/lib/
    )

endif()


if(ASSIMP_INCLUDE_DIR AND ASSIMP_LIBRARIES)
	set(ASSIMPM_FOUND TRUE)
	message(STATUS "ASSIMP installation found.")
	message(STATUS "ASSIMP_INCLUDE_DIR: ${ASSIMP_INCLUDE_DIR}")
	message(STATUS "ASSIMP_LIBRARIES: ${ASSIMP_LIBRARIES}")
else()
	set(ASSIMPM_FOUND FALSE)
	message(STATUS "ASSIMP not found. Please set ASSIMP_ROOT to your installation directory or install via a suitable package manager.")
endif()
