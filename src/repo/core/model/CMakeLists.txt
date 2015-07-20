add_subdirectory(bson)
set(HEADERS
	${HEADERS}
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_global.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_node_properties.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_node_utils.h
	CACHE STRING "header files" FORCE)

