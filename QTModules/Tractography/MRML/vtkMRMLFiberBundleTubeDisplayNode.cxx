/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLFiberBundleTubeDisplayNode.cxx,v $
Date:      $Date: 2006/03/03 22:26:39 $
Version:   $Revision: 1.3 $

=========================================================================auto=*/
#include <sstream>

#include "vtkObjectFactory.h"
#include "vtkCallbackCommand.h"
#include "vtkCellData.h"

#include "vtkPolyDataTensorToColor.h"
#include "vtkTubeFilter.h"

#include "vtkMRMLDiffusionTensorDisplayPropertiesNode.h"
#include "vtkMRMLFiberBundleTubeDisplayNode.h"

//------------------------------------------------------------------------------
vtkMRMLFiberBundleTubeDisplayNode* vtkMRMLFiberBundleTubeDisplayNode::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMRMLFiberBundleTubeDisplayNode");
  if(ret)
    {
    return (vtkMRMLFiberBundleTubeDisplayNode*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMRMLFiberBundleTubeDisplayNode;
}

//-----------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLFiberBundleTubeDisplayNode::CreateNodeInstance()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMRMLFiberBundleTubeDisplayNode");
  if(ret)
    {
    return (vtkMRMLFiberBundleTubeDisplayNode*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMRMLFiberBundleTubeDisplayNode;
}


//----------------------------------------------------------------------------
vtkMRMLFiberBundleTubeDisplayNode::vtkMRMLFiberBundleTubeDisplayNode()
{
  this->TensorToColor = vtkPolyDataTensorToColor::New();
  this->ColorMode = vtkMRMLFiberBundleDisplayNode::colorModeScalar;

  this->TubeFilter = vtkTubeFilter::New();
  this->TubeNumberOfSides = 6;
  this->TubeRadius = 0.5;
  
  this->TensorToColor->SetInput(this->TubeFilter->GetOutput());

}


//----------------------------------------------------------------------------
vtkMRMLFiberBundleTubeDisplayNode::~vtkMRMLFiberBundleTubeDisplayNode()
{
  this->RemoveObservers ( vtkCommand::ModifiedEvent, this->MRMLCallbackCommand );
  this->TubeFilter->Delete();
  this->TensorToColor->Delete();
}

//----------------------------------------------------------------------------
void vtkMRMLFiberBundleTubeDisplayNode::WriteXML(ostream& of, int nIndent)
{
  // Write all attributes not equal to their defaults
  
  Superclass::WriteXML(of, nIndent);

  vtkIndent indent(nIndent);
  of << indent << " tubeRadius =\"" << this->TubeRadius << "\"";
  of << indent << " tubeNumberOfSides =\"" << this->TubeNumberOfSides << "\"";
}



//----------------------------------------------------------------------------
void vtkMRMLFiberBundleTubeDisplayNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "tubeRadius")) 
      {
      std::stringstream ss;
      ss << attValue;
      ss >> TubeRadius;
      }

    if (!strcmp(attName, "tubeNumberOfSides")) 
      {
      std::stringstream ss;
      ss << attValue;
      ss >> TubeNumberOfSides;
      }
    }  

  this->EndModify(disabledModify);

}


//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, ID
void vtkMRMLFiberBundleTubeDisplayNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);
  vtkMRMLFiberBundleTubeDisplayNode *node = (vtkMRMLFiberBundleTubeDisplayNode *) anode;

  this->SetTubeNumberOfSides(node->TubeNumberOfSides);
  this->SetTubeRadius(node->TubeRadius);

  this->EndModify(disabledModify);

}

//----------------------------------------------------------------------------
void vtkMRMLFiberBundleTubeDisplayNode::PrintSelf(ostream& os, vtkIndent indent)
{
  //int idx;
  
  Superclass::PrintSelf(os,indent);
  os << indent << "TubeNumberOfSides:             " << this->TubeNumberOfSides << "\n";
  os << indent << "TubeRadius:             " << this->TubeRadius << "\n";
}

 
//----------------------------------------------------------------------------
void vtkMRMLFiberBundleTubeDisplayNode::SetPolyData(vtkPolyData *glyphPolyData)
{
  if (this->TubeFilter)
    {
    this->TubeFilter->SetInput(glyphPolyData);
    }
}

//----------------------------------------------------------------------------
vtkPolyData* vtkMRMLFiberBundleTubeDisplayNode::GetPolyData()
{
  if ( this->TubeFilter && this->TubeFilter->GetInput() && this->TensorToColor)
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
      this->TubeFilter->Update();
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
void vtkMRMLFiberBundleTubeDisplayNode::UpdatePolyDataPipeline() 
{
  this->TubeFilter->SetRadius(this->GetTubeRadius ( ) );
  this->TubeFilter->SetNumberOfSides(this->GetTubeNumberOfSides ( ) );

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
      this->TensorToColor->SetExtractScalar(0);
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
    
  if ( this->GetScalarVisibility() && this->TubeFilter->GetInput() != NULL )
    {
      if (this->GetColorMode ( ) != vtkMRMLFiberBundleDisplayNode::colorModeUseCellScalars)
    
     {
    this->TensorToColor->Update();
    double *range = this->TensorToColor->GetOutput()->GetScalarRange();
    this->ScalarRange[0] = range[0];
    this->ScalarRange[1] = range[1];
    // avoid Set not to cause event loops
    //this->SetScalarRange( this->DiffusionTensorGlyphFilter->GetOutput()->GetScalarRange() );
      }}
}

 


