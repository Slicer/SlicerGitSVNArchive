/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLFiberBundleTubeDisplayNode.cxx,v $
Date:      $Date: 2006/03/03 22:26:39 $
Version:   $Revision: 1.3 $

=========================================================================auto=*/

// MRML includes
#include "vtkMRMLDiffusionTensorDisplayPropertiesNode.h"
#include "vtkMRMLFiberBundleTubeDisplayNode.h"
#include "vtkMRMLScene.h"

// Teem includes
#include "vtkPolyDataTensorToColor.h"
#include "vtkPolyDataColorLinesByOrientation.h"

// VTK includes
#include <vtkAlgorithmOutput.h>
#include <vtkAssignAttribute.h>
#include <vtkCallbackCommand.h>
#include <vtkCellData.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkTubeFilter.h>

// STD includes
#include <sstream>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLFiberBundleTubeDisplayNode);

//----------------------------------------------------------------------------
vtkMRMLFiberBundleTubeDisplayNode::vtkMRMLFiberBundleTubeDisplayNode()
{
  this->ColorMode = vtkMRMLFiberBundleDisplayNode::colorModeScalar;
  this->ColorLinesByOrientation = vtkPolyDataColorLinesByOrientation::New();
  this->ColorLinesByOrientation->SetInputConnection(
    this->Superclass::GetOutputPolyDataConnection());

  this->TubeFilter = vtkTubeFilter::New();
  this->TubeNumberOfSides = 6;
  this->TubeRadius = 0.5;

  this->TubeFilter->SetNumberOfSides(this->GetTubeNumberOfSides());
  this->TubeFilter->SetRadius(this->GetTubeRadius());
  this->TubeFilter->SetInputConnection(
    this->Superclass::GetOutputPolyDataConnection());

  this->TensorToColor = vtkPolyDataTensorToColor::New();
  this->TensorToColor->SetInputConnection(this->TubeFilter->GetOutputPort());

  this->Ambient = 0.25;
  this->Diffuse = 0.8;
  this->Specular = 0.25;
  this->Power = 20;

  this->UpdatePolyDataPipeline();
}

//----------------------------------------------------------------------------
vtkMRMLFiberBundleTubeDisplayNode::~vtkMRMLFiberBundleTubeDisplayNode()
{
  this->RemoveObservers ( vtkCommand::ModifiedEvent, this->MRMLCallbackCommand );
  this->TubeFilter->Delete();
  this->TensorToColor->Delete();
  this->ColorLinesByOrientation->Delete();
}

//----------------------------------------------------------------------------
void vtkMRMLFiberBundleTubeDisplayNode::WriteXML(ostream& of, int nIndent)
{
  // Write all attributes not equal to their defaults
  this->Superclass::WriteXML(of, nIndent);

  vtkIndent indent(nIndent);
  of << indent << " tubeRadius =\"" << this->TubeRadius << "\"";
  of << indent << " tubeNumberOfSides =\"" << this->TubeNumberOfSides << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLFiberBundleTubeDisplayNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  this->Superclass::ReadXMLAttributes(atts);

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

  this->Superclass::Copy(anode);
  vtkMRMLFiberBundleTubeDisplayNode *node = (vtkMRMLFiberBundleTubeDisplayNode *) anode;

  this->SetTubeNumberOfSides(node->TubeNumberOfSides);
  this->SetTubeRadius(node->TubeRadius);

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLFiberBundleTubeDisplayNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "TubeNumberOfSides:             " << this->TubeNumberOfSides << "\n";
  os << indent << "TubeRadius:             " << this->TubeRadius << "\n";
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkMRMLFiberBundleTubeDisplayNode::GetOutputPolyDataConnection()
{
  if (this->GetColorMode () == vtkMRMLFiberBundleDisplayNode::colorModeScalarData)
    {
    return this->TubeFilter->GetOutputPort();
    }
  return this->TensorToColor->GetOutputPort();
}

//----------------------------------------------------------------------------
void vtkMRMLFiberBundleTubeDisplayNode::UpdatePolyDataPipeline()
{
  this->Superclass::UpdatePolyDataPipeline();

  this->ColorLinesByOrientation->SetInputConnection(
    this->Superclass::GetOutputPolyDataConnection());
  this->TubeFilter->SetInputConnection(
    this->Superclass::GetOutputPolyDataConnection());

  if (!this->Visibility)
    {
    return;
    }

  vtkDebugMacro("Updating the PolyData Pipeline *****************************");
  // set display properties according to the tensor-specific display properties node for glyphs
  vtkMRMLDiffusionTensorDisplayPropertiesNode * DiffusionTensorDisplayPropertiesNode =
    this->GetDiffusionTensorDisplayPropertiesNode( );

  this->TubeFilter->SetNumberOfSides(this->GetTubeNumberOfSides());
  this->TubeFilter->SetRadius(this->GetTubeRadius());

  // set line coloring
  if (this->GetColorMode ( ) == vtkMRMLFiberBundleDisplayNode::colorModeSolid)
    {
    this->TensorToColor->SetExtractScalar(0);
    }
  else if (this->GetColorMode ( ) == vtkMRMLFiberBundleDisplayNode::colorModeUseCellScalars)
    {
    this->TensorToColor->SetExtractScalar(0); // force a copy of the data
    this->SetActiveScalarName("ClusterId");
    if (this->GetInputPolyData()->GetCellData()->HasArray("ClusterId"))
      {
      this->GetInputPolyData()->GetCellData()->GetArray("ClusterId")->GetRange(this->ScalarRange);
      }
    }
  else if (this->GetColorMode ( ) == vtkMRMLFiberBundleDisplayNode::colorModeMeanFiberOrientation)
    {
    vtkDebugMacro("Color by mean fiber orientation");
    this->ColorLinesByOrientation->SetColorMode(
      this->ColorLinesByOrientation->colorModeMeanFiberOrientation);
    this->TubeFilter->SetInputConnection(this->ColorLinesByOrientation->GetOutputPort());
    vtkMRMLNode* ColorNode = this->GetScene()->GetNodeByID("vtkMRMLColorTableNodeFullRainbow");
    if (ColorNode)
      {
      this->SetAndObserveColorNodeID(ColorNode->GetID());
      }
    }
  else if (this->GetColorMode ( ) == vtkMRMLFiberBundleDisplayNode::colorModePointFiberOrientation)
    {
    vtkDebugMacro("Color by segment orientation");
    this->ColorLinesByOrientation->SetColorMode(
      this->ColorLinesByOrientation->colorModePointFiberOrientation);
    this->TubeFilter->SetInputConnection(this->ColorLinesByOrientation->GetOutputPort());
    vtkMRMLNode* ColorNode = this->GetScene()->GetNodeByID("vtkMRMLColorTableNodeFullRainbow");
    if (ColorNode)
      {
      this->SetAndObserveColorNodeID(ColorNode->GetID());
      }
    }
  else if (this->GetColorMode ( ) == vtkMRMLFiberBundleDisplayNode::colorModeScalarData)
    {
    vtkDebugMacro("coloring with Scalar Data==============================");
    }
  else if ((this->GetColorMode ( ) == vtkMRMLFiberBundleDisplayNode::colorModeScalar) &&
           (DiffusionTensorDisplayPropertiesNode != NULL))
    {
    this->TensorToColor->SetExtractScalar(1);

    switch ( DiffusionTensorDisplayPropertiesNode->GetColorGlyphBy( ))
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
      case vtkMRMLDiffusionTensorDisplayPropertiesNode::SphericalMeasure:
        {
        vtkDebugMacro("coloring with spherical measure");
        this->TensorToColor->ColorGlyphsBySphericalMeasure( );
        }
        break;
      case vtkMRMLDiffusionTensorDisplayPropertiesNode::ParallelDiffusivity:
        {
        vtkDebugMacro("coloring with parallel diff");
        this->TensorToColor->ColorGlyphsByParallelDiffusivity( );
        }
        break;
      case vtkMRMLDiffusionTensorDisplayPropertiesNode::PerpendicularDiffusivity:
        {
        vtkDebugMacro("coloring with perpendicular diff");
        this->TensorToColor->ColorGlyphsByPerpendicularDiffusivity( );
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
        vtkErrorMacro("unknown tube coloring mode");
        this->ScalarVisibilityOff( );
        this->TensorToColor->SetExtractScalar(0);
        }
        break;
      }
    }
  else
    {
    this->TensorToColor->SetExtractScalar(0);
    }

  if (this->GetAutoScalarRange() &&
      this->GetScalarVisibility())
    {
    double range[2] = {0., 1.};
    if ((this->GetColorMode ( ) == vtkMRMLFiberBundleDisplayNode::colorModeScalar) ||
        (this->GetColorMode ( ) == vtkMRMLFiberBundleDisplayNode::colorModeFunctionOfScalar))
      {
      int scalarInvariant = 0;
      if (DiffusionTensorDisplayPropertiesNode)
        {
        scalarInvariant = DiffusionTensorDisplayPropertiesNode->GetColorGlyphBy( );
        }
      if (DiffusionTensorDisplayPropertiesNode &&
          vtkMRMLDiffusionTensorDisplayPropertiesNode::ScalarInvariantHasKnownScalarRange(
            scalarInvariant))
        {
        vtkMRMLDiffusionTensorDisplayPropertiesNode::ScalarInvariantKnownScalarRange(
          scalarInvariant, range);
        }
      else if (this->GetInputPolyData())
        {
        this->GetOutputPolyDataConnection()->GetProducer()->Update();
        vtkPointData *pointData = this->GetOutputPolyData()->GetPointData();
        if (pointData)
          {
          double *activeScalarRange = 0;
          if (pointData->GetArray(this->GetActiveScalarName()))
            {
            activeScalarRange = pointData->GetArray(
                                this->GetActiveScalarName())->GetRange();
            }
          else if (pointData->GetArray(0))
            {
            activeScalarRange = pointData->GetArray(0)->GetRange();
            }
          if (activeScalarRange)
            {
            range[0] = activeScalarRange[0];
            range[1] = activeScalarRange[1];
            }
          }
        }
      }
    else if ((this->GetColorMode ( ) == vtkMRMLFiberBundleDisplayNode::colorModeMeanFiberOrientation) ||
          (this->GetColorMode ( ) == vtkMRMLFiberBundleDisplayNode::colorModePointFiberOrientation))
        {
          vtkMRMLDiffusionTensorDisplayPropertiesNode::ScalarInvariantKnownScalarRange(
            vtkMRMLDiffusionTensorDisplayPropertiesNode::ColorOrientation, range);
        }
    else if (this->GetColorMode() == vtkMRMLFiberBundleDisplayNode::colorModeScalarData &&
             this->GetInputPolyData())
      {
      this->GetInputPolyDataConnection()->GetProducer()->Update();
      vtkPointData *pointData = this->GetOutputPolyData()->GetPointData();
      if (pointData &&
          pointData->GetArray(this->GetActiveScalarName()))
        {
        double *activeScalarRange = pointData->GetArray(
            this->GetActiveScalarName())->GetRange();
        if (activeScalarRange)
          {
          range[0] = activeScalarRange[0];
          range[1] = activeScalarRange[1];
          }
        }
      }
    //this->ScalarRange[0] = range[0];
    //this->ScalarRange[1] = range[1];
    this->SetScalarRange(range);
    }
}

