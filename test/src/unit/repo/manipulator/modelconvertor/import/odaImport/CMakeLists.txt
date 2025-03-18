#THIS IS AN AUTOMATICALLY GENERATED FILE - DO NOT OVERWRITE THE CONTENT!
#If you need to update the sources/headers/sub directory information, run updateSources.py at project root level
#If you need to import an extra library or something clever, do it on the CMakeLists.txt at the root level
#If you really need to overwrite this file, be aware that it will be overwritten if updateSources.py is executed.


if(ODA_SUPPORT)
	set(TEST_SOURCES
		${TEST_SOURCES}
		${CMAKE_CURRENT_SOURCE_DIR}/ut_repo_metadata_variant_nwd.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/ut_repo_metadata_variant_rvt.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/ut_repo_model_import_oda.cpp
		CACHE STRING "TEST_SOURCES" FORCE)

endif()