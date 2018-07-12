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

project("3drepobouncer")

cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")


option (REPO_BUILD_COVERAGE "Building with coverage report" OFF)
option (ODA_SUPPORT "If ODA library is present" OFF)
if(REPO_BUILD_COVERAGE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 --coverage")
endif()

#================BOOST SETTINGS=======================
#add_definitions(-DBOOST_ALL_NO_LIB)
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
	find_package(Boost REQUIRED COMPONENTS system thread chrono log log_setup filesystem program_options regex zlib bzip2 iostreams date_time)
else()
	find_package(Boost REQUIRED COMPONENTS system thread chrono log log_setup filesystem program_options regex iostreams date_time)
endif()
add_definitions(${Boost_LIB_DIAGNOSTIC_DEFINITIONS})
add_definitions(-DBOOST_OPTIONAL_USE_OLD_DEFINITION_OF_NONE)

if(NOT ${Boost_FOUND})
	message(FATAL_ERROR "Cannot find boost")
endif()


#================MONGO SETTINGS=======================
find_package(Mongo REQUIRED)
if(NOT ${MONGO_FOUND})
	message(FATAL_ERROR "Cannot find mongo cxx driver")
endif()


#================ASSIMP SETTINGS=======================
find_package(ASSIMPM REQUIRED)
if(NOT ${ASSIMPM_FOUND})
	message(FATAL_ERROR "Cannot find assimp")
endif()

#===============IFCOPENSHELL SETTINGS==================
find_package(OCCT REQUIRED)
find_package(IFCOPENSHELL REQUIRED)
if(NOT ${OCCT_FOUND})
	message(FATAL_ERROR "Cannot find OCCT")
endif()
if(NOT ${IFCOPENSHELL_FOUND})
	message(FATAL_ERROR "Cannot find IFCOpenShell")
endif()

#===================== ODA ============================
if (ODA_SUPPORT)
	add_definitions(-D_TOOLKIT_IN_DLL_)
	add_definitions(-DODA_SUPPORT)
	find_package(ODA REQUIRED)
endif()

#================COMPILE 3D REPO BOUNCER ==============
option (REPO_BUILD_CLIENT "If the Command Line Client is built in addition to the library" ON)
option (REPO_BUILD_TOOLS "If the Command Line Tool is built in addition to the library" ON)
option (REPO_BUILD_TESTS "If the test suite is built in addition to the library" OFF)
option (REPO_NO_GZIP "If zlib is not compiled into boost" OFF)
add_definitions( -DWIN32_LEAN_AND_MEAN )

if(REPO_NO_GZIP)
	add_definitions(-DREPO_BOOST_NO_GZIP)
endif()

#bouncer library
add_subdirectory(bouncer)


#client exe
if (REPO_BUILD_CLIENT)
	add_subdirectory(client)
endif()

#tool exe
if (REPO_BUILD_TOOLS)
	add_subdirectory(tools)
endif()

#test exe
if (REPO_BUILD_TESTS)
	set(gtest_force_shared_crt ON CACHE BOOL "Build gtest as shared library" FORCE)
	add_subdirectory(submodules/googletest)
	add_subdirectory(test)
endif()

