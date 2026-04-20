#THIS IS AN AUTOMATICALLY GENERATED FILE - DO NOT OVERWRITE THE CONTENT!
#If you need to update the sources/headers/sub directory information, run updateSources.py at project root level
#If you need to import an extra library or something clever, do it on the CMakeLists.txt at the root level
#If you really need to overwrite this file, be aware that it will be overwritten if updateSources.py is executed.


add_subdirectory(error)
add_subdirectory(internal)
add_subdirectory(msinttypes)
set(HEADERS
	${HEADERS}
	${CMAKE_CURRENT_SOURCE_DIR}/allocators.h
	${CMAKE_CURRENT_SOURCE_DIR}/cursorstreamwrapper.h
	${CMAKE_CURRENT_SOURCE_DIR}/document.h
	${CMAKE_CURRENT_SOURCE_DIR}/encodedstream.h
	${CMAKE_CURRENT_SOURCE_DIR}/encodings.h
	${CMAKE_CURRENT_SOURCE_DIR}/filereadstream.h
	${CMAKE_CURRENT_SOURCE_DIR}/filewritestream.h
	${CMAKE_CURRENT_SOURCE_DIR}/fwd.h
	${CMAKE_CURRENT_SOURCE_DIR}/istreamwrapper.h
	${CMAKE_CURRENT_SOURCE_DIR}/memorybuffer.h
	${CMAKE_CURRENT_SOURCE_DIR}/memorystream.h
	${CMAKE_CURRENT_SOURCE_DIR}/ostreamwrapper.h
	${CMAKE_CURRENT_SOURCE_DIR}/pointer.h
	${CMAKE_CURRENT_SOURCE_DIR}/prettywriter.h
	${CMAKE_CURRENT_SOURCE_DIR}/rapidjson.h
	${CMAKE_CURRENT_SOURCE_DIR}/reader.h
	${CMAKE_CURRENT_SOURCE_DIR}/schema.h
	${CMAKE_CURRENT_SOURCE_DIR}/stream.h
	${CMAKE_CURRENT_SOURCE_DIR}/stringbuffer.h
	${CMAKE_CURRENT_SOURCE_DIR}/uri.h
	${CMAKE_CURRENT_SOURCE_DIR}/writer.h
	CACHE STRING "HEADERS" FORCE)

