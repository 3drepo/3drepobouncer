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


#include "BimCommon.h"
#include "OdString.h"
#include "CmColorBase.h"
#include "Base/BmObjectId.h"
#include "BmGiDumperImpl.h"
#include "Gi/GiTextStyle.h"
#include "Common/toString.h"
#include "Ge/GeQuaternion.h"
#define STL_USING_STRING
#define STL_USING_IOSTREAM
#include "OdaSTL.h"

OdGiDumperImpl::OdGiDumperImpl()
: m_indentLevel(0)
{
}

OdGiDumperImpl::~OdGiDumperImpl()
{
}

OdGiDumperPtr OdGiDumperImpl::createObject()
{
  auto s = OdRxObjectImpl<OdGiDumperImpl, OdGiDumper>::createObject();
  return s;
}

/************************************************************************/
/* Output a string in the form                                          */
/*   str1:. . . . . . . . . . . .str2                                   */
/************************************************************************/
void OdGiDumperImpl::output(const OdString& str1, 
                            const OdString& str2)
{
  const OdString spaces(OD_T("                                                            "));
  const OdString leader(OD_T(". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . "));

  OdString buffer;

  const int colWidth = 38;

  buffer = str1;
  /**********************************************************************/
  /* If rightString is not specified, just output the indented          */
  /* leftString. Otherwise, fill the space between leftString and       */
  /* rightString with leader characters.                                */
  /**********************************************************************/  
  if (str2 != OD_T(""))
  {
    int leaders = colWidth-(str1.getLength() + m_indentLevel);
    if (leaders > 0){
      buffer = str1 + leader.mid(str1.getLength() + m_indentLevel, leaders) + str2;  
    }
    else
    {
      buffer = OD_T(">") + str1 + OD_T(" ") + str2;  
    }
  }
  output(buffer);
}
/************************************************************************/
/* Output a 3d transformation matrix                                    */
/************************************************************************/
void OdGiDumperImpl::output(const OdString& label, 
                            const OdGeMatrix3d* xfm)
{
  if (xfm)
  {
    for (int i = 0; i < 4; i++)
    {
      OdString leftString = (i) ? OD_T("") : label.c_str();
      OdString rightString;
      rightString = OD_T("[");
      for (int j = 0; j < 4 ; j++)
      {
        if (j)
        {
          rightString += OD_T(" ");
        }
        rightString += toString(*xfm[i][j]);
      }
      rightString += OD_T("]");
      output(leftString, rightString);
    }
  }
  else
  {
    output(label, OD_T("[]"));
  }
}
/************************************************************************/
/* Output and indent a character string                                 */
/************************************************************************/
void OdGiDumperImpl::output(const OdString& str)
{
  OdString indent;
  for (int i = 0; i < m_indentLevel; i++)
    indent += ' ';
  //odPrintConsoleString(L"%ls%ls\n", indent.c_str(), str.c_str());
}
/************************************************************************/
/* Output vertices                                                      */
/************************************************************************/
void OdGiDumperImpl::output(OdInt32 nPoints, 
                            const OdGePoint3d* points)
{
  pushIndent();
  for (OdInt32 i = 0; i < nPoints; ++i)
  {
    output(toString(OD_T("Vertex[%d]"), (int)i), toString(points[i]));
  }
  popIndent();
}
/************************************************************************/
/* Output a text style                                                  */
/************************************************************************/
void OdGiDumperImpl::output(const OdGiTextStyle& textStyle)
{
  output(OD_T("Text Style"));
  pushIndent();
  if (textStyle.isTtfFont())
  {
    OdString typeface;
    bool bold;
    bool italic;
    int charset;
    int pitchAndFamily;
    textStyle.font(typeface, bold, italic, charset, pitchAndFamily);
    output(OD_T("Typeface"),                toString(typeface));
    output(OD_T("Character Set"),           toString(charset));
    output(OD_T("Bold"),                    toString(bold));
    output(OD_T("Italic"),                  toString(italic));
    output(OD_T("Font Pitch and Family"),   toHexString(pitchAndFamily));
  }
  else
  {
    output(OD_T("Filename"),                shortenPath(textStyle.ttfdescriptor().fileName()));
    output(OD_T("BigFont Filename"),        shortenPath(textStyle.bigFontFileName()));
  }
  output(OD_T("Shape File"),                toString(textStyle.isShape()));
  output(OD_T("Text Height"),               toString(textStyle.textSize()));
  output(OD_T("Width Factor"),              toString(textStyle.xScale()));
  output(OD_T("Obliquing Angle"),           toDegreeString(textStyle.obliquingAngle()));
  output(OD_T("Tracking Percentage"),       toDegreeString(textStyle.trackingPercent()));
  output(OD_T("Backwards"),                 toString(textStyle.isBackward()));
  output(OD_T("Vertical"),                  toString(textStyle.isVertical()));
  output(OD_T("Upside Down"),               toString(textStyle.isUpsideDown()));
  output(OD_T("Underlined"),                toString(textStyle.isUnderlined()));
  output(OD_T("Overlined"),                 toString(textStyle.isOverlined()));

  popIndent();
}
/************************************************************************/
/* Output ACI colors                                                    */
/************************************************************************/
void OdGiDumperImpl::outputColors(const OdUInt16* colors, 
                                  OdInt32 numColors, 
                                  const OdString& name)
{
  if (colors)
  {
    output(OdString(name) + OD_T(" Colors"));
    pushIndent();
    OdInt32 i;
    for (i = 0; i < numColors; i++)
    {
      output(toString(name + OD_T("[%d]"), (int)i), toString(OD_T("ACI %d"), colors[i]));
    }
    popIndent();
  }
}
/************************************************************************/
/* Output true colors                                                   */
/************************************************************************/
void OdGiDumperImpl::outputTrueColors(const OdCmEntityColor* trueColors, 
                                      OdInt32 numColors,
                                      const OdString& name)
{
  if (trueColors)
  {
    output(name + OD_T(" TrueColors"));
    pushIndent();
    OdInt32 i;
    for (i = 0; i < numColors; i++)
    {
      output(toString(name + OD_T("[%d]"), (int)i), toString(trueColors[i]));
    }
    popIndent();
  }
}
/************************************************************************/
/* Output transparencies                                                */
/************************************************************************/
void OdGiDumperImpl::outputTransparencies(const OdCmTransparency* transparencies, OdInt32 numColors, const OdString& name)
{
  if (transparencies)
  {
    output(name + OD_T(" Transparency"));
    pushIndent();
    OdInt32 i;
    for (i = 0; i < numColors; i++)
    {
      output(toString(name + OD_T("[%d]"), (int)i), toString(transparencies[i]));
    }
    popIndent();
  }
}

/************************************************************************/
/* Output object ids                                                    */
/************************************************************************/
void OdGiDumperImpl::outputIds(OdDbStub** ids, 
                               OdInt32 numIds, 
                               const OdString& name, 
                               const OdString& table)
{
  if (ids)
  {
    output(name + OD_T(" ") + table);
    pushIndent();
    OdInt32 i;
    for (i = 0; i < numIds; i++)
    {
      OdBmObjectId id(ids[i]);
      output(toString(name + OD_T("[%d]"), (int)i), toString(OdBmObjectId(id).getHandle()));
    }
    popIndent();
  }
}
/************************************************************************/
/* Output selection markers                                             */
/************************************************************************/
void OdGiDumperImpl::outputSelectionMarkers(const OdGsMarker* selectionMarkers, 
                                            OdInt32 numMarkers, 
                                            const OdString& name)
{
  if (selectionMarkers)
  {
    output(name + OD_T(" Selection Markers") );
    pushIndent();
    OdInt32 i;
    for (i = 0; i < numMarkers; i++)
    {
      output(toString(OdString(name) + OD_T("[%d]"), (int)i), toString((int)selectionMarkers[i]));
    }
    popIndent();
  }
}

/************************************************************************/
/* Output visibility                                                    */
/************************************************************************/
void OdGiDumperImpl::outputVisibility(const OdUInt8* visibility, 
                                      OdInt32 count, 
                                      const OdString& name)
{
  if (visibility)
  {
    output(name + OD_T(" Visibility"));
    pushIndent();
    OdInt32 i;
    for (i = 0; i < count; i++)
    {
      OdString vis (OD_T("???"));
      switch (visibility[i])
      {
      case kOdGiInvisible   : vis = OD_T("kOdGiInvisible");   break;
      case kOdGiVisible     : vis = OD_T("kOdGiVisible");     break;
      case kOdGiSilhouette  : vis = OD_T("kOdGiSilhouette");  break;
      }
      output(toString(name + OD_T("[%d]"), (int)i), vis);
    }
    popIndent();
  }
}

void OdGiDumperImpl::outputMappers(const OdGiMapper* mappers, OdInt32 count, const OdString& name)
{
  if (mappers)
  {
    output(name + OD_T(" Mappers"));
    pushIndent();
    OdInt32 i;
    for (i = 0; i < count; i++)
    {
      const OdGiMapper *pMapper = mappers + i;
      const OdGeMatrix3d &transform = pMapper->transform();
      OdString inName(toString(name + OD_T("[%d]"), (int)i));
      output(inName + OD_T(" projection"),    toString(pMapper->projection()));
      output(inName + OD_T(" uTiling"),       toString(pMapper->uTiling()));
      output(inName + OD_T(" vTiling"),       toString(pMapper->vTiling()));
      output(inName + OD_T(" autoTransform"), toString(pMapper->autoTransform()));
      output(inName + OD_T(" transform"),     toString(OdGeQuaternion(transform[0][0], transform[0][1], transform[0][2], transform[0][3])));
      output(inName + OD_T(" transform"),     toString(OdGeQuaternion(transform[1][0], transform[1][1], transform[1][2], transform[1][3])));
      output(inName + OD_T(" transform"),     toString(OdGeQuaternion(transform[2][0], transform[2][1], transform[2][2], transform[2][3])));
      output(inName + OD_T(" transform"),     toString(OdGeQuaternion(transform[3][0], transform[3][1], transform[3][2], transform[3][3])));
    }
    popIndent();
  }
}

/************************************************************************/
/* Output Face Data                                                     */
/************************************************************************/
void OdGiDumperImpl::outputFaceData(const OdGiFaceData* pFaceData, 
                                    OdInt32 numFaces)
{
  if (pFaceData)
  {
    outputColors(pFaceData->colors(),                     numFaces, OD_T("Face"));
    outputTrueColors(pFaceData->trueColors(),             numFaces, OD_T("Face"));
    outputIds(pFaceData->layerIds(),                      numFaces, OD_T("Face"), OD_T("Layers"));
    outputSelectionMarkers(pFaceData->selectionMarkers(), numFaces, OD_T("Face"));
    outputVisibility(pFaceData->visibility(),             numFaces, OD_T("Face"));
    outputIds(pFaceData->materials(),                     numFaces, OD_T("Face"), OD_T("Materials"));
    outputMappers(pFaceData->mappers(),                   numFaces, OD_T("Face"));
    outputTransparencies(pFaceData->transparency(),       numFaces, OD_T("Face"));
  }
}
/************************************************************************/
/* Output edge data                                                     */
/************************************************************************/
void OdGiDumperImpl::outputEdgeData(const OdGiEdgeData* pEdgeData, 
                                    OdInt32 numEdges)
{
  if (pEdgeData)
  {
    outputColors(pEdgeData->colors(),                     numEdges, OD_T("Edge"));
    outputTrueColors(pEdgeData->trueColors(),             numEdges, OD_T("Edge"));
    outputIds(pEdgeData->layerIds(),                      numEdges, OD_T("Edge"), OD_T("Layers"));
    outputIds(pEdgeData->linetypeIds(),                   numEdges, OD_T("Edge"), OD_T("Linetypes"));
    outputSelectionMarkers(pEdgeData->selectionMarkers(), numEdges, OD_T("Edge"));
    outputVisibility(pEdgeData->visibility(),             numEdges, OD_T("Edge"));
  }
}

/************************************************************************/
/* Output Vertex Data                                                   */
/************************************************************************/
void OdGiDumperImpl::outputVertexData(const OdGiVertexData* pVertexData, 
                                      OdInt32 numVerts)
{
  if (pVertexData)
  {
    const OdGeVector3d* normals = pVertexData->normals();
    if (normals)
    {
      OdString orientation (OD_T("???"));
      switch (pVertexData->orientationFlag())
      {
      case kOdGiCounterClockwise    : orientation = OD_T("kOdGiCounterClockwise ");  break;
      case kOdGiNoOrientation       : orientation = OD_T("kOdGiNoOrientation");      break;
      case kOdGiClockwise           : orientation = OD_T("kOdGiClockwise");          break;
      }
      output(OD_T("Vertex Orientation Flag"), orientation);

      output(OD_T("Vertex Normals"));
      pushIndent();
      for (OdInt32 i = 0; i < numVerts; ++i)
      {
        output(toString(OD_T("Vertex[%d]"), (int)i), toString(normals[i]));
      }
      popIndent();
    }
    outputTrueColors(pVertexData->trueColors(), numVerts, OD_T("Vertex"));
    if (pVertexData->mappingCoords(OdGiVertexData::kAllChannels))
    {
      const OdGePoint3d *pTexCoords = pVertexData->mappingCoords(OdGiVertexData::kAllChannels);
      output(OD_T("Vertex Texture Coordinates"));
      pushIndent();
      for (OdInt32 i = 0; i < numVerts; ++i)
        output(toString(OD_T("Vertex[%d]"), (int)i), toString(pTexCoords[i]));
      popIndent();
    }
  }
}
