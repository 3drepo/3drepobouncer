#THIS IS AN AUTOMATICALLY GENERATED FILE - DO NOT OVERWRITE THE CONTENT!
#If you need to update the sources/headers/sub directory information, run updateSources.py at project root level
#If you need to import an extra library or something clever, do it on the CMakeLists.txt at the root level
#If you really need to overwrite this file, be aware that it will be overwritten if updateSources.py is executed.


add_subdirectory(core)
add_subdirectory(lib)
add_subdirectory(manipulator)
set(SOURCES
	${SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/repo_controller.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/repo_controller.cpp.inl
	${CMAKE_CURRENT_SOURCE_DIR}/repo_controller_internal.cpp.inl
	CACHE STRING "SOURCES" FORCE)

set(HEADERS
	${HEADERS}
	${CMAKE_CURRENT_SOURCE_DIR}/error_codes.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_bouncer_global.h
	${CMAKE_CURRENT_SOURCE_DIR}/repo_controller.h
	CACHE STRING "HEADERS" FORCE)

