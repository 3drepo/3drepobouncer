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

#ifndef ODBMEXGSSIMPLEDEVICE
#define ODBMEXGSSIMPLEDEVICE


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Gs/GsBaseInclude.h"

#include "BmGiDumper.h"
#include "BmGiConveyorGeometryDumper.h"

#include <iostream>

class ExSimpleView;

/************************************************************************/
/* OdGsBaseVectorizeDevice objects own, update, and refresh one or more */
/* OdGsView objects.                                                    */
/************************************************************************/
class ExGsSimpleDevice : 
  public OdGsBaseVectorizeDevice
{
  OdGiConveyorGeometryDumperPtr m_pDestGeometry;
  OdGiDumperPtr                 m_pDumper;
public:
  enum DeviceType
  {
    k2dDevice,
    k3dDevice
  };

  ExGsSimpleDevice();

  /**********************************************************************/
  /* Set the target data stream and the type.                           */
  /**********************************************************************/
  static OdGsDevicePtr createObject(DeviceType type, GeometryCollector* gcollector);

  /**********************************************************************/
  /* Called by the Teigha vectorization framework to update             */
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
  OdGiDumper* dumper();

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
  GeometryCollector* gcollector;
  DeviceType              m_type;
}; // end ExGsSimpleDevice

/************************************************************************/
/* This template class is a specialization of the OdSmartPtr class for  */
/* ExGsSimpleDevice object pointers                                     */
/************************************************************************/
typedef OdSmartPtr<ExGsSimpleDevice> ExGsSimpleDevicePtr;

/************************************************************************/
/* Example client view class that demonstrates how to receive           */
/* geometry from the vectorization process.                             */
/************************************************************************/
class ExSimpleView : public OdGsBaseVectorizeViewDef
{
  OdGiClipBoundary        m_eyeClip;
protected:

  /**********************************************************************/
  /* Returns the ExGsSimpleDevice instance that owns this view.         */
  /**********************************************************************/
  virtual void beginViewVectorization();

  ExGsSimpleDevice* device()
  { 
    return (ExGsSimpleDevice*)OdGsBaseVectorizeView::device(); 
  }

  ExSimpleView()
  {
    m_eyeClip.m_bDrawBoundary = false;
  }

  friend class ExGsSimpleDevice;

  /**********************************************************************/
  /* PseudoConstructor                                                  */
  /**********************************************************************/
  static OdGsViewPtr createObject()
  {
    return OdRxObjectImpl<ExSimpleView,OdGsView>::createObject();
  }

  /**********************************************************************/
  /* To know when to set traits                                         */
  /**********************************************************************/
  void setEntityTraits();

public:
  /**********************************************************************/
  /* Called by the Teigha vectorization framework to vectorize          */
  /* each entity in this view.  This override allows a client           */
  /* application to examine each entity before it is vectorized.        */
  /* The override should call the parent version of this function,      */
  /* OdGsBaseVectorizeView::draw().                                     */
  /**********************************************************************/
  void draw(const OdGiDrawable*);

  /**********************************************************************/
  /* Called by the Teigha vectorization framework to give the           */
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
    // to force Teigha vectorization framework to use some reasonable value.
    return 0.000001; 
  }

  /**********************************************************************/
  /* Flushes any queued graphics to the display device.                 */
  /**********************************************************************/
  void updateViewport();
  
  TD_USING(OdGsBaseVectorizeViewDef::updateViewport);

  /**********************************************************************/
  /* Notification function called by the vectorization framework        */
  /* whenever the rendering attributes have changed.                    */
  /*                                                                    */
  /* Retrieves the current vectorization traits from Teigha (color      */
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
}; // end ExSimpleView 


#endif 
