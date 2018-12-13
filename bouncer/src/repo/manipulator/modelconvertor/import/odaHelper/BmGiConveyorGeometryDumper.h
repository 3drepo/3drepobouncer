///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2002-2018, Open Design Alliance (the "Alliance").
// All rights reserved.
//
// This software and its documentation and related materials are owned by
// the Alliance. The software may only be incorporated into application
// programs owned by members of the Alliance, subject to a signed
// Membership Agreement and Supplemental Software License Agreement with the
// Alliance. The structure and organization of this software are the valuable
// trade secrets of the Alliance and its suppliers. The software is also
// protected by copyright law and international treaty provisions. Application
// programs incorporating this software must include the following statement
// with their copyright notices:
//
//   This application incorporates Teigha(R) software pursuant to a license
//   agreement with Open Design Alliance.
//   Teigha(R) Copyright (C) 2002-2018 by Open Design Alliance.
//   All rights reserved.
//
// By use of this software, its documentation or related materials, you
// acknowledge and accept the above terms.
///////////////////////////////////////////////////////////////////////////////

#ifndef __ODBM_GI_CONVEYOR_GEOMETRY_DUMPER__
#define __ODBM_GI_CONVEYOR_GEOMETRY_DUMPER__

#include "SharedPtr.h"
#include "Gi/GiGeometrySimplifier.h"
#include "BmGiDumper.h"
#include "geometry_collector.h"

using namespace repo;
using namespace manipulator;
using namespace modelconvertor;
using namespace odaHelper;

class OdGiConveyorGeometryDumper : public OdGiGeometrySimplifier
{
  OdGiDumperPtr m_pDumper;
  OdGiConveyorGeometryDumper();
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

  static OdSharedPtr<OdGiConveyorGeometryDumper> createObject(OdGiDumper* m_pDumper, GeometryCollector* collector);


  /**********************************************************************/
  /* The overrides of OdGiConveyorGeometry.  Each view sends            */
  /* its vectorized entity data to these functions, due to the          */
  /* link established between the view and the device in                */
  /* ExGsSimpleDevice::createView().                                    */
  /**********************************************************************/
  
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
  
  /**********************************************************************/
  /* The client's version of this function will not be called in        */
  /* the current version of Teigha                                      */
  /**********************************************************************/
  void shapeProc(const OdGePoint3d& position,
    const OdGeVector3d& direction, const OdGeVector3d& upVector,
    int shapeNumber, const OdGiTextStyle* pTextStyle,
    const OdGeVector3d* pExtrusion = 0);

  /**********************************************************************/
  /* The client's version of this function will not be called in        */
  /* the current version of Teigha                                      */
  /**********************************************************************/
  void xlineProc(const OdGePoint3d& firstPoint, const OdGePoint3d& secondPoint);
  
  /**********************************************************************/
  /* The client's version of this function will not be called in        */
  /* the current version of Teigha                                      */
  /**********************************************************************/
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
    const OdGiRasterImage* pImage, // image object
    const OdGePoint2d* uvBoundary, // may not be null
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
    bool dcAligned = true,       // reserved
    bool allowClipping = false); // reserved

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

typedef OdSharedPtr<OdGiConveyorGeometryDumper> OdGiConveyorGeometryDumperPtr;

#endif // __ODBM_GI_CONVEYOR_GEOMETRY_DUMPER__
