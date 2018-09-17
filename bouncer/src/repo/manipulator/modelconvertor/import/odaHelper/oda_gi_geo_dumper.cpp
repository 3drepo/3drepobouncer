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
{
}

/************************************************************************/
/* Process polygon data                                                 */
/************************************************************************/
void OdGiConveyorGeometryDumper::polygonOut(OdInt32 numPoints,
	const OdGePoint3d* vertexList,
	const OdGeVector3d* pNormal)
{
	//All polygons needs to become triangles.
	OdGiGeometrySimplifier::polygonOut(numPoints, vertexList, pNormal);
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
	recordingMesh = true;
	//Call parent shellProc, which triangulates all polygon vertices (resulting faces would 
	// appear in Polygon Out.)
	OdGiGeometrySimplifier::meshProc(rows, columns, pVertexList, pEdgeData, pFaceData, pVertexData);
	recordingMesh = false;
	if (vertices.size() && faces.size())
		collector->addMeshEntry(vertices, faces, boundingBox);
	
	vertices.clear();
	faces.clear();
	boundingBox.clear();
	vToVIndex.clear();
}

void OdGiConveyorGeometryDumper::triangleOut(const OdInt32* p3Vertices, const OdGeVector3d* pNormal)
{
	const OdGePoint3d*  pVertexDataList = vertexDataList();
	const OdGeVector3d* pNormals = NULL;
	
	if ((pVertexDataList + p3Vertices[0]) != (pVertexDataList + p3Vertices[1]) &&
		(pVertexDataList + p3Vertices[0]) != (pVertexDataList + p3Vertices[2]) &&
		(pVertexDataList + p3Vertices[1]) != (pVertexDataList + p3Vertices[2]))
	{
		repo_face_t face;
		for (int i = 0; i < 3; ++i)
		{
			std::stringstream ss;
			ss.precision(17);
			ss << std::fixed << pVertexDataList[p3Vertices[i]].x << "," << std::fixed << pVertexDataList[p3Vertices[i]].y << "," << std::fixed << pVertexDataList[p3Vertices[i]].z;
			auto vStr = ss.str();
			if (vToVIndex.find(vStr) == vToVIndex.end())
			{
				vToVIndex[vStr] = vertices.size();
				vertices.push_back({ pVertexDataList[p3Vertices[i]].x , pVertexDataList[p3Vertices[i]].y, pVertexDataList[p3Vertices[i]].z });
				if (boundingBox.size()) {
					boundingBox[0][0] = boundingBox[0][0] > pVertexDataList[p3Vertices[i]].x ? pVertexDataList[p3Vertices[i]].x : boundingBox[0][0];
					boundingBox[0][1] = boundingBox[0][1] > pVertexDataList[p3Vertices[i]].y ? pVertexDataList[p3Vertices[i]].y : boundingBox[0][1];
					boundingBox[0][2] = boundingBox[0][2] > pVertexDataList[p3Vertices[i]].z ? pVertexDataList[p3Vertices[i]].z : boundingBox[0][2];

					boundingBox[1][0] = boundingBox[1][0] < pVertexDataList[p3Vertices[i]].x ? pVertexDataList[p3Vertices[i]].x : boundingBox[1][0];
					boundingBox[1][1] = boundingBox[1][1] < pVertexDataList[p3Vertices[i]].y ? pVertexDataList[p3Vertices[i]].y : boundingBox[1][1];
					boundingBox[1][2] = boundingBox[1][2] < pVertexDataList[p3Vertices[i]].z ? pVertexDataList[p3Vertices[i]].z : boundingBox[1][2];
				}
				else {
					boundingBox.push_back({ pVertexDataList[p3Vertices[i]].x, pVertexDataList[p3Vertices[i]].y, pVertexDataList[p3Vertices[i]].z });
					boundingBox.push_back({ pVertexDataList[p3Vertices[i]].x, pVertexDataList[p3Vertices[i]].y, pVertexDataList[p3Vertices[i]].z });
				}
			}
			face.push_back(vToVIndex[vStr]);

			
		}
		faces.push_back(face);

		if (!recordingMesh) {
			collector->addMeshEntry(vertices, faces, boundingBox);
			vertices.clear();
			faces.clear();
			boundingBox.clear();
			vToVIndex.clear();
		}
	}

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
	recordingMesh = true;

	//Call parent shellProc, which triangulates all polygon vertices (resulting faces would 
	// appear in Polygon Out.)
	OdGiGeometrySimplifier::shellProc(
		numVertices,
		vertexList,
		faceListSize,
		faceList,
		pEdgeData,
		pFaceData,
		pVertexData);
	recordingMesh = false;
	if(vertices.size() && faces.size())
		collector->addMeshEntry(vertices, faces, boundingBox);

	vertices.clear();
	faces.clear();
	boundingBox.clear();
	vToVIndex.clear();
}
