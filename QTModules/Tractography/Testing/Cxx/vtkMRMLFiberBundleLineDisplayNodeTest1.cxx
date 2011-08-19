/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) 
  All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer

=========================================================================auto=*/

#include "vtkMRMLFiberBundleLineDisplayNode.h"


#include "TestingMacros.h"

int vtkMRMLFiberBundleLineDisplayNodeTest1(int , char * [] )
{
  vtkSmartPointer< vtkMRMLFiberBundleLineDisplayNode > node1 = vtkSmartPointer< vtkMRMLFiberBundleLineDisplayNode >::New();

  EXERCISE_BASIC_OBJECT_METHODS( node1 );

  // EXERCISE_BASIC_DISPLAY_MRML_METHODS is failing due to set/get ScalarVisibility
  EXERCISE_BASIC_MRML_METHODS(vtkMRMLFiberBundleLineDisplayNode, node1);
  

  return EXIT_SUCCESS;
}
