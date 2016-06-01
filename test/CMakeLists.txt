add_subdirectory(src)

enable_testing()
add_definitions(-DREPO_API_LIBRARY)	
	

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR} src ../bouncer/src ../client/src
	${Boost_INCLUDE_DIR} ${MONGO_INCLUDE_DIR} ${ASSIMP_INCLUDE_DIR})
add_executable(3drepobouncerTest ${TEST_SOURCES} ${SOURCES}) 
target_link_libraries(3drepobouncerTest gtest 
	${Boost_LIBRARIES} ${MONGO_LIBRARIES} ${ASSIMP_LIBRARIES})
add_test(3drepobouncerTest 3drepobouncerTest)




install(TARGETS 3drepobouncerTest DESTINATION bin)
