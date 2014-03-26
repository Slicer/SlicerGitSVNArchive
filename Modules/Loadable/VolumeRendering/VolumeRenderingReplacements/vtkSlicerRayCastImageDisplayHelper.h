/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSlicerRayCastImageDisplayHelper.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkSlicerRayCastImageDisplayHelper - helper class that draws the image to the screen
// .SECTION Description
// This is a helper class for drawing images created from ray casting on the screen.
// This is the abstract device-independent superclass.

// .SECTION see also
// vtkVolumeRayCastMapper vtkUnstructuredGridVolumeRayCastMapper
// vtkSlicerOpenGLRayCastImageDisplayHelper

#ifndef __vtkSlicerRayCastImageDisplayHelper_h
#define __vtkSlicerRayCastImageDisplayHelper_h

#include "vtkObject.h"
#include "VolumeRenderingReplacementsExport.h"


class vtkVolume;
class vtkRenderer;
class vtkSlicerFixedPointRayCastImage;

/// \ingroup Slicer_QtModules_VolumeRendering
class Q_SLICER_QTMODULES_VOLUMERENDERING_REPLACEMENTS_EXPORT vtkSlicerRayCastImageDisplayHelper : public vtkObject
{
public:
  static vtkSlicerRayCastImageDisplayHelper *New();
  vtkTypeRevisionMacro(vtkSlicerRayCastImageDisplayHelper,vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  virtual void RenderTexture( vtkVolume *vol, vtkRenderer *ren,
                              int imageMemorySize[2],
                              int imageViewportSize[2],
                              int imageInUseSize[2],
                              int imageOrigin[2],
                              float requestedDepth,
                              unsigned char *image ) = 0;

  virtual void RenderTexture( vtkVolume *vol, vtkRenderer *ren,
                              int imageMemorySize[2],
                              int imageViewportSize[2],
                              int imageInUseSize[2],
                              int imageOrigin[2],
                              float requestedDepth,
                              unsigned short *image ) = 0;

  virtual void RenderTexture( vtkVolume *vol, vtkRenderer *ren,
                              vtkSlicerFixedPointRayCastImage *image,
                              float requestedDepth ) = 0;

  vtkSetClampMacro( PreMultipliedColors, int, 0, 1 );
  vtkGetMacro( PreMultipliedColors, int );
  vtkBooleanMacro( PreMultipliedColors, int );


  // Description:
  // Set / Get the pixel scale to be applied to the image before display.
  // Can be set to scale the incoming pixel values - for example the
  // fixed point mapper uses the unsigned short API but with 15 bit
  // values so needs a scale of 2.0.
  vtkSetMacro( PixelScale, float );
  vtkGetMacro( PixelScale, float );


protected:
  vtkSlicerRayCastImageDisplayHelper();
  ~vtkSlicerRayCastImageDisplayHelper();

  // Description:
  // Have the colors already been multiplied by alpha?
  int PreMultipliedColors;

  float PixelScale;

private:
  vtkSlicerRayCastImageDisplayHelper(const vtkSlicerRayCastImageDisplayHelper&);  // Not implemented.
  void operator=(const vtkSlicerRayCastImageDisplayHelper&);  // Not implemented.
};

#endif

