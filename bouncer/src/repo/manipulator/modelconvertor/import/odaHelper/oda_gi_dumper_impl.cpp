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

#include "oda_gi_dumper_impl.h"

#include <OdaCommon.h>
#include <OdString.h>
#include <CmColor.h>
#include <DbObjectId.h>
#include <Gi/GiTextStyle.h>
#include <toString.h>
#include <DgElementId.h>
#include <DgElement.h>
#include <DgTableRecord.h>
#include <ExDgnHostAppServices.h>
using namespace repo::manipulator::modelconvertor::odaHelper;

OdArray<OdDgStlTriangleFace> OdaGiDumper::m_arrTriangles;

// Auxiliary function to display different messages
//void oddgPrintConsoleString(const wchar_t* fmt, ...);

/************************************************************************/
/* Convert the specified value to an OdDgElementId string               */
/************************************************************************/
//OdString toString(const OdDgElementId& val)
//{
//  if (val.isNull()) 
//  {
//    return OD_T("Null");
//  }
//
//  /**********************************************************************/
//  /* Open the object                                                    */
//  /**********************************************************************/
//  OdDgElementPtr pElm = val.safeOpenObject();
//
//  /**********************************************************************/
//  /* Return the name of an OdDgTableRecord                              */
//  /**********************************************************************/
//  if (pElm->isKindOf(OdDgTableRecord::desc()))
//  {
//    OdDgTableRecordPtr pRec = pElm;
//    return pRec->getName(); 
//  }
//
//  /**********************************************************************/
//  /* We don't know what it is, so return the description of the object  */
//  /* object specified by the ObjectId                                   */
//  /**********************************************************************/
//  return toString(pElm->isA());
//}


#define STL_USING_STRING
#define STL_USING_IOSTREAM
#include "OdaSTL.h"

OdaGiDumperImpl::OdaGiDumperImpl()
	: m_indentLevel(0)
{
}

OdaGiDumperImpl::~OdaGiDumperImpl()
{
}

OdaGiDumperPtr OdaGiDumperImpl::createObject()
{
	return OdRxObjectImpl<OdaGiDumperImpl, OdaGiDumper>::createObject();;
}

/************************************************************************/
/* Output a string in the form                                          */
/*   str1:. . . . . . . . . . . .str2                                   */
/************************************************************************/
void OdaGiDumperImpl::output(const OdString& str1,
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
		int leaders = colWidth - (str1.getLength() + m_indentLevel);
		if (leaders > 0) {
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
void OdaGiDumperImpl::output(const OdString& label,
	const OdGeMatrix3d* xfm)
{
	if (xfm)
	{
		for (int i = 0; i < 4; i++)
		{
			OdString leftString = (i) ? OD_T("") : label.c_str();
			OdString rightString;
			rightString = OD_T("[");
			for (int j = 0; j < 4; j++)
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
void OdaGiDumperImpl::output(const OdString& str)
{
	OdString indent;
	for (int i = 0; i < m_indentLevel; i++)
		indent += ' ';
	//std::cout << indent << str << std::endl;
	//oddgPrintConsoleString(L"%ls%ls\n", indent.c_str(), str.c_str());
}
/************************************************************************/
/* Output vertices                                                      */
/************************************************************************/
void OdaGiDumperImpl::output(OdInt32 nPoints,
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
void OdaGiDumperImpl::output(const OdGiTextStyle& textStyle)
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
		output(OD_T("Typeface"), toString(typeface));
		output(OD_T("Character Set"), toString(charset));
		output(OD_T("Bold"), toString(bold));
		output(OD_T("Italic"), toString(italic));
		output(OD_T("Font Pitch and Family"), toHexString(pitchAndFamily));
	}
	else
	{
		output(OD_T("Filename"), shortenPath(textStyle.ttfdescriptor().fileName()));
		output(OD_T("BigFont Filename"), shortenPath(textStyle.bigFontFileName()));
	}
	output(OD_T("Shape File"), toString(textStyle.isShape()));
	output(OD_T("Text Height"), toString(textStyle.textSize()));
	output(OD_T("Width Factor"), toString(textStyle.xScale()));
	output(OD_T("Obliquing Angle"), toDegreeString(textStyle.obliquingAngle()));
	output(OD_T("Tracking Percentage"), toDegreeString(textStyle.trackingPercent()));
	output(OD_T("Backwards"), toString(textStyle.isBackward()));
	output(OD_T("Vertical"), toString(textStyle.isVertical()));
	output(OD_T("Upside Down"), toString(textStyle.isUpsideDown()));
	output(OD_T("Underlined"), toString(textStyle.isUnderlined()));
	output(OD_T("Overlined"), toString(textStyle.isOverlined()));

	popIndent();
}
/************************************************************************/
/* Output ACI colors                                                    */
/************************************************************************/
void OdaGiDumperImpl::outputColors(const OdUInt16* colors,
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
void OdaGiDumperImpl::outputTrueColors(const OdCmEntityColor* trueColors,
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
			OdDgCmEntityColor dgColor = trueColors[i];
			output(toString(name + OD_T("[%d]"), (int)i), toString(dgColor));
		}
		popIndent();
	}
}

/************************************************************************/
/* Output object ids                                                    */
/************************************************************************/
void OdaGiDumperImpl::outputIds(OdDbStub** ids,
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
			OdDgElementId id(ids[i]);
			output(toString(name + OD_T("[%d]"), (int)i), toString(id));
		}
		popIndent();
	}
}
/************************************************************************/
/* Output selection markers                                             */
/************************************************************************/
void OdaGiDumperImpl::outputSelectionMarkers(const OdGsMarker* selectionMarkers,
	OdInt32 numMarkers,
	const OdString& name)
{
	if (selectionMarkers)
	{
		output(name + OD_T(" Selection Markers"));
		pushIndent();
		OdInt32 i;
		for (i = 0; i < numMarkers; i++)
		{
			output(toString(OdString(name) + OD_T("[%d]"), (int)i), toString((int)selectionMarkers[i]));
		}
	}
}

/************************************************************************/
/* Output visibility                                                    */
/************************************************************************/
void OdaGiDumperImpl::outputVisibility(const OdUInt8* visibility,
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
			OdString vis(OD_T("???"));
			switch (visibility[i])
			{
			case kOdGiInvisible: vis = OD_T("kOdGiInvisible");   break;
			case kOdGiVisible: vis = OD_T("kOdGiVisible");     break;
			case kOdGiSilhouette: vis = OD_T("kOdGiSilhouette");  break;
			}
			output(toString(name + OD_T("[%d]"), (int)i), vis);
		}
		popIndent();
	}
}

/************************************************************************/
/* Output Face Data                                                     */
/************************************************************************/
void OdaGiDumperImpl::outputFaceData(const OdGiFaceData* pFaceData,
	OdInt32 numFaces)
{
	if (pFaceData)
	{
		outputColors(pFaceData->colors(), numFaces, OD_T("Face"));
		outputTrueColors(pFaceData->trueColors(), numFaces, OD_T("Face"));
		outputIds(pFaceData->layerIds(), numFaces, OD_T("Face"), OD_T("Layers"));
		outputSelectionMarkers(pFaceData->selectionMarkers(), numFaces, OD_T("Face"));
		outputVisibility(pFaceData->visibility(), numFaces, OD_T("Face"));
	}
}
/************************************************************************/
/* Output edge data                                                     */
/************************************************************************/
void OdaGiDumperImpl::outputEdgeData(const OdGiEdgeData* pEdgeData,
	OdInt32 numEdges)
{
	if (pEdgeData)
	{
		outputColors(pEdgeData->colors(), numEdges, OD_T("Edge"));
		outputTrueColors(pEdgeData->trueColors(), numEdges, OD_T("Edge"));
		outputIds(pEdgeData->layerIds(), numEdges, OD_T("Edge"), OD_T("Layers"));
		outputIds(pEdgeData->linetypeIds(), numEdges, OD_T("Edge"), OD_T("Linetypes"));
		outputSelectionMarkers(pEdgeData->selectionMarkers(), numEdges, OD_T("Edge"));
		outputVisibility(pEdgeData->visibility(), numEdges, OD_T("Edge"));
	}
}

/************************************************************************/
/* Output Vertex Data                                                   */
/************************************************************************/
void OdaGiDumperImpl::outputVertexData(const OdGiVertexData* pVertexData,
	OdInt32 numVerts)
{
	if (pVertexData)
	{
		const OdGeVector3d* normals = pVertexData->normals();
		if (normals)
		{
			OdString orientation(OD_T("???"));
			switch (pVertexData->orientationFlag())
			{
			case kOdGiCounterClockwise: orientation = OD_T("kOdGiCounterClockwise ");  break;
			case kOdGiNoOrientation: orientation = OD_T("kOdGiNoOrientation");      break;
			case kOdGiClockwise: orientation = OD_T("kOdGiClockwise");          break;
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

/************************************************************************/
/* Output Triangles to STL file                                         */
/************************************************************************/

void OdaGiDumper::addStlTriangle(const OdDgStlTriangleFace& triangleData)
{
	m_arrTriangles.push_back(triangleData);
}

OdArray<OdDgStlTriangleFace> OdaGiDumper::getStlTriangles()
{
	return m_arrTriangles;
}

void OdaGiDumper::clearStlTriangles()
{
	m_arrTriangles.clear();
}

void OdaGiDumper::writeFaceDataToStlFile(const OdString& strFileName,
	const OdArray<OdDgStlTriangleFace>& arrTriangles)
{
	if (!arrTriangles.size())
		return;

	FILE *file;

	OdAnsiString strFileNameAnsi(strFileName);

	if ((file = fopen(strFileNameAnsi.c_str(), "wb")) == NULL)
		return;

	char header[80 + 1];
	unsigned long triLength = arrTriangles.size();
	unsigned short ibuff2 = 0;

	memset(header, 32, 80);
	sprintf(header, "%s", "Vectoriesed data Output.\n");

	fwrite(header, 1, 80, file);

	fwrite(&triLength, 1, 4, file);

	for (OdUInt32 i = 0; i < arrTriangles.size(); i++)
	{
		OdGeVector3d vr1 = arrTriangles[i].m_pt2 - arrTriangles[i].m_pt1;
		OdGeVector3d vr2 = arrTriangles[i].m_pt3 - arrTriangles[i].m_pt1;

		OdGeVector3d vrNormal = OdGeVector3d::kZAxis;

		if (!vr1.isZeroLength() && !vr2.isZeroLength() && vr1.isParallelTo(vr2))
		{
			vrNormal = vr1.crossProduct(vr2);

			if (!vrNormal.isZeroLength())
				vrNormal.normalize();
		}

		float n[3];

		n[0] = vrNormal.x;  n[1] = vrNormal.y;  n[2] = vrNormal.z;
		fwrite(n, 4, 3, file);
		n[0] = arrTriangles[i].m_pt1.x;  n[1] = arrTriangles[i].m_pt1.y;  n[2] = arrTriangles[i].m_pt1.z;
		fwrite(n, 4, 3, file);
		n[0] = arrTriangles[i].m_pt2.x;  n[1] = arrTriangles[i].m_pt2.y;  n[2] = arrTriangles[i].m_pt2.z;
		fwrite(n, 4, 3, file);
		n[0] = arrTriangles[i].m_pt3.x;  n[1] = arrTriangles[i].m_pt3.y;  n[2] = arrTriangles[i].m_pt3.z;
		fwrite(n, 4, 3, file);

		fwrite(&ibuff2, 2, 1, file);
	}

	fclose(file);
}
