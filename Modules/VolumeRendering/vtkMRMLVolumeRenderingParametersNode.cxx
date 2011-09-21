/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women\"s Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLVolumeRenderingParametersNode.cxx,v $
Date:      $Date: 2006/03/17 15:10:10 $
Version:   $Revision: 1.2 $

=========================================================================auto=*/

#include <string>
#include <iostream>
#include <sstream>

#include "vtkObjectFactory.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLVolumeRenderingParametersNode.h"


//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLVolumeRenderingParametersNode);

//----------------------------------------------------------------------------
vtkMRMLVolumeRenderingParametersNode::vtkMRMLVolumeRenderingParametersNode()
{
  this->HideFromEditors = 1;

  this->VolumeNodeID = NULL;
  this->VolumeNode = NULL;

  this->VolumePropertyNodeID = NULL;
  this->VolumePropertyNode = NULL;

  this->FgVolumeNodeID = NULL;
  this->FgVolumeNode = NULL;

  this->FgVolumePropertyNodeID = NULL;
  this->FgVolumePropertyNode = NULL;

  this->ROINodeID = NULL;
  this->ROINode = NULL;

  this->ExpectedFPS = 8;
  this->EstimatedSampleDistance = 1.0;

  this->CurrentVolumeMapper = -1;
  this->GPUMemorySize = 1;

  this->CPURaycastMode = 0;

  this->DepthPeelingThreshold = 0.0f;
  this->DistanceColorBlending = 0.0f;

  this->ICPEScale = 1.0f;
  this->ICPESmoothness = 0.5f;

  this->GPURaycastTechnique = 0;
  
  this->GPURaycastTechniqueII = 0;
  this->GPURaycastTechniqueIIFg = 0;

  this->GPURaycastTechnique3 = 0;
  
  this->CroppingEnabled = 0;//by default cropping is not enabled
  
  this->Threshold[0] = 0.0;
  this->Threshold[1] = 1.0;
  
  this->UseThreshold = 0; // by default volume property widget is used
  
  this->ThresholdFg[0] = 0.0;
  this->ThresholdFg[1] = 1.0;
  
  this->UseFgThreshold = 0; // by default volume property widget is used
  
  this->GPURaycastIIBgFgRatio = 0.0f;//default display bg volume
  
  this->GPURaycastIIFusion = 0;

  this->FollowVolumeDisplayNode = 0;// by default do not follow volume display node
  this->UseSingleVolumeProperty = 0;
  
  this->WindowLevel[0] = 0.0;
  this->WindowLevel[1] = 0.0;

  this->WindowLevelFg[0] = 0.0;
  this->WindowLevelFg[1] = 0.0;

  this->PerformanceControl = 0;
}

//----------------------------------------------------------------------------
vtkMRMLVolumeRenderingParametersNode::~vtkMRMLVolumeRenderingParametersNode()
{
  if (this->VolumeNodeID)
    {
    SetAndObserveVolumeNodeID(NULL);
    }

  if (this->VolumePropertyNodeID)
    {
    SetAndObserveVolumePropertyNodeID(NULL);
    }

  if (this->FgVolumeNodeID)
    {
    SetAndObserveFgVolumeNodeID(NULL);
    }

  if (this->FgVolumePropertyNodeID)
    {
    SetAndObserveFgVolumePropertyNodeID(NULL);
    }

  if (this->ROINodeID)
    {
    SetAndObserveROINodeID(NULL);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLVolumeRenderingParametersNode::ReadXMLAttributes(const char** atts)
{
  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL)
  {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "volumeNodeID"))
    {
      this->SetVolumeNodeID(attValue);
      continue;
    }
    if (!strcmp(attName, "fgVolumeNodeID"))
    {
      this->SetFgVolumeNodeID(attValue);
      continue;
    }
    if (!strcmp(attName, "ROINodeID"))
    {
      this->SetROINodeID(attValue);
      continue;
    }
    if (!strcmp(attName, "volumePropertyNodeID"))
    {
      this->SetVolumePropertyNodeID(attValue);
      continue;
    }
    if (!strcmp(attName, "fgVolumePropertyNodeID"))
    {
      this->SetFgVolumePropertyNodeID(attValue);
      continue;
    }
    if (!strcmp(attName,"croppingEnabled"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->CroppingEnabled;
      continue;
    }
    if (!strcmp(attName,"useThreshold"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->UseThreshold;
      continue;
    }
    if (!strcmp(attName,"useFgThreshold"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->UseFgThreshold;
      continue;
    }
    if (!strcmp(attName,"currentVolumeMapper"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->CurrentVolumeMapper;
      continue;
    }
    if (!strcmp(attName,"cpuRaycastMode"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->CPURaycastMode;
      continue;
    }
    if (!strcmp(attName,"depthPeelingThreshold"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->DepthPeelingThreshold;
      continue;
    }
    if (!strcmp(attName,"distanceColorBlending"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->DistanceColorBlending;
      continue;
    }
    if (!strcmp(attName,"icpeScale"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->ICPEScale;
      continue;
    }
    if (!strcmp(attName,"icpeSmoothness"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->ICPESmoothness;
      continue;
    }
    if (!strcmp(attName,"gpuRaycastIIBgFgRatio"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->GPURaycastIIBgFgRatio;
      continue;
    }
    if (!strcmp(attName,"gpuRaycastTechnique"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->GPURaycastTechnique;
      continue;
    }
    if (!strcmp(attName,"gpuRaycastTechniqueII"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->GPURaycastTechniqueII;
      continue;
    }
    if (!strcmp(attName,"gpuRaycastTechniqueIIFg"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->GPURaycastTechniqueIIFg;
      continue;
    }
    if (!strcmp(attName,"gpuRaycastIIFusion"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->GPURaycastIIFusion;
      continue;
    }
    if (!strcmp(attName,"gpuRaycastTechnique3"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->GPURaycastTechnique3;
      continue;
    }
    if (!strcmp(attName,"threshold"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->Threshold[0];
      ss >> this->Threshold[1];
      continue;
    }
    if (!strcmp(attName,"thresholdFg"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->ThresholdFg[0];
      ss >> this->ThresholdFg[1];
      continue;
    }
    if (!strcmp(attName,"windowLevel"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->WindowLevel[0];
      ss >> this->WindowLevel[1];
      continue;
    }
    if (!strcmp(attName,"windowLevelFg"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->WindowLevelFg[0];
      ss >> this->WindowLevelFg[1];
      continue;
    }
    if (!strcmp(attName,"followVolumeDisplayNode"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->FollowVolumeDisplayNode;
      continue;
    }
    if (!strcmp(attName, "useSingleVolumeProperty"))
    {
      std::stringstream ss;
      ss << attValue;
      ss >> this->UseSingleVolumeProperty;
    }
  }
}

//----------------------------------------------------------------------------
void vtkMRMLVolumeRenderingParametersNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  vtkIndent indent(nIndent);

  of << indent << " volumeNodeID=\"" << (this->VolumeNodeID ? this->VolumeNodeID : "NULL") << "\"";
  of << indent << " fgVolumeNodeID=\"" << (this->FgVolumeNodeID ? this->FgVolumeNodeID : "NULL") << "\"";
  of << indent << " croppingEnabled=\""<< this->CroppingEnabled << "\"";
  of << indent << " ROINodeID=\"" << (this->ROINodeID ? this->ROINodeID : "NULL") << "\"";
  of << indent << " volumePropertyNodeID=\"" << (this->VolumePropertyNodeID ? this->VolumePropertyNodeID : "NULL") << "\"";
  of << indent << " fgVolumePropertyNodeID=\"" << (this->FgVolumePropertyNodeID ? this->FgVolumePropertyNodeID : "NULL") << "\"";
  of << indent << " currentVolumeMapper=\"" << this->CurrentVolumeMapper << "\"";
  of << indent << " cpuRaycastMode=\"" << this->CPURaycastMode << "\"";
  of << indent << " depthPeelingThreshold=\"" << this->DepthPeelingThreshold << "\"";
  of << indent << " distanceColorBlending=\"" << this->DistanceColorBlending << "\"";
  of << indent << " icpeScale=\"" << this->ICPEScale << "\"";
  of << indent << " icpeSmoothness=\"" << this->ICPESmoothness << "\"";
  of << indent << " gpuRaycastTechnique=\"" << this->GPURaycastTechnique << "\"";
  of << indent << " gpuRaycastTechniqueII=\"" << this->GPURaycastTechniqueII << "\"";
  of << indent << " gpuRaycastTechniqueIIFg=\"" << this->GPURaycastTechniqueIIFg << "\"";
  of << indent << " gpuRaycastIIFusion=\"" << this->GPURaycastIIFusion << "\"";
  of << indent << " gpuRaycastTechnique3=\"" << this->GPURaycastTechnique3 << "\"";
  of << indent << " threshold=\"" << this->Threshold[0] << " " << this->Threshold[1] << "\"";
  of << indent << " useThreshold=\"" << this->UseThreshold << "\"";
  of << indent << " thresholdFg=\"" << this->ThresholdFg[0] << " " << this->ThresholdFg[1] << "\"";
  of << indent << " useFgThreshold=\"" << this->UseFgThreshold << "\"";
  of << indent << " gpuRaycastIIBgFgRatio=\"" << this->GPURaycastIIBgFgRatio << "\"";
  of << indent << " followVolumeDisplayNode=\"" << this->FollowVolumeDisplayNode << "\"";
  of << indent << " useSingleVolumeProperty=\"" << this->UseSingleVolumeProperty << "\"";
  of << indent << " windowLevel=\"" << this->WindowLevel[0] << " " << this->WindowLevel[1] << "\"";
  of << indent << " windowLevelFg=\"" << this->WindowLevelFg[0] << " " << this->WindowLevelFg[1] << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLVolumeRenderingParametersNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  if (this->VolumeNodeID && !strcmp(oldID, this->VolumeNodeID))
    {
    this->SetAndObserveVolumeNodeID(newID);
    }
  if (this->FgVolumeNodeID && !strcmp(oldID, this->FgVolumeNodeID))
    {
    this->SetAndObserveFgVolumeNodeID(newID);
    }
  if (this->ROINodeID && !strcmp(oldID, this->ROINodeID))
    {
    this->SetAndObserveROINodeID(newID);
    }
  if (this->VolumePropertyNodeID && !strcmp(oldID, this->VolumePropertyNodeID))
    {
    this->SetAndObserveVolumePropertyNodeID(newID);
    }
  if (this->FgVolumePropertyNodeID && !strcmp(oldID, this->FgVolumePropertyNodeID))
    {
    this->SetAndObserveFgVolumePropertyNodeID(newID);
    }
}

//-----------------------------------------------------------
void vtkMRMLVolumeRenderingParametersNode::UpdateReferences()
{
   Superclass::UpdateReferences();

  if (this->VolumeNodeID != NULL && this->Scene->GetNodeByID(this->VolumeNodeID) == NULL)
    {
    this->SetAndObserveVolumeNodeID(NULL);
    }
  if (this->FgVolumeNodeID != NULL && this->Scene->GetNodeByID(this->FgVolumeNodeID) == NULL)
    {
    this->SetAndObserveFgVolumeNodeID(NULL);
    }
  if (this->ROINodeID != NULL && this->Scene->GetNodeByID(this->ROINodeID) == NULL)
    {
    this->SetAndObserveROINodeID(NULL);
    }
  if (this->VolumePropertyNodeID != NULL && this->Scene->GetNodeByID(this->VolumePropertyNodeID) == NULL)
    {
    this->SetAndObserveVolumePropertyNodeID(NULL);
    }
  if (this->FgVolumePropertyNodeID != NULL && this->Scene->GetNodeByID(this->FgVolumePropertyNodeID) == NULL)
    {
    this->SetAndObserveFgVolumePropertyNodeID(NULL);
    }
}

//----------------------------------------------------------------------------
// Copy the node\"s attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, SliceID
void vtkMRMLVolumeRenderingParametersNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  vtkMRMLVolumeRenderingParametersNode *node = vtkMRMLVolumeRenderingParametersNode::SafeDownCast(anode);
  this->DisableModifiedEventOn();

  this->SetVolumeNodeID(node->GetVolumeNodeID());
  this->SetFgVolumeNodeID(node->GetFgVolumeNodeID());
  this->SetVolumePropertyNodeID(node->GetVolumePropertyNodeID());
  this->SetFgVolumePropertyNodeID(node->GetFgVolumePropertyNodeID());
  this->SetROINodeID(node->GetROINodeID());
  this->SetCroppingEnabled(node->GetCroppingEnabled());
  this->SetCurrentVolumeMapper(node->GetCurrentVolumeMapper());
  this->SetGPUMemorySize(node->GetGPUMemorySize());
  this->SetEstimatedSampleDistance(node->GetEstimatedSampleDistance());
  this->SetDepthPeelingThreshold(node->GetDepthPeelingThreshold());
  this->SetDistanceColorBlending(node->GetDistanceColorBlending());
  this->SetICPEScale(node->GetICPEScale());
  this->SetICPESmoothness(node->GetICPESmoothness());
  this->SetCPURaycastMode(node->GetCPURaycastMode());
  this->SetGPURaycastTechnique(node->GetGPURaycastTechnique());
  this->SetGPURaycastTechniqueII(node->GetGPURaycastTechniqueII());
  this->SetGPURaycastTechniqueIIFg(node->GetGPURaycastTechniqueIIFg());
  this->SetGPURaycastTechnique3(node->GetGPURaycastTechnique3());
  this->SetThreshold(node->GetThreshold());
  this->SetUseThreshold(node->GetUseThreshold());
  this->SetThresholdFg(node->GetThresholdFg());
  this->SetUseFgThreshold(node->GetUseFgThreshold());
  this->SetGPURaycastIIBgFgRatio(node->GetGPURaycastIIBgFgRatio());
  this->SetGPURaycastIIFusion(node->GetGPURaycastIIFusion());
  this->SetWindowLevel(node->GetWindowLevel());
  this->SetWindowLevelFg(node->GetWindowLevelFg());
  this->SetFollowVolumeDisplayNode(node->GetFollowVolumeDisplayNode());
  this->SetUseSingleVolumeProperty(node->GetUseSingleVolumeProperty());
  
  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();

}

//----------------------------------------------------------------------------
void vtkMRMLVolumeRenderingParametersNode::SetAndObserveVolumeNodeID(const char *volumeNodeID)
{
  vtkSetAndObserveMRMLObjectMacro(this->VolumeNode, NULL);

  if (volumeNodeID != NULL)
  {
    this->SetVolumeNodeID(volumeNodeID);
    vtkMRMLVolumeNode *node = this->GetVolumeNode();
    vtkSetAndObserveMRMLObjectMacro(this->VolumeNode, node);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLVolumeRenderingParametersNode::SetAndObserveFgVolumeNodeID(const char *volumeNodeID)
{
  vtkSetAndObserveMRMLObjectMacro(this->FgVolumeNode, NULL);

  if (volumeNodeID != NULL)
  {
    this->SetFgVolumeNodeID(volumeNodeID);
    vtkMRMLVolumeNode *node = this->GetFgVolumeNode();
    vtkSetAndObserveMRMLObjectMacro(this->FgVolumeNode, node);
  }
}

//----------------------------------------------------------------------------
vtkMRMLVolumeNode* vtkMRMLVolumeRenderingParametersNode::GetVolumeNode()
{
  if (this->VolumeNodeID == NULL)
    {
    vtkSetAndObserveMRMLObjectMacro(this->VolumeNode, NULL);
    }
  else if (this->GetScene() &&
           ((this->VolumeNode != NULL && strcmp(this->VolumeNode->GetID(), this->VolumeNodeID)) ||
            (this->VolumeNode == NULL)) )
    {
    vtkMRMLNode* snode = this->GetScene()->GetNodeByID(this->VolumeNodeID);
    vtkSetAndObserveMRMLObjectMacro(this->VolumeNode, vtkMRMLVolumeNode::SafeDownCast(snode));
    }
  return this->VolumeNode;
}

//----------------------------------------------------------------------------
vtkMRMLVolumeNode* vtkMRMLVolumeRenderingParametersNode::GetFgVolumeNode()
{
  if (this->FgVolumeNodeID == NULL)
    {
    vtkSetAndObserveMRMLObjectMacro(this->FgVolumeNode, NULL);
    }
  else if (this->GetScene() &&
           ((this->FgVolumeNode != NULL && strcmp(this->FgVolumeNode->GetID(), this->FgVolumeNodeID)) ||
            (this->FgVolumeNode == NULL)) )
    {
    vtkMRMLNode* snode = this->GetScene()->GetNodeByID(this->FgVolumeNodeID);
    vtkSetAndObserveMRMLObjectMacro(this->FgVolumeNode, vtkMRMLVolumeNode::SafeDownCast(snode));
    }
  return this->FgVolumeNode;
}

//----------------------------------------------------------------------------
void vtkMRMLVolumeRenderingParametersNode::SetAndObserveVolumePropertyNodeID(const char *VolumePropertyNodeID)
{
  vtkSetAndObserveMRMLObjectMacro(this->VolumePropertyNode, NULL);

  if (VolumePropertyNodeID != NULL)
  {
    this->SetVolumePropertyNodeID(VolumePropertyNodeID);
    vtkMRMLVolumePropertyNode *node = this->GetVolumePropertyNode();
    vtkSetAndObserveMRMLObjectMacro(this->VolumePropertyNode, node);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLVolumeRenderingParametersNode::SetAndObserveFgVolumePropertyNodeID(const char *VolumePropertyNodeID)
{
  vtkSetAndObserveMRMLObjectMacro(this->FgVolumePropertyNode, NULL);

  if (VolumePropertyNodeID != NULL)
  {
    this->SetFgVolumePropertyNodeID(VolumePropertyNodeID);
    vtkMRMLVolumePropertyNode *node = this->GetFgVolumePropertyNode();
    vtkSetAndObserveMRMLObjectMacro(this->FgVolumePropertyNode, node);
  }
}

//----------------------------------------------------------------------------
vtkMRMLVolumePropertyNode* vtkMRMLVolumeRenderingParametersNode::GetVolumePropertyNode()
{
  if (this->VolumePropertyNodeID == NULL)
    {
    vtkSetAndObserveMRMLObjectMacro(this->VolumePropertyNode, NULL);
    }
  else if (this->GetScene() &&
           ((this->VolumePropertyNode != NULL && strcmp(this->VolumePropertyNode->GetID(), this->VolumePropertyNodeID)) ||
            (this->VolumePropertyNode == NULL)) )
    {
    vtkMRMLNode* snode = this->GetScene()->GetNodeByID(this->VolumePropertyNodeID);
    vtkSetAndObserveMRMLObjectMacro(this->VolumePropertyNode, vtkMRMLVolumePropertyNode::SafeDownCast(snode));
    }
  return this->VolumePropertyNode;
}

//----------------------------------------------------------------------------
vtkMRMLVolumePropertyNode* vtkMRMLVolumeRenderingParametersNode::GetFgVolumePropertyNode()
{
  if (this->FgVolumePropertyNodeID == NULL)
    {
    vtkSetAndObserveMRMLObjectMacro(this->FgVolumePropertyNode, NULL);
    }
  else if (this->GetScene() &&
           ((this->FgVolumePropertyNode != NULL && strcmp(this->FgVolumePropertyNode->GetID(), this->FgVolumePropertyNodeID)) ||
            (this->FgVolumePropertyNode == NULL)) )
    {
    vtkMRMLNode* snode = this->GetScene()->GetNodeByID(this->FgVolumePropertyNodeID);
    vtkSetAndObserveMRMLObjectMacro(this->FgVolumePropertyNode, vtkMRMLVolumePropertyNode::SafeDownCast(snode));
    }
  return this->FgVolumePropertyNode;
}

//----------------------------------------------------------------------------
void vtkMRMLVolumeRenderingParametersNode::SetAndObserveROINodeID(const char *ROINodeID)
{
  vtkSetAndObserveMRMLObjectMacro(this->ROINode, NULL);

  this->SetROINodeID(ROINodeID);

  vtkMRMLROINode *node = this->GetROINode();

  vtkSetAndObserveMRMLObjectMacro(this->ROINode, node);
}

//----------------------------------------------------------------------------
vtkMRMLROINode* vtkMRMLVolumeRenderingParametersNode::GetROINode()
{
  if (this->ROINodeID == NULL)
    {
    vtkSetAndObserveMRMLObjectMacro(this->ROINode, NULL);
    }
  else if (this->GetScene() &&
           ((this->ROINode != NULL && strcmp(this->ROINode->GetID(), this->ROINodeID)) ||
            (this->ROINode == NULL)) )
    {
    vtkMRMLNode* snode = this->GetScene()->GetNodeByID(this->ROINodeID);
    vtkSetAndObserveMRMLObjectMacro(this->ROINode, vtkMRMLROINode::SafeDownCast(snode));
    }
  return this->ROINode;
}

//-----------------------------------------------------------
void vtkMRMLVolumeRenderingParametersNode::UpdateScene(vtkMRMLScene *scene)
{
  Superclass::UpdateScene(scene);
  this->SetAndObserveVolumeNodeID(this->VolumeNodeID);
  this->SetAndObserveVolumePropertyNodeID(this->VolumePropertyNodeID);
  this->SetAndObserveFgVolumeNodeID(this->FgVolumeNodeID);
  this->SetAndObserveFgVolumePropertyNodeID(this->FgVolumePropertyNodeID);
  this->SetAndObserveROINodeID(this->ROINodeID);
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingParametersNode::ProcessMRMLEvents ( vtkObject *caller,
                                                    unsigned long event,
                                                    void *callData )
{
    Superclass::ProcessMRMLEvents(caller, event, callData);
    this->InvokeEvent(vtkCommand::ModifiedEvent, NULL);
    return;
}

//----------------------------------------------------------------------------
void vtkMRMLVolumeRenderingParametersNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << "VolumeNodeID: " << ( (this->VolumeNodeID) ? this->VolumeNodeID : "None" ) << "\n";
  os << "FgVolumeNodeID: " << ( (this->FgVolumeNodeID) ? this->FgVolumeNodeID : "None" ) << "\n";
  os << "ROINodeID: " << ( (this->VolumeNodeID) ? this->ROINodeID : "None" ) << "\n";
  os << "VolumePropertyNodeID: " << ( (this->VolumePropertyNodeID) ? this->VolumePropertyNodeID : "None" ) << "\n";
  os << "FgVolumePropertyNodeID: " << ( (this->FgVolumePropertyNodeID) ? this->FgVolumePropertyNodeID : "None" ) << "\n";
  os << "CroppingEnabled: " << this->CroppingEnabled << "\n";
}

// End
