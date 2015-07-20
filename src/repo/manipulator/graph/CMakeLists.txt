set(SOURCES
	${SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/repo_graph_abstract.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_graph_revision.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_scene.cpp
	CACHE STRING "source files" FORCE)

set(HEADERS
	${HEADERS}
	${CMAKE_CURRENT_SOURCE_DIR}/repo_graph_abstract.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_graph_revision.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_scene.h
	CACHE STRING "header files" FORCE)

