/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkGradientAnisotropicDiffusionFilterLogic.h,v $
  Date:      $Date: 2006/03/19 17:12:29 $
  Version:   $Revision: 1.3 $

=========================================================================auto=*/
#ifndef __vtkLabelStatisticsLogic_h
#define __vtkLabelStatisticsLogic_h

#include "vtkSlicerModuleLogic.h"
#include "vtkMRMLScene.h"

#include "vtkLabelStatistics.h"
#include "vtkMRMLLabelStatisticsNode.h"


class VTK_LABELSTATISTICS_EXPORT vtkLabelStatisticsLogic : public vtkSlicerModuleLogic
{
  public:
  enum VolumeLogicEventIDs {
    StartLabelStats= vtkCommand::UserEvent,
    EndLabelStats,
    LabelStatsOuterLoop,
    LabelStatsInnerLoop
  };
  static vtkLabelStatisticsLogic *New();
  vtkTypeMacro(vtkLabelStatisticsLogic,vtkSlicerModuleLogic);

  void PrintSelf(ostream& os, vtkIndent indent);

  // TODO: do we need to observe MRML here?
  virtual void ProcessMrmlEvents(vtkObject * vtkNotUsed(caller),
                                 unsigned long vtkNotUsed(event),
                                 void *vtkNotUsed(callData)){};

  // Description: Get/Set MRML node storing parameter values
  vtkGetObjectMacro (LabelStatisticsNode, vtkMRMLLabelStatisticsNode);
  void SetAndObserveLabelStatisticsNode(vtkMRMLLabelStatisticsNode *n) 
  {
    vtkSetAndObserveMRMLNodeMacro( this->LabelStatisticsNode, n);
  }
  
  // The method that creates and runs VTK or ITK pipeline
  void Apply();
  
  float GetProgress();
  void SetProgress(float p);
  vtkGetStringMacro(Res);
  vtkSetStringMacro(Res);
 
 protected:
  vtkLabelStatisticsLogic();
  virtual ~vtkLabelStatisticsLogic();
  vtkLabelStatisticsLogic(const vtkLabelStatisticsLogic&);
  void operator=(const vtkLabelStatisticsLogic&);
  
  vtkMRMLLabelStatisticsNode* LabelStatisticsNode;
  float Progress; //progress of labelStats processing in percent
  char* Res;
  float TestFloat;
 };

#endif

