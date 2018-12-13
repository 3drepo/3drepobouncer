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

#pragma once
#ifndef __ODBM_GI_DUMPER_IMPL__
#define __ODBM_GI_DUMPER_IMPL__

//#include "Common/toString.h"
#include "RxObjectImpl.h"
#include "BmGiConveyorGeometryDumper.h"

#define STL_USING_IOSTREAM
#include "OdaSTL.h"

class OdGiDumperImpl : public OdGiDumper
{
  int   m_indentLevel;
  GeometryCollector* gcollector;
public:
  static OdGiDumperPtr createObject();

  OdGiDumperImpl(); // it isn't possible to create object using this constructor, use 
                    // above createObject method for such purpose
  virtual ~OdGiDumperImpl();

  virtual void output(const OdString& label, const OdGeMatrix3d* xfm);
  virtual void output(const OdString& str);
  virtual void output(const OdString& str1, const OdString& str2);
  virtual void output(OdInt32 nPoints, const OdGePoint3d* points);
  virtual void output(const OdGiTextStyle& textStyle);
  virtual void outputEdgeData(const OdGiEdgeData* pEdgeData, OdInt32 numEdges);
  virtual void outputFaceData(const OdGiFaceData* pFaceData, OdInt32 numFaces);
  virtual void outputVertexData(const OdGiVertexData* pVertexData, OdInt32 numVerts);
  virtual void outputColors(const OdUInt16* colors, OdInt32 numColors, const OdString& name);
  virtual void outputTrueColors(const OdCmEntityColor* trueColors, OdInt32 numColors, const OdString& name);
  virtual void outputIds(OdDbStub** ids, OdInt32 numIds, const OdString& name, const OdString& table);
  virtual void outputSelectionMarkers(const OdGsMarker* selectionMarkers, OdInt32 numMarkers, const OdString& name);
  virtual void outputVisibility(const OdUInt8* visibility, OdInt32 count, const OdString& name);
  virtual void outputMappers(const OdGiMapper* mappers, OdInt32 count, const OdString& name);
  virtual void outputTransparencies(const OdCmTransparency* transparencies, OdInt32 numColors, const OdString& name);

  virtual void pushIndent() { m_indentLevel += 2; }
  virtual void popIndent() { m_indentLevel -= 2; }
  virtual int  currIndent() {return m_indentLevel;}
};

#endif // __ODBM_GI_DUMPER_IMPL__
