include_directories(${Boost_INCLUDE_DIRS} ${MONGO_INCLUDE_DIR} ${MONGO_CXX_DRIVER_MONGO_INCLUDE_DIR} ${MONGO_CXX_DRIVER_BSON_INCLUDE_DIR} ${IFCOPENSHELL_INCLUDE_DIR} ${OCCT_INCLUDE_DIR} ../bouncer/src)

if(NOT SCHEMA_VERSIONS)
	set(SCHEMA_VERSIONS "2x3" "4")
endif()


foreach(schema ${SCHEMA_VERSIONS})
	add_library("ifcUtils_${schema}" 
		SHARED 
		./repo_ifc_utils_geometry_handler.cpp 
		./repo_ifc_utils_tree_parser.cpp
	)
	
	include_directories(
		../log
	)
	
	set_target_properties("ifcUtils_${schema}" 
	PROPERTIES 
		COMPILE_FLAGS "-DIFC_UTILS_API_LIBRARY=1 -DIfcSchema=Ifc${schema} -DDEFINED_Ifc${schema}"
	)
	
	target_link_libraries("ifcUtils_${schema}" 
		${Boost_LIBRARIES} 
		${MONGO_LIBRARIES} 
		${MONGO_CXX_DRIVER_LIBRARIES} 
		${OCCT_LIBRARIES} 
		${IFCOPENSHELL_PARSERLIB} 
		${IFCOPENSHELL_GEOMLIB}
		log
	)
	install(TARGETS "ifcUtils_${schema}" DESTINATION lib)
endforeach()

