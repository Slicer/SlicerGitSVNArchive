/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSlicerFixedPointVolumeRayCastMIPHelper.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSlicerFixedPointVolumeRayCastMIPHelper - A helper that generates MIP images for the volume ray cast mapper
// .SECTION Description
// This is one of the helper classes for the vtkSlicerFixedPointVolumeRayCastMapper.
// It will generate maximum intensity images.
// This class should not be used directly, it is a helper class for
// the mapper and has no user-level API.
//
// .SECTION see also
// vtkSlicerFixedPointVolumeRayCastMapper

#ifndef __vtkSlicerFixedPointVolumeRayCastMIPHelper_h
#define __vtkSlicerFixedPointVolumeRayCastMIPHelper_h

#include "vtkSlicerFixedPointVolumeRayCastHelper.h"
#include "VolumeRenderingReplacementsExport.h"


class vtkSlicerFixedPointVolumeRayCastMapper;
class vtkVolume;

/// \ingroup Slicer_QtModules_VolumeRendering
class Q_SLICER_QTMODULES_VOLUMERENDERING_REPLACEMENTS_EXPORT vtkSlicerFixedPointVolumeRayCastMIPHelper : public vtkSlicerFixedPointVolumeRayCastHelper
{
public:
  static vtkSlicerFixedPointVolumeRayCastMIPHelper *New();
  vtkTypeRevisionMacro(vtkSlicerFixedPointVolumeRayCastMIPHelper,vtkSlicerFixedPointVolumeRayCastHelper);
  void PrintSelf( ostream& os, vtkIndent indent );

  virtual void  GenerateImage( int threadID,
                               int threadCount,
                               vtkVolume *vol,
                               vtkSlicerFixedPointVolumeRayCastMapper *mapper);

protected:
  vtkSlicerFixedPointVolumeRayCastMIPHelper();
  ~vtkSlicerFixedPointVolumeRayCastMIPHelper();

private:
  vtkSlicerFixedPointVolumeRayCastMIPHelper(const vtkSlicerFixedPointVolumeRayCastMIPHelper&);  // Not implemented.
  void operator=(const vtkSlicerFixedPointVolumeRayCastMIPHelper&);  // Not implemented.
};

#endif



