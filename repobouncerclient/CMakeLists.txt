add_subdirectory(src)

include_directories(src ../bouncerlib/src 
	${Boost_INCLUDE_DIR} ${MONGO_INCLUDE_DIR} ${ASSIMP_INCLUDE_DIR})
add_executable(3drepobouncerClient ${CLIENT_SOURCES}) 
target_link_libraries(3drepobouncerClient 3drepobouncer 
	${Boost_LIBRARIES} ${MOMGO_LIBRARIES} ${ASSIMP_LIBRARIES})
