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

#include "oda_gi_dumper.h"
#include "oda_geometry_collector.h"

#include <SharedPtr.h>
#include <Gi/GiGeometrySimplifier.h>
#include <string>
#include <fstream>

#include "../../../../core/model/bson/repo_node_mesh.h"

#include <vector>
namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class OdGiConveyorGeometryDumper : public OdGiGeometrySimplifier
				{
					OdaGiDumperPtr m_pDumper;
					OdGiConveyorGeometryDumper();

					int m_dumpLevel = Maximal_Simplification;
					OdaGeometryCollector *collector;

					bool recordingMesh = false;
					std::vector<repo::lib::RepoVector3D64> vertices;
					std::vector<repo_face_t> faces;
					std::vector<std::vector<double>> boundingBox;
					std::unordered_map<std::string, unsigned int> vToVIndex;

				public:
					enum
					{
						Maximal_Simplification,
						Minimal_Simplification
					};
					void setDumpLevel(int dumpLevel) { m_dumpLevel = dumpLevel; }
					int dumpLevel() const { return m_dumpLevel; }

					static OdSharedPtr<OdGiConveyorGeometryDumper> createObject(OdaGiDumper* m_pDumper);
					void setMeshCollector(OdaGeometryCollector *const geoCollector);


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

					void triangleOut(const OdInt32* p3Vertices, const OdGeVector3d* pNormal);

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
					/* the current version of Teigha for .dwg files                       */
					/**********************************************************************/
					void shapeProc(const OdGePoint3d& position,
						const OdGeVector3d& direction, const OdGeVector3d& upVector,
						int shapeNumber, const OdGiTextStyle* pTextStyle,
						const OdGeVector3d* pExtrusion = 0);

					/**********************************************************************/
					/* The client's version of this function will not be called in        */
					/* the current version of Teigha for .dwg files                       */
					/**********************************************************************/
					void xlineProc(const OdGePoint3d& firstPoint, const OdGePoint3d& secondPoint);

					/**********************************************************************/
					/* The client's version of this function will not be called in        */
					/* the current version of Teigha for .dwg files                       */
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
						const OdCmTransparency* pTransparency = 0,
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
			}
		}
	}
}