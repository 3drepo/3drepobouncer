add_subdirectory(src)

include_directories(
	src
	../bouncer/src
	../log
	${Boost_INCLUDE_DIR}
	${MONGO_INCLUDE_DIR}
	${MONGO_CXX_DRIVER_MONGO_INCLUDE_DIR}
	${MONGO_CXX_DRIVER_BSON_INCLUDE_DIR}
	${ASSIMP_INCLUDE_DIR}
	${OCCT_INCLUDE_DIR}
	${IFCOPENSHELL_INCLUDE_DIR}
	${AWSSDK_INCLUDE_DIR}
)

add_executable(3drepobouncerClient
	${CLIENT_SOURCES}
)

target_link_libraries(3drepobouncerClient
	3drepobouncer
	log
	${Boost_LIBRARIES} 
	${MONGO_LIBRARIES} 
	${MONGO_CXX_DRIVER_LIBRARIES} 
	${ASSIMP_LIBRARIES} 
	${OCCT_LIBRARIES} 
	${IFCOPENSHELL_GEOMLIB} 
	${IFCOPENSHELL_PARSERLIB}
	${AWSSDK_LIBRARIES}
)

install(TARGETS 3drepobouncerClient DESTINATION bin)
