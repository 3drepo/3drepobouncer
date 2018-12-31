/**
*  Copyright (C) 2018 3D Repo Ltd
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

#include <BimCommon.h>
#include <OdString.h>
#include <Ge/GeCircArc3d.h>
#include <Ge/GeEllipArc3d.h>
#include <Ge/GeNurbCurve3d.h>
#include <toString.h>

#include "geometry_dumper_rvt.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

void GeometryDumperRvt::init(GeometryCollector* const geoCollector)
{
	collector = geoCollector;
}

void GeometryDumperRvt::triangleOut(const OdInt32* p3Vertices, const OdGeVector3d* pNormal)
{
	const OdGePoint3d*  pVertexDataList = vertexDataList();
	const int numVertices = 3;

	if ((pVertexDataList + p3Vertices[0]) != (pVertexDataList + p3Vertices[1]) &&
		(pVertexDataList + p3Vertices[0]) != (pVertexDataList + p3Vertices[2]) &&
		(pVertexDataList + p3Vertices[1]) != (pVertexDataList + p3Vertices[2]))
	{
		std::vector<repo::lib::RepoVector3D64> vertices;
		for (int i = 0; i < numVertices; ++i)
		{
			vertices.push_back({ pVertexDataList[p3Vertices[i]].x , pVertexDataList[p3Vertices[i]].y, pVertexDataList[p3Vertices[i]].z });
		}
		collector->addFace(vertices);
	}
}
