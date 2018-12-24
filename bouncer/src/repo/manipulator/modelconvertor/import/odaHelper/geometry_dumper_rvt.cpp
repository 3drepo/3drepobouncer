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

void GeometryDumperRvt::init(GeometryCollector *const geoCollector) 
{
	collector = geoCollector;
}

void GeometryDumperRvt::polygonOut(
	OdInt32 numPoints,
	const OdGePoint3d* vertexList,
	const OdGeVector3d* pNormal)
{
	std::vector<repo::lib::RepoVector3D64> vec;
	repo::lib::RepoVector3D64 vec3;
	for (int i = 0; i < numPoints; i++)
	{
		vec3.x = vertexList[i].x;
		vec3.y = vertexList[i].y;
		vec3.z = vertexList[i].z;
		vec.push_back(vec3);
	}
	collector->addFace(vec);
}

void GeometryDumperRvt::polygonProc(
	OdInt32 numPoints,
	const OdGePoint3d* vertexList,
	const OdGeVector3d* pNormal,
	const OdGeVector3d* pExtrusion)
{
	const OdGePoint3d* vPtr = vertexList;
	std::vector<repo::lib::RepoVector3D64> vec;
	for (int i = 0; i < numPoints; i++)
	{
		vec.push_back({ vPtr->x, vPtr->y, vPtr->z });
		vPtr++;
	}
	OdGiGeometrySimplifier::polygonProc(numPoints, vertexList, pNormal, pExtrusion);
}

void GeometryDumperRvt::meshProc(
	OdInt32 rows,
	OdInt32 columns,
	const OdGePoint3d* pVertexList,
	const OdGiEdgeData* pEdgeData,
	const OdGiFaceData* pFaceData,
	const OdGiVertexData* pVertexData)
{
	OdString s;

	const OdGePoint3d* pV = pVertexList;
	repo_material_t mat;
	mat.ambient = { 0,0.5,0 };////////////////////refactoring
	mat.diffuse = { 1,0,0 };
	mat.emissive = { 0,1,0 };
	mat.specular = { 0,1,0 };

	for (OdInt32 row = 0; row < rows; row++)
	{
		for (OdInt32 col = 0; col < columns; col++)
		{
			s.format(OD_T("Vertex[%d, %d]"), col, row);

			std::vector<repo::lib::RepoVector3D64> verts;
			const OdGePoint3d* pVt = pV;
			verts.push_back(repo::lib::RepoVector3D64(pV->x, pV->y, pV->z));
			pVt++;
			verts.push_back(repo::lib::RepoVector3D64(pV->x, pV->y, pV->z));
			pVt++;
			verts.push_back(repo::lib::RepoVector3D64(pV->x, pV->y, pV->z));

			pV++;
		}
	}

	OdGiGeometrySimplifier::meshProc(rows, columns, pVertexList, pEdgeData, pFaceData, pVertexData);
}

void GeometryDumperRvt::shellProc(
	OdInt32 numVertices,
	const OdGePoint3d* vertexList,
	OdInt32 faceListSize,
	const OdInt32* faceList,
	const OdGiEdgeData* pEdgeData,
	const OdGiFaceData* pFaceData,
	const OdGiVertexData* pVertexData)
{
	collector->stopMeshEntry();
	collector->startMeshEntry();
	repo_material_t mat;
	mat.ambient = { 0,0.5,0 };////////////////////refactoring
	mat.diffuse = { 1,0,0 };
	mat.emissive = { 0,1,0 };
	mat.specular = { 0,1,0 };
	collector->setCurrentMaterial(mat);

	OdGiGeometrySimplifier::shellProc(numVertices, vertexList, faceListSize, faceList, pEdgeData, pFaceData, pVertexData);
}


