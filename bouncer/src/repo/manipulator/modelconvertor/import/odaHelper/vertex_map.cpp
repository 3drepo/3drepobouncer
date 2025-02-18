/**
*  Copyright (C) 2021 3D Repo Ltd
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

#include "vertex_map.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

size_t VertexMap::insert(const repo::lib::RepoVector3D64& position)
{
	auto idx = vertices.size();
	vertices.push_back(position);
	return idx;
}

size_t VertexMap::insert(const repo::lib::RepoVector3D64& position, const repo::lib::RepoVector3D64& normal)
{
	auto idx = vertices.size();
	vertices.push_back(position);
	normals.push_back(normal);
	return idx;
}

size_t VertexMap::insert(const repo::lib::RepoVector3D64& position, const repo::lib::RepoVector3D64& normal, const repo::lib::RepoVector2D& uv)
{
	auto idx = vertices.size();
	vertices.push_back(position);
	normals.push_back(normal);
	uvs.push_back(uv);
	return idx;
}

