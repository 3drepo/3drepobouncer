#THIS IS AN AUTOMATICALLY GENERATED FILE - DO NOT OVERWRITE THE CONTENT!
#If you need to update the sources/headers/sub directory information, run updateSources.py at project root level
#If you need to import an extra library or something clever, do it on the CMakeLists.txt at the root level
#If you really need to overwrite this file, be aware that it will be overwritten if updateSources.py is executed.


add_subdirectory(spatialpartitioning)
set(SOURCES
	${SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/repo_maker_selection_tree.cpp
	CACHE STRING "SOURCES" FORCE)

set(HEADERS
	${HEADERS}
	${CMAKE_CURRENT_SOURCE_DIR}/repo_maker_selection_tree.h
	CACHE STRING "HEADERS" FORCE)

