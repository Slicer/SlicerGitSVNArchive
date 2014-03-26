/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLViewNode.cxx,v $
Date:      $Date: 2006/03/03 22:26:39 $
Version:   $Revision: 1.3 $

=========================================================================auto=*/

// MRML includes
#include "vtkMRMLScene.h"
#include "vtkMRMLViewNode.h"

// VTK includes
#include <vtkObjectFactory.h>

// STD includes
#include <sstream>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLViewNode);


//----------------------------------------------------------------------------
vtkMRMLViewNode::vtkMRMLViewNode()
{
  this->BoxVisible = 1;
  this->AxisLabelsVisible = 1;
  this->AxisLabelsCameraDependent = 1;
  this->FiducialsVisible = 1;
  this->FiducialLabelsVisible = 1;
  this->FieldOfView = 200;
  this->LetterSize = 0.05;
  this->AnimationMode = vtkMRMLViewNode::Off;
  this->ViewAxisMode = vtkMRMLViewNode::LookFrom;
  this->SpinDegrees = 2.0;
  this->RotateDegrees = 5.0;
  this->SpinDirection = vtkMRMLViewNode::YawLeft;
  this->AnimationMs = 5;
  this->RockLength = 200;
  this->RockCount = 0;
  this->StereoType = vtkMRMLViewNode::NoStereo;
  this->RenderMode = vtkMRMLViewNode::Perspective;
  this->BackgroundColor[0] = this->defaultBackgroundColor()[0];
  this->BackgroundColor[1] = this->defaultBackgroundColor()[1];
  this->BackgroundColor[2] = this->defaultBackgroundColor()[2];
  this->BackgroundColor2[0] = this->defaultBackgroundColor2()[0];
  this->BackgroundColor2[1] = this->defaultBackgroundColor2()[1];
  this->BackgroundColor2[2] = this->defaultBackgroundColor2()[2];
  this->UseDepthPeeling = 0;
  this->FPSVisible = 0;
 }

//----------------------------------------------------------------------------
vtkMRMLViewNode::~vtkMRMLViewNode()
{
}

//----------------------------------------------------------------------------
const char* vtkMRMLViewNode::GetNodeTagName()
{
  return "View";
}

//----------------------------------------------------------------------------
void vtkMRMLViewNode::WriteXML(ostream& of, int nIndent)
{
  // Write all attributes not equal to their defaults

  this->Superclass::WriteXML(of, nIndent);

  vtkIndent indent(nIndent);

  of << indent << " fieldOfView=\"" << this->GetFieldOfView() << "\"";
  of << indent << " letterSize=\"" << this->GetLetterSize() << "\"";
  of << indent << " boxVisible=\"" << (this->BoxVisible ? "true" : "false") << "\"";
  of << indent << " fiducialsVisible=\"" << (this->FiducialsVisible ? "true" : "false") << "\"";
  of << indent << " fiducialLabelsVisible=\"" << (this->FiducialLabelsVisible ? "true" : "false") << "\"";
  of << indent << " axisLabelsVisible=\"" << (this->AxisLabelsVisible ? "true" : "false") << "\"";
  of << indent << " axisLabelsCameraDependent=\"" << (this->AxisLabelsCameraDependent ? "true" : "false") << "\"";

  // spin or rock?
  if ( this->GetAnimationMode() == vtkMRMLViewNode::Off )
    {
    of << indent << " animationMode=\"" << "Off" << "\"";
    }
  else if ( this->GetAnimationMode() == vtkMRMLViewNode::Spin )
    {
    of << indent << " animationMode=\"" << "Spin" << "\"";
    }
  else if ( this->GetAnimationMode() == vtkMRMLViewNode::Rock )
    {
    of << indent << " animationMode=\"" << "Rock" << "\"";
    }

  if ( this->GetViewAxisMode() == vtkMRMLViewNode::LookFrom )
    {
    of << indent << " viewAxisMode=\"" << "LookFrom" << "\"";
    }
  else if ( this->GetViewAxisMode() == vtkMRMLViewNode::RotateAround )
    {
    of << indent << " viewAxisMode=\"" << "RotateAround" << "\"";
    }

  // configure spin
  of << indent << " spinDegrees=\"" << this->GetSpinDegrees() << "\"";
  of << indent << " spinMs=\"" << this->GetAnimationMs() << "\"";
  if ( this->GetSpinDirection() == vtkMRMLViewNode::PitchUp )
    {
    of << indent << " spinDirection=\"" << "PitchUp" << "\"";
    }
  else if ( this->GetSpinDirection() == vtkMRMLViewNode::PitchDown )
    {
    of << indent << " spinDirection=\"" << "PitchDown" << "\"";
    }
  else if ( this->GetSpinDirection() == vtkMRMLViewNode::RollLeft )
    {
    of << indent << " spinDirection=\"" << "RollLeft" << "\"";
    }
  else if ( this->GetSpinDirection() == vtkMRMLViewNode::RollRight )
    {
    of << indent << " spinDirection=\"" << "RollRight" << "\"";
    }
  else if ( this->GetSpinDirection() == vtkMRMLViewNode::YawLeft )
    {
    of << indent << " spinDirection=\"" << "YawLeft" << "\"";
    }
  else if ( this->GetSpinDirection() == vtkMRMLViewNode::YawRight )
    {
    of << indent << " spinDirection=\"" << "YawRight" << "\"";
    }

  of << indent << " rotateDegrees=\"" << this->GetRotateDegrees() << "\"";

  // configure rock
  of << indent << " rockLength=\"" << this->GetRockLength() << "\"";
  of << indent << " rockCount=\"" << this->GetRockCount() << "\"";

  // configure stereo
  if ( this->GetStereoType() == vtkMRMLViewNode::NoStereo )
    {
    of << indent << " stereoType=\"" << "NoStereo" << "\"";
    }
  else if ( this->GetStereoType() == vtkMRMLViewNode::RedBlue )
    {
    of << indent << " stereoType=\"" << "RedBlue" << "\"";
    }
  else if ( this->GetStereoType() == vtkMRMLViewNode::Anaglyph )
    {
    of << indent << " stereoType=\"" << "Anaglyph" << "\"";
    }
  else if ( this->GetStereoType() == vtkMRMLViewNode::QuadBuffer )
    {
    of << indent << " stereoType=\"" << "QuadBuffer" << "\"";
    }
  else if ( this->GetStereoType() == vtkMRMLViewNode::Interlaced )
    {
    of << indent << " stereoType=\"" << "Interlaced" << "\"";
    }
  else if ( this->GetStereoType() == vtkMRMLViewNode::UserDefined_1 )
    {
    of << indent << " stereoType=\"" << "UserDefined_1" << "\"";
    }
  else if ( this->GetStereoType() == vtkMRMLViewNode::UserDefined_2 )
    {
    of << indent << " stereoType=\"" << "UserDefined_2" << "\"";
    }
  else if ( this->GetStereoType() == vtkMRMLViewNode::UserDefined_3 )
    {
    of << indent << " stereoType=\"" << "UserDefined_3" << "\"";
    }

  // configure render mode
  if (this->GetRenderMode() == vtkMRMLViewNode::Perspective )
    {
    of << indent << " renderMode=\"" << "Perspective" << "\"";
    }
  else if ( this->GetRenderMode() == vtkMRMLViewNode::Orthographic )
    {
    of << indent << " renderMode=\"" << "Orthographic" << "\"";
    }

  of << indent << " useDepthPeeling=\"" << this->GetUseDepthPeeling() << "\"";

}

//----------------------------------------------------------------------------
void vtkMRMLViewNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  this->Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL)
    {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "fieldOfView"))
      {
      std::stringstream ss;
      ss << attValue;
      double fov;
      ss >> fov;
      this->FieldOfView = fov;
      }
    else if (!strcmp(attName, "letterSize"))
      {
      std::stringstream ss;
      ss << attValue;
      double fov;
      ss >> fov;
      this->LetterSize = fov;
      }
    else if (!strcmp(attName, "boxVisible"))
      {
      if (!strcmp(attValue,"true"))
        {
        this->BoxVisible = 1;
        }
      else
        {
        this->BoxVisible = 0;
        }
      }
    else if (!strcmp(attName, "fiducialsVisible"))
      {
      if (!strcmp(attValue,"true"))
        {
        this->FiducialsVisible = 1;
        }
      else
        {
        this->FiducialsVisible = 0;
        }
      }
    else if (!strcmp(attName, "fiducialLabelsVisible"))
      {
      if (!strcmp(attValue,"true"))
        {
        this->FiducialLabelsVisible = 1;
        }
      else
        {
        this->FiducialLabelsVisible = 0;
        }
      }
    else if (!strcmp(attName, "axisLabelsVisible"))
      {
      if (!strcmp(attValue,"true"))
        {
        this->AxisLabelsVisible = 1;
        }
      else
        {
        this->AxisLabelsVisible = 0;
        }
      }
    else if (!strcmp(attName, "axisLabelsCameraDependent"))
      {
      if (!strcmp(attValue,"true"))
        {
        this->AxisLabelsCameraDependent = 1;
        }
      else
        {
        this->AxisLabelsCameraDependent = 0;
        }
      }
    else if (!strcmp(attName, "stereoType"))
      {
      if (!strcmp(attValue,"NoStereo"))
        {
        this->StereoType = vtkMRMLViewNode::NoStereo;
        }
      else if ( !strcmp (attValue, "RedBlue" ))
        {
        this->StereoType = vtkMRMLViewNode::RedBlue;
        }
      else if ( !strcmp (attValue, "Anaglyph" ))
        {
        this->StereoType = vtkMRMLViewNode::Anaglyph;
        }
      else if ( !strcmp (attValue, "QuadBuffer" ))
        {
        this->StereoType = vtkMRMLViewNode::QuadBuffer;
        }
      else if ( !strcmp (attValue, "Interlaced" ))
        {
        this->StereoType = vtkMRMLViewNode::Interlaced;
        }
      else if ( !strcmp (attValue, "UserDefined_1" ))
        {
        this->StereoType = vtkMRMLViewNode::UserDefined_1;
        }
      else if ( !strcmp (attValue, "UserDefined_2" ))
        {
        this->StereoType = vtkMRMLViewNode::UserDefined_2;
        }
      else if ( !strcmp (attValue, "UserDefined_3" ))
        {
        this->StereoType = vtkMRMLViewNode::UserDefined_3;
        }
      }
    else if (!strcmp(attName, "rockLength" ))
      {
      std::stringstream ss;
      ss << attValue;
      int len;
      ss >> len;
      this->RockLength = len;
      }
    else if (!strcmp(attName, "rockCount" ))
      {
      std::stringstream ss;
      ss << attValue;
      int count;
      ss >> count;
      this->RockCount = count;
      }
    else if (!strcmp(attName, "animationMode"))
      {
      if (!strcmp(attValue,"Off"))
        {
        this->AnimationMode = vtkMRMLViewNode::Off;
        }
      else if (!strcmp(attValue,"Spin"))
        {
        this->AnimationMode = vtkMRMLViewNode::Spin;
        }
      else if (!strcmp(attValue,"Rock"))
        {
        this->AnimationMode = vtkMRMLViewNode::Rock;
        }
      }
    else if (!strcmp (attName, "viewAxisMode"))
      {
      if (!strcmp (attValue, "RotateAround"))
        {
        this->ViewAxisMode = vtkMRMLViewNode::RotateAround;
        }
      else if (!strcmp (attValue, "LookFrom"))
        {
        this->ViewAxisMode = vtkMRMLViewNode::LookFrom;
        }
      }
    else if (!strcmp(attName, "spinDegrees" ))
      {
      std::stringstream ss;
      ss << attValue;
      double deg;
      ss >> deg;
      this->SpinDegrees = deg;
      }
    else if ( !strcmp ( attName, "rotateDegrees"))
      {
      std::stringstream ss;
      ss << attValue;
      double deg;
      ss >> deg;
      this->RotateDegrees = deg;
      }
    else if (!strcmp(attName, "spinMs" ))
      {
      std::stringstream ss;
      ss << attValue;
      int ms;
      ss >> ms;
      this->AnimationMs = ms;
      }
    else if (!strcmp(attName, "spinDirection"))
      {
      if (!strcmp(attValue,"RollLeft"))
        {
        this->SpinDirection = vtkMRMLViewNode::RollLeft;
        }
      else if ( !strcmp (attValue, "RollRight" ))
        {
        this->SpinDirection = vtkMRMLViewNode::RollRight;
        }
      else if (!strcmp(attValue,"YawLeft"))
        {
        this->SpinDirection = vtkMRMLViewNode::YawLeft;
        }
      else if ( !strcmp (attValue, "YawRight" ))
        {
        this->SpinDirection = vtkMRMLViewNode::YawRight;
        }
      else if ( !strcmp (attValue, "PitchUp" ))
        {
        this->SpinDirection = vtkMRMLViewNode::PitchUp;
        }
      else if ( !strcmp (attValue, "PitchDown" ))
        {
        this->SpinDirection = vtkMRMLViewNode::PitchDown;
        }
      }
    else if (!strcmp(attName, "renderMode"))
      {
      if (!strcmp(attValue,"Perspective"))
        {
        this->RenderMode = vtkMRMLViewNode::Perspective;
        }
      else if ( !strcmp (attValue, "Orthographic" ))
        {
        this->RenderMode = vtkMRMLViewNode::Orthographic;
        }
      }
    else if (!strcmp(attName, "useDepthPeeling" ))
      {
      std::stringstream ss;
      ss << attValue;
      int use;
      ss >> use;
      this->SetUseDepthPeeling(use);
      }
    }
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, ID
void vtkMRMLViewNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  this->Superclass::Copy(anode);
  vtkMRMLViewNode *node = (vtkMRMLViewNode *) anode;

  this->SetBoxVisible(node->GetBoxVisible());
  this->SetFiducialsVisible(node->GetFiducialsVisible());
  this->SetFiducialLabelsVisible(node->GetFiducialLabelsVisible());
  this->SetAxisLabelsVisible(node->GetAxisLabelsVisible());
  this->SetAxisLabelsCameraDependent(node->GetAxisLabelsCameraDependent());
  this->SetFieldOfView(node->GetFieldOfView());
  this->SetLetterSize(node->GetLetterSize());
  this->SetAnimationMode ( node->GetAnimationMode ( ) );
  this->SetViewAxisMode ( node->GetViewAxisMode ( ) );
  this->SetSpinDirection ( node->GetSpinDirection ( ) );
  this->SetAnimationMs ( node->GetAnimationMs() );
  this->SetSpinDegrees (node->GetSpinDegrees ( ));
  this->SetRotateDegrees (node->GetRotateDegrees ( ));
  this->SetRockLength ( node->GetRockLength () );
  this->SetRockCount ( node->GetRockCount ( ) );
  this->SetStereoType ( node->GetStereoType ( ) );
  this->SetRenderMode ( node->GetRenderMode() );
  this->SetUseDepthPeeling ( node->GetUseDepthPeeling() );
  this->SetFPSVisible ( node->GetFPSVisible() );

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLViewNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "BoxVisible:        " << this->BoxVisible << "\n";
  os << indent << "FiducialsVisible:        " << this->FiducialsVisible << "\n";
  os << indent << "FiducialLabelsVisible:        " << this->FiducialLabelsVisible << "\n";
  os << indent << "AxisLabelsVisible: " << this->AxisLabelsVisible << "\n";
  os << indent << "AxisLabelsCameraDependent: " << this->AxisLabelsCameraDependent << "\n";
  os << indent << "FieldOfView:       " << this->FieldOfView << "\n";
  os << indent << "LetterSize:       " << this->LetterSize << "\n";
  os << indent << "SpinDirection:       " << this->SpinDirection << "\n";
  os << indent << "AnimationMs:       " << this->AnimationMs << "\n";
  os << indent << "SpinDegrees:       " << this->SpinDegrees << "\n";
  os << indent << "RotateDegrees:       " << this->RotateDegrees << "\n";
  os << indent << "AnimationMode:       " << this->AnimationMode << "\n";
  os << indent << "ViewAxisMode:       " << this->ViewAxisMode << "\n";
  os << indent << "RockLength:       " << this->RockLength << "\n";
  os << indent << "RockCount:       " << this->RockCount << "\n";
  os << indent << "StereoType:       " << this->StereoType << "\n";
  os << indent << "RenderMode:       " << this->RenderMode << "\n";
  os << indent << "UseDepthPeeling:       " << this->UseDepthPeeling << "\n";
  os << indent << "FPSVisible:       " << this->FPSVisible << "\n";
}

//------------------------------------------------------------------------------
double* vtkMRMLViewNode::defaultBackgroundColor()
{
  //static double backgroundColor[3] = {0.70196, 0.70196, 0.90588};
  static double backgroundColor[3] = {0.7568627450980392,
                                      0.7647058823529412,
                                      0.9098039215686275};
  return backgroundColor;
}

//------------------------------------------------------------------------------
double* vtkMRMLViewNode::defaultBackgroundColor2()
{
  static double backgroundColor2[3] = {0.4549019607843137,
                                       0.4705882352941176,
                                       0.7450980392156863};
  return backgroundColor2;
}
