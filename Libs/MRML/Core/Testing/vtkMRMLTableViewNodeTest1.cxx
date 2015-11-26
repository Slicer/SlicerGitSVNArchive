/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH)
  All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer

=========================================================================auto=*/

#include "vtkMRMLScene.h"
#include "vtkMRMLTableNode.h"
#include "vtkMRMLTableViewNode.h"

#include "vtkMRMLCoreTestingMacros.h"
#include "vtkMRMLCoreTestingUtilities.h"

int vtkMRMLTableViewNodeTest1(int , char * [] )
{
  vtkNew<vtkMRMLTableViewNode> node1;

  EXERCISE_BASIC_OBJECT_METHODS( node1.GetPointer() );

  EXERCISE_BASIC_MRML_METHODS(vtkMRMLTableViewNode, node1.GetPointer());

  // Check if modified eventes are only fired if and only if table node ID is changed

  vtkNew<vtkMRMLScene> scene;
  vtkNew<vtkMRMLTableNode> tableNode1;
  vtkNew<vtkMRMLTableNode> tableNode2;
  scene->AddNode(tableNode1.GetPointer());
  scene->AddNode(tableNode2.GetPointer());

  vtkNew<vtkMRMLNodeCallback> callback;
  node1->AddObserver(vtkCommand::AnyEvent, callback.GetPointer());

  callback->ResetNumberOfEvents();
  node1->SetTableNodeID(tableNode1->GetID());
  CHECK_INT(callback->GetNumberOfModified(),1);

  callback->ResetNumberOfEvents();
  node1->SetTableNodeID(tableNode2->GetID());
  CHECK_INT(callback->GetNumberOfModified(),1);

  callback->ResetNumberOfEvents();
  node1->SetTableNodeID(tableNode2->GetID());
  CHECK_INT(callback->GetNumberOfModified(),0);

  callback->ResetNumberOfEvents();
  node1->SetTableNodeID(NULL);
  CHECK_INT(callback->GetNumberOfModified(),1);

  callback->ResetNumberOfEvents();
  node1->SetTableNodeID(NULL);
  CHECK_INT(callback->GetNumberOfModified(),0);

  std::cout << "vtkMRMLTableViewNodeTest1 completed successfully" << std::endl;
  return EXIT_SUCCESS;
}
