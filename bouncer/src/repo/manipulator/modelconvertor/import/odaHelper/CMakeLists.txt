#THIS IS AN AUTOMATICALLY GENERATED FILE - DO NOT OVERWRITE THE CONTENT!
#If you need to update the sources/headers/sub directory information, run updateSources.py at project root level
#If you need to import an extra library or something clever, do it on the CMakeLists.txt at the root level
#If you really need to overwrite this file, be aware that it will be overwritten if updateSources.py is executed.


if(ODA_SUPPORT)
	set(SOURCES
		${SOURCES}
		${CMAKE_CURRENT_SOURCE_DIR}/data_processor.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/data_processor_dgn.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/data_processor_dwg.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/data_processor_nwd.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/data_processor_rvt.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/file_processor.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/file_processor_dgn.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/file_processor_dwg.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/file_processor_nwd.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/file_processor_rvt.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/geometry_collector.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/helper_functions.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/repo_mesh_builder.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/repo_system_services.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/vectorise_device_rvt.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/vertex_map.cpp
		CACHE STRING "SOURCES" FORCE)

	set(HEADERS
		${HEADERS}
		${CMAKE_CURRENT_SOURCE_DIR}/data_processor.h
		${CMAKE_CURRENT_SOURCE_DIR}/data_processor_dgn.h
		${CMAKE_CURRENT_SOURCE_DIR}/data_processor_dwg.h
		${CMAKE_CURRENT_SOURCE_DIR}/data_processor_nwd.h
		${CMAKE_CURRENT_SOURCE_DIR}/data_processor_rvt.h
		${CMAKE_CURRENT_SOURCE_DIR}/file_processor.h
		${CMAKE_CURRENT_SOURCE_DIR}/file_processor_dgn.h
		${CMAKE_CURRENT_SOURCE_DIR}/file_processor_dwg.h
		${CMAKE_CURRENT_SOURCE_DIR}/file_processor_nwd.h
		${CMAKE_CURRENT_SOURCE_DIR}/file_processor_rvt.h
		${CMAKE_CURRENT_SOURCE_DIR}/geometry_collector.h
		${CMAKE_CURRENT_SOURCE_DIR}/helper_functions.h
		${CMAKE_CURRENT_SOURCE_DIR}/repo_mesh_builder.h
		${CMAKE_CURRENT_SOURCE_DIR}/repo_system_services.h
		${CMAKE_CURRENT_SOURCE_DIR}/vectorise_device_dgn.h
		${CMAKE_CURRENT_SOURCE_DIR}/vectorise_device_rvt.h
		${CMAKE_CURRENT_SOURCE_DIR}/vertex_map.h
		CACHE STRING "HEADERS" FORCE)

endif()