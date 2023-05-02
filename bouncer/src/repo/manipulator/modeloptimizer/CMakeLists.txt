#THIS IS AN AUTOMATICALLY GENERATED FILE - DO NOT OVERWRITE THE CONTENT!
#If you need to update the sources/headers/sub directory information, run updateSources.py at project root level
#If you need to import an extra library or something clever, do it on the CMakeLists.txt at the root level
#If you really need to overwrite this file, be aware that it will be overwritten if updateSources.py is executed.

include_directories(${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(Eigen)
add_subdirectory(bvh)
add_subdirectory(pmp)
set(SOURCES
	${SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/repo_optimizer_abstract.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_optimizer_mesh_simplification.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_optimizer_multipart.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_optimizer_trans_reduction.cpp
	CACHE STRING "SOURCES" FORCE)

set(HEADERS
	${HEADERS}
	${CMAKE_CURRENT_SOURCE_DIR}/repo_optimizer_abstract.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_optimizer_mesh_simplification.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_optimizer_multipart.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_optimizer_trans_reduction.h
	CACHE STRING "HEADERS" FORCE)

