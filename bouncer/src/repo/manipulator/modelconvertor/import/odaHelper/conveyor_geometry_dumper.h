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

#pragma once
#include "SharedPtr.h"
#include "Gi/GiGeometrySimplifier.h"
#include "geometry_collector.h"

using namespace repo;
using namespace manipulator;
using namespace modelconvertor;
using namespace odaHelper;

class ConveyorGeometryDumper : public OdGiGeometrySimplifier
{
  ConveyorGeometryDumper();
  GeometryCollector* geometryCollector;

  int m_dumpLevel;
public:
  enum 
  {
    Maximal_Simplification,
    Minimal_Simplification
  };

  void setDumpLevel(int dumpLevel) { m_dumpLevel = dumpLevel; }

  int dumpLevel() const { return m_dumpLevel; }

  static OdSharedPtr<ConveyorGeometryDumper> createObject(GeometryCollector* collector);
  
  void plineProc(const OdGiPolyline& lwBuf,
                 const OdGeMatrix3d* pXform,
                 OdUInt32 fromIndex,
                 OdUInt32 numSegs);

  void polylineOut(OdInt32 numPoints, const OdGePoint3d* vertexList);

  void polygonOut(OdInt32 numPoints, const OdGePoint3d* vertexList, const OdGeVector3d* pNormal = 0);

  void circleProc(const OdGePoint3d& center, 
    double radius, 
    const OdGeVector3d& normal, 
    const OdGeVector3d* pExtrusion = 0);
  
  void circleProc(const OdGePoint3d& firstPoint, 
    const OdGePoint3d& secondPoint, 
    const OdGePoint3d& thirdPoint, 
    const OdGeVector3d* pExtrusion = 0);
  
  void circularArcProc(const OdGePoint3d& center,
    double radius,
    const OdGeVector3d& normal,
    const OdGeVector3d& startVector,
    double sweepAngle,
    OdGiArcType arcType = kOdGiArcSimple, const OdGeVector3d* pExtrusion = 0);
  
  void circularArcProc(const OdGePoint3d& firstPoint,
    const OdGePoint3d& secondPoint,
    const OdGePoint3d& thirdPoint,
    OdGiArcType arcType = kOdGiArcSimple, const OdGeVector3d* pExtrusion = 0);
  
  void polylineProc(OdInt32 numPoints, const OdGePoint3d* vertexList,
    const OdGeVector3d* pNormal = 0,
    const OdGeVector3d* pExtrusion = 0, OdGsMarker baseSubEntMarker = -1);
  
  void polygonProc(OdInt32 nbPoints, const OdGePoint3d* pVertexList,
    const OdGeVector3d* pNormal = 0,
    const OdGeVector3d* pExtrusion = 0);

  void meshProc(OdInt32 rows,
    OdInt32 columns,
    const OdGePoint3d* pVertexList,
    const OdGiEdgeData* pEdgeData = 0,
    const OdGiFaceData* pFaceData = 0,
    const OdGiVertexData* pVertexData = 0);
  
  void shellProc(OdInt32 numVertices,
    const OdGePoint3d* vertexList,
    OdInt32 faceListSize,
    const OdInt32* faceList,
    const OdGiEdgeData* pEdgeData = 0,
    const OdGiFaceData* pFaceData = 0,
    const OdGiVertexData* pVertexData = 0);
  
  void textProc(const OdGePoint3d& position,
    const OdGeVector3d& direction, const OdGeVector3d& upVector,
    const OdChar* msg, OdInt32 numBytes, bool raw, const OdGiTextStyle* pTextStyle,
    const OdGeVector3d* pExtrusion = 0);
  
  void shapeProc(const OdGePoint3d& position,
    const OdGeVector3d& direction, const OdGeVector3d& upVector,
    int shapeNumber, const OdGiTextStyle* pTextStyle,
    const OdGeVector3d* pExtrusion = 0);

  void xlineProc(const OdGePoint3d& firstPoint, const OdGePoint3d& secondPoint);
  
  void rayProc(const OdGePoint3d& basePoint, const OdGePoint3d& throughPoint);
  
  void nurbsProc(const OdGeNurbCurve3d& nurbs);

  void ellipArcProc(const OdGeEllipArc3d& ellipArc,
    const OdGePoint3d* endPointsOverrides,
    OdGiArcType arcType = kOdGiArcSimple, 
    const OdGeVector3d* pExtrusion = 0);

  void rasterImageProc(
    const OdGePoint3d& origin,
    const OdGeVector3d& u,
    const OdGeVector3d& v,
    const OdGiRasterImage* pImage,
    const OdGePoint2d* uvBoundary,
    OdUInt32 numBoundPts,
    bool transparency = false,
    double brightness = 50.0,
    double contrast = 50.0,
    double fade = 0.0);

  void metafileProc(
    const OdGePoint3d& origin,
    const OdGeVector3d& u,
    const OdGeVector3d& v,
    const OdGiMetafile* pMetafile,
    bool dcAligned = true,
    bool allowClipping = false);

  void polypointProc(
    OdInt32 numPoints,
    const OdGePoint3d* vertexList,
    const OdCmEntityColor* pColors,
    const OdCmTransparency* pTransparency,
    const OdGeVector3d* pNormals = 0,
    const OdGeVector3d* pExtrusions = 0,
    const OdGsMarker* pSubEntMarkers = 0,
    OdInt32 nPointSize = 0);

  void rowOfDotsProc(
    OdInt32 numPoints,
    const OdGePoint3d& startPoint,
    const OdGeVector3d& dirToNextPoint);

  void edgeProc(
    const OdGiEdge2dArray& edges,
    const OdGeMatrix3d* pXform = 0);

  TD_USING(OdGiGeometrySimplifier::polylineOut);
};

typedef OdSharedPtr<ConveyorGeometryDumper> ConveyorGeometryDumperPtr;

