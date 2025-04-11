#THIS IS AN AUTOMATICALLY GENERATED FILE - DO NOT OVERWRITE THE CONTENT!
#If you need to update the sources/headers/sub directory information, run updateSources.py at project root level
#If you need to import an extra library or something clever, do it on the CMakeLists.txt at the root level
#If you really need to overwrite this file, be aware that it will be overwritten if updateSources.py is executed.


add_subdirectory(odaHelper)
add_subdirectory(rapidjson)
set(SOURCES
	${SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/repo_drawing_import_manager.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_metadata_import_csv.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_import_3drepo.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_import_abstract.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_import_assimp.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_import_ifc.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_import_manager.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_import_oda.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_import_synchro.cpp
	CACHE STRING "SOURCES" FORCE)

set(HEADERS
	${HEADERS}
	${CMAKE_CURRENT_SOURCE_DIR}/repo_drawing_import_manager.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_metadata_import_csv.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_import_3drepo.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_import_abstract.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_import_assimp.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_import_config.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_import_config_default_values.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_import_ifc.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_import_manager.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_import_oda.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_import_synchro.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_model_units.h
	CACHE STRING "HEADERS" FORCE)

