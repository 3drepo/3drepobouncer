#THIS IS AN AUTOMATICALLY GENERATED FILE - DO NOT OVERWRITE THE CONTENT!
#If you need to update the sources/headers/sub directory information, run updateSources.py at project root level
#If you need to import an extra library or something clever, do it on the CMakeLists.txt at the root level
#If you really need to overwrite this file, be aware that it will be overwritten if updateSources.py is executed.


add_subdirectory(odaImport)
set(TEST_SOURCES
	${TEST_SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/ut_repo_drawing_import_manager.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ut_repo_metadata_variant_assimp.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ut_repo_model_import_3drepo.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ut_repo_model_import_assimp.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ut_repo_model_import_ifc.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ut_repo_model_import_synchro.cpp
	CACHE STRING "TEST_SOURCES" FORCE)

