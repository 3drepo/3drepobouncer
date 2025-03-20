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

# Attempt to find GNU Multiple Precision Floating Point library,
# https://www.mpfr.org/.
# This cmake file will only look for the libraries, since the headers
# are not required to compile bouncer.

SET(MPFR_LIB_NAMES
	mpfr
)

set(PATHS_TO_SEARCH 
	/usr/lib/
	/usr/local/lib/
	/opt/local/lib/
	/usr/lib/x86_64-linux-gnu
)

if(DEFINED ENV{MPFR_ROOT})
	set(MPFR_ROOT $ENV{MPFR_ROOT})
	message(STATUS "$MPFR_ROOT defined: ${MPFR_ROOT}")
	set(PATHS_TO_SEARCH ${MPFR_ROOT} ${PATHS_TO_SEARCH})
endif()

set(MPFR_FOUND TRUE)

foreach(libName ${MPFR_LIB_NAMES})
	find_library(libPath${libName} NAMES ${libName} PATHS ${PATHS_TO_SEARCH})
	
	if(NOT libPath${libName})
		set(MPFR_FOUND FALSE)
		message("Could not find " ${libName})
	endif()

	set(MPFR_LIBRARIES ${MPFR_LIBRARIES} ${libPath${libName}})
endforeach()

if(MPFR_FOUND)
	message(STATUS "MPFR installation found.")
else()
	message(STATUS "MPFR not found. Please set MPFR_ROOT to your installation directory or install via a suitable package manager.")
endif()
