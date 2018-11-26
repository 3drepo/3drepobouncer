#  Copyright (C) 2018 3D Repo Ltd
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

#attempt to find AWS SDK library
#if AWS SDK is found, AWSSDK_FOUND is set to true
#AWSSDK_INCLUDE_DIR will point to the include folder of the installation
#AWSSDK_LIBRARIES will point to the libraries

SET(AWSSDK_LIB_NAMES
	aws-cpp-sdk-core aws-cpp-sdk-s3 aws-cpp-sdk-kms aws-cpp-sdk-s3-encryption aws-cpp-sdk-transfer
)

if(DEFINED ENV{AWSSDK_ROOT})
	set(AWSSDK_ROOT $ENV{AWSSDK_ROOT})
	message(STATUS "$AWSSDK_ROOT defined: ${AWSSDK_ROOT}")
	find_path(AWSSDK_INCLUDE_DIR aws/core aws/kms aws/s3 aws/s3-encryption aws/transfer
		${AWSSDK_ROOT}/include
		)
	foreach(libName ${AWSSDK_LIB_NAMES})
		find_library(libPathRelease${libName} NAMES ${libName}
			PATHS
			${AWSSDK_ROOT}/lib
			${AWSSDK_LIB_DIR}
			)
		set(AWSSDK_LIBRARIES_RELEASE ${AWSSDK_LIBRARIES_RELEASE} ${libPathRelease${libName}})
	endforeach()

	set(AWSSDK_LIBRARIES
		debug ${AWSSDK_LIBRARIES_RELEASE}
		optimized ${AWSSDK_LIBRARIES_RELEASE}
		)
endif()

if(AWSSDK_INCLUDE_DIR AND AWSSDK_LIBRARIES)
	set(AWSSDK_FOUND TRUE)

else(AWSSDK_INCLUDE_DIR AND AWSSDK_LIBRARIES)
	find_path(AWSSDK_INCLUDE_DIR aws
		/usr/include
		/usr/local/include
		/opt/local/include
    )

	foreach(libName ${AWSSDK_LIB_NAMES})
		find_library(libPathRelease${libName} NAMES ${libName}
			PATHS
		/usr/lib/
		/usr/local/lib/
		/opt/local/lib/
		)
		set(AWSSDK_LIBRARIES_RELEASE ${AWSSDK_LIBRARIES_RELEASE} ${libPathRelease${libName}})
	endforeach()
	
	set(AWSSDK_LIBRARIES
		debug ${AWSSDK_LIBRARIES_RELEASE}
		optimized ${AWSSDK_LIBRARIES_RELEASE}
		)
endif(AWSSDK_INCLUDE_DIR AND AWSSDK_LIBRARIES)



if(AWSSDK_INCLUDE_DIR AND AWSSDK_LIBRARIES)
	set(AWSSDK_FOUND TRUE)
	message(STATUS "AWSSDK installation found.")
	message(STATUS "AWSSDK_INCLUDE_DIR: ${AWSSDK_INCLUDE_DIR}")
	message(STATUS "AWSSDK_LIBRARIES: ${AWSSDK_LIBRARIES}")
	
else(AWSSDK_INCLUDE_DIR AND AWSSDK_LIBRARIES)
	set(AWSSDK_FOUND FALSE)
	message(STATUS "AWSSDK not found. Please set AWSSDK_ROOT to your installation directory")
endif(AWSSDK_INCLUDE_DIR AND AWSSDK_LIBRARIES)
