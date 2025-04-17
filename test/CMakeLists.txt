add_subdirectory(src)

enable_testing()

add_definitions(-DREPO_API_LIBRARY)

include_directories(
	${gtest_SOURCE_DIR}/include
	${gtest_SOURCE_DIR}
	src
	../bouncer/src
	../client/src
	../log
	../
	${Boost_INCLUDE_DIR}
	${MONGO_CXX_DRIVER_MONGO_INCLUDE_DIR}
	${MONGO_CXX_DRIVER_BSON_INCLUDE_DIR}
	${ASSIMP_INCLUDE_DIR}
	${IFCUTILS_INCLUDE_DIR}
	${ODA_INCLUDE_DIR}
	${AWSSDK_INCLUDE_DIR}
	${SYNCHRO_READER_INCLUDE_DIR}
	${CRYPTOLENS_INCLUDE_DIR}
)

add_executable(3drepobouncerTest ${TEST_SOURCES} ${SOURCES})

target_link_libraries(
	3drepobouncerTest
	gtest
	gmock
	log
	${Boost_LIBRARIES}
	${MONGO_CXX_DRIVER_LIBRARIES}
	${ASSIMP_LIBRARIES}
	${IFCUTILS_LIBRARIES}
	${ODA_LIB}
	${SYNCHRO_READER_LIBRARIES}
	${THRIFT_LIBRARIES}
	${ZLIB_LIBRARIES}
	${SYNCHRO_LIBRARIES}
	${AWSSDK_LIBRARIES}
	${CRYPTOLENS_LIBRARIES}
)

add_test(3drepobouncerTest 3drepobouncerTest)

install(TARGETS 3drepobouncerTest DESTINATION bin)
