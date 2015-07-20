project("testCmake")

cmake_minimum_required(VERSION 3.3)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)

#================BOOST SETTINGS=======================
add_definitions(-DBOOST_ALL_NO_LIB)
add_definitions(-DBOOST_LOG_DYN_LINK)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

unset(Boost_INCLUDE_DIR CACHE)
unset(Boost_LIBRARY_DIRS CACHE)

find_package(Boost REQUIRED COMPONENTS system thread chrono log filesystem)


#================Mongo SETTINGS=======================
find_package(Mongo REQUIRED)


#================ASSIMP SETTINGS=======================
find_package(Assimp REQUIRED)

#================Source and header files===============
add_subdirectory(src)

if(${Boost_FOUND} AND ${Mongo_FOUND} AND ${Assimp_FOUND})
	include_directories(${Boost_INCLUDE_DIRS} ${Mongo_INCLUDE_DIR} ${Assimp_INCLUDE_DIR} ${HEADERS})
	add_definitions(${Boost_LIB_DIAGNOSTIC_DEFINITIONS})
	

	add_executable(bouncerTest ${SOURCES})

	target_link_libraries(bouncerTest ${Boost_LIBRARIES} ${Mongo_LIBRARIES} ${Assimp_LIBRARIES})

endif()
