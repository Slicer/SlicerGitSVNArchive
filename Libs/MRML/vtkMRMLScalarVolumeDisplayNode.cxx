/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women\"s Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLVolumeDisplayNode.cxx,v $
Date:      $Date: 2006/03/17 15:10:10 $
Version:   $Revision: 1.2 $

=========================================================================auto=*/

// MRML includes
#include "vtkEventBroker.h"
#include "vtkMRMLScalarVolumeDisplayNode.h"
#include "vtkMRMLProceduralColorNode.h"
#include "vtkMRMLVolumeNode.h"

// VTK includes
#include <vtkCallbackCommand.h>
#include <vtkColorTransferFunction.h>
#include <vtkImageAccumulateDiscrete.h>
#include <vtkImageAppendComponents.h>
#include <vtkImageBimodalAnalysis.h>
#include <vtkImageCast.h>
#include <vtkImageData.h>
#include <vtkImageLogic.h>
#include <vtkImageMapToWindowLevelColors.h>
#include <vtkImageThreshold.h>
#include <vtkObjectFactory.h>
#include <vtkLookupTable.h>

// STD includes

//------------------------------------------------------------------------------
vtkMRMLScalarVolumeDisplayNode* vtkMRMLScalarVolumeDisplayNode::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMRMLScalarVolumeDisplayNode");
  if(ret)
    {
    return (vtkMRMLScalarVolumeDisplayNode*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMRMLScalarVolumeDisplayNode;
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLScalarVolumeDisplayNode::CreateNodeInstance()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMRMLScalarVolumeDisplayNode");
  if(ret)
    {
    return (vtkMRMLScalarVolumeDisplayNode*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMRMLScalarVolumeDisplayNode;
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeDisplayNode::vtkMRMLScalarVolumeDisplayNode()
{
  // Strings
  this->Interpolate = 1;
  this->AutoWindowLevel = 1;
  this->AutoThreshold = 0;
  this->ApplyThreshold = 0;
  //this->LowerThreshold = VTK_SHORT_MIN;
  //this->UpperThreshold = VTK_SHORT_MAX;

  // try setting a default greyscale color map
  //this->SetDefaultColorMap(0);

  // create and set visulaization pipeline
  this->ResliceAlphaCast = vtkImageCast::New();
  this->AlphaLogic = vtkImageLogic::New();
  this->MapToColors = vtkImageMapToColors::New();
  this->Threshold = vtkImageThreshold::New();
  this->AppendComponents = vtkImageAppendComponents::New();
  
  this->MapToWindowLevelColors = vtkImageMapToWindowLevelColors::New();
  this->MapToWindowLevelColors->SetOutputFormatToLuminance();
  this->MapToWindowLevelColors->SetWindow(256.);
  this->MapToWindowLevelColors->SetLevel(128.);

  this->MapToColors->SetOutputFormatToRGB();
  this->MapToColors->SetInput( this->MapToWindowLevelColors->GetOutput() );

  this->Threshold->ReplaceInOn();
  this->Threshold->SetInValue(255);
  this->Threshold->ReplaceOutOn();
  this->Threshold->SetOutValue(255);
  this->Threshold->SetOutputScalarTypeToUnsignedChar();
  this->Threshold->ThresholdBetween(VTK_SHORT_MIN, VTK_SHORT_MAX);
  this->ResliceAlphaCast->SetOutputScalarTypeToUnsignedChar();

  this->AlphaLogic->SetOperationToAnd();
  this->AlphaLogic->SetOutputTrueValue(255);
  this->AlphaLogic->SetInput1( this->Threshold->GetOutput() );
  //this->AlphaLogic->SetInput2( this->Threshold->GetOutput() );
  this->AlphaLogic->SetInput2( this->ResliceAlphaCast->GetOutput() );

  this->AppendComponents->RemoveAllInputs();
  this->AppendComponents->SetInputConnection(0, this->MapToColors->GetOutput()->GetProducerPort() );
  this->AppendComponents->AddInputConnection(0, this->AlphaLogic->GetOutput()->GetProducerPort() );

  this->Bimodal = NULL;
  this->Accumulate = NULL;
  this->IsInCalculateAutoLevels = false;
  
  vtkEventBroker::GetInstance()->AddObservation(
    this, vtkCommand::ModifiedEvent, this, this->MRMLCallbackCommand  , 10000.);
}

//----------------------------------------------------------------------------
vtkMRMLScalarVolumeDisplayNode::~vtkMRMLScalarVolumeDisplayNode()
{
  this->SetAndObserveColorNodeID( NULL);

  this->ResliceAlphaCast->Delete();
  this->AlphaLogic->Delete();
  this->MapToColors->Delete();
  this->Threshold->Delete();
  this->AppendComponents->Delete();
  this->MapToWindowLevelColors->Delete();

  if (this->Bimodal)
    {
    this->Bimodal->Delete();
    this->Bimodal = NULL;
    }
  if (this->Accumulate)
    {
    this->Accumulate->Delete();
    this->Accumulate = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkMRMLScalarVolumeDisplayNode::SetDefaultColorMap()
{
  this->SetAndObserveColorNodeID("vtkMRMLColorTableNodeGrey");
}

//----------------------------------------------------------------------------
void vtkMRMLScalarVolumeDisplayNode::SetInputImageData(vtkImageData *imageData)
{
  if (this->GetInputImageData() != NULL)
    {
    vtkEventBroker::GetInstance()->RemoveObservations(
      this->GetInputImageData(), vtkCommand::ModifiedEvent, this, this->MRMLCallbackCommand );
    }

  this->Superclass::SetInputImageData(imageData);

  if (this->GetInputImageData() != NULL)
    {
    vtkEventBroker::GetInstance()->AddObservation(
      this->GetInputImageData(), vtkCommand::ModifiedEvent, this, this->MRMLCallbackCommand );
    }
}

//----------------------------------------------------------------------------
void vtkMRMLScalarVolumeDisplayNode::SetInputToImageDataPipeline(vtkImageData *imageData)
{
  this->Threshold->SetInput(imageData);
  this->MapToWindowLevelColors->SetInput(imageData);
}

//----------------------------------------------------------------------------
void vtkMRMLScalarVolumeDisplayNode::SetBackgroundImageData(vtkImageData *imageData)
{
  this->ResliceAlphaCast->SetInput(imageData);
}

//----------------------------------------------------------------------------
vtkImageData* vtkMRMLScalarVolumeDisplayNode::GetInputImageData()
{
  return vtkImageData::SafeDownCast(this->MapToWindowLevelColors->GetInput());
}

//----------------------------------------------------------------------------
vtkImageData* vtkMRMLScalarVolumeDisplayNode::GetOutputImageData()
{
  return this->AppendComponents->GetOutput();
}

//----------------------------------------------------------------------------
void vtkMRMLScalarVolumeDisplayNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  vtkIndent indent(nIndent);

  {
  std::stringstream ss;
  ss << this->GetWindow();
  of << indent << " window=\"" << ss.str() << "\"";
  }
  {
  std::stringstream ss;
  ss << this->GetLevel();
  of << indent << " level=\"" << ss.str() << "\"";
  }
  {
  std::stringstream ss;
  ss << this->GetUpperThreshold();
  of << indent << " upperThreshold=\"" << ss.str() << "\"";
  }
  {    
  std::stringstream ss;
  ss << this->GetLowerThreshold();
  of << indent << " lowerThreshold=\"" << ss.str() << "\"";
  }
  {   
  std::stringstream ss;
  ss << this->Interpolate;
  of << indent << " interpolate=\"" << ss.str() << "\"";
  }
  {
  std::stringstream ss;
  ss << this->AutoWindowLevel;
  of << indent << " autoWindowLevel=\"" << ss.str() << "\"";
  }
  {
  std::stringstream ss;
  ss << this->ApplyThreshold;
  of << indent << " applyThreshold=\"" << ss.str() << "\"";
  }
  {
  std::stringstream ss;
  ss << this->AutoThreshold;
  of << indent << " autoThreshold=\"" << ss.str() << "\"";
  }
  if (this->WindowLevelPresets.size() > 0)
    {
    for (int p = 0; p < this->GetNumberOfWindowLevelPresets(); p++)
      {
      std::stringstream ss;
      ss << this->WindowLevelPresets[p].Window;
      ss << "|";
      ss << this->WindowLevelPresets[p].Level;
      of << indent << " windowLevelPreset" << p << "=\"" << ss.str() << "\"";
      }
    }
}

//----------------------------------------------------------------------------
void vtkMRMLScalarVolumeDisplayNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "window")) 
      {
      std::stringstream ss;
      ss << attValue;
      double window;
      ss >> window;
      this->SetWindow(window);
      }
    else if (!strcmp(attName, "level")) 
      {
      std::stringstream ss;
      ss << attValue;
      double level;
      ss >> level;
      this->SetLevel(level);
      }
    else if (!strcmp(attName, "upperThreshold")) 
      {
      std::stringstream ss;
      ss << attValue;
      double threshold;
      ss >> threshold;
      this->SetUpperThreshold(threshold);
      }
    else if (!strcmp(attName, "lowerThreshold")) 
      {
      std::stringstream ss;
      ss << attValue;
      double threshold;
      ss >> threshold;
      this->SetLowerThreshold(threshold);
      }
    else if (!strcmp(attName, "interpolate")) 
      {
      std::stringstream ss;
      ss << attValue;
      ss >> this->Interpolate;
      }
    else if (!strcmp(attName, "autoWindowLevel")) 
      {
      std::stringstream ss;
      ss << attValue;
      ss >> this->AutoWindowLevel;
      }
    else if (!strcmp(attName, "applyThreshold")) 
      {
      std::stringstream ss;
      ss << attValue;
      ss >> this->ApplyThreshold;
      }
    else if (!strcmp(attName, "autoThreshold")) 
      {
      std::stringstream ss;
      ss << attValue;
      ss >> this->AutoThreshold;
      }
    else if (!strncmp(attName, "windowLevelPreset", 17)) 
      {
      this->AddWindowLevelPresetFromString(attValue);
      }
    }  
    
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
// Copy the node\"s attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLScalarVolumeDisplayNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);
  vtkMRMLScalarVolumeDisplayNode *node = (vtkMRMLScalarVolumeDisplayNode *) anode;
  
  this->SetAutoWindowLevel( node->GetAutoWindowLevel() );
  this->SetWindowLevel(node->GetWindow(), node->GetLevel());
  this->SetAutoThreshold( node->GetAutoThreshold() ); // don't want to run CalculateAutoLevel
  this->SetApplyThreshold(node->GetApplyThreshold());
  this->SetThreshold(node->GetLowerThreshold(), node->GetUpperThreshold());
  this->SetInterpolate(node->Interpolate);
  for (int p = 0; p < node->GetNumberOfWindowLevelPresets(); p++)
    {
    this->AddWindowLevelPreset(node->GetWindowPreset(p), node->GetLevelPreset(p));
    }

  this->EndModify(disabledModify);
  
}

//----------------------------------------------------------------------------
void vtkMRMLScalarVolumeDisplayNode::PrintSelf(ostream& os, vtkIndent indent)
{
  
  Superclass::PrintSelf(os,indent);

  os << indent << "AutoWindowLevel:   " << this->AutoWindowLevel << "\n";
  os << indent << "Window:            " << this->GetWindow() << "\n";
  os << indent << "Level:             " << this->GetLevel() << "\n";
  os << indent << "Window Level Presets:\n";
  for (int p = 0; p < this->GetNumberOfWindowLevelPresets(); p++)
    {
    os << indent.GetNextIndent() << p << " Window: " << this->GetWindowPreset(p) << " | Level: " << this->GetLevelPreset(p) << "\n";
    }
  os << indent << "AutoThreshold:     " << this->AutoThreshold << "\n";
  os << indent << "ApplyThreshold:    " << this->GetApplyThreshold() << "\n";
  os << indent << "UpperThreshold:    " << this->GetUpperThreshold() << "\n";
  os << indent << "LowerThreshold:    " << this->GetLowerThreshold() << "\n";
  os << indent << "Interpolate:       " << this->Interpolate << "\n";
}

//---------------------------------------------------------------------------
void vtkMRMLScalarVolumeDisplayNode::ProcessMRMLEvents ( vtkObject *caller,
                                           unsigned long event, 
                                           void *callData )
{
  vtkMRMLColorNode* cnode = vtkMRMLColorNode::SafeDownCast(caller);
  if (cnode && event == vtkCommand::ModifiedEvent)
    {
    this->UpdateLookupTable(cnode);
    }
  if (vtkImageData::SafeDownCast(caller) == this->GetScalarImageData() &&
      event == vtkCommand::ModifiedEvent)
    {
    this->CalculateAutoLevels();
    }
  if (caller == this && event == vtkCommand::ModifiedEvent &&
      !this->IsInCalculateAutoLevels)
    {
    this->SetDisableModifiedEvent(1);
    this->CalculateAutoLevels();
    // TODO: Reset the pending event counter.
    this->SetDisableModifiedEvent(0);
    }
  Superclass::ProcessMRMLEvents(caller, event, callData);
}

//---------------------------------------------------------------------------
double vtkMRMLScalarVolumeDisplayNode::GetWindow()
{
  return this->MapToWindowLevelColors->GetWindow();
}

//---------------------------------------------------------------------------
void vtkMRMLScalarVolumeDisplayNode::SetWindow(double window)
{
  if (this->MapToWindowLevelColors->GetWindow() == window)
    {
    return;
    }
  
  this->MapToWindowLevelColors->SetWindow(window);
  this->Modified();
}

//---------------------------------------------------------------------------
double vtkMRMLScalarVolumeDisplayNode::GetLevel()
{
  return this->MapToWindowLevelColors->GetLevel();
}

//---------------------------------------------------------------------------
void vtkMRMLScalarVolumeDisplayNode::SetLevel(double level)
{
  if (this->MapToWindowLevelColors->GetLevel() == level)
    {
    return;
    }

  this->MapToWindowLevelColors->SetLevel(level);
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkMRMLScalarVolumeDisplayNode::SetWindowLevel(double window, double level)
{
  if (this->MapToWindowLevelColors->GetWindow() == window &&
      this->MapToWindowLevelColors->GetLevel() == level)
    {
    return;
    }

  this->MapToWindowLevelColors->SetWindow(window);
  this->MapToWindowLevelColors->SetLevel(level);
  this->Modified();
}

//---------------------------------------------------------------------------
double vtkMRMLScalarVolumeDisplayNode::GetWindowLevelMin()
{
  const double window = this->GetWindow();
  const double level = this->GetLevel();
  return level - 0.5 * window;
}

//---------------------------------------------------------------------------
double vtkMRMLScalarVolumeDisplayNode::GetWindowLevelMax()
{
  const double window = this->GetWindow();
  const double level = this->GetLevel();
  return level + 0.5 * window;
}

//---------------------------------------------------------------------------
void vtkMRMLScalarVolumeDisplayNode::SetApplyThreshold(int apply)
{
  if (this->ApplyThreshold == apply)
    {
    return;
    }
  this->ApplyThreshold = apply;
  this->Threshold->SetOutValue(apply ? 0 : 255);
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkMRMLScalarVolumeDisplayNode::SetLowerThreshold(double threshold)
{
  this->SetThreshold(threshold, this->GetUpperThreshold());
}

//---------------------------------------------------------------------------
double vtkMRMLScalarVolumeDisplayNode::GetLowerThreshold()
{
  return this->Threshold->GetLowerThreshold();
}

//---------------------------------------------------------------------------
void vtkMRMLScalarVolumeDisplayNode::SetUpperThreshold(double threshold)
{
  this->SetThreshold(this->GetLowerThreshold(), threshold);
}

//---------------------------------------------------------------------------
double vtkMRMLScalarVolumeDisplayNode::GetUpperThreshold()
{
  return this->Threshold->GetUpperThreshold();
}

//---------------------------------------------------------------------------
void vtkMRMLScalarVolumeDisplayNode::SetThreshold(double lowerThreshold, double upperThreshold)
{
  if (this->GetLowerThreshold() == lowerThreshold &&
      this->GetUpperThreshold() == upperThreshold)
    {
    return;
    }
  this->Threshold->ThresholdBetween( lowerThreshold, upperThreshold );
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkMRMLScalarVolumeDisplayNode::SetColorNodeInternal(vtkMRMLColorNode* newColorNode)
{
  this->Superclass::SetColorNodeInternal(newColorNode);
  this->UpdateLookupTable(newColorNode);
}

//---------------------------------------------------------------------------
void vtkMRMLScalarVolumeDisplayNode::UpdateLookupTable(vtkMRMLColorNode* newColorNode)
{
  vtkScalarsToColors *lookupTable = NULL;
  if (newColorNode)
    {
    lookupTable = newColorNode->GetLookupTable();
    if (lookupTable == NULL)
      {
      if (vtkMRMLProceduralColorNode::SafeDownCast(newColorNode) != NULL)
        {
        vtkDebugMacro("UpdateImageDataPipeline: getting color transfer function");
        lookupTable = vtkMRMLProceduralColorNode::SafeDownCast(newColorNode)->GetColorTransferFunction();
        }
      }
    }
  this->MapToColors->SetLookupTable(lookupTable);
}

//---------------------------------------------------------------------------
void vtkMRMLScalarVolumeDisplayNode::AddWindowLevelPresetFromString(const char *preset)
{
  // the string is double|double
  double window = 0.0;
  double level = 0.0;

  if (preset == NULL)
    {
    vtkErrorMacro("AddWindowLevelPresetFromString: null input string!");
    return;
    }
  // parse the string
  std::string presetString = std::string(preset);
  char *presetChars = new char [presetString.size()+1];
  strcpy(presetChars, presetString.c_str());
  char *pos = strtok(presetChars, "|");
  if (pos != NULL)
    {
    window = atof(pos);
    }
  pos = strtok(NULL, "|");
  if (pos != NULL)
    {
    level = atof(pos);
    }
  delete[] presetChars;
  this->AddWindowLevelPreset(window, level);
}

//---------------------------------------------------------------------------
void vtkMRMLScalarVolumeDisplayNode::AddWindowLevelPreset(double window, double level)
{
  vtkMRMLScalarVolumeDisplayNode::WindowLevelPreset preset;
  preset.Window = window;
  preset.Level = level;

  this->WindowLevelPresets.push_back(preset);
}

//---------------------------------------------------------------------------
int vtkMRMLScalarVolumeDisplayNode::GetNumberOfWindowLevelPresets()
{
  return static_cast<int>(this->WindowLevelPresets.size());
}

//---------------------------------------------------------------------------
void vtkMRMLScalarVolumeDisplayNode::SetWindowLevelFromPreset(int p)
{
  if (p < 0 || p >= this->GetNumberOfWindowLevelPresets())
    {
    vtkErrorMacro("SetWindowLevelFromPreset: index " << p << " out of range 0 to " << this->GetNumberOfWindowLevelPresets() - 1);
    return;
    }
  // TODO: change this flag to a regular int showing which preset is being used
  this->SetAutoWindowLevel(0);
  this->SetWindowLevel(this->GetWindowPreset(p), this->GetLevelPreset(p));
}

//---------------------------------------------------------------------------
double vtkMRMLScalarVolumeDisplayNode::GetWindowPreset(int p)
{
  if (p < 0 || p >= this->GetNumberOfWindowLevelPresets())
    {
    vtkErrorMacro("GetWindowPreset: index " << p << " out of range 0 to " << this->GetNumberOfWindowLevelPresets() - 1);
    return 0.0;
    }

  return this->WindowLevelPresets[p].Window;
}

//---------------------------------------------------------------------------
double vtkMRMLScalarVolumeDisplayNode::GetLevelPreset(int p)
{
  if (p < 0 || p >= this->GetNumberOfWindowLevelPresets())
    {
    vtkErrorMacro("GetLevelPreset: index " << p << " out of range 0 to " << this->GetNumberOfWindowLevelPresets() - 1);
    return 0.0;
    }

  return this->WindowLevelPresets[p].Level;
}

//---------------------------------------------------------------------------
void vtkMRMLScalarVolumeDisplayNode::ResetWindowLevelPresets()
{
  this->WindowLevelPresets.clear();
}

//---------------------------------------------------------------------------
vtkImageData* vtkMRMLScalarVolumeDisplayNode::GetScalarImageData()
{
  return this->GetInputImageData();
}

//---------------------------------------------------------------------------
void vtkMRMLScalarVolumeDisplayNode::GetDisplayScalarRange(double range[2])
{
  range[0] = 0;
  range[1] = 255.;

  vtkImageData *imageData = this->GetScalarImageData();
  if (!imageData)
    {
    assert( !this->GetVolumeNode() || !this->GetVolumeNode()->GetImageData());
    vtkWarningMacro( << "No valid image data, returning default values [0, 255]");
    return;
    }
  imageData->GetScalarRange(range);
  if (imageData->GetNumberOfScalarComponents() >=3 &&
      fabs(range[0]) < 0.000001 && fabs(range[1]) < 0.000001) 
    {
    range[0] = 0;
    range[1] = 255;
    }
}

//---------------------------------------------------------------------------
void vtkMRMLScalarVolumeDisplayNode::CalculateAutoLevels()
{
  if (!this->GetAutoWindowLevel() && !this->GetAutoThreshold())
    {
    vtkDebugMacro("CalculateScalarAutoLevels: " << (this->GetID() == NULL ? "nullid" : this->GetID())
                  << ": Auto window level not turned on, returning.");
    return;
    }

  vtkImageData *imageDataScalar = this->GetScalarImageData();

  if (!imageDataScalar)
    {
    vtkDebugMacro("CalculateScalarAutoLevels: input image data is null");
    return;
    }

  double window = 0;
  double level = 0;
  double lower = 0;
  double upper = 0;

  int needAdHoc = 0;
  int scalarType = imageDataScalar->GetScalarType();

  if (imageDataScalar->GetNumberOfScalarComponents() >=3)
    {
    needAdHoc = 1;
    }
  else if (scalarType != VTK_INT && 
           scalarType != VTK_SHORT &&
           scalarType != VTK_CHAR &&
           scalarType != VTK_SIGNED_CHAR &&
           scalarType != VTK_UNSIGNED_CHAR &&
           scalarType != VTK_UNSIGNED_SHORT &&
           scalarType != VTK_UNSIGNED_INT)
    {
    // if not an integer type, estimate with ad hoc method
    needAdHoc = 1; 
    }
  else 
    {
    // data type is VTK_INT or similar, so calculate window/level
    // check the scalar type, bimodal analysis only works on int
    if (this->Bimodal == NULL)
      {
      this->Bimodal = vtkImageBimodalAnalysis::New();
      }
    if (this->Accumulate == NULL)
      {
      this->Accumulate = vtkImageAccumulateDiscrete::New();
      }

    this->Accumulate->SetInput(imageDataScalar);
    this->Bimodal->SetInput(this->Accumulate->GetOutput());
    this->Bimodal->Update();
    // Workaround for image data where all accumulate samples fall
    // within the same histogram bin
    if ( this->Bimodal->GetWindow() == 0.0 &&
         this->Bimodal->GetLevel() == 0.0 )
      {
      needAdHoc = 1; 
      }
    }
    
  if ( needAdHoc )
    {
    vtkDebugMacro("CalculateScalarAutoLevels: image data scalar type is not integer,"
                  " doing ad hoc calc of window/level.");
    double range[2];
    this->GetDisplayScalarRange(range);

    double min = range[0];
    double max = range[1];

    window = max-min;
    level = 0.5*(max+min);
    lower = this->GetLevel();
    upper = range[1];
    }
  else
    {
    window = this->Bimodal->GetWindow();
    level = this->Bimodal->GetLevel();
    lower = this->Bimodal->GetThreshold();
    upper = this->Bimodal->GetMax();
    }

  this->IsInCalculateAutoLevels = true;
  int disabledModify = this->StartModify();
  if (this->GetAutoWindowLevel())
    {
    this->SetWindowLevel(window, level);
    }
  if (this->GetAutoThreshold())
    {
    this->SetThreshold(lower, upper);
    }
  vtkDebugMacro("CalculateScalarAutoLevels:"
                << " window: " << window << " level: " << level
                << " lower: " << lower << " upper: " << upper);
  this->EndModify(disabledModify);
  this->IsInCalculateAutoLevels = false;
}
