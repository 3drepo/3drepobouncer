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

#include "BimCommon.h"
#include "OdString.h"
#include "conveyor_geometry_dumper.h"
#include "Ge/GeCircArc3d.h"
#include "Ge/GeEllipArc3d.h"
#include "Ge/GeNurbCurve3d.h"
#include "toString.h"

ConveyorGeometryDumper::ConveyorGeometryDumper()
	: m_dumpLevel(Maximal_Simplification)
{
}

OdSharedPtr<ConveyorGeometryDumper> ConveyorGeometryDumper::createObject(GeometryCollector* collector)
{
	OdSharedPtr<ConveyorGeometryDumper> pNewObject(new ConveyorGeometryDumper());
	pNewObject->geometryCollector = collector;
	return pNewObject;
}

void ConveyorGeometryDumper::plineProc(const OdGiPolyline& lwBuf,
	const OdGeMatrix3d* pXform,
	OdUInt32 fromIndex,
	OdUInt32 numSegs)
{
	if (m_dumpLevel == Maximal_Simplification)
	{
		OdGiGeometrySimplifier::plineProc(lwBuf, pXform, fromIndex, numSegs);
	}
}

void ConveyorGeometryDumper::polylineOut(OdInt32 numPoints, const OdGePoint3d* vertexList)
{
}

void ConveyorGeometryDumper::polygonOut(OdInt32 numPoints,
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
	geometryCollector->addFace(vec);
}

void ConveyorGeometryDumper::polylineProc(OdInt32 numPoints,
	const OdGePoint3d* vertexList,
	const OdGeVector3d* pNormal,
	const OdGeVector3d* pExtrusion,
	OdGsMarker baseSubEntMarker)
{
	if (m_dumpLevel == Maximal_Simplification)
	{
		OdGiGeometrySimplifier::polylineProc(numPoints, vertexList, pNormal, pExtrusion, baseSubEntMarker);
	}
}

void ConveyorGeometryDumper::polygonProc(OdInt32 numPoints,
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
	if (m_dumpLevel == Maximal_Simplification)
	{
		OdGiGeometrySimplifier::polygonProc(numPoints, vertexList, pNormal, pExtrusion);
	}
}

void ConveyorGeometryDumper::circleProc(const OdGePoint3d& center,
	double radius,
	const OdGeVector3d& normal,
	const OdGeVector3d* pExtrusion)
{
	if (m_dumpLevel == Maximal_Simplification)
	{
		OdGiGeometrySimplifier::circleProc(center, radius, normal, pExtrusion);
	}
}

void ConveyorGeometryDumper::circleProc(const OdGePoint3d& firstPoint,
	const OdGePoint3d& secondPoint,
	const OdGePoint3d& thirdPoint,
	const OdGeVector3d* pExtrusion)
{
	// Convert 3 point circle to center/radius circle.
	OdGeCircArc3d circ(firstPoint, secondPoint, thirdPoint);
	ConveyorGeometryDumper::circleProc(circ.center(), circ.radius(),
		circ.normal(), pExtrusion);
}

void ConveyorGeometryDumper::circularArcProc(const OdGePoint3d& center,
	double radius,
	const OdGeVector3d& normal,
	const OdGeVector3d& startVector,
	double sweepAngle,
	OdGiArcType arcType,
	const OdGeVector3d* pExtrusion)
{
	if (m_dumpLevel == Maximal_Simplification)
	{
		OdGiGeometrySimplifier::circularArcProc(center, radius, normal,
			startVector, sweepAngle, arcType, pExtrusion);
	}
}

void ConveyorGeometryDumper::circularArcProc(const OdGePoint3d& firstPoint,
	const OdGePoint3d& secondPoint,
	const OdGePoint3d& thirdPoint,
	OdGiArcType arcType,
	const OdGeVector3d* pExtrusion)
{
	if (m_dumpLevel == Maximal_Simplification)
	{
		OdGiGeometrySimplifier::circularArcProc(firstPoint, secondPoint, thirdPoint,
			arcType, pExtrusion);
	}
}

void ConveyorGeometryDumper::meshProc(OdInt32 rows,
	OdInt32 columns,
	const OdGePoint3d* pVertexList,
	const OdGiEdgeData* pEdgeData,
	const OdGiFaceData* pFaceData,
	const OdGiVertexData* pVertexData)
{
	OdString s;

	const OdGePoint3d* pV = pVertexList;
	repo_material_t mat;
	mat.ambient = { 0,0.5,0 };
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

	if (m_dumpLevel == Maximal_Simplification)
	{
		OdGiGeometrySimplifier::meshProc(rows, columns, pVertexList, pEdgeData, pFaceData, pVertexData);
	}
}

void ConveyorGeometryDumper::shellProc(OdInt32 numVertices,
	const OdGePoint3d* vertexList,
	OdInt32 faceListSize,
	const OdInt32* faceList,
	const OdGiEdgeData* pEdgeData,
	const OdGiFaceData* pFaceData,
	const OdGiVertexData* pVertexData)
{
	geometryCollector->stopMeshEntry();
	geometryCollector->startMeshEntry();
	repo_material_t mat;
	mat.ambient = { 0,0.5,0 };
	mat.diffuse = { 1,0,0 };
	mat.emissive = { 0,1,0 };
	mat.specular = { 0,1,0 };
	geometryCollector->setCurrentMaterial(mat);

	if (m_dumpLevel == Maximal_Simplification)
	{
		OdGiGeometrySimplifier::shellProc(numVertices, vertexList,
			faceListSize, faceList, pEdgeData, pFaceData, pVertexData);
	}
}

void ConveyorGeometryDumper::textProc(const OdGePoint3d& position,
	const OdGeVector3d& direction,
	const OdGeVector3d& upVector,
	const OdChar* msg, OdInt32 numChars,
	bool raw,
	const OdGiTextStyle* pTextStyle,
	const OdGeVector3d* pExtrusion)
{
	if (m_dumpLevel == Maximal_Simplification)
	{
		OdGiGeometrySimplifier::textProc(position, direction, upVector, msg, numChars, raw, pTextStyle, pExtrusion);
	}
}

void ConveyorGeometryDumper::shapeProc(const OdGePoint3d& position,
	const OdGeVector3d& direction,
	const OdGeVector3d& upVector,
	int shapeNumber,
	const OdGiTextStyle* pTextStyle,
	const OdGeVector3d* pExtrusion)

{
	if (m_dumpLevel == Maximal_Simplification)
	{
		OdGiGeometrySimplifier::shapeProc(position, direction, upVector, shapeNumber, pTextStyle, pExtrusion);
	}
}

void ConveyorGeometryDumper::xlineProc(const OdGePoint3d& firstPoint, const OdGePoint3d& secondPoint)
{
	if (m_dumpLevel == Maximal_Simplification)
	{
		OdGiGeometrySimplifier::xlineProc(firstPoint, secondPoint);
	}
}

void ConveyorGeometryDumper::rayProc(const OdGePoint3d& basePoint, const OdGePoint3d& throughPoint)
{
	if (m_dumpLevel == Maximal_Simplification)
	{
		OdGiGeometrySimplifier::rayProc(basePoint, throughPoint);
	}
}

void ConveyorGeometryDumper::nurbsProc(const OdGeNurbCurve3d& nurbs)
{
	if (m_dumpLevel == Maximal_Simplification)
	{
		OdGiGeometrySimplifier::nurbsProc(nurbs);
	}
}

void ConveyorGeometryDumper::ellipArcProc(const OdGeEllipArc3d& ellipArc,
	const OdGePoint3d* endPointsOverrides,
	OdGiArcType arcType,
	const OdGeVector3d* pExtrusion)
{
	if (m_dumpLevel == Maximal_Simplification)
	{
		OdGiGeometrySimplifier::ellipArcProc(ellipArc, endPointsOverrides, arcType, pExtrusion);
	}
}

void ConveyorGeometryDumper::rasterImageProc(const OdGePoint3d& origin,
	const OdGeVector3d& u,
	const OdGeVector3d& v,
	const OdGiRasterImage* /*pImg*/, // image object
	const OdGePoint2d* uvBoundary, // may not be null
	OdUInt32 numBoundPts,
	bool transparency,
	double brightness,
	double contrast,
	double fade)
{
}

void ConveyorGeometryDumper::metafileProc(const OdGePoint3d& origin,
	const OdGeVector3d& u,
	const OdGeVector3d& v,
	const OdGiMetafile* /*pMetafile*/,
	bool dcAligned,
	bool allowClipping)
{
}

void ConveyorGeometryDumper::polypointProc(OdInt32 numPoints,
	const OdGePoint3d* vertexList,
	const OdCmEntityColor* pColors,
	const OdCmTransparency* pTransparency,
	const OdGeVector3d* pNormals,
	const OdGeVector3d* pExtrusions,
	const OdGsMarker* pSubEntMarkers,
	OdInt32 nPointSize)
{
	if (m_dumpLevel == Maximal_Simplification)
	{
		OdGiGeometrySimplifier::polypointProc(numPoints, vertexList, pColors, pTransparency, pNormals, pExtrusions, pSubEntMarkers, nPointSize);
	}
}

void ConveyorGeometryDumper::rowOfDotsProc(OdInt32 numPoints,
	const OdGePoint3d& startPoint,
	const OdGeVector3d& dirToNextPoint)
{
}

void ConveyorGeometryDumper::edgeProc(const OdGiEdge2dArray& edges,
	const OdGeMatrix3d* pXform)
{
	if (m_dumpLevel == Maximal_Simplification)
	{
		OdGiGeometrySimplifier::edgeProc(edges, pXform);
	}
}

