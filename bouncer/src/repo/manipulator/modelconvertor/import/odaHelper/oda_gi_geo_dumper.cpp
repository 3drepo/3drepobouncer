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

#include <OdaCommon.h>
#include <OdString.h>

#include <Ge/GeCircArc3d.h>
#include <Ge/GeEllipArc3d.h>
#include <Ge/GeNurbCurve3d.h>
#include <toString.h>


#include "oda_gi_geo_dumper.h"
#include "../../../../core/model/bson/repo_bson_factory.h"
using namespace repo::manipulator::modelconvertor::odaHelper;


OdGiConveyorGeometryDumper::OdGiConveyorGeometryDumper()
	: m_dumpLevel(Maximal_Simplification)
{
}

void OdGiConveyorGeometryDumper::setMeshCollector(OdaGeometryCollector *const geoCollector) {
	collector = geoCollector;
}

OdSharedPtr<OdGiConveyorGeometryDumper> OdGiConveyorGeometryDumper::createObject(OdaGiDumper* m_pDumper)
{
	OdSharedPtr<OdGiConveyorGeometryDumper> pNewObject(new OdGiConveyorGeometryDumper);
	pNewObject->m_pDumper = m_pDumper;
	return pNewObject;
}


/************************************************************************/
/* Process OdGiPolyline data                                            */
/************************************************************************/
void OdGiConveyorGeometryDumper::plineProc(const OdGiPolyline& lwBuf,
	const OdGeMatrix3d* pXform,
	OdUInt32 fromIndex,
	OdUInt32 numSegs)
{
	//m_pDumper->output(OD_T("Start plineProc"));
	//m_pDumper->pushIndent();
	//m_pDumper->output(OD_T("Xform"), pXform);
	//m_pDumper->output(OD_T("fromIndex"), toString((int)fromIndex));
	//m_pDumper->output(OD_T("numSegs"), toString((int)numSegs));
	if (m_dumpLevel == Maximal_Simplification)
	{
		//m_pDumper->output(OD_T("Reduced pline data"));
		//m_pDumper->pushIndent();
		OdGiGeometrySimplifier::plineProc(lwBuf, pXform, fromIndex, numSegs);
		//m_pDumper->popIndent();
	}
	//m_pDumper->popIndent();
	//m_pDumper->output(OD_T("End plineProc"));
}

/************************************************************************/
/* Process polyline data                                                */
/************************************************************************/
void OdGiConveyorGeometryDumper::polylineOut(OdInt32 numPoints, const OdGePoint3d* vertexList)
{
	//m_pDumper->output(OD_T("Start polylineOut"));
	//m_pDumper->pushIndent();
	//m_pDumper->output(OD_T("numPoints"), toString((int)numPoints));
	//m_pDumper->output(numPoints, vertexList);
	//m_pDumper->popIndent();
	//m_pDumper->output(OD_T("End polylineOut"));
}

/************************************************************************/
/* Process polygon data                                                 */
/************************************************************************/
void OdGiConveyorGeometryDumper::polygonOut(OdInt32 numPoints,
	const OdGePoint3d* vertexList,
	const OdGeVector3d* pNormal)
{
	//m_pDumper->output(OD_T("Start polygonOut"));
	//m_pDumper->pushIndent();
	//m_pDumper->output(OD_T("numPoints"), toString((int)numPoints));
	//m_pDumper->output(numPoints, vertexList);
	//m_pDumper->popIndent();
	//m_pDumper->output(OD_T("End polygonOut"));

	if (numPoints == 3)
	{
		OdDgStlTriangleFace newFace;
		newFace.m_pt1 = vertexList[0];
		newFace.m_pt2 = vertexList[1];
		newFace.m_pt3 = vertexList[2];

		OdaGiDumper::addStlTriangle(newFace);
	}
}

/************************************************************************/
/* Process simple polyline data                                         */
/************************************************************************/
void OdGiConveyorGeometryDumper::polylineProc(OdInt32 numPoints,
	const OdGePoint3d* vertexList,
	const OdGeVector3d* pNormal,
	const OdGeVector3d* pExtrusion,
	OdGsMarker baseSubEntMarker)
{
	//m_pDumper->output(OD_T("Start polylineProc"));
	/* //m_pDumper->pushIndent();
	//m_pDumper->output(OD_T("normal"),           toString(pNormal));
	//m_pDumper->output(OD_T("extrusion"),        toString(pExtrusion));
	//m_pDumper->output(OD_T("baseSubEntMarker"), toString((int)baseSubEntMarker));
	//m_pDumper->output(OD_T("numPoints"),        toString((int)numPoints));
	//m_pDumper->output(numPoints,          vertexList);
	if (m_dumpLevel == Maximal_Simplification)
	{
	//m_pDumper->output(OD_T("Reduced polyline data"));
	//m_pDumper->pushIndent();
	OdGiGeometrySimplifier::polylineProc(numPoints, vertexList, pNormal, pExtrusion, baseSubEntMarker);
	//m_pDumper->popIndent();
	}
	//m_pDumper->popIndent();*/
	//m_pDumper->output(OD_T("End polylineProc"));
}
/************************************************************************/
/* Process polygon data                                                 */
/************************************************************************/
void OdGiConveyorGeometryDumper::polygonProc(OdInt32 numPoints,
	const OdGePoint3d* vertexList,
	const OdGeVector3d* pNormal,
	const OdGeVector3d* pExtrusion)
{
	//m_pDumper->output(OD_T("Start polygonProc"));
	/* //m_pDumper->pushIndent();
	//m_pDumper->output(OD_T("normal"),   toString(pNormal));
	//m_pDumper->output(OD_T("extrusion"),toString(pExtrusion));
	//m_pDumper->output(OD_T("numPoints"),toString((int)numPoints));
	//m_pDumper->output(numPoints, vertexList);
	if (m_dumpLevel == Maximal_Simplification)
	{
	//m_pDumper->output(OD_T("Reduced polygon data"));
	//m_pDumper->pushIndent();
	OdGiGeometrySimplifier::polygonProc(numPoints, vertexList, pNormal, pExtrusion);
	//m_pDumper->popIndent();
	}
	//m_pDumper->popIndent();*/
	//m_pDumper->output(OD_T("End polygonProc"));
}
/************************************************************************/
/* Process center-radius circle data                                    */
/************************************************************************/
void OdGiConveyorGeometryDumper::circleProc(const OdGePoint3d& center,
	double radius,
	const OdGeVector3d& normal,
	const OdGeVector3d* pExtrusion)
{
	//m_pDumper->output(OD_T("Start circleProc"));
	/* //m_pDumper->pushIndent();
	//m_pDumper->output(OD_T("center"),   toString(center));
	//m_pDumper->output(OD_T("radius"),   toString(radius));
	//m_pDumper->output(OD_T("normal"),   toString(normal));
	//m_pDumper->output(OD_T("extrusion"),toString(pExtrusion));
	if (m_dumpLevel == Maximal_Simplification)
	{
	//m_pDumper->output(OD_T("Reduced circle data"));
	//m_pDumper->pushIndent();
	OdGiGeometrySimplifier::circleProc(center, radius, normal, pExtrusion);
	//m_pDumper->popIndent();
	}
	//m_pDumper->popIndent();*/
	//m_pDumper->output(OD_T("End circleProc"));
}

/************************************************************************/
/* Process three-point circle data                                      */
/************************************************************************/
void OdGiConveyorGeometryDumper::circleProc(const OdGePoint3d& firstPoint,
	const OdGePoint3d& secondPoint,
	const OdGePoint3d& thirdPoint,
	const OdGeVector3d* pExtrusion)
{
	// Convert 3 point circle to center/radius circle.
	OdGeCircArc3d circ(firstPoint, secondPoint, thirdPoint);
	OdGiConveyorGeometryDumper::circleProc(circ.center(), circ.radius(),
		circ.normal(), pExtrusion);
}
/************************************************************************/
/* Process center-radius circular arc                                  */
/************************************************************************/
void OdGiConveyorGeometryDumper::circularArcProc(const OdGePoint3d& center,
	double radius,
	const OdGeVector3d& normal,
	const OdGeVector3d& startVector,
	double sweepAngle,
	OdGiArcType arcType,
	const OdGeVector3d* pExtrusion)
{
	//m_pDumper->output(OD_T("Start circularArcProc"));
	////m_pDumper->pushIndent();
	////m_pDumper->output(OD_T("center"),     toString(center));
	////m_pDumper->output(OD_T("radius"),     toString(radius));
	////m_pDumper->output(OD_T("normal"),     toString(normal));
	////m_pDumper->output(OD_T("startVector"),toString(startVector));
	////m_pDumper->output(OD_T("sweepAngle"), toString(sweepAngle));
	////m_pDumper->output(OD_T("arcType"),    toString(arcType));
	////m_pDumper->output(OD_T("extrusion"),  toString(pExtrusion));
	//if (m_dumpLevel == Maximal_Simplification)
	//{
	//  //m_pDumper->output(OD_T("Reduced circularArc data"));
	//  //m_pDumper->pushIndent();
	//  OdGiGeometrySimplifier::circularArcProc(center, radius, normal, 
	//    startVector, sweepAngle, arcType, pExtrusion);
	//  //m_pDumper->popIndent();
	//}
	////m_pDumper->popIndent();
	//m_pDumper->output(OD_T("End circularArcProc"));
}

/************************************************************************/
/* Process three-point circular arc                                     */
/************************************************************************/
void OdGiConveyorGeometryDumper::circularArcProc(const OdGePoint3d& firstPoint,
	const OdGePoint3d& secondPoint,
	const OdGePoint3d& thirdPoint,
	OdGiArcType arcType,
	const OdGeVector3d* pExtrusion)
{
	//m_pDumper->output(OD_T("Start circularArcProc"));
	/*//m_pDumper->pushIndent();
	//m_pDumper->output(OD_T("firstPoint"),   toString(firstPoint));
	//m_pDumper->output(OD_T("secondPoint"),  toString(secondPoint));
	//m_pDumper->output(OD_T("thirdPoint"),   toString(thirdPoint));
	//m_pDumper->output(OD_T("arcType"),      toString(arcType));
	//m_pDumper->output(OD_T("extrusion"),    toString(pExtrusion));
	if (m_dumpLevel == Maximal_Simplification)
	{
	//m_pDumper->output(OD_T("Reduced circularArc data"));
	//m_pDumper->pushIndent();
	OdGiGeometrySimplifier::circularArcProc(firstPoint, secondPoint, thirdPoint,
	arcType, pExtrusion);
	//m_pDumper->popIndent();
	}
	//m_pDumper->popIndent();*/
	//m_pDumper->output(OD_T("End circularArcProc"));
}

/************************************************************************/
/* Process mesh data                                                    */
/************************************************************************/
void OdGiConveyorGeometryDumper::meshProc(OdInt32 rows,
	OdInt32 columns,
	const OdGePoint3d* pVertexList,
	const OdGiEdgeData* pEdgeData,
	const OdGiFaceData* pFaceData,
	const OdGiVertexData* pVertexData)
{
	OdString s;

	//m_pDumper->output(OD_T("Start meshProc"));
	/*//m_pDumper->pushIndent();

	//m_pDumper->output(OD_T("Rows"), toString((int)rows));
	//m_pDumper->output(OD_T("Columns"), toString((int)columns));

	const OdGePoint3d* pV = pVertexList;

	//m_pDumper->pushIndent();
	for (OdInt32 row = 0; row < rows; row++)
	{
	for (OdInt32 col = 0; col < columns; col++)
	{
	s.format(OD_T("Vertex[%d, %d]"),col, row);
	//m_pDumper->output(s, toString(*pV));
	pV++;
	}
	}

	m_pDumper->outputEdgeData(pEdgeData, ((rows - 1) * columns) + ((columns - 1) * rows));
	m_pDumper->outputFaceData(pFaceData, (rows - 1) * (columns - 1));
	m_pDumper->outputVertexData(pVertexData, rows * columns);

	if (m_dumpLevel == Maximal_Simplification)
	{
	//m_pDumper->output(OD_T("Reduced shell data"));
	//m_pDumper->pushIndent();
	OdGiGeometrySimplifier::meshProc(rows, columns, pVertexList, pEdgeData, pFaceData, pVertexData);
	//m_pDumper->popIndent();
	}

	//m_pDumper->popIndent();*/
	//m_pDumper->output(OD_T("End meshProc"));
}

/************************************************************************/
/* Process shell data                                                   */
/************************************************************************/
void OdGiConveyorGeometryDumper::shellProc(OdInt32 numVertices,
	const OdGePoint3d* vertexList,
	OdInt32 faceListSize,
	const OdInt32* faceList,
	const OdGiEdgeData* pEdgeData,
	const OdGiFaceData* pFaceData,
	const OdGiVertexData* pVertexData)
{

	std::vector<repo::lib::RepoVector3D64> vertices;
	std::vector<repo_face_t> faces;
	


	std::vector<std::vector<double>> boundingBox;

	
	for (OdInt32 i = 0; i < numVertices; ++i)
	{
		vertices.push_back(repo::lib::RepoVector3D64(vertexList[i].x, vertexList[i].y, vertexList[i].z));

		if (i == 0) {
			boundingBox.push_back({ vertexList[i].x, vertexList[i].y, vertexList[i].z });
			boundingBox.push_back({ vertexList[i].x, vertexList[i].y, vertexList[i].z });
		}
		else {
			boundingBox[0][0] = boundingBox[0][0] > vertexList[i].x ? vertexList[i].x : boundingBox[0][0];
			boundingBox[0][1] = boundingBox[0][1] > vertexList[i].y ? vertexList[i].y : boundingBox[0][1];
			boundingBox[0][2] = boundingBox[0][2] > vertexList[i].z ? vertexList[i].z : boundingBox[0][2];

			boundingBox[1][0] = boundingBox[1][0] < vertexList[i].x ? vertexList[i].x : boundingBox[1][0];
			boundingBox[1][1] = boundingBox[1][1] < vertexList[i].y ? vertexList[i].y : boundingBox[1][1];
			boundingBox[1][2] = boundingBox[1][2] < vertexList[i].z ? vertexList[i].z : boundingBox[1][2];
		}		
	}

	/**********************************************************************/
	/* Count and dump faces, count edges                                  */
	/**********************************************************************/
	OdInt32 i = 0;
	while (i < faceListSize)
	{
		OdInt32 count = faceList[i++];
		if (count < 0) count *= -1;
		repo_face_t face;
		for (OdInt32 j = 0; j < count; j++, i++)
		{
			if (j < 3)
				face.push_back(faceList[i]);
		}

		faces.push_back(face);
	}

	collector->addMeshEntry(vertices, faces, boundingBox);
}

/************************************************************************/
/* Process text                                                         */
/************************************************************************/
void OdGiConveyorGeometryDumper::textProc(const OdGePoint3d& position,
	const OdGeVector3d& direction,
	const OdGeVector3d& upVector,
	const OdChar* msg, OdInt32 numChars,
	bool raw,
	const OdGiTextStyle* pTextStyle,
	const OdGeVector3d* pExtrusion)
{
	//m_pDumper->output(OD_T("Start textProc"));
	/*//m_pDumper->pushIndent();
	//m_pDumper->output(OD_T("position"),         toString(position));
	//m_pDumper->output(OD_T("direction"),        toString(direction));
	//m_pDumper->output(OD_T("upVector"),         toString(upVector));
	//m_pDumper->output(OD_T("msg"),              toString(msg));
	//m_pDumper->output(OD_T("numBytes"),         toString((int)numChars));
	//m_pDumper->output(OD_T("raw"),              toString(raw));
	//m_pDumper->output(OD_T("Extrusion vector"), toString(pExtrusion));
	//m_pDumper->output(*pTextStyle);

	if (m_dumpLevel == Maximal_Simplification)
	{
	//m_pDumper->output(OD_T("Reduced text data"));
	//m_pDumper->pushIndent();
	OdGiGeometrySimplifier::textProc(position, direction, upVector, msg, numChars, raw, pTextStyle, pExtrusion);
	//m_pDumper->popIndent();
	}

	//m_pDumper->popIndent();*/
	//m_pDumper->output(OD_T("End textProc"));
}
/************************************************************************/
/* Process shape data                                                   */
/************************************************************************/
void OdGiConveyorGeometryDumper::shapeProc(const OdGePoint3d& position,
	const OdGeVector3d& direction,
	const OdGeVector3d& upVector,
	int shapeNumber,
	const OdGiTextStyle* pTextStyle,
	const OdGeVector3d* pExtrusion)

{
	//m_pDumper->output(OD_T("Start shapeProc"));
	/* //m_pDumper->pushIndent();
	//m_pDumper->output(OD_T("position"),         toString(position));
	//m_pDumper->output(OD_T("direction"),        toString(direction));
	//m_pDumper->output(OD_T("upVector"),         toString(upVector));
	//m_pDumper->output(OD_T("shapeNumber"),      toString(shapeNumber));
	//m_pDumper->output(OD_T("Extrusion vector"), toString(pExtrusion));
	//m_pDumper->output(*pTextStyle);

	if (m_dumpLevel == Maximal_Simplification)
	{
	//m_pDumper->output(OD_T("Reduced shape data"));
	//m_pDumper->pushIndent();
	OdGiGeometrySimplifier::shapeProc(position, direction, upVector, shapeNumber, pTextStyle, pExtrusion);
	//m_pDumper->popIndent();
	}

	//m_pDumper->popIndent();*/
	//m_pDumper->output(OD_T("End shapeProc"));
}

/************************************************************************/
/* Process xline data                                                   */
/************************************************************************/
void OdGiConveyorGeometryDumper::xlineProc(const OdGePoint3d& firstPoint, const OdGePoint3d& secondPoint)
{
	//m_pDumper->output(OD_T("Start xlineProc"));
	/*//m_pDumper->pushIndent();
	//m_pDumper->output(OD_T("firstPoint"), toString(firstPoint));
	//m_pDumper->output(OD_T("secondPoint"), toString(secondPoint));
	if (m_dumpLevel == Maximal_Simplification)
	{
	//m_pDumper->output(OD_T("Reduced xline data"));
	//m_pDumper->pushIndent();
	OdGiGeometrySimplifier::xlineProc(firstPoint, secondPoint);
	//m_pDumper->popIndent();
	}
	//m_pDumper->popIndent();*/
	//m_pDumper->output(OD_T("End xlineProc"));
}

/************************************************************************/
/* Process ray data                                                     */
/************************************************************************/
void OdGiConveyorGeometryDumper::rayProc(const OdGePoint3d& basePoint, const OdGePoint3d& throughPoint)
{
	//m_pDumper->output(OD_T("Start rayProc"));
	////m_pDumper->pushIndent();
	////m_pDumper->output(OD_T("basePoint"), toString(basePoint));
	////m_pDumper->output(OD_T("throughPoint"), toString(throughPoint));
	//if (m_dumpLevel == Maximal_Simplification)
	//{
	//  //m_pDumper->output(OD_T("Reduced ray data"));
	//  //m_pDumper->pushIndent();
	//  OdGiGeometrySimplifier::rayProc(basePoint, throughPoint);
	//  //m_pDumper->popIndent();
	//}
	////m_pDumper->popIndent();
	//m_pDumper->output(OD_T("End rayProc"));
}

/************************************************************************/
/* Process nurbs data                                                      */
/************************************************************************/
void OdGiConveyorGeometryDumper::nurbsProc(const OdGeNurbCurve3d& nurbs)
{

	//m_pDumper->output(OD_T("Start nurbsProc"));
	/* //m_pDumper->pushIndent();

	//m_pDumper->output(OD_T("degree"),   toString(nurbs.degree()));
	//m_pDumper->output(OD_T("order"),    toString(nurbs.order()));
	//m_pDumper->output(OD_T("isClosed"),   toString(nurbs.isClosed()));
	//m_pDumper->output(OD_T("isRational"),   toString(nurbs.isRational()));
	//m_pDumper->output(OD_T("startParam"),   toString(nurbs.startParam()));
	//m_pDumper->output(OD_T("endParam"),   toString(nurbs.endParam()));


	//m_pDumper->output(OD_T("Knots"), toString(nurbs.numKnots()));
	//m_pDumper->pushIndent();
	int i;
	for (i = 0; i < nurbs.numKnots(); i++)
	{
	//m_pDumper->output(toString(OD_T("Knot[%d]"), i), toString(nurbs.knotAt(i)));
	}
	//m_pDumper->popIndent();

	//m_pDumper->output(OD_T("Control points"), toString(nurbs.numControlPoints()));
	//m_pDumper->pushIndent();
	for (i = 0; i < nurbs.numControlPoints(); i++)
	{
	//m_pDumper->output(toString(OD_T("Control point[%d]"), i), toString(nurbs.controlPointAt(i)));
	}
	//m_pDumper->popIndent();

	//m_pDumper->output(OD_T("Fit points"), toString(nurbs.numFitPoints()));
	//m_pDumper->pushIndent();
	for (i = 0; i < nurbs.numFitPoints(); i++)
	{
	OdGePoint3d point;
	nurbs.getFitPointAt(i, point);
	//m_pDumper->output(toString(OD_T("Fit point[%d]"), i), toString(point));
	}

	if (nurbs.isRational())
	{
	//m_pDumper->output(OD_T("Weights"), toString(nurbs.numWeights()));
	//m_pDumper->pushIndent();
	for (i = 0; i < nurbs.numWeights(); i++)
	{
	//m_pDumper->output(toString(OD_T("Weight[%d]"), i), toString(nurbs.weightAt(i)));
	}
	//m_pDumper->popIndent();
	}

	if (m_dumpLevel == Maximal_Simplification)
	{
	//m_pDumper->output(OD_T("Reduced nurbs data"));
	//m_pDumper->pushIndent();
	OdGiGeometrySimplifier::nurbsProc(nurbs);
	//m_pDumper->popIndent();
	}

	//m_pDumper->popIndent();*/
	//m_pDumper->output(OD_T("End nurbsProc"));
}

/************************************************************************/
/* Process ellipArc data                                                */
/************************************************************************/
void OdGiConveyorGeometryDumper::ellipArcProc(const OdGeEllipArc3d& ellipArc,
	const OdGePoint3d* endPointsOverrides,
	OdGiArcType arcType,
	const OdGeVector3d* pExtrusion)
{
	//m_pDumper->output(OD_T("Start ellipArcProc"));
	/* //m_pDumper->pushIndent();
	//m_pDumper->output(OD_T("Center"),         toString(ellipArc.center()));
	//m_pDumper->output(OD_T("Normal"),         toString(ellipArc.normal()));
	//m_pDumper->output(OD_T("Major Radius"),   toString(ellipArc.majorRadius()));
	//m_pDumper->output(OD_T("Minor Radius"),   toString(ellipArc.minorRadius()));
	//m_pDumper->output(OD_T("Major Axis"),     toString(ellipArc.majorAxis()));
	//m_pDumper->output(OD_T("Minor Axis"),     toString(ellipArc.minorAxis()));
	//m_pDumper->output(OD_T("Start Angle"),    toDegreeString(ellipArc.startAng()));
	//m_pDumper->output(OD_T("End Angle"),      toDegreeString(ellipArc.endAng()));

	if (endPointsOverrides)
	{
	//m_pDumper->output(OD_T("Start point override"), toString(endPointsOverrides[0]));
	//m_pDumper->output(OD_T("End point override"), toString(endPointsOverrides[1]));
	}

	//m_pDumper->output(OD_T("arcType"),          toString(arcType));
	//m_pDumper->output(OD_T("Extrusion vector"), toString(pExtrusion));

	if (m_dumpLevel == Maximal_Simplification)
	{
	//m_pDumper->output(OD_T("Reduced ellipArc data"));
	//m_pDumper->pushIndent();
	OdGiGeometrySimplifier::ellipArcProc(ellipArc, endPointsOverrides, arcType, pExtrusion);
	//m_pDumper->popIndent();
	}
	//m_pDumper->popIndent();*/
	//m_pDumper->output(OD_T("End ellipArcProc"));
}

/************************************************************************/
/* Process rasterImage data                                             */
/************************************************************************/
void OdGiConveyorGeometryDumper::rasterImageProc(const OdGePoint3d& origin,
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
	//m_pDumper->output(OD_T("Start rasterImageProc"));
	/*//m_pDumper->pushIndent();

	//m_pDumper->output(OD_T("origin"), toString(origin));
	//m_pDumper->output(OD_T("u"), toString(u));
	//m_pDumper->output(OD_T("v"), toString(v));
	//m_pDumper->output(OD_T("transparency"), toString(transparency));
	//m_pDumper->output(OD_T("brightness"), toString(brightness));
	//m_pDumper->output(OD_T("contrast"), toString(contrast));
	//m_pDumper->output(OD_T("fade"), toString(fade));

	//m_pDumper->output(OD_T("uvBoundary"));
	//m_pDumper->pushIndent();
	for (OdUInt32 i = 0; i < numBoundPts; i++)
	{
	//m_pDumper->output(toString(OD_T("Vertex[%d]"), (int) i), toString(uvBoundary[i]));
	}
	//m_pDumper->popIndent();

	//m_pDumper->popIndent();*/
	//m_pDumper->output(OD_T("End rasterImageProc"));
}

// TD does not call this method in 1.13
/************************************************************************/
/* Process metafile data                                                */
/************************************************************************/
void OdGiConveyorGeometryDumper::metafileProc(const OdGePoint3d& origin,
	const OdGeVector3d& u,
	const OdGeVector3d& v,
	const OdGiMetafile* /*pMetafile*/,
	bool dcAligned,
	bool allowClipping)
{
	//m_pDumper->output(OD_T("Start metafileProc"));
	/* //m_pDumper->pushIndent();
	//m_pDumper->output(OD_T("origin"), toString(origin));
	//m_pDumper->output(OD_T("u"), toString(u));
	//m_pDumper->output(OD_T("v"), toString(v));
	//m_pDumper->output(OD_T("dcAligned"), toString(dcAligned));
	//m_pDumper->output(OD_T("allowClipping"), toString(allowClipping));
	//m_pDumper->popIndent();*/
	//m_pDumper->output(OD_T("End metafileProc"));
}

/************************************************************************/
/* Process polypoint data                                                */
/************************************************************************/
void OdGiConveyorGeometryDumper::polypointProc(OdInt32 numPoints,
	const OdGePoint3d* vertexList,
	const OdCmEntityColor* pColors,
	const OdCmTransparency* pTransparency,
	const OdGeVector3d* pNormals,
	const OdGeVector3d* pExtrusions,
	const OdGsMarker* pSubEntMarkers,
	OdInt32 nPointSize)
{
	//m_pDumper->output(OD_T("Start polypointProc"));
	/* //m_pDumper->pushIndent();
	//m_pDumper->output(OD_T("numPoints"),        toString((int)numPoints));
	//m_pDumper->output(numPoints,                vertexList);
	//m_pDumper->output(OD_T("Colors"),           toString((pColors) ? (int)numPoints : 0));
	if (pColors)
	{
	//m_pDumper->pushIndent();
	for (OdInt32 i = 0; i < numPoints; ++i)
	//m_pDumper->output(toString(OD_T("Color[%d]"), (int)i), toString(pColors[i]));
	//m_pDumper->popIndent();
	}
	//m_pDumper->output(OD_T("Transparency"),     toString((pTransparency) ? (int)numPoints : 0));
	if (pTransparency)
	{
	//m_pDumper->pushIndent();
	for (OdInt32 i = 0; i < numPoints; ++i)
	//m_pDumper->output(toString(OD_T("Transparency[%d]"), (int)i), toString(pTransparency[i]));
	//m_pDumper->popIndent();
	}
	//m_pDumper->output(OD_T("Normals"),          toString((pNormals) ? (int)numPoints : 0));
	if (pNormals)
	{
	//m_pDumper->pushIndent();
	for (OdInt32 i = 0; i < numPoints; ++i)
	//m_pDumper->output(toString(OD_T("Normal[%d]"), (int)i), toString(pNormals[i]));
	//m_pDumper->popIndent();
	}
	//m_pDumper->output(OD_T("Extrusions"),       toString((pExtrusions) ? (int)numPoints : 0));
	if (pExtrusions)
	{
	//m_pDumper->pushIndent();
	for (OdInt32 i = 0; i < numPoints; ++i)
	//m_pDumper->output(toString(OD_T("Extrusion[%d]"), (int)i), toString(pExtrusions[i]));
	//m_pDumper->popIndent();
	}
	//m_pDumper->output(OD_T("SubEntMarkers"),    toString((pSubEntMarkers) ? (int)numPoints : 0));
	if (pSubEntMarkers)
	{
	//m_pDumper->pushIndent();
	for (OdInt32 i = 0; i < numPoints; ++i)
	//m_pDumper->output(toString(OD_T("SubEntMarker[%d]"), (int)i), toString((int)pSubEntMarkers[i]));
	//m_pDumper->popIndent();
	}
	//m_pDumper->output(OD_T("PointSize"),        toString((int)nPointSize));
	if (m_dumpLevel == Maximal_Simplification)
	{
	//m_pDumper->output(OD_T("Reduced polypoint data"));
	//m_pDumper->pushIndent();
	OdGiGeometrySimplifier::polypointProc(numPoints, vertexList, pColors, pTransparency, pNormals, pExtrusions, pSubEntMarkers, nPointSize);
	//m_pDumper->popIndent();
	}
	//m_pDumper->popIndent();*/
	//m_pDumper->output(OD_T("End polypointProc"));
}

/************************************************************************/
/* Process row of dots data                                             */
/************************************************************************/
void OdGiConveyorGeometryDumper::rowOfDotsProc(OdInt32 numPoints,
	const OdGePoint3d& startPoint,
	const OdGeVector3d& dirToNextPoint)
{
	//m_pDumper->output(OD_T("Start rowOfDotsProc"));
	/*//m_pDumper->pushIndent();
	//m_pDumper->output(OD_T("numPoints"),  toString((int)numPoints));
	//m_pDumper->output(OD_T("startPoint"), toString(startPoint));
	//m_pDumper->output(OD_T("dirToNext"),  toString(dirToNextPoint));
	//m_pDumper->popIndent();*/
	//m_pDumper->output(OD_T("End rowOfDotsProc"));
}

/************************************************************************/
/* Process edges data                                                   */
/************************************************************************/
void OdGiConveyorGeometryDumper::edgeProc(const OdGiEdge2dArray& edges,
	const OdGeMatrix3d* pXform)
{
	//m_pDumper->output(OD_T("Start edgeProc"));
	/* //m_pDumper->pushIndent();
	//m_pDumper->output(OD_T("numCurves"), toString((int)edges.size()));
	//m_pDumper->output(OD_T("Xform"),     pXform);
	if (m_dumpLevel == Maximal_Simplification)
	{
	//m_pDumper->output(OD_T("Reduced edge data"));
	//m_pDumper->pushIndent();
	OdGiGeometrySimplifier::edgeProc(edges, pXform);
	//m_pDumper->popIndent();
	}
	//m_pDumper->popIndent();*/
	//m_pDumper->output(OD_T("End edgeProc"));
}
