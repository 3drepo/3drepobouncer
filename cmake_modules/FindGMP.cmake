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

# Attempt to find GNU Multiple Precision library. https://gmplib.org/
# This cmake file will only look for the libraries, since the headers
# are not required to compile bouncer.

SET(GMP_LIB_NAMES
	gmp
	gmpxx
)

set(PATHS_TO_SEARCH 
	/usr/lib/
	/usr/local/lib/
	/opt/local/lib/
	/usr/lib/x86_64-linux-gnu
)

if(DEFINED ENV{GMP_ROOT})
	set(GMP_ROOT $ENV{GMP_ROOT})
	message(STATUS "$GMP_ROOT defined: ${GMP_ROOT}")
	set(PATHS_TO_SEARCH ${GMP_ROOT} ${PATHS_TO_SEARCH})
endif()

set(GMP_FOUND TRUE)

foreach(libName ${GMP_LIB_NAMES})
	find_library(libPath${libName} NAMES ${libName} PATHS ${PATHS_TO_SEARCH})
	
	if(NOT libPath${libName})
		set(GMP_FOUND FALSE)
		message("Could not find " ${libName})
	endif()

	set(GMP_LIBRARIES ${GMP_LIBRARIES} ${libPath${libName}})
endforeach()

if(GMP_FOUND)
	message(STATUS "GMP installation found.")
else()
	message(STATUS "GMP not found. Please set GMP_ROOT to your installation directory or install via a suitable package manager.")
endif()
