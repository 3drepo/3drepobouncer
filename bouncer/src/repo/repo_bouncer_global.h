/**
*  Copyright (C) 2015 3D Repo Ltd
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU Affero General Public License as
*  published by the Free Software Foundation, either version 3 of the
*  License, or (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU Affero General Public License for more details.
*
*  You should have received a copy of the GNU Affero General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include <cstdint>
#if defined(_WIN32) || defined(_WIN64)
#   define REPO_DECL_EXPORT __declspec(dllexport)
#   define REPO_DECL_IMPORT __declspec(dllimport)
#else
#   define REPO_DECL_EXPORT
#   define REPO_DECL_IMPORT
#endif

//------------------------------------------------------------------------------
#if defined(REPO_API_LIBRARY)
#   define REPO_API_EXPORT REPO_DECL_EXPORT
#else
#   define REPO_API_EXPORT REPO_DECL_IMPORT
#endif

//------------------------------------------------------------------------------
#define BOUNCER_VMAJOR 5

#define BOUNCER_VMINOR "20_0"
#define REPO_MAX_OBJ_SIZE (16 * 1024 * 1024)

//
