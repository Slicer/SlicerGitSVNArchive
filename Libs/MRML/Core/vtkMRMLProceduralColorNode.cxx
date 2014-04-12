/*=auto=========================================================================

Portions (c) Copyright 2006 Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLProceduralColorNode.cxx,v $
Date:      $Date: 2006/03/03 22:26:39 $
Version:   $Revision: 1.0 $

=========================================================================auto=*/

#include "vtkMRMLProceduralColorNode.h"

// VTK includes
#include <vtkColorTransferFunction.h>
#include <vtkCommand.h>
#include <vtkEventBroker.h>
#include <vtkObjectFactory.h>

// STD includes
#include <sstream>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLProceduralColorNode);


//----------------------------------------------------------------------------
vtkMRMLProceduralColorNode::vtkMRMLProceduralColorNode()
{
  this->Name = NULL;
  this->SetName("");
  this->FileName = NULL;
  this->StoreColorTransferFunctionInScene = true;

  this->ColorTransferFunction = NULL;
  vtkColorTransferFunction* ctf=vtkColorTransferFunction::New();
  this->SetAndObserveColorTransferFunction(ctf);
  ctf->Delete();
}

//----------------------------------------------------------------------------
vtkMRMLProceduralColorNode::~vtkMRMLProceduralColorNode()
{
  this->SetAndObserveColorTransferFunction(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLProceduralColorNode::WriteXML(ostream& of, int nIndent)
{
  // Write all attributes not equal to their defaults
  Superclass::WriteXML(of, nIndent);

  vtkIndent indent(nIndent);

  if (this->GetStoreColorTransferFunctionInScene())
    {
    of << indent << " ColorMap=\""<< this->GetColorMapAsString() << "\"";
    }
}

//----------------------------------------------------------------------------
void vtkMRMLProceduralColorNode::ReadXMLAttributes(const char** atts)
{
  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL)
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName,"ColorMap"))
      {
      if (this->GetStoreColorTransferFunctionInScene())
        {
        SetColorMapFromString(attValue);
        }
      continue;
      }
    }

}


//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, ID
void vtkMRMLProceduralColorNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  vtkMRMLProceduralColorNode *node = (vtkMRMLProceduralColorNode *) anode;

  this->DeepCopyColorTransferFunction(node->GetColorTransferFunction());
}

//----------------------------------------------------------------------------
void vtkMRMLProceduralColorNode::PrintSelf(ostream& os, vtkIndent indent)
{

  Superclass::PrintSelf(os,indent);
  if (this->ColorTransferFunction != NULL)
    {
    os << indent << "ColorTransferFunction:" << endl;
    this->ColorTransferFunction->PrintSelf(os, indent.GetNextIndent());
    }
}

//-----------------------------------------------------------

void vtkMRMLProceduralColorNode::UpdateScene(vtkMRMLScene *scene)
{
  Superclass::UpdateScene(scene);
}

//---------------------------------------------------------------------------
void vtkMRMLProceduralColorNode::ProcessMRMLEvents ( vtkObject *caller,
                                           unsigned long event,
                                           void *callData )
{
  Superclass::ProcessMRMLEvents(caller, event, callData);
  if (caller!=NULL && caller==this->ColorTransferFunction && event==vtkCommand::ModifiedEvent)
    {
    Modified();
    }
  return;
}

//-----------------------------------------------------------
vtkScalarsToColors* vtkMRMLProceduralColorNode::GetScalarsToColors()
{
  return this->GetColorTransferFunction();
}

//---------------------------------------------------------------------------
const char * vtkMRMLProceduralColorNode::GetTypeAsString()
{
  return this->GetName();
}

//---------------------------------------------------------------------------
bool vtkMRMLProceduralColorNode::SetNameFromColor(int index)
{
  double colour[4];
  this->GetColor(index, colour);
  //this->ColorTransferFunction->GetColor(index, colour);
  std::stringstream ss;
  ss.precision(3);
  ss.setf(std::ios::fixed, std::ios::floatfield);
  ss << "R=";
  ss << colour[0];
  ss << " G=";
  ss << colour[1];
  ss << " B=";
  ss << colour[2];
  if (this->SetColorName(index, ss.str().c_str()) == 0)
    {
    vtkErrorMacro("SetNamesFromColors: error setting name " <<  ss.str().c_str() << " for color index " << index);
    return false;
    }
  return true;
}

//---------------------------------------------------------------------------
int vtkMRMLProceduralColorNode::GetNumberOfColors()
{
  /*
  double *range = this->ColorTransferFunction->GetRange();
  if (!range)
    {
    return 0;
    }
  int numPoints = static_cast<int>(floor(range[1] - range[0]));
  if (range[0] < 0 && range[1] >= 0)
    {
    // add one for zero
    numPoints++;
    }
  return numPoints;
  */
  return this->ColorTransferFunction->GetSize();
}

//---------------------------------------------------------------------------
bool vtkMRMLProceduralColorNode::GetColor(int entry, double* color)
{
  if (entry < 0 || entry >= this->GetNumberOfColors())
    {
    vtkErrorMacro( "vtkMRMLColorTableNode::SetColor: requested entry " << entry << " is out of table range: 0 - " << this->GetNumberOfColors() << ", call SetNumberOfColors" << endl);
    return false;
    }
  /*
  double *range = this->ColorTransferFunction->GetRange();
  if (!range)
    {
    return false;
    }
  this->ColorTransferFunction->GetColor(range[0] + entry, color);
  color[3] = this->ColorTransferFunction->GetAlpha();
  return true;
  */
  double val[6];
  this->ColorTransferFunction->GetNodeValue(entry, val);
  color[0] = val[1]; // r
  color[1] = val[2]; // g
  color[2] = val[3]; // b
  color[3] = this->ColorTransferFunction->GetAlpha();
  return true;
}

//----------------------------------------------------------------------------
void vtkMRMLProceduralColorNode::SetAndObserveColorTransferFunction(vtkColorTransferFunction *ctf)
{
  if (ctf==this->ColorTransferFunction)
    {
    return;
    }
  if (this->ColorTransferFunction != NULL)
    {
    vtkEventBroker::GetInstance()->RemoveObservations(
      this->ColorTransferFunction, vtkCommand::ModifiedEvent, this, this->MRMLCallbackCommand );
    this->ColorTransferFunction->UnRegister(this);
    this->ColorTransferFunction=NULL;
    }
  this->ColorTransferFunction=ctf;
  if ( this->ColorTransferFunction )
    {
    this->ColorTransferFunction->Register(this);
    vtkEventBroker::GetInstance()->AddObservation (
      this->ColorTransferFunction, vtkCommand::ModifiedEvent, this, this->MRMLCallbackCommand );
    }
  this->Modified();
}

//----------------------------------------------------------------------------
std::string vtkMRMLProceduralColorNode::GetColorMapAsString()
{
  if (this->ColorTransferFunction==NULL)
    {
    return "";
    }
  int arraySize=this->ColorTransferFunction->GetSize()*4;
  double* doubleArray=this->ColorTransferFunction->GetDataPointer();
  std::stringstream ss;
  for (int i=0; i<arraySize; i++)
    {
    if (i>0)
      {
      ss << ' ';
      }
    ss << doubleArray[i];
    }
  return ss.str();
}

//----------------------------------------------------------------------------
void vtkMRMLProceduralColorNode::SetColorMapFromString(const char* str)
{
  if (this->ColorTransferFunction==NULL)
    {
    vtkErrorMacro("Failed to set colormap from string: colormap is unavailable");
    return;
    }

  std::vector<double> values;
  std::stringstream ss(str);
  std::string itemString;
  double itemDouble;
  while (std::getline(ss, itemString, ' '))
    {
    std::stringstream itemStream;
    itemStream << itemString;
    itemStream >> itemDouble;
    values.push_back(itemDouble);
    }

  if (values.size()%4!=0)
    {
    vtkErrorMacro("vtkMRMLProceduralColorNode::SetColorTransferFunctionFromString failed: N*4 values are expected");
    return;
    }

  if (values.size()==0)
    {
    vtkErrorMacro("vtkMRMLProceduralColorNode::SetColorTransferFunctionFromString failed: no values are defined");
    return;
    }

  this->ColorTransferFunction->FillFromDataPointer(values.size()/4,&(values[0]));
}

//----------------------------------------------------------------------------
bool vtkMRMLProceduralColorNode::IsColorMapEqual(vtkColorTransferFunction* tf1, vtkColorTransferFunction* tf2)
{
  if (tf1==NULL && tf2==NULL)
    {
    return true;
    }
  if (tf1==NULL || tf2==NULL)
    {
    return false;
    }
  if (tf1->GetSize()!=tf2->GetSize())
    {
    return false;
    }
  int n=4*tf1->GetSize();
  if (n==0)
    {
    return true;
    }
  double* dp1=tf1->GetDataPointer();
  double* dp2=tf2->GetDataPointer();
  for (int i=0; i<n; i++)
    {
    if (dp1[i] != dp2[i])
      {
      return false;
      }
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkMRMLProceduralColorNode::DeepCopyColorTransferFunction(vtkColorTransferFunction* newColorTransferFunction)
{
  int oldModified=this->StartModify();
  if (this->ColorTransferFunction==NULL)
    {
    vtkColorTransferFunction* ctf=vtkColorTransferFunction::New();
    this->SetAndObserveColorTransferFunction(ctf);
    ctf->Delete();
    }
  if (!vtkMRMLProceduralColorNode::IsColorMapEqual(newColorTransferFunction,this->ColorTransferFunction))
    {
    this->ColorTransferFunction->DeepCopy(newColorTransferFunction);
    }
  this->EndModify(oldModified);
}
