/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkImageBimodalAnalysis.cxx,v $
  Date:      $Date: 2006/06/14 20:44:14 $
  Version:   $Revision: 1.21 $

=========================================================================auto=*/
#include "vtkImageBimodalAnalysis.h"

#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkImageData.h"

//#include <math.h>
//#include <cstdlib>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkImageBimodalAnalysis);

//----------------------------------------------------------------------------
// Constructor sets default values
vtkImageBimodalAnalysis::vtkImageBimodalAnalysis()
{
  this->Modality  = VTK_BIMODAL_MODALITY_CT;
  this->Threshold = 0;
  this->Window    = 0;
  this->Level     = 0;
  this->Min       = 0;
  this->Max       = 0;
  this->Offset    = 0;

  for (int i = 0; i < 2; ++i)
    {
    this->SignalRange[i] = 0;
    }
  for (int i = 0; i < 6; ++i)
    {
    this->ClipExtent[i] = 0;
    }
}

//----------------------------------------------------------------------------
int vtkImageBimodalAnalysis::RequestInformation(
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_FLOAT, 1);
  return 1;
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageBimodalAnalysisExecute(vtkImageBimodalAnalysis *self,
                      vtkImageData *inData, T *inPtr,
                      vtkImageData *outData, float *outPtr)
{
  int x, k, offset, clipExt[6];
  int min0, max0, min1, max1, min2, max2;
  int noise = 1, width = 5;
  float fwidth = 1.0 / 5.0;
  T tmp, minSignal, maxSignal;
  double sum, wsum;
  int ct = (self->GetModality() == VTK_BIMODAL_MODALITY_CT) ? 1 : 0;
  int centroid, noiseCentroid, trough, window, threshold, min, max;
  double origin[3], spacing[3];

  // Process x dimension only
  outData->GetExtent(min0, max0, min1, max1, min2, max2);
  inData->GetOrigin(origin);
  inData->GetSpacing(spacing);

  offset = (int)origin[0];

  // Zero output
  memset((void *)outPtr, 0, (max0-min0+1)*sizeof(int));

  // For CT data, ignore -32768
  if (ct)
    {
    min0 = 1;
    }

  // Find min (first non-zero value in histogram)
  min = x = min0;
  while (!inPtr[x] && x <= max0)
    {
    x++;
    }
  if (x <= max0)
    {
    min = x;
    }

  // Find max (last non-zero value in histogram)
  max = x = max0;
  while (!inPtr[x] && x >= min0)
    {
    x--;
    }
  if (x >= min0)
    {
    max = x;
    }

  // Smooth
  for (x = min; x <= max; x++)
    {
    for (k=0; k < width; k++)
      {
      if (x+k <= max0) // skip any that would be outside range of outPtr see Bug #3429
        {
        outPtr[x] += (float)inPtr[x+k];
        }
      }
    outPtr[x] *= fwidth;
    }

  // Find first trough of smoothed histogram
  x = min;
  trough = min-1;
  noise = 1;
  while (x < max && trough < min)
    {
    if (noise)
      {
      if (outPtr[x] > outPtr[x+1])
        {
        if (x > min)
          {
          noise = 0;
          }
        }
      }
    else
      {
      if (outPtr[x] < outPtr[x+1])
        {
        trough = x;
        }
      }
    x++;
    }

  // Compute centroid of the histogram that PRECEEDS the trough
  // (noise lobe)
  wsum = sum = 0;
  for (x=min; x <= trough; x++)
    {
    tmp = inPtr[x];
    wsum += (double)x*tmp;
    sum  += (double)  tmp;
    }
  if (sum)
    {
    noiseCentroid = (int)(wsum / sum);
    }
  else
    {
    noiseCentroid = trough;
    }

  // Compute centroid of the histogram that FOLLOWS the trough
  // (signal lobe, and not noise lobe)
  wsum = sum = 0;
  minSignal = maxSignal = inPtr[trough];
  for (x=trough; x <= max; x++)
    {
    tmp = inPtr[x];
    if (tmp > maxSignal)
      {
      maxSignal = tmp;
      }
    else if (tmp < minSignal)
      {
      minSignal = tmp;
      }
    wsum += (double)x*tmp;
    sum  += (double)  tmp;
    }
  if (sum)
    {
    centroid = (int)(wsum / sum);
    }
  else
    {
    centroid = trough;
    }

  // Threshold
  threshold = trough;

  // Compute the window as twice the width as the smaller half
  // of the signal lobe
  if (centroid - noiseCentroid < max - centroid)
    {
    window = (centroid - noiseCentroid)*2;
    }
  else
    {
    window = (max - centroid)*2;
    }

  // Record findings
  self->SetOffset(offset);
  self->SetThreshold(threshold + offset);
  self->SetMin(min + offset);
  self->SetMax(max + offset);
  self->SetLevel(centroid + offset);
  self->SetWindow(window);
  self->SetSignalRange((int)minSignal, (int)maxSignal);

  outData->GetExtent(clipExt);
  clipExt[0] = min;
  clipExt[1] = max;
  self->SetClipExtent(clipExt);
}



//----------------------------------------------------------------------------
// This method is passed a input and output Data, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the Datas data types.
void vtkImageBimodalAnalysis::ExecuteDataWithInformation(vtkDataObject *out, vtkInformation* outInfo)
{
    vtkImageData *inData = vtkImageData::SafeDownCast(this->GetInput());
    void *inPtr;
    float *outPtr;
    vtkImageData *outData = this->AllocateOutputData(out, outInfo);

  inPtr  = inData->GetScalarPointer();
  outPtr = (float *)outData->GetScalarPointer();

  // Components turned into x, y and z
  int c = inData->GetNumberOfScalarComponents();
  if (c > 1)
    {
    vtkErrorMacro("This filter requires 1 scalar component, not " << c);
    return;
    }

  // this filter expects that output is type float.
  if (outData->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "ExecuteData: out ScalarType " << outData->GetScalarType()
      << " must be float\n");
    return;
    }

  switch (inData->GetScalarType())
    {
    vtkTemplateMacro(vtkImageBimodalAnalysisExecute(this, inData,
                                                    static_cast<VTK_TT *>(inPtr),
                                                    outData, outPtr));
    default:
      vtkErrorMacro(<< "ExecuteData: Unsupported ScalarType");
      return;
    }
}

//----------------------------------------------------------------------------
void vtkImageBimodalAnalysis::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Modality: " << this->Modality << " (" << (this->Modality == VTK_BIMODAL_MODALITY_CT ? "CT" : "MR") << ")\n";
  os << indent << "Offset: " << this->Offset << "\n";
  os << indent << "Threshold: " << this->Threshold << "\n";
  os << indent << "Window: " << this->Window << "\n";
  os << indent << "Level: " << this->Level << "\n";
  os << indent << "Min: " << this->Min << "\n";
  os << indent << "Max: " << this->Max << "\n";
  os << indent << "ClipExtent: " << this->ClipExtent[0] << "," << this->ClipExtent[1] << "," << this->ClipExtent[2] << "," << this->ClipExtent[3] << "," << this->ClipExtent[4] << "," << this->ClipExtent[5] << "\n";
  os << indent << "SignalRange: " << this->SignalRange[0] << "," << this->SignalRange[1] << "\n";

}

