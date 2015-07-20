add_subdirectory(connectionpool)
set(SOURCES
	${SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/repo_database_handler_abstract.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_database_handler_mongo.cpp
	CACHE STRING "source files" FORCE)

set(HEADERS
	${HEADERS}
	${CMAKE_CURRENT_SOURCE_DIR}/repo_database_handler_abstract.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_database_handler_mongo.h
	CACHE STRING "header files" FORCE)

