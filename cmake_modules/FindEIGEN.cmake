#  Copyright (C) 2025 3D Repo Ltd
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Affero General Public License as
#  published by the Free Software Foundation, either version 3 of the
#  License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Affero General Public License for more details.
#
#  You should have received a copy of the GNU Affero General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Attempt to find EIGEN library. Eigen is a header only library. If Eigen
# is found, #EIGEN_INCLUDE_DIR will point to the include folder and
# EIGEN_FOUND will be set to true.

if(DEFINED ENV{EIGEN_ROOT})
	set(EIGEN_ROOT $ENV{EIGEN_ROOT})
	message(STATUS "$EIGEN_ROOT defined: ${EIGEN_ROOT}")
	find_path(EIGEN_INCLUDE_DIR Eigen/Eigen
		${EIGEN_ROOT}
		${EIGEN_ROOT}/include
	)
else()
	find_path(EIGEN_INCLUDE_DIR Eigen/Eigen
		/usr/include/eigen3
	)
endif()

if(EIGEN_INCLUDE_DIR)
	set(EIGEN_FOUND TRUE)
	message(STATUS "EIGEN installation found.")
	message(STATUS "EIGEN_INCLUDE_DIR: ${EIGEN_INCLUDE_DIR}")
else()
	set(EIGEN_FOUND FALSE)
	message(STATUS "Eigen not found. Please set EIGEN_ROOT to your installation directory or install via a suitable package manager.")
endif()
