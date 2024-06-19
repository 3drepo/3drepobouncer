#THIS IS AN AUTOMATICALLY GENERATED FILE - DO NOT OVERWRITE THE CONTENT!
#If you need to update the sources/headers/sub directory information, run updateSources.py at project root level
#If you need to import an extra library or something clever, do it on the CMakeLists.txt at the root level
#If you really need to overwrite this file, be aware that it will be overwritten if updateSources.py is executed.


add_subdirectory(spatialpartitioning)
set(SOURCES
	${SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/repo_drawing.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_drawing_manager.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_maker_selection_tree.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_mesh_map_reorganiser.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_scene_manager.cpp
	CACHE STRING "SOURCES" FORCE)

set(HEADERS
	${HEADERS}
	${CMAKE_CURRENT_SOURCE_DIR}/repo_drawing.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_drawing_manager.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_maker_selection_tree.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_mesh_map_reorganiser.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_scene_manager.h
	CACHE STRING "HEADERS" FORCE)

