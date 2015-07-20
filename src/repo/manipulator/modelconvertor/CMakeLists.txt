set(SOURCES
	${SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_convertor_abstract.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_convertor_assimp.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_convertor_config.cpp
	CACHE STRING "source files" FORCE)

set(HEADERS
	${HEADERS}
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_convertor_abstract.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_convertor_assimp.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_convertor_config.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_convertor_config_default_values.h
	CACHE STRING "header files" FORCE)

