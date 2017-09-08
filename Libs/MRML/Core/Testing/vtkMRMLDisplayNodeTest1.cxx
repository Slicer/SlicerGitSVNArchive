/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH)
  All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer

=========================================================================auto=*/

// MRML includes
#include "vtkMRMLCoreTestingMacros.h"
#include "vtkMRMLDisplayNode.h"

// VTK includes
#include <vtkObjectFactory.h>

//----------------------------------------------------------------------------
class vtkMRMLDisplayNodeTestHelper1 : public vtkMRMLDisplayNode
{
public:
  // Provide a concrete New.
  static vtkMRMLDisplayNodeTestHelper1 *New();

  vtkTypeMacro(vtkMRMLDisplayNodeTestHelper1,vtkMRMLDisplayNode);

  virtual vtkMRMLNode* CreateNodeInstance() VTK_OVERRIDE
    {
    return vtkMRMLDisplayNodeTestHelper1::New();
    }

  const char * GetTypeAsString()
    {
    return "vtkMRMLDisplayNodeTestHelper1";
    }

  int ReadFile()
    {
    std::cout << "vtkMRMLDisplayNodeTestHelper1 pretending to read a file " << std::endl;
    return EXIT_SUCCESS;
    }

  virtual const char* GetNodeTagName() VTK_OVERRIDE
    {
    return "Testing is good";
    }
};
vtkStandardNewMacro(vtkMRMLDisplayNodeTestHelper1);

//----------------------------------------------------------------------------
int vtkMRMLDisplayNodeTest1(int , char * [])
{
  vtkNew<vtkMRMLDisplayNodeTestHelper1> node1;
  EXERCISE_ALL_BASIC_MRML_METHODS(node1.GetPointer());
  return EXIT_SUCCESS;
}
