#THIS IS AN AUTOMATICALLY GENERATED FILE - DO NOT OVERWRITE THE CONTENT!
#If you need to update the sources/headers/sub directory information, run updateSources.py at project root level
#If you need to import an extra library or something clever, do it on the CMakeLists.txt at the root level
#If you really need to overwrite this file, be aware that it will be overwritten if updateSources.py is executed.


add_subdirectory(repo)
set(TEST_SOURCES
	${TEST_SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/repo_test_mesh_utils.cpp
	CACHE STRING "TEST_SOURCES" FORCE)

set(TEST_HEADERS
	${TEST_HEADERS}
	${CMAKE_CURRENT_SOURCE_DIR}/repo_test_database_info.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_test_fileservice_info.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_test_mesh_utils.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_test_utils.h
	CACHE STRING "TEST_HEADERS" FORCE)

