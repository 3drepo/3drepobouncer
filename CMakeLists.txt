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

# Code to automatically generate CMakeList.txt in sub directories
# This project compiles a library with source files span across multiple directories
# So this generates a CMakeList.txt for each sub directory within the src folder
# update the SOURCES and HEADERS variable, and include their child directory

include(CMakeDependentOption)

cmake_minimum_required(VERSION 3.11)

project("3drepobouncer")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -pthread")

if(${APPLE})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -framework Foundation")
endif()

if(${WIN32})
	add_definitions(
		-DWIN32_LEAN_AND_MEAN
		-DNOMINMAX
	)

	# Tells Visual Studio to enable multithreaded/parallel compilation for all
	# projects on Windows.
	add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)

	# This flag tells MSVC to interpret everything as utf8 - the default behaviour
	# will sometimes interpret files as one of Windows' local encodings, which is
	# a problem because our source files define string literals that are meant to
	# be unicode.
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
endif()

option (REPO_BUILD_COVERAGE "Building with coverage report" OFF)
option (ODA_SUPPORT "If ODA library is present" OFF)
option (SYNCHRO_SUPPORT "If Synchro library is present" OFF)

option (LICENSE_CHECK  "Build with a license check" OFF)

if(REPO_BUILD_COVERAGE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 --coverage")
endif()

if (WIN32 AND ODA_SUPPORT)
	# OdaCommon.h will conditionally define _WIN32_WINNT, if not already.
	# This can cause conflicts with Boost, if two object files end up targetting
	# a different ABI as a result.
	add_definitions(
		-D_WIN32_WINNT=0x0A00
	)
endif()

#================BOOST SETTINGS=======================
set(Boost_NO_BOOST_CMAKE ON)
# focal currently uses cmake 3.16, which is too old to use newer Boost's CONFIGs.
# For now we can ignore the warnings as FindBoost still works, but we need to decide
# for subsequent versions whether to continue to use Boost (and fix this properly)
# or not and drop this entirely.
set(Boost_NO_WARN_NEW_VERSIONS ON)
add_definitions(-DBOOST_ALL_DYN_LINK)
add_definitions(-DBOOST_LOG_DYN_LINK)
add_definitions(-DBOOST_PROGRAM_OPTIONS_DYN_LINK)
add_definitions(-DBOOST_UUID_NO_SIMD)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

unset(Boost_INCLUDE_DIR CACHE)
unset(Boost_LIBRARY_DIRS CACHE)
if(WIN32)
	find_package(Boost REQUIRED COMPONENTS system thread chrono log log_setup filesystem program_options regex zlib bzip2 iostreams date_time locale)
else()
	find_package(Boost REQUIRED COMPONENTS system thread chrono log log_setup filesystem program_options regex iostreams date_time)
endif()
add_definitions(${Boost_LIB_DIAGNOSTIC_DEFINITIONS})
add_definitions(-DBOOST_OPTIONAL_USE_OLD_DEFINITION_OF_NONE)

if(NOT ${Boost_FOUND})
	message(FATAL_ERROR "Cannot find boost")
endif()

#================MONGO CXX DRIVER SETTINGS=======================
find_package(MongoCXXDriver REQUIRED)
if(NOT ${MONGO_CXX_DRIVER_FOUND})
	message(FATAL_ERROR "Cannot find mongo cxx driver")
endif()

#================ASSIMP SETTINGS=======================
find_package(ASSIMPM REQUIRED)
if(NOT ${ASSIMPM_FOUND})
	message(FATAL_ERROR "Cannot find assimp")
endif()

#===================== ODA ============================
if (ODA_SUPPORT)
	add_definitions(-D_TOOLKIT_IN_DLL_)
	add_definitions(-DODA_SUPPORT)
	find_package(ODA REQUIRED)
endif()

#===================== SYNCHRO ============================

if (SYNCHRO_SUPPORT)
	add_definitions(-DSYNCHRO_SUPPORT)
	find_package(SYNCHROREADER REQUIRED)
endif()



#===================== CRYPTOLENS ============================

function (ensureNotEmpty varName data)
	if(data STREQUAL "")
		message(FATAL_ERROR "${varName} cannot be empty when LICENSE_CHECK is set.")
	endif()

endfunction()

if (LICENSE_CHECK )
	find_package(CRYPTOLENS REQUIRED)
	set(LICENSE_RSA_PUB_KEY_MOD $ENV{LICENSE_RSA_PUB_KEY_MOD} CACHE STRING "Licensing public token modulus")
	set(LICENSE_RSA_PUB_KEY_EXP $ENV{LICENSE_RSA_PUB_KEY_EXP} CACHE STRING "Licensing public token exponent")
	set(LICENSE_AUTH_TOKEN $ENV{LICENSE_AUTH_TOKEN} CACHE STRING "Access token to communicate with licence server")
	set(LICENSE_PRODUCT_ID $ENV{LICENSE_PRODUCT_ID} CACHE STRING "The product ID assigned in the licensing server")
	set(LICENSE_TIMEOUT_SECONDS "30" CACHE STRING "Time after which an occupied seat is released")

	ensureNotEmpty(LICENSE_RSA_PUB_KEY_MOD "${LICENSE_RSA_PUB_KEY_MOD}")
	ensureNotEmpty(LICENSE_RSA_PUB_KEY_EXP "${LICENSE_RSA_PUB_KEY_EXP}")
	ensureNotEmpty(LICENSE_AUTH_TOKEN "${LICENSE_AUTH_TOKEN}")
	ensureNotEmpty(LICENSE_PRODUCT_ID "${LICENSE_PRODUCT_ID}")

	add_definitions(-DREPO_LICENSE_CHECK)
	add_definitions(-DLICENSE_RSA_PUB_KEY_MOD="${LICENSE_RSA_PUB_KEY_MOD}")
	add_definitions(-DLICENSE_RSA_PUB_KEY_EXP="${LICENSE_RSA_PUB_KEY_EXP}")
	add_definitions(-DLICENSE_AUTH_TOKEN="${LICENSE_AUTH_TOKEN}")
	add_definitions(-DLICENSE_PRODUCT_ID=${LICENSE_PRODUCT_ID})
	add_definitions(-DLICENSE_TIMEOUT_SECONDS=${LICENSE_TIMEOUT_SECONDS})
	
	# Cryptolens uses a third-party library, optional. This can be replaced
	# with std::optional on newer versions of C++. However the shared
	# library currently only exports symbols for the non-standard version.
	# Until this changes, the following definition tells the Cryptolens code
	# in bouncer to continue to link using the non-standard version.
	add_definitions(-Doptional_CONFIG_SELECT_OPTIONAL=optional_OPTIONAL_NONSTD)

endif()

#================COMPILE 3D REPO BOUNCER ==============
option (REPO_BUILD_CLIENT "If the Command Line Client is built in addition to the library" ON)
option (REPO_BUILD_TOOLS "If the Command Line Tool is built in addition to the library" OFF)
option (REPO_BUILD_TESTS "If the test suite for the core bouncer logic is built in addition to the library" OFF)
option (REPO_NO_GZIP "If zlib is not compiled into boost" OFF)
option (REPO_ASSET_GENERATOR_SUPPORT "If the AssetGenerator is present and compiled into the library" ON)
option (REPO_SVG_EXPORT_SUPPORT "If the customised Svg Exporter is present and compiled into the library" ON)

# Logging is implemented in a separate shared library so all dependencies can
# use the same instance.
add_subdirectory(log)

# Build IFCUtils
add_subdirectory(ifcUtils)
message("IFC Utils: " ${IFCUTILS_INCLUDE_DIR} ", " ${IFCUTILS_LIBRARIES})

if(REPO_NO_GZIP)
	add_definitions(-DREPO_BOOST_NO_GZIP)
endif()

# Include the asset generators. This should be called before the bouncer
# subdirectory is added, as that is the point the SOURCES variable is
# committed to the 3drepobouncer target.
if (REPO_ASSET_GENERATOR_SUPPORT)
	add_definitions(-DREPO_ASSET_GENERATOR_SUPPORT)
	add_subdirectory(submodules/asset_generator)
endif()

if (ODA_SUPPORT AND REPO_SVG_EXPORT_SUPPORT) # (The Svg exporter is an ODA extension so ODA is required)
	add_definitions(-DREPO_SVG_EXPORT_SUPPORT)
	add_subdirectory(submodules/svgexport)
endif()

#bouncer library
add_subdirectory(bouncer)

#client exe
if (REPO_BUILD_CLIENT)
	add_subdirectory(client)
endif()

# gtest
if(REPO_BUILD_TESTS)
	set(gtest_force_shared_crt ON CACHE BOOL "Build gtest as shared library" FORCE)
	add_subdirectory(submodules/googletest)
endif()

#tool exe
if (REPO_BUILD_TOOLS)
	add_subdirectory(tools)
endif()

#test exe
if (REPO_BUILD_TESTS)
	add_subdirectory(test)
endif()
