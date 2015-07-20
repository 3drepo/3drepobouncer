#THIS IS AN AUTOMATICALLY GENERATED FILE - DO NOT OVERWRITE THE CONTENT!
#If you need to update the sources/headers/sub directory information, run updateSources.py at project root level
#If you need to import an extra library or something clever, do it on the CMakeLists.txt at the root level
#If you really need to overwrite this file, be aware that it will be overwritten if updateSources.py is executed.


add_subdirectory(bson)
set(HEADERS
	${HEADERS}
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_global.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_node_properties.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_node_utils.h
	CACHE STRING "header files" FORCE)

