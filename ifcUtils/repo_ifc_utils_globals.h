/**
*  Copyright (C) 2021 3D IFC_UTILS Ltd
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
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <string>
#if defined(_WIN32) || defined(_WIN64)
#   define IFC_UTILS_DECL_EXPORT __declspec(dllexport)
#   define IFC_UTILS_DECL_IMPORT __declspec(dllimport)
#else
#   define IFC_UTILS_DECL_EXPORT
#   define IFC_UTILS_DECL_IMPORT
#endif

//------------------------------------------------------------------------------
#if defined(IFC_UTILS_API_LIBRARY)
#   define IFC_UTILS_API_EXPORT IFC_UTILS_DECL_EXPORT
#else
#   define IFC_UTILS_API_EXPORT IFC_UTILS_DECL_IMPORT
#endif

//------------------------------------------------------------------------------

#define TO_STRING(x) #x
#define __CREATE_SCHEMA_NS(a, b)  a ## b
//For some reason this extra abstraction is needed or it won't append IfcSchema properly..
#define _CREATE_SCHEMA_NS(a, b)  __CREATE_SCHEMA_NS(a, b)
#define CREATE_SCHEMA_NS(prefix) _CREATE_SCHEMA_NS(prefix, IfcSchema)
#define SCHEMA_NS CREATE_SCHEMA_NS(Schema_)

struct TransNode {
	std::vector<float> transformation;
	std::string name;
	std::string guid;
	std::unordered_map<std::string, std::string> meta;
	std::vector<TransNode> children;
	bool createNode = false;
	bool meshTakeName = false;
	bool isIfcSpace = false;
};