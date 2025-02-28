
# Ifc Utils is built around IfcOpenShell; find IFCOS but also
# any dependencies that are not in its cmake find module (OCCT).

find_package(IFCOPENSHELL REQUIRED)
if(NOT ${OCCT_FOUND})
	message(FATAL_ERROR "Cannot find OCCT")
endif()


find_package(OCCT REQUIRED)
if(NOT ${IFCOPENSHELL_FOUND})
	message(FATAL_ERROR "Cannot find IFCOpenShell")
endif()

include_directories(
	${Boost_INCLUDE_DIR}
	${OCCT_INCLUDE_DIR}
	${IFCOPENSHELL_INCLUDE_DIR}
	../bouncer/src
	../log
)

if(NOT SCHEMA_VERSIONS)
	set(SCHEMA_VERSIONS "2x3" "4" "4x3")
endif()


foreach(schema ${SCHEMA_VERSIONS})
	add_library("ifcSerialiser_${schema}" STATIC
		./repo_ifc_serialiser.cpp
	)
	
	set_target_properties("ifcSerialiser_${schema}" PROPERTIES
		COMPILE_FLAGS "-DREPO_API_LIBRARY -DIfcSchema=Ifc${schema} -DDEFINED_Ifc${schema}"
	)
	
	target_link_libraries("ifcSerialiser_${schema}"
		${Boost_LIBRARIES}
		${OCCT_LIBRARIES}
		${IFCOPENSHELL_LIBRARIES}
		log
	)
		
	set(IFCSERIALISER_LIBARIES ${IFCSERIALISER_LIBARIES} "ifcSerialiser_${schema}")
endforeach()

# This library is the entry point for the AbstractImporter - it will figure out
# which schema to use and build the appropriate serialiser.

add_library("ifcUtils" STATIC
	./repo_ifc_utils.cpp
)

set_target_properties("ifcUtils" PROPERTIES
	COMPILE_FLAGS "-DREPO_API_LIBRARY"
)

target_link_libraries("ifcUtils"
	${IFCSERIALISER_LIBARIES}
)

set(IFCUTILS_LIBRARIES 
		ifcUtils 
	PARENT_SCOPE
)
set(IFCUTILS_INCLUDE_DIR
		${CMAKE_CURRENT_SOURCE_DIR}
	PARENT_SCOPE
)