/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLFiberBundleLineDisplayNode.cxx,v $
Date:      $Date: 2006/03/03 22:26:39 $
Version:   $Revision: 1.3 $

=========================================================================auto=*/

#include "vtkObjectFactory.h"
#include "vtkCallbackCommand.h"
#include "vtkCellData.h"

#include "vtkPolyDataTensorToColor.h"

#include "vtkMRMLScene.h"
#include "vtkMRMLNode.h"
#include "vtkMRMLFiberBundleLineDisplayNode.h"
#include "vtkMRMLDiffusionTensorDisplayPropertiesNode.h"

//------------------------------------------------------------------------------
vtkMRMLFiberBundleLineDisplayNode* vtkMRMLFiberBundleLineDisplayNode::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMRMLFiberBundleLineDisplayNode");
  if(ret)
    {
    return (vtkMRMLFiberBundleLineDisplayNode*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMRMLFiberBundleLineDisplayNode;
}

//-----------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLFiberBundleLineDisplayNode::CreateNodeInstance()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMRMLFiberBundleLineDisplayNode");
  if(ret)
    {
    return (vtkMRMLFiberBundleLineDisplayNode*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMRMLFiberBundleLineDisplayNode;
}


//----------------------------------------------------------------------------
vtkMRMLFiberBundleLineDisplayNode::vtkMRMLFiberBundleLineDisplayNode()
{
  this->TensorToColor = vtkPolyDataTensorToColor::New();
  this->ColorMode = vtkMRMLFiberBundleDisplayNode::colorModeScalar;
}


//----------------------------------------------------------------------------
vtkMRMLFiberBundleLineDisplayNode::~vtkMRMLFiberBundleLineDisplayNode()
{
  this->RemoveObservers ( vtkCommand::ModifiedEvent, this->MRMLCallbackCommand );
  this->TensorToColor->Delete();
}

//----------------------------------------------------------------------------
void vtkMRMLFiberBundleLineDisplayNode::WriteXML(ostream& of, int nIndent)
{
  // Write all attributes not equal to their defaults
  
  Superclass::WriteXML(of, nIndent);
}



//----------------------------------------------------------------------------
void vtkMRMLFiberBundleLineDisplayNode::ReadXMLAttributes(const char** atts)
{

  Superclass::ReadXMLAttributes(atts);
}


//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, ID
void vtkMRMLFiberBundleLineDisplayNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  //vtkMRMLFiberBundleLineDisplayNode *node = (vtkMRMLFiberBundleLineDisplayNode *) anode;

}

//----------------------------------------------------------------------------
void vtkMRMLFiberBundleLineDisplayNode::PrintSelf(ostream& os, vtkIndent indent)
{
  //int idx;
  
  Superclass::PrintSelf(os,indent);
}

 
//----------------------------------------------------------------------------
void vtkMRMLFiberBundleLineDisplayNode::SetPolyData(vtkPolyData *glyphPolyData)
{
  if (this->TensorToColor)
    {
    this->TensorToColor->SetInput(glyphPolyData);
    }
}

//----------------------------------------------------------------------------
vtkPolyData* vtkMRMLFiberBundleLineDisplayNode::GetPolyData()
{
  if (this->TensorToColor)
    {
      if (this->GetColorMode ( ) == vtkMRMLFiberBundleDisplayNode::colorModeUseCellScalars)
      {
        vtkPolyData* CurrentPolyData =vtkPolyData::SafeDownCast(this->TensorToColor->GetInput());
        this->SetActiveScalarName("ClusterId");
        if (CurrentPolyData->GetCellData()->HasArray("ClusterId"))
        {
          CurrentPolyData->GetCellData()->GetArray("ClusterId")->GetRange(this->ScalarRange);
        }
        return CurrentPolyData;
      }
      else 
      {
        this->SetActiveScalarName("FA");
        this->UpdatePolyDataPipeline();
        this->TensorToColor->Update();
        return this->TensorToColor->GetOutput();
      }
    }
  else
    {
    return NULL;
    }
}

//----------------------------------------------------------------------------
void vtkMRMLFiberBundleLineDisplayNode::UpdatePolyDataPipeline() 
{
  // set display properties according to the tensor-specific display properties node for glyphs
  vtkMRMLDiffusionTensorDisplayPropertiesNode * DiffusionTensorDisplayNode = this->GetDiffusionTensorDisplayPropertiesNode( );
  
  if (DiffusionTensorDisplayNode != NULL) {
    // TO DO: need filter to calculate FA, average FA, etc. as requested
    
    
    // set line coloring
   
    if (this->GetColorMode ( ) == vtkMRMLFiberBundleDisplayNode::colorModeSolid)
      {
      this->ScalarVisibilityOff( );
      this->TensorToColor->SetExtractScalar(0);
      }
    else if (this->GetColorMode ( ) == vtkMRMLFiberBundleDisplayNode::colorModeUseCellScalars)
    {
      this->ScalarVisibilityOn( );
      this->TensorToColor->SetExtractScalar(0); // force a copy of the data
    }
    else  
      {
      if (this->GetColorMode ( ) == vtkMRMLFiberBundleDisplayNode::colorModeScalar)
        {
        this->ScalarVisibilityOn( );
        this->TensorToColor->SetExtractScalar(1);

        switch ( DiffusionTensorDisplayNode->GetColorGlyphBy( ))
          {
          case vtkMRMLDiffusionTensorDisplayPropertiesNode::FractionalAnisotropy:
            {
              vtkDebugMacro("coloring with FA==============================");
              this->TensorToColor->ColorGlyphsByFractionalAnisotropy( );
            }
            break;
          case vtkMRMLDiffusionTensorDisplayPropertiesNode::LinearMeasure:
            {
              vtkDebugMacro("coloring with Cl=============================");
              this->TensorToColor->ColorGlyphsByLinearMeasure( );
            }
            break;
          case vtkMRMLDiffusionTensorDisplayPropertiesNode::Trace:
            {
              vtkDebugMacro("coloring with trace =================");
              this->TensorToColor->ColorGlyphsByTrace( );
            }
            break;
          case vtkMRMLDiffusionTensorDisplayPropertiesNode::ColorOrientation:
            {
              vtkDebugMacro("coloring with orientation =================");
              this->TensorToColor->ColorGlyphsByOrientation( );
                vtkMRMLNode* ColorNode = this->GetScene()->GetNodeByID("vtkMRMLColorTableNodeFullRainbow");
                if (ColorNode)
                {
                  this->SetAndObserveColorNodeID(ColorNode->GetID());
                }
            }
            break;
          case vtkMRMLDiffusionTensorDisplayPropertiesNode::PlanarMeasure:
            {
              vtkDebugMacro("coloring with planar");
              this->TensorToColor->ColorGlyphsByPlanarMeasure( );
            }
            break;
          case vtkMRMLDiffusionTensorDisplayPropertiesNode::MaxEigenvalue:
            {
              vtkDebugMacro("coloring with max eigenval");
              this->TensorToColor->ColorGlyphsByMaxEigenvalue( );
            }
            break;
          case vtkMRMLDiffusionTensorDisplayPropertiesNode::MidEigenvalue:
            {
              vtkDebugMacro("coloring with mid eigenval");
              this->TensorToColor->ColorGlyphsByMidEigenvalue( );
            }
            break;
          case vtkMRMLDiffusionTensorDisplayPropertiesNode::MinEigenvalue:
            {
              vtkDebugMacro("coloring with min eigenval");
              this->TensorToColor->ColorGlyphsByMinEigenvalue( );
            }
            break;
          case vtkMRMLDiffusionTensorDisplayPropertiesNode::RelativeAnisotropy:
            {
              vtkDebugMacro("coloring with relative anisotropy");
              this->TensorToColor->ColorGlyphsByRelativeAnisotropy( );
            }
            break;
          default:
            {
            vtkDebugMacro("coloring with relative anisotropy");
            this->ScalarVisibilityOff( );
            this->TensorToColor->SetExtractScalar(0);
            }
            break;
            
          }
        }
      }   
    }
  else
    {
    this->ScalarVisibilityOff( );
    this->TensorToColor->SetExtractScalar(0);
    }

  if ( this->GetScalarVisibility() && this->TensorToColor->GetInput() != NULL )
    {
      if (this->GetColorMode ( ) != vtkMRMLFiberBundleDisplayNode::colorModeUseCellScalars)
      {
        const int ScalarInvariant = this->GetColorMode();
        double range[2];
        if (vtkMRMLDiffusionTensorDisplayPropertiesNode::ScalarInvariantHasKnownScalarRange(ScalarInvariant))
        {
          vtkMRMLDiffusionTensorDisplayPropertiesNode::ScalarInvariantKnownScalarRange(ScalarInvariant, range);
        } else {
          this->TensorToColor->Update();
          this->TensorToColor->GetOutput()->GetScalarRange(range);
        }
        this->ScalarRange[0] = range[0];
        this->ScalarRange[1] = range[1];
    }}
}

