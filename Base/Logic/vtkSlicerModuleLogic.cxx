/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

=========================================================================auto=*/

// SlicerLogic includes
#include "vtkSlicerModuleLogic.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkSlicerModuleLogic, "$Revision$");
vtkStandardNewMacro(vtkSlicerModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerModuleLogic::vtkSlicerModuleLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerModuleLogic::~vtkSlicerModuleLogic()
{
  this->SetApplicationLogic(0);
}

//----------------------------------------------------------------------------
vtkSlicerApplicationLogic* vtkSlicerModuleLogic::GetApplicationLogic()
{
  return vtkSlicerApplicationLogic::SafeDownCast(this->GetMRMLApplicationLogic());
}

//----------------------------------------------------------------------------
void vtkSlicerModuleLogic::SetApplicationLogic(vtkSlicerApplicationLogic* logic)
{
  this->SetMRMLApplicationLogic(logic);
}

//----------------------------------------------------------------------------
std::string vtkSlicerModuleLogic::GetModuleShareDirectory()const
{
  return this->ModuleShareDirectory;
}

//----------------------------------------------------------------------------
void vtkSlicerModuleLogic::SetModuleShareDirectory(const std::string& shareDirectory)
{
  if (this->ModuleShareDirectory != shareDirectory)
    {
    this->ModuleShareDirectory = shareDirectory;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkSlicerModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
