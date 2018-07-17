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

#include <RxObject.h>
#include <Ge/GePoint3d.h>
#include <Gi/GiGeometry.h>

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				struct OdDgStlTriangleFace
				{
					OdGePoint3d  m_pt1;
					OdGePoint3d  m_pt2;
					OdGePoint3d  m_pt3;
				};

				class OdaGiDumper : public OdRxObject
				{
				public:
					virtual void output(const OdString& label, const OdGeMatrix3d* xfm) = 0;
					virtual void output(const OdString& str) = 0;
					virtual void output(const OdString& str1, const OdString& str2) = 0;
					virtual void output(OdInt32 nPoints, const OdGePoint3d* points) = 0;
					virtual void output(const OdGiTextStyle& textStyle) = 0;
					virtual void outputEdgeData(const OdGiEdgeData* pEdgeData, OdInt32 count) = 0;
					virtual void outputFaceData(const OdGiFaceData* pFaceData, OdInt32 numFaces) = 0;
					virtual void outputVertexData(const OdGiVertexData* pVertexData, OdInt32 count) = 0;
					virtual void outputColors(const OdUInt16* colors, OdInt32 count, const OdString& name) = 0;
					virtual void outputTrueColors(const OdCmEntityColor* trueColors, OdInt32 count, const OdString& name) = 0;
					virtual void outputIds(OdDbStub** ids, OdInt32 count, const OdString& name, const OdString& table) = 0;
					virtual void outputSelectionMarkers(const OdGsMarker* selectionMarkers, OdInt32 count, const OdString& name) = 0;
					virtual void outputVisibility(const OdUInt8* visibility, OdInt32 count, const OdString& name) = 0;

					virtual void pushIndent() = 0;
					virtual void popIndent() = 0;

					static void writeFaceDataToStlFile(const OdString& strFileName, const OdArray<OdDgStlTriangleFace>& arrTriangles);
					static void addStlTriangle(const OdDgStlTriangleFace& triangleData);
					static OdArray<OdDgStlTriangleFace> getStlTriangles();
					static void clearStlTriangles();

				protected:

					static OdArray<OdDgStlTriangleFace> m_arrTriangles;
				};

				typedef OdSmartPtr<OdaGiDumper> OdaGiDumperPtr;
			}
		}
	}
}
