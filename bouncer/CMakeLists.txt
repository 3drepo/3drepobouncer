
#================Source and header files===============
add_subdirectory(src)

include_directories(${Boost_INCLUDE_DIRS} ${MONGO_INCLUDE_DIR} ${ASSIMP_INCLUDE_DIR} src)
add_definitions(-DREPO_API_LIBRARY)	

add_library(3drepobouncer SHARED ${SOURCES})
set_target_properties(3drepobouncer PROPERTIES DEBUG_POSTFIX "d")
target_link_libraries(3drepobouncer ${Boost_LIBRARIES} ${MONGO_LIBRARIES} ${ASSIMP_LIBRARIES})

install(DIRECTORY src/ DESTINATION include FILES_MATCHING PATTERN "*.h")
install(TARGETS 3drepobouncer DESTINATION lib)

