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

#include "oda_geometry_collector.h"

#include <SharedPtr.h>
#include <Gi/GiGeometrySimplifier.h>
#include <Gs/GsBaseMaterialView.h>
#include <string>
#include <fstream>

#include "oda_vectorise_device.h"
#include "../../../../core/model/bson/repo_node_mesh.h"

#include <vector>
namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class OdGiConveyorGeometryDumper : public OdGiGeometrySimplifier, public OdGsBaseMaterialView
				{
					OdaGeometryCollector *collector;

					bool recordingMesh = false;
					std::vector<repo::lib::RepoVector3D64> vertices;
					std::vector<repo_face_t> faces;
					std::vector<std::vector<double>> boundingBox;
					std::unordered_map<std::string, unsigned int> vToVIndex;

				public:
					OdGiConveyorGeometryDumper();
					
					virtual double deviation(const OdGiDeviationType deviationType, const OdGePoint3d& pointOnCurve) const {
						return 0;
					}

					OdaVectoriseDevice* device()
					{
						return static_cast<OdaVectoriseDevice*>(OdGsBaseVectorizeView::device());
					}

					bool doDraw(OdUInt32 i, const OdGiDrawable* pDrawable)
					{						
						return OdGsBaseMaterialView::doDraw(i, pDrawable);
					}

					void shellFaceOut(OdInt32 faceListSize, const OdInt32* pFaceList, const OdGeVector3d* pNormal)
					{		
						OdGiGeometrySimplifier::shellFaceOut(faceListSize, pFaceList, pNormal);
					}

					static OdCmEntityColor fixByACI(const ODCOLORREF *ids, const OdCmEntityColor &color)
					{
						if (color.isByACI() || color.isByDgnIndex())
						{
							return OdCmEntityColor(ODGETRED(ids[color.colorIndex()]), ODGETGREEN(ids[color.colorIndex()]), ODGETBLUE(ids[color.colorIndex()]));
						}
						else if (!color.isByColor())
						{
							return OdCmEntityColor(0, 0, 0);
						}
						return color;
					}

					OdGiMaterialItemPtr fillMaterialCache(OdGiMaterialItemPtr prevCache, OdDbStub* materialId, const OdGiMaterialTraitsData & materialData)
					{
						auto id = (OdUInt64)(OdIntPtr)materialId;

						OdGiMaterialColor diffuseColor; OdGiMaterialMap diffuseMap;
						OdGiMaterialColor ambientColor;
						OdGiMaterialColor specularColor; OdGiMaterialMap specularMap; double glossFactor;
						double opacityPercentage; OdGiMaterialMap opacityMap;
						double refrIndex; OdGiMaterialMap refrMap;

						materialData.diffuse(diffuseColor, diffuseMap);
						materialData.ambient(ambientColor);
						materialData.specular(specularColor, specularMap, glossFactor);
						materialData.opacity(opacityPercentage, opacityMap);
						materialData.refraction(refrIndex, refrMap);

						OdGiMaterialMap bumpMap;
						materialData.bump(bumpMap);


						ODCOLORREF colorDiffuse(0), colorAmbient(0), colorSpecular(0);
						if (diffuseColor.color().colorMethod() == OdCmEntityColor::kByColor)
						{
							colorDiffuse = ODTOCOLORREF(diffuseColor.color());
						}
						else if (diffuseColor.color().colorMethod() == OdCmEntityColor::kByACI)
						{
							colorDiffuse = OdCmEntityColor::lookUpRGB((OdUInt8)diffuseColor.color().colorIndex());
						}
						if (ambientColor.color().colorMethod() == OdCmEntityColor::kByColor)
						{
							colorAmbient = ODTOCOLORREF(ambientColor.color());
						}
						else if (ambientColor.color().colorMethod() == OdCmEntityColor::kByACI)
						{
							colorAmbient = OdCmEntityColor::lookUpRGB((OdUInt8)ambientColor.color().colorIndex());
						}
						if (specularColor.color().colorMethod() == OdCmEntityColor::kByColor)
						{
							colorSpecular = ODTOCOLORREF(specularColor.color());
						}
						else if (specularColor.color().colorMethod() == OdCmEntityColor::kByACI)
						{
							colorSpecular = OdCmEntityColor::lookUpRGB((OdUInt8)specularColor.color().colorIndex());
						}


						OdCmEntityColor color = fixByACI(this->device()->getPalette(), effectiveTraits().trueColor());
						repo_material_t material;
						// diffuse
						if (diffuseColor.method() == OdGiMaterialColor::kOverride)
							material.diffuse = { ODGETRED(colorDiffuse) / 255.0f, ODGETGREEN(colorDiffuse) / 255.0f, ODGETBLUE(colorDiffuse) / 255.0f, 1.0f };
						else
							material.diffuse = { color.red() / 255.0f, color.green() / 255.0f, color.blue() / 255.0f, 1.0f };
						/*
						TODO: texture support
						mData.bDiffuseChannelEnabled = (GETBIT(materialData.channelFlags(), OdGiMaterialTraits::kUseDiffuse)) ? true : false;
						if (mData.bDiffuseChannelEnabled && diffuseMap.source() == OdGiMaterialMap::kFile && !diffuseMap.sourceFileName().isEmpty())
						{
						mData.bDiffuseHasTexture = true;
						mData.sDiffuseFileSource = diffuseMap.sourceFileName();
						}*/

						// specular
						if (specularColor.method() == OdGiMaterialColor::kOverride)
							material.specular = { ODGETRED(colorSpecular) / 255.0f, ODGETGREEN(colorSpecular) / 255.0f, ODGETBLUE(colorSpecular) / 255.0f, 1.0f };
						else
							material.specular = { color.red() / 255.0f, color.green() / 255.0f, color.blue() / 255.0f, 1.0f };

						material.shininessStrength = 1 - glossFactor;
						material.shininess = materialData.reflectivity();

						// opacity
						material.opacity = opacityPercentage;


						// refraction
						/*mData.bRefractionChannelEnabled = (GETBIT(materialData.channelFlags(), OdGiMaterialTraits::kUseRefraction)) ? 1 : 0;
						mData.dRefractionIndex = materialData.reflectivity();*/

						// transclucence
						//mData.dTranslucence = materialData.translucence();						
						
						collector->addMaterial(id, material);

						return OdGiMaterialItemPtr();
					}

					void init(OdaGeometryCollector *const geoCollector) {
						collector = geoCollector;
					}

					void beginViewVectorization()
					{
						OdGsBaseMaterialView::beginViewVectorization();
						setEyeToOutputTransform(getEyeToWorldTransform());

						OdGiGeometrySimplifier::setDrawContext(OdGsBaseMaterialView::drawContext());
						output().setDestGeometry((OdGiGeometrySimplifier&)*this);
					}

					void setMode(OdGsView::RenderMode mode)
					{
						OdGsBaseVectorizeView::m_renderMode = kGouraudShaded;
						m_regenerationType = kOdGiRenderCommand;
						OdGiGeometrySimplifier::m_renderMode = OdGsBaseVectorizeView::m_renderMode;
					}


					void polygonOut(OdInt32 numPoints, const OdGePoint3d* vertexList, const OdGeVector3d* pNormal = 0);

					void triangleOut(const OdInt32* p3Vertices, const OdGeVector3d* pNormal);	

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
				};
				typedef OdSharedPtr<OdGiConveyorGeometryDumper> OdGiConveyorGeometryDumperPtr;
			}
		}
	}
}