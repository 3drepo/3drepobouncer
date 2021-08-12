include_directories(${Boost_INCLUDE_DIRS} ${MONGO_INCLUDE_DIR} ${IFCOPENSHELL_INCLUDE_DIR} ${OCCT_INCLUDE_DIR} )

if(NOT SCHEMA_VERSIONS)
	set(SCHEMA_VERSIONS "2x3")
endif()


foreach(schema ${SCHEMA_VERSIONS})
	add_library("ifcUtils_${schema}" SHARED ./geometry_handler.cpp ./tree_parser.cpp)
	set_target_properties("ifcUtils_${schema}" PROPERTIES COMPILE_FLAGS "-DIFC_UTILS_API_LIBRARY=1 -DIfcSchema=Ifc${schema} -DDEFINED_Ifc${schema}")
	target_link_libraries("ifcUtils_${schema}" ${Boost_LIBRARIES} ${MONGO_LIBRARIES} ${OCCT_LIBRARIES} ${IFCOPENSHELL_PARSERLIB} ${IFCOPENSHELL_GEOMLIB})
	install(TARGETS "ifcUtils_${schema}" DESTINATION lib)
endforeach()

