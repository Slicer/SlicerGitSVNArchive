/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLBSplineTransformNode.cxx,v $
Date:      $Date: 2006/03/17 17:01:53 $
Version:   $Revision: 1.14 $

=========================================================================auto=*/


#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include "vtkITKBSplineTransform.h"

#include "vtkMRMLBSplineTransformNode.h"


//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLBSplineTransformNode);

//----------------------------------------------------------------------------
vtkMRMLBSplineTransformNode::vtkMRMLBSplineTransformNode()
{
  vtkITKBSplineTransform *spline = vtkITKBSplineTransform::New();
  this->SetAndObserveWarpTransformToParent(spline);
  spline->Delete();
}

//----------------------------------------------------------------------------
vtkMRMLBSplineTransformNode::~vtkMRMLBSplineTransformNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLBSplineTransformNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  if (this->WarpTransformToParent != NULL)
    {
    // this transform should be a b-spline
    vtkITKBSplineTransform *spline = dynamic_cast<vtkITKBSplineTransform*>(this->WarpTransformToParent);
    if( spline == NULL )
      {
      vtkErrorMacro("Transform is not a BSpline");
      return;
      }
    of << " order=\"" << spline->GetSplineOrder() << "\" ";
    of << " fixedParam=\"";
    unsigned Nfp = spline->GetNumberOfFixedParameters();
    double const* fp = spline->GetFixedParameters();
    for( unsigned i = 0; i < Nfp; ++i )
      {
      of << " " << fp[i];
      }
    of << "\"";
    of << " switchCoord="
       << (spline->GetSwitchCoordinateSystem()?"\"true\"":"\"false\"");
    double bulk_linear[3][3];
    double bulk_offset[3];
    spline->GetBulkTransform( bulk_linear, bulk_offset );
    of << " bulk=\"";
    for( unsigned i=0; i<3; ++i )
      {
      for( unsigned j=0; j<3; ++j )
        {
        of << " " << bulk_linear[i][j];
        }
      of << " " << bulk_offset[i];
      }
    of << "\"";
    of << " param=\"";
    unsigned Np = spline->GetNumberOfParameters();
    double const* p = spline->GetParameters();
    for( unsigned i = 0; i < Np; ++i )
      {
      of << " " << p[i];
      }
    of << "\"";
    }
}

//----------------------------------------------------------------------------
void vtkMRMLBSplineTransformNode::ReadXMLAttributes(const char** atts)
{
  Superclass::ReadXMLAttributes(atts);

  vtkSmartPointer<vtkITKBSplineTransform> spline;

  const char* attName;
  const char* attValue;
  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "order"))
      {
      std::stringstream ss;
      unsigned val;
      ss << attValue;
      if( ss >> val )
        {
        spline = vtkSmartPointer<vtkITKBSplineTransform>::New();
        spline->SetSplineOrder( val );
        }
      else
        {
        vtkErrorMacro( "couldn't parse bspline order" );
        return;
        }
      }
    else if (!strcmp(attName, "switchCoord"))
      {
      if( spline.GetPointer() == 0 )
        {
        vtkErrorMacro( "order attribute must be processed before parameter attributes" );
        return;
        }
      if (!strcmp(attValue, "true"))
        {
        spline->SetSwitchCoordinateSystem( true );
        }
      else if (!strcmp(attValue, "false"))
        {
        spline->SetSwitchCoordinateSystem( false );
        }
      else
        {
        vtkErrorMacro( "\"" << attValue << "\" is not a valid value for the switchCoord attribute" );
        }
      }
    else if (!strcmp(attName, "fixedParam"))
      {
      if( spline.GetPointer() == 0 )
        {
        vtkErrorMacro( "order attribute must be processed before parameter attributes" );
        return;
        }
      std::stringstream ss;
      double val;
      ss << attValue;
      std::vector<double> vals;
      while( ss >> val )
        {
        vals.push_back( val );
        }
      if( vals.size() != spline->GetNumberOfFixedParameters() )
        {
        vtkErrorMacro( "Incorrect number of fixed parameters: expecting "
                       << spline->GetNumberOfFixedParameters() << "; got "
                       << vals.size() );
        return;
        }
      spline->SetFixedParameters( &vals[0], vals.size() );
      }
    else if (!strcmp(attName, "bulk"))
      {
      if( spline.GetPointer() == 0 )
        {
        vtkErrorMacro( "order attribute must be processed before parameter attributes" );
        return;
        }
      std::stringstream ss;
      double val;
      ss << attValue;
      std::vector<double> vals;
      while( ss >> val )
        {
        vals.push_back( val );
        }
      if( vals.size() != 12 )
        {
        vtkErrorMacro( "Incorrect number of bulk parameters: expecting 12; got "
                       << vals.size() );
        return;
        }
      double linear[3][3];
      double offset[3];
      unsigned k=0;
      for( unsigned i=0; i<3; ++i )
        {
        for( unsigned j=0; j<3; ++j )
          {
          linear[i][j] = vals[k];
          ++k;
          }
        offset[i] = vals[k];
        ++k;
        }
      spline->SetBulkTransform( linear, offset );
      }
    else if (!strcmp(attName, "param"))
      {
      if( spline.GetPointer() == 0 )
        {
        vtkErrorMacro( "order attribute must be processed before parameter attributes" );
        return;
        }
      std::stringstream ss;
      double val;
      ss << attValue;
      std::vector<double> vals;
      while( ss >> val )
        {
        vals.push_back( val );
        }
      if( vals.size() != spline->GetNumberOfParameters() )
        {
        vtkErrorMacro( "Incorrect number of spline parameters: expecting "
                       << spline->GetNumberOfParameters() << "; got "
                       << vals.size() );
        return;
        }
      spline->SetParameters( &vals[0] );
      }
    }

  if( spline.GetPointer() != 0 )
    {
    this->SetAndObserveWarpTransformToParent( spline );
    }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLBSplineTransformNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);

  // BUG: this explicit setting is required because the vtkITKBSplineTransform
  // seems not to implement DeepCopy correctly (spline's Helper is NULL)
  vtkMRMLBSplineTransformNode *otherBSpline = vtkMRMLBSplineTransformNode::SafeDownCast(anode);
  if (otherBSpline)
    {
    this->SetAndObserveWarpTransformToParent(otherBSpline->GetWarpTransformToParent());
    }
}

//----------------------------------------------------------------------------
void vtkMRMLBSplineTransformNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}



// End
