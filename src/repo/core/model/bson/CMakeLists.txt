set(SOURCES
	${SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/repo_bson.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_bson_builder.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_bson_factory.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_bson_project_settings.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_node.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_node_camera.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_node_map.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_node_material.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_node_mesh.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_node_metadata.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_node_reference.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_node_revision.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_node_texture.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_node_transformation.cpp
	CACHE STRING "source files" FORCE)

set(HEADERS
	${HEADERS}
	${CMAKE_CURRENT_SOURCE_DIR}/repo_bson.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_bson_builder.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_bson_factory.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_bson_project_settings.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_node.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_node_camera.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_node_map.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_node_material.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_node_mesh.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_node_metadata.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_node_reference.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_node_revision.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_node_texture.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_node_transformation.h
	CACHE STRING "header files" FORCE)

