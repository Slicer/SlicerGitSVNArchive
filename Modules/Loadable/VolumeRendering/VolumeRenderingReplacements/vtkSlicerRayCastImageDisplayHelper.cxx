/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSlicerRayCastImageDisplayHelper.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSlicerRayCastImageDisplayHelper.h"
#include <vtkObjectFactory.h>
#include <vtkVersion.h>

vtkAbstractObjectFactoryNewMacro(vtkSlicerRayCastImageDisplayHelper);

// Construct a new vtkSlicerRayCastImageDisplayHelper with default values
vtkSlicerRayCastImageDisplayHelper::vtkSlicerRayCastImageDisplayHelper()
{
  this->PreMultipliedColors = 0;
  this->PixelScale = 1.0;
}

// Destruct a vtkSlicerRayCastImageDisplayHelper - clean up any memory used
vtkSlicerRayCastImageDisplayHelper::~vtkSlicerRayCastImageDisplayHelper()
{
}

void vtkSlicerRayCastImageDisplayHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PreMultiplied Colors: "
     << (this->PreMultipliedColors ? "On" : "Off") << endl;

  os << indent << "Pixel Scale: " << this->PixelScale << endl;
}

