/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women\"s Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLSliceNode.cxx,v $
Date:      $Date: 2006/03/17 15:10:10 $
Version:   $Revision: 1.2 $

=========================================================================auto=*/


#include "vtkObjectFactory.h"
#include "vtkMRMLSliceNode.h"
#include "vtkMRMLVolumeNode.h"
#include "vtkMRMLTransformNode.h"
#include "vtkMRMLScene.h"

#include "vtkSmartPointer.h"
#include "vtkMatrix4x4.h"
#include "vtkMath.h"

#include "vnl/vnl_double_3.h"

vtkCxxSetObjectMacro(vtkMRMLSliceNode, SliceToRAS, vtkMatrix4x4);

//------------------------------------------------------------------------------
vtkMRMLSliceNode* vtkMRMLSliceNode::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMRMLSliceNode");
  if(ret)
    {
      return (vtkMRMLSliceNode*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMRMLSliceNode;
}

//----------------------------------------------------------------------------

vtkMRMLNode* vtkMRMLSliceNode::CreateNodeInstance()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMRMLSliceNode");
  if(ret)
    {
      return (vtkMRMLSliceNode*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMRMLSliceNode;
}

//----------------------------------------------------------------------------
// Constructor
vtkMRMLSliceNode::vtkMRMLSliceNode()
{
    // set by user
  this->SliceToRAS = vtkMatrix4x4::New();
  this->SliceToRAS->Identity();

  this->JumpMode = OffsetJumpSlice;
  
  this->OrientationString = NULL;
  this->OrientationReference = NULL;

    // calculated by UpdateMatrices()
  this->XYToSlice = vtkMatrix4x4::New();
  this->XYToRAS = vtkMatrix4x4::New();

  // set the default field of view to a convenient size for looking 
  // at slices through human heads (a 1 pixel thick slab 25x25 cm)
  // TODO: how best to represent this as a slab rather than infinitessimal slice?
  this->FieldOfView[0] = 250.0;
  this->FieldOfView[1] = 250.0;
  this->FieldOfView[2] = 1.0;

  this->Dimensions[0] = 256;
  this->Dimensions[1] = 256;
  this->Dimensions[2] = 1;
  this->SliceVisible = 0;
  this->WidgetVisible = 0;
  this->UseLabelOutline = 0;

  this->LayoutGridColumns = 1;
  this->LayoutGridRows = 1;

  this->SetOrientationToAxial();

  this->PrescribedSliceSpacing[0] = this->PrescribedSliceSpacing[1] = this->PrescribedSliceSpacing[2] = 1;
  this->SliceSpacingMode = AutomaticSliceSpacingMode;

  this->ActiveSlice = 0;

  this->Interacting = 0;
  this->InteractionFlags = 0;

  this->LayoutLabel = NULL;
}

//----------------------------------------------------------------------------
vtkMRMLSliceNode::~vtkMRMLSliceNode()
{
  if ( this->SliceToRAS != NULL) 
    {
    this->SliceToRAS->Delete();
    }
  if ( this->XYToSlice != NULL) 
    {
    this->XYToSlice->Delete();
    }
  if ( this->XYToRAS != NULL) 
    {
    this->XYToRAS->Delete();
    }
  if ( this->OrientationString )
    {
    delete [] this->OrientationString;
    }
  if ( this->OrientationReference )
    {
    delete [] this->OrientationReference;
    }
  if ( this->LayoutLabel )
    {
    delete [] this->LayoutLabel;
    }
  this->SetLayoutName(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLSliceNode::SetOrientation(const char* orientation)
{
  if (!orientation)
    {
    return;
    }
 if (!strcmp(orientation, "Axial"))
    {
    this->SetOrientationToAxial();
    }
  else if (!strcmp(orientation, "Sagittal"))
    {
    this->SetOrientationToSagittal();
    }
  else if (!strcmp(orientation, "Coronal"))
    {
    this->SetOrientationToCoronal();
    }
  else if (!strcmp(orientation, "Reformat"))
    {
    this->SetOrientationToReformat();
    }
  else
    {
    vtkErrorMacro("SetOrientation: invalid orientation: " << orientation);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLSliceNode::SetOrientationToReformat()
{
    // Don't need to do anything.  Leave the matrices where they were
    // so the reformat starts where you were.

    this->SetOrientationString( "Reformat" );
}


//----------------------------------------------------------------------------
void vtkMRMLSliceNode::SetOrientationToAxial()
{
    // Px -> Patient Left
    this->SliceToRAS->SetElement(0, 0, -1.0);
    this->SliceToRAS->SetElement(1, 0,  0.0);
    this->SliceToRAS->SetElement(2, 0,  0.0);
    // Py -> Patient Anterior
    this->SliceToRAS->SetElement(0, 1,  0.0);
    this->SliceToRAS->SetElement(1, 1,  1.0);
    this->SliceToRAS->SetElement(2, 1,  0.0);
    // Pz -> Patient Inferior
    this->SliceToRAS->SetElement(0, 2,  0.0);
    this->SliceToRAS->SetElement(1, 2,  0.0);
    this->SliceToRAS->SetElement(2, 2,  1.0);

    this->SetOrientationString( "Axial" );
    this->SetOrientationReference( "Axial" );
    this->UpdateMatrices();
}

//----------------------------------------------------------------------------
void vtkMRMLSliceNode::SetOrientationToSagittal()
{
    // Px -> Patient Posterior
    this->SliceToRAS->SetElement(0, 0,  0.0);
    this->SliceToRAS->SetElement(1, 0, -1.0);
    this->SliceToRAS->SetElement(2, 0,  0.0);
    // Py -> Patient Inferior
    this->SliceToRAS->SetElement(0, 1,  0.0);
    this->SliceToRAS->SetElement(1, 1,  0.0);
    this->SliceToRAS->SetElement(2, 1,  1.0);
    // Pz -> Patient Right
    this->SliceToRAS->SetElement(0, 2,  1.0);
    this->SliceToRAS->SetElement(1, 2,  0.0);
    this->SliceToRAS->SetElement(2, 2,  0.0);

    this->SetOrientationString( "Sagittal" );
    this->SetOrientationReference( "Sagittal" );
    this->UpdateMatrices();
}


//----------------------------------------------------------------------------
void vtkMRMLSliceNode::SetOrientationToCoronal()
{
    // Px -> Patient Left
    this->SliceToRAS->SetElement(0, 0, -1.0);
    this->SliceToRAS->SetElement(1, 0,  0.0);
    this->SliceToRAS->SetElement(2, 0,  0.0);
    // Py -> Patient Inferior
    this->SliceToRAS->SetElement(0, 1,  0.0);
    this->SliceToRAS->SetElement(1, 1,  0.0);
    this->SliceToRAS->SetElement(2, 1,  1.0);
    // Pz -> Patient Anterior
    this->SliceToRAS->SetElement(0, 2,  0.0);
    this->SliceToRAS->SetElement(1, 2,  1.0);
    this->SliceToRAS->SetElement(2, 2,  0.0);

    this->SetOrientationString( "Coronal" );
    this->SetOrientationReference( "Coronal" );
    this->UpdateMatrices();
}

//----------------------------------------------------------------------------
// Local helper to compare matrices -- TODO: is there a standard version of this?
int vtkMRMLSliceNode::Matrix4x4AreEqual(vtkMatrix4x4 *m1, vtkMatrix4x4 *m2)
{
  int i,j;
  for (i = 0; i < 4; i++)
    {
    for (j = 0; j < 4; j++)
      {
      if ( m1->GetElement(i, j) != m2->GetElement(i, j) )
        {
        return 0;
        }
      }
    }
    return 1;
}

//----------------------------------------------------------------------------
//  Set the SliceToRAS matrix by the postion and orientation of the locator
//
void vtkMRMLSliceNode::SetSliceToRASByNTP (double Nx, double Ny, double Nz,
                         double Tx, double Ty, double Tz,
                         double Px, double Py, double Pz,
                         int Orientation)
{
    vnl_double_3 n, t, c;
    vnl_double_3 negN, negT, negC;

    n[0] = Nx; 
    n[1] = Ny; 
    n[2] = Nz; 
    t[0] = Tx; 
    t[1] = Ty; 
    t[2] = Tz; 

    // Ensure N, T orthogonal:
    //    C = N x T
    //    T = C x N
    c = vnl_cross_3d(n, t);
    t = vnl_cross_3d(c, n);

    // Ensure vectors are normalized
    n.normalize();
    t.normalize();
    c.normalize();

    // Get negative vectors
    negN = -n;
    negT = -t;
    negC = -c;

    this->SliceToRAS->Identity();
    // Tip location
    this->SliceToRAS->SetElement(0, 3, Px);
    this->SliceToRAS->SetElement(1, 3, Py);
    this->SliceToRAS->SetElement(2, 3, Pz);

    switch (Orientation)
    {
        // para-Axial 
        case 0: 
            // N
            this->SliceToRAS->SetElement(0, 2, n[0]);
            this->SliceToRAS->SetElement(1, 2, n[1]);
            this->SliceToRAS->SetElement(2, 2, n[2]);

            // C 
            this->SliceToRAS->SetElement(0, 1, c[0]);
            this->SliceToRAS->SetElement(1, 1, c[1]);
            this->SliceToRAS->SetElement(2, 1, c[2]);
            // T 
            this->SliceToRAS->SetElement(0, 0, t[0]);
            this->SliceToRAS->SetElement(1, 0, t[1]);
            this->SliceToRAS->SetElement(2, 0, t[2]);

            break;

        // para-Sagittal 
        case 1: 
            // T
            this->SliceToRAS->SetElement(0, 2, t[0]);
            this->SliceToRAS->SetElement(1, 2, t[1]);
            this->SliceToRAS->SetElement(2, 2, t[2]);

            // negN 
            this->SliceToRAS->SetElement(0, 1, negN[0]);
            this->SliceToRAS->SetElement(1, 1, negN[1]);
            this->SliceToRAS->SetElement(2, 1, negN[2]);
            // negC 
            this->SliceToRAS->SetElement(0, 0, negC[0]);
            this->SliceToRAS->SetElement(1, 0, negC[1]);
            this->SliceToRAS->SetElement(2, 0, negC[2]);

            break;

        // para-Coronal 
        case 2: 
            // C 
            this->SliceToRAS->SetElement(0, 2, c[0]);
            this->SliceToRAS->SetElement(1, 2, c[1]);
            this->SliceToRAS->SetElement(2, 2, c[2]);
            // negN 
            this->SliceToRAS->SetElement(0, 1, negN[0]);
            this->SliceToRAS->SetElement(1, 1, negN[1]);
            this->SliceToRAS->SetElement(2, 1, negN[2]);
            // T 
            this->SliceToRAS->SetElement(0, 0, t[0]);
            this->SliceToRAS->SetElement(1, 0, t[1]);
            this->SliceToRAS->SetElement(2, 0, t[2]);

            break;
    }

    this->UpdateMatrices();  
}

//----------------------------------------------------------------------------
//  Calculate XYToSlice and XYToRAS 
//  Inputs: Dimenionss, FieldOfView, SliceToRAS
//
void vtkMRMLSliceNode::UpdateMatrices()
{
    double spacing[3];
    unsigned int i;
    vtkSmartPointer<vtkMatrix4x4> xyToSlice = vtkSmartPointer<vtkMatrix4x4>::New();
    vtkSmartPointer<vtkMatrix4x4> xyToRAS = vtkSmartPointer<vtkMatrix4x4>::New();

    int disabledModify = this->StartModify();

    // the mapping from XY output slice pixels to Slice Plane coordinate
    xyToSlice->Identity();
    if (this->Dimensions[0] > 0 &&
        this->Dimensions[1] > 0 &&
        this->Dimensions[2] > 0)
      {
      for (i = 0; i < 3; i++)
        {
        spacing[i] = this->FieldOfView[i] / this->Dimensions[i];
        xyToSlice->SetElement(i, i, spacing[i]);
        xyToSlice->SetElement(i, 3, -this->FieldOfView[i] / 2.);
        }
      //vtkWarningMacro( << "FieldOfView[2] = " << this->FieldOfView[2] << ", Dimensions[2] = " << this->Dimensions[2] );
      //xyToSlice->SetElement(2, 2, 1.);

      xyToSlice->SetElement(2, 3, 0.);
      }

    // the mapping from slice plane coordinates to RAS 
    // (the Orienation as in Axial, Sagittal, Coronal)
    // 
    // The combined transform:
    //
    // | R | = [Slice to RAS ] [ XY to Slice ]  | X |
    // | A |                                    | Y |
    // | S |                                    | Z |
    // | 1 |                                    | 1 |
    //
    // or
    //
    // RAS = XYToRAS * XY
    //
    vtkMatrix4x4::Multiply4x4(this->SliceToRAS, xyToSlice, xyToRAS);

    // check to see if the matrix actually changed
    if ( !Matrix4x4AreEqual (xyToRAS, this->XYToRAS) )
      {
      this->XYToSlice->DeepCopy( xyToSlice );
      this->XYToRAS->DeepCopy( xyToRAS );
      this->Modified();  // RSierra 3/9/07 This triggesr the update on
                         // the windows. In the IGT module when the
                         // slices are updated by the tracker it might
                         // be better to trigger the update AFTER all
                         // positions are modified to synchoronously
                         // update what the user sees ...
      }

    const char *orientationString = "Reformat";
    if ( this->SliceToRAS->GetElement(0, 0) == -1.0 &&
         this->SliceToRAS->GetElement(1, 0) ==  0.0 &&
         this->SliceToRAS->GetElement(2, 0) ==  0.0 &&
         this->SliceToRAS->GetElement(0, 1) ==  0.0 &&
         this->SliceToRAS->GetElement(1, 1) ==  1.0 &&
         this->SliceToRAS->GetElement(2, 1) ==  0.0 &&
         this->SliceToRAS->GetElement(0, 2) ==  0.0 &&
         this->SliceToRAS->GetElement(1, 2) ==  0.0 &&
         this->SliceToRAS->GetElement(2, 2) ==  1.0 )
      {
        orientationString = "Axial";
      }

    if ( this->SliceToRAS->GetElement(0, 0) ==  0.0 &&
         this->SliceToRAS->GetElement(1, 0) == -1.0 &&
         this->SliceToRAS->GetElement(2, 0) ==  0.0 &&
         this->SliceToRAS->GetElement(0, 1) ==  0.0 &&
         this->SliceToRAS->GetElement(1, 1) ==  0.0 &&
         this->SliceToRAS->GetElement(2, 1) ==  1.0 &&
         this->SliceToRAS->GetElement(0, 2) ==  1.0 &&
         this->SliceToRAS->GetElement(1, 2) ==  0.0 &&
         this->SliceToRAS->GetElement(2, 2) ==  0.0 )
      {
        orientationString = "Sagittal";
      }

    if ( this->SliceToRAS->GetElement(0, 0) == -1.0 &&
         this->SliceToRAS->GetElement(1, 0) ==  0.0 &&
         this->SliceToRAS->GetElement(2, 0) ==  0.0 &&
         this->SliceToRAS->GetElement(0, 1) ==  0.0 &&
         this->SliceToRAS->GetElement(1, 1) ==  0.0 &&
         this->SliceToRAS->GetElement(2, 1) ==  1.0 &&
         this->SliceToRAS->GetElement(0, 2) ==  0.0 &&
         this->SliceToRAS->GetElement(1, 2) ==  1.0 &&
         this->SliceToRAS->GetElement(2, 2) ==  0.0 )
      {
        orientationString = "Coronal";
      }

    this->SetOrientationString( orientationString );

    // as UpdateMatrices can be called from CopyWithSceneWithoutModifiedEvent
    // (typically when the scene is closed, slice nodes are reset but shouldn't
    // fire events. We should respect the modifiedWasDisabled flag.
    this->EndModify(disabledModify);
}


//----------------------------------------------------------------------------
void vtkMRMLSliceNode::WriteXML(ostream& of, int nIndent)
{
  int i;

  Superclass::WriteXML(of, nIndent);

  vtkIndent indent(nIndent);

  of << indent << " fieldOfView=\"" << 
        this->FieldOfView[0] << " " <<
        this->FieldOfView[1] << " " <<
        this->FieldOfView[2] << "\"";

  of << indent << " dimensions=\"" << 
        this->Dimensions[0] << " " <<
        this->Dimensions[1] << " " <<
        this->Dimensions[2] << "\"";

  of << indent << " activeSlice=\"" << this->ActiveSlice << "\"";
  
  of << indent << " layoutGridRows=\"" << 
        this->LayoutGridRows << "\"";

  of << indent << " layoutGridColumns=\"" << 
        this->LayoutGridColumns << "\"";

  std::stringstream ss;
  int j;
  for (i=0; i<4; i++) 
    {
    for (j=0; j<4; j++) 
      {
      ss << this->SliceToRAS->GetElement(i,j);
      if ( !( i==3 && j==3) )
        {
        ss << " ";
        }
      }
    }
  of << indent << " sliceToRAS=\"" << ss.str().c_str() << "\"";
  if (this->GetLayoutName())
    {
    of << indent << " layoutName=\"" << this->GetLayoutName() << "\"";
    }
  if (this->OrientationString)
    {
    of << indent << " orientation=\"" << this->OrientationString << "\"";
    }
  if (this->OrientationReference)
    {
    of << indent << " orientationReference=\"" << this->OrientationReference << "\"";
    }
  of << indent << " jumpMode=\"" << this->JumpMode << "\"";
  of << indent << " sliceVisibility=\"" << (this->SliceVisible ? "true" : "false") << "\"";
  of << indent << " widgetVisibility=\"" << (this->WidgetVisible ? "true" : "false") << "\"";
  of << indent << " useLabelOutline=\"" << (this->UseLabelOutline ? "true" : "false") << "\"";
  of << indent << " sliceSpacingMode=\"" << this->SliceSpacingMode << "\"";
  of << indent << " prescribedSliceSpacing=\""
     << this->PrescribedSliceSpacing[0] << " "
     << this->PrescribedSliceSpacing[1] << " "
     << this->PrescribedSliceSpacing[2] << "\"";


}

//----------------------------------------------------------------------------
void vtkMRMLSliceNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "layoutLabel")) 
      {
      this->SetLayoutLabel( attValue );
      }
    else if (!strcmp(attName, "fieldOfView")) 
      {
      std::stringstream ss;
      double val;
      ss << attValue;
      int i;
      for (i=0; i<3; i++) 
        {
        ss >> val;
        this->FieldOfView[i] = val;
        }
      }
    else if (!strcmp(attName, "activeSlice")) 
      {
      std::stringstream ss;
      int val;
      ss << attValue;
      ss >> val;
      
      this->ActiveSlice = val;
      }
    else if (!strcmp(attName, "layoutGridRows")) 
      {
      std::stringstream ss;
      int val;
      ss << attValue;
      ss >> val;
      
      this->LayoutGridRows = val;
      }
    else if (!strcmp(attName, "layoutGridColumns")) 
      {
      std::stringstream ss;
      int val;
      ss << attValue;
      ss >> val;
      
      this->LayoutGridColumns = val;
      }
    else if (!strcmp(attName, "activeSlice")) 
      {
      std::stringstream ss;
      int val;
      ss << attValue;
      ss >> val;
      
      this->ActiveSlice = val;
      }
    else if (!strcmp(attName, "jumpMode")) 
      {
      std::stringstream ss;
      int val;
      ss << attValue;
      ss >> val;
      
      this->JumpMode = val;
      }
    else if (!strcmp(attName, "sliceVisibility")) 
      {
      if (!strcmp(attValue,"true")) 
        {
        this->SliceVisible = 1;
        }
      else
        {
        this->SliceVisible = 0;
        }
      }
    else if (!strcmp(attName, "widgetVisibility")) 
      {
      if (!strcmp(attValue,"true")) 
        {
        this->WidgetVisible = 1;
        }
      else
        {
        this->WidgetVisible = 0;
        }
      }
    else if (!strcmp(attName, "useLabelOutline")) 
      {
      if (!strcmp(attValue,"true")) 
        {
        this->UseLabelOutline = 1;
        }
      else
        {
        this->UseLabelOutline = 0;
        }
      }
   else if (!strcmp(attName, "orientation")) 
      {
      this->SetOrientationString( attValue );
      }
   else if (!strcmp(attName, "orientationReference")) 
      {
      this->SetOrientationReference( attValue );
      }
    else if (!strcmp(attName, "layoutName")) 
      {
      this->SetLayoutName( attValue );
      }
   else if (!strcmp(attName, "dimensions")) 
      {
      std::stringstream ss;
      unsigned int val;
      ss << attValue;
      int i;
      for (i=0; i<3; i++) 
        {
        ss >> val;
        this->Dimensions[i] = val;
        }
      }
    else if (!strcmp(attName, "sliceToRAS")) 
      {
      std::stringstream ss;
      double val;
      ss << attValue;
      int i, j;
      for (i=0; i<4; i++) 
        {
        for (j=0; j<4; j++) 
          {
          ss >> val;
          this->SliceToRAS->SetElement(i,j,val);
          }
        }
      }
    else if (!strcmp(attName, "prescribedSliceSpacing"))
      {
      std::stringstream ss;
      double val;
      ss << attValue;
      int i;
      for (i=0; i<3; i++) 
        {
        ss >> val;
        this->PrescribedSliceSpacing[i] = val;
        }
      }
    else if (!strcmp(attName, "sliceSpacingMode"))
      {
      std::stringstream ss;
      int val;
      ss << attValue;
      ss >> val;

      this->SetSliceSpacingMode( val );
      }
    }
  this->UpdateMatrices();

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
// Copy the node\"s attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, SliceID
void vtkMRMLSliceNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);
  vtkMRMLSliceNode *node = vtkMRMLSliceNode::SafeDownCast(anode);

  this->SetLayoutLabel(node->GetLayoutLabel());

  this->SetSliceVisible(node->GetSliceVisible());
  this->SliceToRAS->DeepCopy(node->GetSliceToRAS());
  this->SetOrientationString(node->GetOrientationString());
  this->SetOrientationReference(node->GetOrientationReference());

  this->JumpMode = node->JumpMode;
  this->ActiveSlice = node->ActiveSlice;

  this->LayoutGridColumns = node->LayoutGridColumns;
  this->LayoutGridRows = node->LayoutGridRows;
  
  this->SliceSpacingMode = node->SliceSpacingMode;

  this->WidgetVisible = node->WidgetVisible;
  this->UseLabelOutline = node->UseLabelOutline;

  int i;
  for(i=0; i<3; i++) 
    {
    this->FieldOfView[i] = node->FieldOfView[i];
    this->Dimensions[i] = node->Dimensions[i];
    this->PrescribedSliceSpacing[i] = node->PrescribedSliceSpacing[i];
    }
  this->UpdateMatrices();

  this->EndModify(disabledModify);
  
}

//----------------------------------------------------------------------------
void vtkMRMLSliceNode::Reset()
{
  // The LayoutName is preserved by vtkMRMLNode::Reset, however the orientation
  // (typically associated with the layoutName)is not preserved automatically.
  // This require a custom behavior implemented here.
  std::string orientation = this->GetOrientationString();
  this->Superclass::Reset();
  this->DisableModifiedEventOn();
  this->SetOrientation(orientation.c_str());
  this->DisableModifiedEventOff();
}

//----------------------------------------------------------------------------
void vtkMRMLSliceNode::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  Superclass::PrintSelf(os,indent);
  os << "LayoutLabel: " << this->LayoutLabel << std::endl;

  os << "FieldOfView:\n ";
  for (idx = 0; idx < 3; ++idx) {
    os << indent << " " << this->FieldOfView[idx];
  }
  os << "\n";

  os << "Dimensions:\n ";
  for (idx = 0; idx < 3; ++idx) {
    os << indent << " " << this->Dimensions[idx];
  }
  os << "\n";

  os << indent << "Layout grid: " << this->LayoutGridRows << "x" << this->LayoutGridColumns << "\n";
  os << indent << "Active slice: " << this->ActiveSlice << "\n";
  
  os << indent << "SliceVisible: " <<
    (this->SliceVisible ? "true" : "false") << "\n";
  os << indent << "WidgetVisible: " <<
    (this->WidgetVisible ? "true" : "false") << "\n";
  os << indent << "UseLabelOutline: " <<
    (this->UseLabelOutline ? "true" : "false") << "\n";

  os << indent << "Jump mode: ";
  if (this->JumpMode == CenteredJumpSlice)
    {
    std::cout << "Centered\n";
    }
  else
    {
    std::cout << "Offset\n";
    }
  os << indent << "SliceToRAS: \n";
  this->SliceToRAS->PrintSelf(os, indent.GetNextIndent());

  os << indent << "XYToRAS: \n";
  this->XYToRAS->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Slice spacing mode: " << (this->SliceSpacingMode == AutomaticSliceSpacingMode ? "Automatic" : "Prescribed") << "\n";
  os << indent << "Prescribed slice spacing: (" << this->PrescribedSliceSpacing[0] << ", "
                               << this->PrescribedSliceSpacing[1] << ", "
                               << this->PrescribedSliceSpacing[2] << ")\n";
  os << indent << "Interacting: " <<
    (this->Interacting ? "on" : "off") << "\n";
}

void vtkMRMLSliceNode::JumpSlice(double r, double a, double s)
{
  if (this->JumpMode == CenteredJumpSlice)
    {
    this->JumpSliceByCentering(r, a, s);
    }
  else if (this->JumpMode == OffsetJumpSlice)
    {
    this->JumpSliceByOffsetting(r, a, s);
    }
}


void vtkMRMLSliceNode::JumpSliceByCentering(double r, double a, double s)
{
  vtkMatrix4x4 *sliceToRAS = this->GetSliceToRAS();
  double sr = sliceToRAS->GetElement(0, 3);
  double sa = sliceToRAS->GetElement(1, 3);
  double ss = sliceToRAS->GetElement(2, 3);

  // deduce the slice spacing
  vtkMatrix4x4 *xyzToRAS = this->GetXYToRAS();

  double p1xyz[4] = {0.0,0.0,0.0,1.0};
  double p2xyz[4] = {0.0,0.0,1.0,1.0};

  double p1ras[4], p2ras[4];

  xyzToRAS->MultiplyPoint(p1xyz, p1ras);
  xyzToRAS->MultiplyPoint(p2xyz, p2ras);

  double sliceSpacing = sqrt(vtkMath::Distance2BetweenPoints(p2ras, p1ras));

  if ( r != sr || a != sa || s != ss )
    {
    sliceToRAS->SetElement( 0, 3, r - this->ActiveSlice*sliceSpacing*sliceToRAS->GetElement(0,2) );
    sliceToRAS->SetElement( 1, 3, a - this->ActiveSlice*sliceSpacing*sliceToRAS->GetElement(1,2));
    sliceToRAS->SetElement( 2, 3, s - this->ActiveSlice*sliceSpacing*sliceToRAS->GetElement(2,2) );
    this->UpdateMatrices();
    }
}


void vtkMRMLSliceNode::JumpSliceByOffsetting(double r, double a, double s)
{
  vtkMatrix4x4 *sliceToRAS = this->GetSliceToRAS();
  double sr = sliceToRAS->GetElement(0, 3);
  double sa = sliceToRAS->GetElement(1, 3);
  double ss = sliceToRAS->GetElement(2, 3);

  // deduce the slice spacing
  vtkMatrix4x4 *xyzToRAS = this->GetXYToRAS();

  double p1xyz[4] = {0.0,0.0,0.0,1.0};
  double p2xyz[4] = {0.0,0.0,1.0,1.0};

  double p1ras[4], p2ras[4];

  xyzToRAS->MultiplyPoint(p1xyz, p1ras);
  xyzToRAS->MultiplyPoint(p2xyz, p2ras);

  double sliceSpacing = sqrt(vtkMath::Distance2BetweenPoints(p2ras, p1ras));

  double d;
  d = (r-sr)*sliceToRAS->GetElement(0,2)
      + (a-sa)*sliceToRAS->GetElement(1,2)
      + (s-ss)*sliceToRAS->GetElement(2,2);
  sr += (d - this->ActiveSlice*sliceSpacing)*sliceToRAS->GetElement(0,2);
  sa += (d - this->ActiveSlice*sliceSpacing)*sliceToRAS->GetElement(1,2);
  ss += (d - this->ActiveSlice*sliceSpacing)*sliceToRAS->GetElement(2,2);
  
  sliceToRAS->SetElement( 0, 3, sr );
  sliceToRAS->SetElement( 1, 3, sa );
  sliceToRAS->SetElement( 2, 3, ss );
  this->UpdateMatrices();
}

void vtkMRMLSliceNode::JumpSliceByOffsetting(int k, double r, double a, double s)
{
  // Jump the slice such that the kth slice is at the specified
  // ras. If there are not k slices, then jump the first slice to the
  // specified ras

  if (!(k >=0 && k < this->LayoutGridColumns * this->LayoutGridRows))
    {
    k = 0;
    }
  
  //int oldActiveSlice = this->ActiveSlice;
  this->ActiveSlice = k;
  this->JumpSliceByOffsetting(r, a, s);
  //this->ActiveSlice = oldActiveSlice;
}

void vtkMRMLSliceNode::JumpAllSlices(double r, double a, double s)
{
  vtkMRMLSliceNode *node= NULL;
  vtkMRMLScene *scene = this->GetScene();
  int nnodes = scene->GetNumberOfNodesByClass("vtkMRMLSliceNode");
  for (int n=0; n<nnodes; n++)
    {
    node = vtkMRMLSliceNode::SafeDownCast (
          scene->GetNthNodeByClass(n, "vtkMRMLSliceNode"));
    if ( node != NULL && node != this )
      {
      node->JumpSlice(r, a, s);
      }
    }
}

void vtkMRMLSliceNode::SetFieldOfView(double x, double y, double z)
{
  if ( x != this->FieldOfView[0] || y != this->FieldOfView[1]
       || z != this->FieldOfView[2] )
    {
    this->FieldOfView[0] = x;
    this->FieldOfView[1] = y;
    this->FieldOfView[2] = z;
    this->UpdateMatrices();
    }
}

void vtkMRMLSliceNode::SetDimensions(int x, int y,
                                     int z)
{
  if ( x != this->Dimensions[0] || y != this->Dimensions[1]
       || z != this->Dimensions[2] )
    {
    this->Dimensions[0] = x;
    this->Dimensions[1] = y;
    this->Dimensions[2] = z;
    this->UpdateMatrices();
    }
}

void vtkMRMLSliceNode::SetLayoutGrid(int rows, int columns)
{
  // Much of this code looks more like application logic than data
  // code. Should the adjustments to Dimensions and FieldOfView be
  // pulled out the SetLayoutGrid*() methods and put in the logic/gui
  // level? 
  if (( rows != this->LayoutGridRows )
      || ( columns != this->LayoutGridColumns ))
    {
    // Calculate the scaling and "scaling magnitudes"
    double scaling[3];
    scaling[0] = this->LayoutGridColumns/(double) columns;
    scaling[1] = this->LayoutGridRows / (double) rows;
    scaling[2] = 1.0; // ???

    double scaleMagnitude[3];
    scaleMagnitude[0] = (scaling[0] < 1.0 ? 1.0/scaling[0] : scaling[0]);
    scaleMagnitude[1] = (scaling[1] < 1.0 ? 1.0/scaling[1] : scaling[1]);
    scaleMagnitude[2] = 1.0;
   
    // A change in the LightBox layout changes the dimensions of the
    // slice and the FieldOfView in Z
    this->Dimensions[0] = int( this->Dimensions[0] * scaling[0] );
    this->Dimensions[1] = int( this->Dimensions[1] * scaling[1] );
    this->Dimensions[2] = rows*columns;

    // adjust the field of view in x and y to maintain aspect ratio
    if (scaleMagnitude[0] < scaleMagnitude[1])
      {
      // keep x fov the same, adjust y
      this->FieldOfView[1] *= (scaling[1] / scaling[0]);
      }
    else
      {
      // keep y fov the same, adjust x
      this->FieldOfView[0] *= (scaling[0] / scaling[1]);
      }
    
    // keep the same pixel spacing in z, i.e. update FieldOfView[2]
    this->FieldOfView[2]
      *= (rows*columns
          / (double)(this->LayoutGridRows*this->LayoutGridColumns));

    // cache the layout
    this->LayoutGridRows = rows;
    this->LayoutGridColumns = columns;        
    
    // if the active slice is not on the lightbox, then reset active
    // slice to the last slice in the lightbox
    if (this->ActiveSlice >= this->LayoutGridRows*this->LayoutGridColumns)
      {
      this->ActiveSlice = this->LayoutGridRows*this->LayoutGridColumns - 1;
      }

    this->UpdateMatrices();
    }
}

void vtkMRMLSliceNode::SetLayoutGridRows(int rows)
{
  // Much of this code looks more like application logic than data
  // code. Should the adjustments to Dimensions and FieldOfView be
  // pulled out the SetLayoutGrid*() methods and put in the logic/gui
  // level? 
  if ( rows != this->LayoutGridRows )
    {
    // Calculate the scaling
    double scaling;
    scaling = this->LayoutGridRows / (double) rows;

    // A change in the LightBox layout changes the dimensions of the
    // slice and the FieldOfView in Z
    this->Dimensions[1] = int( this->Dimensions[1] * scaling );
    this->Dimensions[2] = rows*this->LayoutGridColumns;

    // adjust the field of view in x to maintain aspect ratio
    this->FieldOfView[0] /= scaling;
    
    // keep the same pixel spacing in z, i.e. update FieldOfView[2]
    this->FieldOfView[2] *= (rows / (double)this->LayoutGridRows);
    
    // cache the layout
    this->LayoutGridRows = rows;
    
    // if the active slice is not on the lightbox, then reset active
    // slice to the last slice in the lightbox
    if (this->ActiveSlice >= this->LayoutGridRows*this->LayoutGridColumns)
      {
      this->ActiveSlice = this->LayoutGridRows*this->LayoutGridColumns - 1;
      }

    this->UpdateMatrices();
    }
}

void vtkMRMLSliceNode::SetLayoutGridColumns(int cols)
{
  // Much of this code looks more like application logic than data
  // code. Should the adjustments to Dimensions and FieldOfView be
  // pulled out the SetLayoutGrid*() methods and put in the logic/gui
  // level? 
  if ( cols != this->LayoutGridColumns )
    {
    // Calculate the scaling
    double scaling;
    scaling = this->LayoutGridColumns / (double) cols;

    // A change in the LightBox layout changes the dimensions of the
    // slice and the FieldOfView in Z
    this->Dimensions[0] = int( this->Dimensions[0]
                               * (this->LayoutGridColumns / (double) cols));
    this->Dimensions[2] = this->LayoutGridRows*cols;

    // adjust the field of view in y to maintain aspect ratio
    this->FieldOfView[1] /= scaling;
    
    // keep the same pixel spacing in z, i.e. update FieldOfView[2]
    this->FieldOfView[2] *= (cols / (double)this->LayoutGridColumns);
    
    // cache the layout
    this->LayoutGridColumns = cols;
    
    // if the active slice is not on the lightbox, then reset active
    // slice to the last slice in the lightbox
    if (this->ActiveSlice >= this->LayoutGridRows*this->LayoutGridColumns)
      {
      this->ActiveSlice = this->LayoutGridRows*this->LayoutGridColumns - 1;
      }

    this->UpdateMatrices();
    }
}
  
  

void
vtkMRMLSliceNode::SetSliceSpacingModeToAutomatic()
{
  this->SetSliceSpacingMode(AutomaticSliceSpacingMode);
}


void
vtkMRMLSliceNode::SetSliceSpacingModeToPrescribed()
{
  this->SetSliceSpacingMode(PrescribedSliceSpacingMode);
}

void
vtkMRMLSliceNode::SetJumpModeToCentered()
{
  this->SetJumpMode(CenteredJumpSlice);
}

void
vtkMRMLSliceNode::SetJumpModeToOffset()
{
  this->SetJumpMode(OffsetJumpSlice);
}


//----------------------------------------------------------------------------
// Get/Set the current distance from the origin to the slice plane
double vtkMRMLSliceNode::GetSliceOffset()
{
  //
  // - get the current translation in RAS space and convert it to Slice space
  //   by transforming it by the inverse of the upper 3x3 of SliceToRAS
  // - pull out the Z translation part
  //

  vtkSmartPointer<vtkMatrix4x4> sliceToRAS = vtkSmartPointer<vtkMatrix4x4>::New();
  sliceToRAS->DeepCopy( this->GetSliceToRAS() );
  for (int i = 0; i < 3; i++)
    {
    sliceToRAS->SetElement( i, 3, 0.0 );  // Zero out the tranlation portion
    }
  sliceToRAS->Invert();
  double v1[4], v2[4];
  for (int i = 0; i < 4; i++)
    { // get the translation back as a vector
    v1[i] = this->GetSliceToRAS()->GetElement( i, 3 );
    }
  // bring the translation into slice space
  // and overwrite the z part
  sliceToRAS->MultiplyPoint(v1, v2);

  return ( v2[2] );

}

//----------------------------------------------------------------------------
void vtkMRMLSliceNode::SetSliceOffset(double offset)
{
  //
  // Set the Offset
  // - get the current translation in RAS space and convert it to Slice space
  //   by transforming it by the invers of the upper 3x3 of SliceToRAS
  // - replace the z value of the translation with the new value given by the slider
  // - this preserves whatever translation was already in place
  //

  double oldOffset = this->GetSliceOffset();
  if (fabs(offset - oldOffset) <= 1.0e-6)
    {
    return;
    }

  vtkSmartPointer<vtkMatrix4x4> sliceToRAS = vtkSmartPointer<vtkMatrix4x4>::New();
  sliceToRAS->DeepCopy( this->GetSliceToRAS() );
  for (int i = 0; i < 3; i++)
    {
    sliceToRAS->SetElement( i, 3, 0.0 );  // Zero out the tranlation portion
    }
  vtkSmartPointer<vtkMatrix4x4> sliceToRASInverted = vtkSmartPointer<vtkMatrix4x4>::New(); // inverse sliceToRAS
  sliceToRASInverted->DeepCopy( sliceToRAS );
  sliceToRASInverted->Invert();
  double v1[4], v2[4], v3[4];
  for (int i = 0; i < 4; i++)
    { // get the translation back as a vector
    v1[i] = this->GetSliceToRAS()->GetElement( i, 3 );
    }
  // bring the translation into slice space
  // and overwrite the z part
  sliceToRASInverted->MultiplyPoint(v1, v2);

  v2[2] = offset;

  // Now bring the new translation vector back into RAS space
  sliceToRAS->MultiplyPoint(v2, v3);

  // if the translation has changed, update the rest of the matrices
  double eps=1.0e-6;
  if ( fabs(v1[0] - v3[0]) > eps ||
       fabs(v1[1] - v3[1]) > eps ||
       fabs(v1[2] - v3[2]) > eps )
    {
    // copy new translation into sliceToRAS
    for (int i = 0; i < 4; i++)
      {
      sliceToRAS->SetElement( i, 3, v3[i] );
      }
    this->GetSliceToRAS()->DeepCopy( sliceToRAS );
    this->UpdateMatrices();
    }
}

void vtkMRMLSliceNode::RotateToVolumePlane(vtkMRMLVolumeNode *volumeNode)
{

  //
  // unfortunately, I can't think of a simpler way to calculate this, since 
  // the definition of something like "Coronal of an axial oblique" doesn't reduce down to 
  // just a rotation -- could include flips etc.
  //
  // instead:
  // - calculate world space vectors for array axes
  // - find the closest match to patient coordinate to define 'Right' in image space
  // - pick the right vectors to put in the slice matrix to match existing orientation
  //

  if ( volumeNode == NULL ) 
    {
    return;
    }

  vtkSmartPointer<vtkMatrix4x4> ijkToRAS = vtkSmartPointer<vtkMatrix4x4>::New();
  volumeNode->GetIJKToRASMatrix(ijkToRAS);

  // apply the transform 
  vtkMRMLTransformNode *transformNode  = volumeNode->GetParentTransformNode();
  if ( transformNode != NULL ) 
    {
    if ( transformNode->IsTransformToWorldLinear() )
      {
      vtkSmartPointer<vtkMatrix4x4> rasToRAS = vtkSmartPointer<vtkMatrix4x4>::New();
      transformNode->GetMatrixTransformToWorld( rasToRAS );
      rasToRAS->Multiply4x4( rasToRAS, ijkToRAS, ijkToRAS );
      } 
    else 
      {
      vtkErrorMacro( "Cannot handle non-linear transforms" );
      }
    }

  // calculate vectors indicating transformed axis directions in RAS space (normalized)
  // e.g. toRAS[0] is the three-vector in RAS space that points along the row axis in ijk space
  // (toRAS[1] is the column, and toRAS[2] is slice)
  double toRAS[3][3];

  double len[3]; // length of each column vector
  double ele;
  int col, row;
  for (col = 0; col < 3; col++)
    {
    len[col] = 0;
    for (row = 0; row < 3; row++)
      {
      ele = ijkToRAS->GetElement(row, col);
      len[col] += ele*ele;
      }
    len[col] = sqrt(len[col]);
    for (row = 0; row < 3; row++)
      {
      toRAS[col][row] = ijkToRAS->GetElement( row, col ) / len[col];
      }
    }


  //
  // find the closest direction for each of the major axes
  //
 
  // define major directions
  double directions [6][3] = {
                   {  1,  0,  0 },   // right
                   { -1,  0,  0 },   // left
                   {  0,  1,  0 },   // anterior
                   {  0, -1,  0 },   // posterior
                   {  0,  0,  1 },   // superior
                   {  0,  0, -1 } }; // inferior
  
  int closestAxis[3];
  double closestDot[3];

  for (col = 0; col < 3; col++)
    {
    closestDot[col] = -1;
    }
  int direction;
  for (direction = 0; direction < 6; direction++)
    {
    double dot[3];
    for (col = 0; col < 3; col++)
      {
      dot[col] = 0;
      int i;
      for (i = 0; i < 3; i++)
        {
        dot[col] += toRAS[col][i] * directions[direction][i];
        }
      if (dot[col] > closestDot[col]) 
        {
        closestDot[col] = dot[col];
        closestAxis[col] = direction;
        }
      }
    }

  //
  // assign the vectors that correspond to each major direction
  //
  double alignedRAS[6][3];
  for (col = 0; col < 3; col++)
    {
    for (row = 0; row < 3; row++)
      {
      switch (closestAxis[col])
        {
        case 0:  // R
          alignedRAS[0][row] =  toRAS[col][row];
          alignedRAS[1][row] = -toRAS[col][row];
          break;
        case 1:  // L
          alignedRAS[0][row] = -toRAS[col][row];
          alignedRAS[1][row] =  toRAS[col][row];
          break;
        case 2:  // A
          alignedRAS[2][row] =  toRAS[col][row];
          alignedRAS[3][row] = -toRAS[col][row];
          break;
        case 3:  // P
          alignedRAS[2][row] = -toRAS[col][row];
          alignedRAS[3][row] =  toRAS[col][row];
          break;
        case 4:  // S
          alignedRAS[4][row] =  toRAS[col][row];
          alignedRAS[5][row] = -toRAS[col][row];
          break;
        case 5:  // I
          alignedRAS[4][row] = -toRAS[col][row];
          alignedRAS[5][row] =  toRAS[col][row];
          break;
        }
      }
    }


  //
  // plug vectors into slice matrix to best approximate requested orientation
  //

  for (row = 0; row < 3; row++)
    {
    if ( !strcmp(this->GetOrientationReference(), "Sagittal") )
      {
      // first column is 'Posterior'
      this->SliceToRAS->SetElement(row, 0, alignedRAS[3][row]);
      // second column is 'Superior'
      this->SliceToRAS->SetElement(row, 1, alignedRAS[4][row]);
      // third column is 'Right'
      this->SliceToRAS->SetElement(row, 2, alignedRAS[0][row]);
      }
    else if ( !strcmp(this->GetOrientationReference(), "Coronal") )
      {
      // first column is 'Left'
      this->SliceToRAS->SetElement(row, 0, alignedRAS[1][row]);
      // second column is 'Superior'
      this->SliceToRAS->SetElement(row, 1, alignedRAS[4][row]);
      // third column is 'Anterior'
      this->SliceToRAS->SetElement(row, 2, alignedRAS[2][row]);
      }
    else if ( !strcmp(this->GetOrientationReference(), "Axial") )
      {
      // first column is 'Left'
      this->SliceToRAS->SetElement(row, 0, alignedRAS[1][row]);
      // second column is 'Anterior'
      this->SliceToRAS->SetElement(row, 1, alignedRAS[2][row]);
      // third column is 'Superior'
      this->SliceToRAS->SetElement(row, 2, alignedRAS[4][row]);
      }
    else 
      {
      // if not Axial, Sagittal, or Coronal, then assume it is Axial (could also be 'Reformat')
      // but since we don't have a plan for that, map it to Axial
      // first column is 'Left'
      this->SliceToRAS->SetElement(row, 0, alignedRAS[1][row]);
      // second column is 'Anterior'
      this->SliceToRAS->SetElement(row, 1, alignedRAS[2][row]);
      // third column is 'Superior'
      this->SliceToRAS->SetElement(row, 2, alignedRAS[4][row]);
      }
    }

  this->SetOrientationToReformat(); // just sets the string - indicates that this is not patient aligned

  this->Modified();
}
