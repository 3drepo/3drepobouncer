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

#include <RxObjectImpl.h>
#include "oda_gi_dumper.h"
#include "oda_gi_geo_dumper.h"

#define STL_USING_IOSTREAM
#include "OdaSTL.h"
namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {

				class OdaGiDumperImpl : public OdaGiDumper
				{
					int   m_indentLevel;
				public:
					static OdaGiDumperPtr createObject();

					OdaGiDumperImpl(); // it isn't possible to create object using this constructor, use 
									  // above createObject method for such purpose
					virtual ~OdaGiDumperImpl();

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

					virtual void pushIndent() { m_indentLevel += 2; }
					virtual void popIndent() { m_indentLevel -= 2; }
				};
			}
		}
	}
}