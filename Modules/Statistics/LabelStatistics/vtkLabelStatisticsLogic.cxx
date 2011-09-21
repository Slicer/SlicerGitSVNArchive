/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkLabelStatisticsLogic.cxx,v $
Date:      $Date: 2006/03/17 15:10:10 $
Version:   $Revision: 1.2 $

=========================================================================auto=*/

#include <string>
#include <iostream>
#include <sstream>

#include "vtkObjectFactory.h"

#include "vtkLabelStatisticsLogic.h"
#include "vtkMRMLScalarVolumeNode.h"

#include "vtkImageAccumulate.h"
#include "vtkImageThreshold.h"
#include "vtkImageMathematics.h"
#include "vtkImageToImageStencil.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkLabelStatisticsLogic);


//----------------------------------------------------------------------------
vtkLabelStatisticsLogic::vtkLabelStatisticsLogic()
{
  this->LabelStatisticsNode = NULL;
  this->SetProgress(0);
}

//----------------------------------------------------------------------------
vtkLabelStatisticsLogic::~vtkLabelStatisticsLogic()
{
  vtkSetMRMLNodeMacro(this->LabelStatisticsNode, NULL);
}

//----------------------------------------------------------------------------
void vtkLabelStatisticsLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkLabelStatisticsLogic::Apply()
{
  int lo, hi;
  
  // check if MRML node is present 
  if (this->LabelStatisticsNode == NULL)
    {
    vtkErrorMacro("No input LabelStatisticsNode found");
    return;
    }


  // find input grayscale volume
  vtkMRMLScalarVolumeNode *inGrayscaleVolume = vtkMRMLScalarVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->LabelStatisticsNode->GetInputGrayscaleRef()));
  if (inGrayscaleVolume == NULL)
    {
    vtkErrorMacro("No input volume found with id= " << this->LabelStatisticsNode->GetInputGrayscaleRef());
    return;
    }
  
  // find input labelmap volume
  vtkMRMLScalarVolumeNode *inLabelmapVolume =  vtkMRMLScalarVolumeNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(this->LabelStatisticsNode->GetInputLabelmapRef()));
  if (inLabelmapVolume == NULL)
    {
    vtkErrorMacro("No input volume found with id= " << this->LabelStatisticsNode->GetInputLabelmapRef());
    return;
    }

  const double *spacing = inLabelmapVolume->GetSpacing();
  double cubicMMPerVoxel = spacing[0] * spacing[1] * spacing[2];
  
  this->InvokeEvent(vtkLabelStatisticsLogic::StartLabelStats, (void*)"start label stats");
  

  //clear the LabelStats result list
  this->LabelStatisticsNode->LabelStats.clear();
 
  vtkImageAccumulate *stataccum = vtkImageAccumulate::New();
  stataccum->SetInput(inLabelmapVolume->GetImageData());
  stataccum->Update();
  lo = static_cast<int>(stataccum->GetMin()[0]);
  hi = static_cast<int>(stataccum->GetMax()[0]);
  stataccum->Delete();

  std::string tmpString("Label\tCount\tVolume\tMin\tMax\tMean\tStdDev\n");
  this->LabelStatisticsNode->SetResultText(tmpString.c_str());

  for(int i = lo; i <= hi; i++ ) 
    {
    this->SetProgress((float)i/hi);
    std::string event_message = "Label ";
    std::stringstream s;
    s << i;
    event_message.append(s.str());
    this->InvokeEvent(vtkLabelStatisticsLogic::LabelStatsOuterLoop, (void*)event_message.c_str());
    //logic copied from slicer2 LabelStatistics MaskStat
    // create the binary volume of the label
    vtkImageThreshold *thresholder = vtkImageThreshold::New();
    thresholder->SetInput(inLabelmapVolume->GetImageData());
    thresholder->SetInValue(1);
    thresholder->SetOutValue(0);
    thresholder->ReplaceOutOn();
    thresholder->ThresholdBetween(i,i);
    thresholder->SetOutputScalarType(inGrayscaleVolume->GetImageData()->GetScalarType());
    thresholder->Update();
    
    this->InvokeEvent(vtkLabelStatisticsLogic::LabelStatsInnerLoop, (void*)"0.25");
    
    // use vtk's statistics class with the binary labelmap as a stencil
    vtkImageToImageStencil *stencil = vtkImageToImageStencil::New();
    stencil->SetInput(thresholder->GetOutput());
    stencil->ThresholdBetween(1, 1);
    
    this->InvokeEvent(vtkLabelStatisticsLogic::LabelStatsInnerLoop, (void*)"0.5");
    
   vtkImageAccumulate *stat1 = vtkImageAccumulate::New();
   stat1->SetInput(inGrayscaleVolume->GetImageData());
   stat1->SetStencil(stencil->GetOutput());
   stat1->Update();
   
   stencil->Delete();

    this->InvokeEvent(vtkLabelStatisticsLogic::LabelStatsInnerLoop, (void*)"0.75");

    if ( (stat1->GetVoxelCount()) > 0 ) 
      {
      
      //add an entry to the LabelStats list
      vtkMRMLLabelStatisticsNode::LabelStatsEntry entry;
      entry.Label = i;
      entry.Count = stat1->GetVoxelCount();
      entry.Volume = static_cast<double>(entry.Count) * cubicMMPerVoxel;
      entry.Min = stat1->GetMin()[0];
      entry.Max = stat1->GetMax()[0];
      entry.Mean = (stat1->GetMean())[0];
      entry.StdDev = (stat1->GetStandardDeviation())[0];
      
      this->InvokeEvent(vtkLabelStatisticsLogic::LabelStatsInnerLoop, (void*)"1");

      this->LabelStatisticsNode->LabelStats.push_back(entry);

      {
      std::stringstream ss;
      std::string str;
      ss << i;
      ss >> str;
      tmpString.append(str);
      tmpString.append("\t");
      }  
      {
      std::stringstream ss;
      std::string str;
      ss << stat1->GetVoxelCount();
      ss >> str;
      tmpString.append(str);
      tmpString.append("\t");
      }
      {
      std::stringstream ss;
      std::string str;
      ss << entry.Volume;
      ss >> str;
      tmpString.append(str);
      tmpString.append("\t");
      }
      {
      std::stringstream ss;
      std::string str;
      ss << (stat1->GetMin())[0];
      ss >> str;
      tmpString.append(str);
      tmpString.append("\t");
      }
      {
      std::stringstream ss;
      std::string str;
      ss << (stat1->GetMax())[0];
      ss >> str;
      tmpString.append(str);
      tmpString.append("\t");
      }
      {
      std::stringstream ss;
      std::string str;
      ss << (stat1->GetMean())[0];
      ss >> str;
      tmpString.append(str);
      tmpString.append("\t");
      }
      {
      std::stringstream ss;
      std::string str;
      ss << (stat1->GetStandardDeviation())[0];
      ss >> str;
      tmpString.append(str);
      tmpString.append("\t");
      }

      tmpString.append("\n");
      this->LabelStatisticsNode->SetResultText(tmpString.c_str());
      }
    //  multMath->Delete();
    thresholder->Delete();
    stat1->Delete();
    }
   this->InvokeEvent(vtkLabelStatisticsLogic::EndLabelStats, (void*)"end label stats");
}

float  vtkLabelStatisticsLogic::GetProgress(){
  return this->Progress;
}

void  vtkLabelStatisticsLogic::SetProgress(float p){
  this->Progress = p;
}
