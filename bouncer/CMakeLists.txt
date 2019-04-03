
#================Source and header files===============
add_subdirectory(src)

set(VERSION_MAJOR 3)
set(VERSION_MINOR 4_1)

include_directories(${Boost_INCLUDE_DIRS} ${MONGO_INCLUDE_DIR} ${ASSIMP_INCLUDE_DIR} ${OCCT_INCLUDE_DIR} ${IFCOPENSHELL_INCLUDE_DIR} ${ODA_INCLUDE_DIR} ${AWSSDK_INCLUDE_DIR} src)
add_definitions(-DREPO_API_LIBRARY)

add_library(3drepobouncer SHARED ${SOURCES})
set_target_properties(3drepobouncer PROPERTIES RELEASE_POSTFIX "_${VERSION_MAJOR}_${VERSION_MINOR}")
set_target_properties(3drepobouncer PROPERTIES DEBUG_POSTFIX "_${VERSION_MAJOR}_${VERSION_MINOR}_d")
target_link_libraries(3drepobouncer ${Boost_LIBRARIES} ${MONGO_LIBRARIES} ${ASSIMP_LIBRARIES} ${OCCT_LIBRARIES} ${IFCOPENSHELL_PARSERLIB} ${IFCOPENSHELL_GEOMLIB} ${ODA_LIB} ${AWSSDK_LIBRARIES})

install(DIRECTORY src/ DESTINATION include FILES_MATCHING PATTERN "*.h")
install(TARGETS 3drepobouncer DESTINATION lib)

