#THIS IS AN AUTOMATICALLY GENERATED FILE - DO NOT OVERWRITE THE CONTENT!
#If you need to update the sources/headers/sub directory information, run updateSources.py at project root level
#If you need to import an extra library or something clever, do it on the CMakeLists.txt at the root level
#If you really need to overwrite this file, be aware that it will be overwritten if updateSources.py is executed.


add_subdirectory(clashdetection)
add_subdirectory(rapidjson)
add_subdirectory(spatialpartitioning)
add_subdirectory(spscqueue)
set(SOURCES
	${SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/repo_clash_detection_config.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_clash_detection_engine.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_drawing.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_drawing_manager.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_maker_selection_tree.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_mesh_map_reorganiser.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_scene_builder.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_scene_manager.cpp
	CACHE STRING "SOURCES" FORCE)

set(HEADERS
	${HEADERS}
	${CMAKE_CURRENT_SOURCE_DIR}/repo_clash_detection_config.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_clash_detection_config_fwd.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_clash_detection_engine.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_drawing.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_drawing_manager.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_json_parser.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_maker_selection_tree.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_mesh_map_reorganiser.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_scene_builder.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_scene_manager.h
	CACHE STRING "HEADERS" FORCE)

