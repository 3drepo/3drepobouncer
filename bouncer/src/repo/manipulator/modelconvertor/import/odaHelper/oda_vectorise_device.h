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
#include <Gs/GsBaseInclude.h>
#include <RxObjectImpl.h>
#include <iostream>
#include <Gs/GsBaseMaterialView.h>
#include "oda_gi_dumper.h"
#include "oda_gi_geo_dumper.h"


namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class ExSimpleView;

				/************************************************************************/
				/* OdGsBaseVectorizeDevice objects own, update, and refresh one or more */
				/* OdGsView objects.                                                    */
				/************************************************************************/
				class OdaVectoriseDevice :
					public OdGsBaseVectorizeDevice
				{
					OdGiConveyorGeometryDumperPtr m_pDestGeometry;
					OdaGiDumperPtr                 m_pDumper;
					OdaGeometryCollector * geoCollector;
				public:
					enum DeviceType
					{
						k2dDevice,
						k3dDevice
					};

					OdaVectoriseDevice();

					/**********************************************************************/
					/* Set the target data stream and the type.                           */
					/**********************************************************************/
					static OdGsDevicePtr createObject(DeviceType type, OdaGeometryCollector * geoCollector);

					/**********************************************************************/
					/* Called by the Teigha for .dwg files vectorization framework to update */
					/* the GUI window for this Device object                              */
					/*                                                                    */
					/* pUpdatedRect specifies a rectangle to receive the region updated   */
					/* by this function.                                                  */
					/*                                                                    */
					/* The override should call the parent version of this function,      */
					/* OdGsBaseVectorizeDevice::update().                                 */
					/**********************************************************************/
					void update(OdGsDCRect* pUpdatedRect);

					/**********************************************************************/
					/* Creates a new OdGsView object, and associates it with this Device  */
					/* object.                                                            */
					/*                                                                    */
					/* pInfo is a pointer to the Client View Information for this Device  */
					/* object.                                                            */
					/*                                                                    */
					/* bEnableLayerVisibilityPerView specifies that layer visibility      */
					/* per viewport is to be supported.                                   */
					/**********************************************************************/
					OdGsViewPtr createView(const OdGsClientViewInfo* pInfo = 0,
						bool bEnableLayerVisibilityPerView = false);


					/**********************************************************************/
					/* Returns the geometry receiver associated with this object.         */
					/**********************************************************************/
					OdGiConveyorGeometry* destGeometry();

					/**********************************************************************/
					/* Returns the geometry dumper associated with this object.           */
					/**********************************************************************/
					OdaGiDumper* dumper();

					OdaGeometryCollector* collector() { return geoCollector; }

					/**********************************************************************/
					/* Called by each associated view object to set the current RGB draw  */
					/* color.                                                             */
					/**********************************************************************/
					void draw_color(ODCOLORREF color);

					/**********************************************************************/
					/* Called by each associated view object to set the current ACI draw  */
					/* color.                                                             */
					/**********************************************************************/
					void draw_color_index(OdUInt16 colorIndex);

					/**********************************************************************/
					/* Called by each associated view object to set the current draw      */
					/* lineweight and pixel width                                         */
					/**********************************************************************/
					void draw_lineWidth(OdDb::LineWeight weight, int pixelLineWidth);

					/************************************************************************/
					/* Called by each associated view object to set the current Fill Mode   */
					/************************************************************************/
					void draw_fill_mode(OdGiFillType fillMode);


					void setupSimplifier(const OdGiDeviation* pDeviation);

				private:
					DeviceType              m_type;
				}; // end OdaVectoriseDevice

				   /************************************************************************/
				   /* This template class is a specialization of the OdSmartPtr class for  */
				   /* OdaVectoriseDevice object pointers                                     */
				   /************************************************************************/
				typedef OdSmartPtr<OdaVectoriseDevice> OdaVectoriseDevicePtr;

				/************************************************************************/
				/* Example client view class that demonstrates how to receive           */
				/* geometry from the vectorization process.                             */
				/************************************************************************/
				class ExSimpleView :  public OdGsBaseMaterialView
				{
					OdGiClipBoundary        m_eyeClip;
					OdaGeometryCollector    *geoCollector;
				protected:

					/**********************************************************************/
					/* Returns the OdaVectoriseDevice instance that owns this view.         */
					/**********************************************************************/
					virtual void beginViewVectorization();

					OdaVectoriseDevice* device()
					{
						return (OdaVectoriseDevice*)OdGsBaseMaterialView::device();
					}

					void setGeoCollector(OdaGeometryCollector *collector) {
						geoCollector = collector;
					}

					OdGiMaterialItemPtr fillMaterialCache(OdGiMaterialItemPtr prevCache, OdDbStub* materialId, const OdGiMaterialTraitsData & materialData)
					{
						repoInfo << "!!!! MATERIAL CACHE!!!!";

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

						material.shininessStrength = specularColor.factor();
						material.shininess = glossFactor;

						// opacity
						material.opacity = opacityPercentage;


						// refraction
						/*mData.bRefractionChannelEnabled = (GETBIT(materialData.channelFlags(), OdGiMaterialTraits::kUseRefraction)) ? 1 : 0;
						mData.dRefractionIndex = materialData.reflectivity();*/

						// transclucence
						//mData.dTranslucence = materialData.translucence();						
						auto id = (OdUInt64)(OdIntPtr)materialId;
						geoCollector->addMaterial(id, material);

						return OdGiMaterialItemPtr();
					}

					ExSimpleView()
					{
						m_eyeClip.m_bDrawBoundary = false;
					}

					friend class OdaVectoriseDevice;

					/**********************************************************************/
					/* PseudoConstructor                                                  */
					/**********************************************************************/
					static OdGsViewPtr createObject()
					{
						return OdRxObjectImpl<ExSimpleView, OdGsView>::createObject();
					}

					/**********************************************************************/
					/* To know when to set traits                                         */
					/**********************************************************************/
					void setEntityTraits();

				public:
					/**********************************************************************/
					/* Called by the Teigha for .dwg files vectorization framework to vectorize */
					/* each entity in this view.  This override allows a client           */
					/* application to examine each entity before it is vectorized.        */
					/* The override should call the parent version of this function,      */
					/* OdGsBaseVectorizeView::draw().                                     */
					/**********************************************************************/
					void draw(const OdGiDrawable*);

					/**********************************************************************/
					/* Called by the Teigha for .dwg files vectorization framework to give the */
					/* client application a chance to terminate the current               */
					/* vectorization process.  Returning true from this function will     */
					/* stop the current vectorization operation.                          */
					/**********************************************************************/
					bool regenAbort() const;

					/**********************************************************************/
					/* Returns the recommended maximum deviation of the current           */
					/* vectorization.                                                     */
					/**********************************************************************/
					double deviation(const OdGiDeviationType, const OdGePoint3d&) const
					{
						// NOTE: use some reasonable value
						return 0;
					}

					/**********************************************************************/
					/* Flushes any queued graphics to the display device.                 */
					/**********************************************************************/
					void updateViewport();

					TD_USING(OdGsBaseMaterialView::updateViewport);

					/**********************************************************************/
					/* Notification function called by the vectorization framework        */
					/* whenever the rendering attributes have changed.                    */
					/*                                                                    */
					/* Retrieves the current vectorization traits from Teigha for .dwg files (color */
					/* lineweight, etc.) and sets them in this device.                    */
					/**********************************************************************/
					void onTraitsModified();

					void ownerDrawDc(
						const OdGePoint3d& origin,
						const OdGeVector3d& u,
						const OdGeVector3d& v,
						const OdGiSelfGdiDrawable* pDrawable,
						bool bDcAligned,
						bool bAllowClipping);

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
				}; // end ExSimpleView 

			}
		}
	}
}