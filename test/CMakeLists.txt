add_subdirectory(src)

enable_testing()
	

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR} src ../bouncerlib/src 
	${Boost_INCLUDE_DIR} ${MONGO_INCLUDE_DIR} ${ASSIMP_INCLUDE_DIR})
add_executable(3drepobouncerTest ${TEST_SOURCES}) 
target_link_libraries(3drepobouncerTest gtest 3drepobouncer 
	${Boost_LIBRARIES} ${MOMGO_LIBRARIES} ${ASSIMP_LIBRARIES})
add_test(3drepobouncerTest 3drepobouncerTest)
