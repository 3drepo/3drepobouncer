add_subdirectory(src)

enable_testing()

include_directories(
	src
	${gtest_SOURCE_DIR}/include 
	${gtest_SOURCE_DIR} 
	../bouncer/src 
	../wrapper/src 
	${Boost_INCLUDE_DIR} 
	${MONGO_INCLUDE_DIR} 
	${ASSIMP_INCLUDE_DIR}  
	${OCCT_INCLUDE_DIR} 
	${IFCOPENSHELL_INCLUDE_DIR}  
	${ODA_INCLUDE_DIR} 
	${AWSSDK_INCLUDE_DIR}  
	${SYNCHRO_READER_INCLUDE_DIR})
	
add_executable(3drepobouncerwrapperTest ${WRAPPERTEST_SOURCES})

target_link_libraries(3drepobouncerwrapperTest 
	gtest 
	3drepobouncerwrapper
	3drepobouncer
	${Boost_LIBRARIES} 
	${MONGO_LIBRARIES} 
	${ASSIMP_LIBRARIES} 
	${OCCT_LIBRARIES} 
	${IFCOPENSHELL_GEOMLIB} 
	${IFCOPENSHELL_PARSERLIB}  
	${ODA_LIB}  
	${SYNCHRO_READER_LIBRARIES} 
	${THRIFT_LIBRARIES} 
	${ZLIB_LIBRARIES} 
	${SYNCHRO_LIBRARIES} 
	${AWSSDK_LIBRARIES})
	
add_test(3drepobouncerwrapperTest 3drepobouncerwrapperTest)

install(TARGETS 3drepobouncerwrapperTest DESTINATION bin)
