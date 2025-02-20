include_directories(
	${Boost_INCLUDE_DIRS}
	${MONGO_INCLUDE_DIR}
	${MONGO_CXX_DRIVER_MONGO_INCLUDE_DIR}
	${MONGO_CXX_DRIVER_BSON_INCLUDE_DIR}
	${IFCOPENSHELL_INCLUDE_DIR} 
	${OCCT_INCLUDE_DIR}
	../bouncer/src
	../log
)

add_library("ifcUtils" 
	SHARED
	./repo_ifc_utils_geometry_handler.cpp 
	./repo_ifc_utils_tree_parser.cpp
)

set_target_properties("ifcUtils" 
	PROPERTIES 
	COMPILE_FLAGS "-DIFC_UTILS_API_LIBRARY=1"
)

target_link_libraries("ifcUtils" 
	${Boost_LIBRARIES} 
	${MONGO_LIBRARIES} 
	${MONGO_CXX_DRIVER_LIBRARIES} 
	${OCCT_LIBRARIES} 
	${IFCOPENSHELL_PARSERLIB} 
	${IFCOPENSHELL_GEOMLIB}
	log
)

install(TARGETS "ifcUtils" DESTINATION lib)