/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkImageHistogramNormalization.cxx,v $
  Date:      $Date: 2006/01/31 17:02:27 $
  Version:   $Revision: 1.4 $

=========================================================================auto=*/
#include "vtkImageHistogramNormalization.h"
#include "vtkObjectFactory.h"
#include "vtkImageData.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkImageHistogramNormalization);

vtkImageHistogramNormalization::vtkImageHistogramNormalization()
{
}

vtkImageHistogramNormalization::~vtkImageHistogramNormalization()
{
}

void vtkImageHistogramNormalization::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkImageShiftScale::PrintSelf(os,indent);
}

void vtkImageHistogramNormalization::ExecuteData(vtkDataObject* out)
{
  vtkDebugMacro("Execute to find shift/scale parameters");
  vtkImageData* inData = vtkImageData::SafeDownCast(this->GetInput());
  vtkImageData* outData = this->AllocateOutputData(out);

   
  double typeMax = outData->GetScalarTypeMax();
  vtkFloatingPointType minmax[2];

  inData->GetScalarRange(minmax);
  vtkDebugMacro("Min: " << minmax[0] << " Max: " << minmax[1]);
  
  this->SetShift(-minmax[0]);
  this->SetScale(typeMax/(minmax[1]-minmax[0]));
  this->vtkImageShiftScale::ExecuteData(out);
}
