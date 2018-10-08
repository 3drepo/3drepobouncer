#THIS IS AN AUTOMATICALLY GENERATED FILE - DO NOT OVERWRITE THE CONTENT!
#If you need to update the sources/headers/sub directory information, run updateSources.py at project root level
#If you need to import an extra library or something clever, do it on the CMakeLists.txt at the root level
#If you really need to overwrite this file, be aware that it will be overwritten if updateSources.py is executed.


if(ODA_SUPPORT)
	set(SOURCES
		${SOURCES}
		${CMAKE_CURRENT_SOURCE_DIR}/file_processor.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/geometry_collector.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/geometry_dumper.cpp
		CACHE STRING "SOURCES" FORCE)

	set(HEADERS
		${HEADERS}
		${CMAKE_CURRENT_SOURCE_DIR}/file_processor.h
		${CMAKE_CURRENT_SOURCE_DIR}/geometry_collector.h
		${CMAKE_CURRENT_SOURCE_DIR}/geometry_dumper.h
		${CMAKE_CURRENT_SOURCE_DIR}/od_alloc_op.h
		${CMAKE_CURRENT_SOURCE_DIR}/oda_exsystem_services.h
		${CMAKE_CURRENT_SOURCE_DIR}/vectorise_device.h
		CACHE STRING "HEADERS" FORCE)

endif()