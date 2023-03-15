#THIS IS AN AUTOMATICALLY GENERATED FILE - DO NOT OVERWRITE THE CONTENT!
#If you need to update the sources/headers/sub directory information, run updateSources.py at project root level
#If you need to import an extra library or something clever, do it on the CMakeLists.txt at the root level
#If you really need to overwrite this file, be aware that it will be overwritten if updateSources.py is executed.


add_subdirectory(arch)
set(HEADERS
	${HEADERS}
	${CMAKE_CURRENT_SOURCE_DIR}/AlignedBox.h
	${CMAKE_CURRENT_SOURCE_DIR}/AngleAxis.h
	${CMAKE_CURRENT_SOURCE_DIR}/EulerAngles.h
	${CMAKE_CURRENT_SOURCE_DIR}/Homogeneous.h
	${CMAKE_CURRENT_SOURCE_DIR}/Hyperplane.h
	${CMAKE_CURRENT_SOURCE_DIR}/OrthoMethods.h
	${CMAKE_CURRENT_SOURCE_DIR}/ParametrizedLine.h
	${CMAKE_CURRENT_SOURCE_DIR}/Quaternion.h
	${CMAKE_CURRENT_SOURCE_DIR}/Rotation2D.h
	${CMAKE_CURRENT_SOURCE_DIR}/RotationBase.h
	${CMAKE_CURRENT_SOURCE_DIR}/Scaling.h
	${CMAKE_CURRENT_SOURCE_DIR}/Transform.h
	${CMAKE_CURRENT_SOURCE_DIR}/Translation.h
	${CMAKE_CURRENT_SOURCE_DIR}/Umeyama.h
	CACHE STRING "HEADERS" FORCE)

