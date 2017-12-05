/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer

=========================================================================auto=*/

// Logic includes
#include "vtkSlicerTransformLogic.h"

// MRML includes
#include "vtkMRMLScene.h"

// Slicer includes
#include <vtkSlicerConfigure.h> // For Slicer_* macros

// ITK includes
#ifdef Slicer_USE_ITKFactoryRegistration
#include <itkFactoryRegistration.h>
#endif

//-----------------------------------------------------------------------------
int vtkSlicerTransformLogicTest1(int argc, char * argv [])
{
#ifdef Slicer_USE_ITKFactoryRegistration
  itk::itkFactoryRegistration();
#endif

  if(argc < 2)
    {
    std::cerr << "Missing transform file name." << std::endl;
    return EXIT_FAILURE;
    }

  vtkMRMLScene* scene = vtkMRMLScene::New();

  vtkSlicerTransformLogic* transformModuleLogic = vtkSlicerTransformLogic::New();
  transformModuleLogic->SetMRMLScene(scene);
  if (transformModuleLogic->GetMRMLScene() != scene)
    {
    std::cerr << "A MRML Scene must be set to go further." << std::endl;
    return EXIT_FAILURE;
    }

  vtkMRMLTransformNode* transform = transformModuleLogic->AddTransform(argv[1], scene);
  if (transform == 0)
    {
    std::cerr << "Could not read transform file: " << argv[1] << std::endl;
    return EXIT_FAILURE;
    }

  transformModuleLogic->Delete();
  scene->Delete();

  return EXIT_SUCCESS;
}
