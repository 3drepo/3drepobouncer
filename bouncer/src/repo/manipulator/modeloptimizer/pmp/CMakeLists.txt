#THIS IS AN AUTOMATICALLY GENERATED FILE - DO NOT OVERWRITE THE CONTENT!
#If you need to update the sources/headers/sub directory information, run updateSources.py at project root level
#If you need to import an extra library or something clever, do it on the CMakeLists.txt at the root level
#If you really need to overwrite this file, be aware that it will be overwritten if updateSources.py is executed.


add_subdirectory(algorithms)
add_subdirectory(io)
add_subdirectory(visualization)
set(SOURCES
	${SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/SurfaceMesh.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/utilities.cpp
	CACHE STRING "SOURCES" FORCE)

set(HEADERS
	${HEADERS}
	${CMAKE_CURRENT_SOURCE_DIR}/BoundingBox.h
	${CMAKE_CURRENT_SOURCE_DIR}/Exceptions.h
	${CMAKE_CURRENT_SOURCE_DIR}/MatVec.h
	${CMAKE_CURRENT_SOURCE_DIR}/MemoryUsage.h
	${CMAKE_CURRENT_SOURCE_DIR}/Properties.h
	${CMAKE_CURRENT_SOURCE_DIR}/SurfaceMesh.h
	${CMAKE_CURRENT_SOURCE_DIR}/Timer.h
	${CMAKE_CURRENT_SOURCE_DIR}/Types.h
	${CMAKE_CURRENT_SOURCE_DIR}/utilities.h
	CACHE STRING "HEADERS" FORCE)

