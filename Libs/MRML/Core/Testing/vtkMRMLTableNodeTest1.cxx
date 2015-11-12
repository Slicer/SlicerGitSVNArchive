/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH)
  All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer

=========================================================================auto=*/

#include "vtkMRMLTableNode.h"
#include "vtkMRMLTableStorageNode.h"

#include "vtkTable.h"
#include "vtkTestErrorObserver.h"

#include "vtkMRMLCoreTestingMacros.h"

int vtkMRMLTableNodeTest1(int , char * [] )
{
  vtkSmartPointer< vtkMRMLTableNode > node1 = vtkSmartPointer< vtkMRMLTableNode >::New();

  EXERCISE_BASIC_OBJECT_METHODS( node1 );

  EXERCISE_BASIC_MRML_METHODS(vtkMRMLTableNode, node1);


  vtkSmartPointer< vtkMRMLTableNode > node2 = vtkSmartPointer< vtkMRMLTableNode >::New();

  vtkSmartPointer<vtkTest::ErrorObserver> errorWarningObserver = vtkSmartPointer<vtkTest::ErrorObserver>::New();
  node2->AddObserver(vtkCommand::WarningEvent, errorWarningObserver);
  node2->AddObserver(vtkCommand::ErrorEvent, errorWarningObserver);

  vtkTable* table = node2->GetTable();

  // Verify if a proper storage node is created
  vtkSmartPointer< vtkMRMLTableStorageNode > storageNode = vtkSmartPointer< vtkMRMLTableStorageNode >::Take(vtkMRMLTableStorageNode::SafeDownCast(node2->CreateDefaultStorageNode()));
  if (storageNode==NULL)
    {
    std::cerr << "CreateDefaultStorageNode test failed" << std::endl;
    return EXIT_FAILURE;
    }

  // Verify basic add/remove column methods

  if (node2->AddColumn()==NULL)
    {
    std::cerr << "AddColumn test 1a failed" << std::endl;
    return EXIT_FAILURE;
    }
  if (table->GetNumberOfColumns()!=1)
    {
    std::cerr << "AddColumn test 1b failed" << std::endl;
    return EXIT_FAILURE;
    }

  vtkSmartPointer< vtkStringArray > newEmptyArray = vtkSmartPointer< vtkStringArray >::New();
  if (node2->AddColumn(newEmptyArray)==NULL)
    {
    std::cerr << "AddColumn test 2 failed" << std::endl;
    return EXIT_FAILURE;
    }
  if (table->GetNumberOfColumns()!=2)
    {
    std::cerr << "AddColumn test 2b failed" << std::endl;
    return EXIT_FAILURE;
    }

  if (!node2->RemoveColumn(1))
    {
    std::cerr << "RemoveColumn test 1a failed" << std::endl;
    return EXIT_FAILURE;
    }
  if (table->GetNumberOfColumns()!=1)
    {
    std::cerr << "RemoveColumn test 1b failed" << std::endl;
    return EXIT_FAILURE;
    }

  if (node2->AddEmptyRow()!=0)
    {
    std::cerr << "AddEmptyRow test 1a failed" << std::endl;
    return EXIT_FAILURE;
    }
  if (table->GetNumberOfRows()!=1)
    {
    std::cerr << "AddEmptyRow test 1b failed" << std::endl;
    return EXIT_FAILURE;
    }
  node2->AddEmptyRow();
  node2->AddEmptyRow();
  node2->AddEmptyRow();
  if (table->GetNumberOfRows()!=4)
    {
    std::cerr << "AddEmptyRow test 2 failed" << std::endl;
    return EXIT_FAILURE;
    }

  if (!node2->RemoveRow(1))
    {
    std::cerr << "RemoveRow test 1a failed" << std::endl;
    return EXIT_FAILURE;
    }
  if (table->GetNumberOfRows()!=3)
    {
    std::cerr << "RemoveRow test 1b failed" << std::endl;
    return EXIT_FAILURE;
    }

  // Verify that arrays that are shorter than the table size are extended to match the current table size

  vtkSmartPointer< vtkStringArray > newShortArray = vtkSmartPointer< vtkStringArray >::New();
  newShortArray->InsertNextValue("something");
  if (node2->AddColumn(newShortArray)==NULL)
    {
    std::cerr << "AddColumn test 3a failed" << std::endl;
    return EXIT_FAILURE;
    }
  if (table->GetNumberOfRows()!=3)
    {
    std::cerr << "AddColumn test 3b failed" << std::endl;
    return EXIT_FAILURE;
    }
  if (table->GetNumberOfColumns()!=2)
    {
    std::cerr << "AddColumn test 3c failed" << std::endl;
    return EXIT_FAILURE;
    }
  if (newShortArray->GetNumberOfTuples()!=table->GetNumberOfRows())
    {
    std::cerr << "AddColumn test 3d failed" << std::endl;
    return EXIT_FAILURE;
    }

  // Verify that arrays that are shorter than the table extend the table

  vtkSmartPointer< vtkStringArray > newLongArray = vtkSmartPointer< vtkStringArray >::New();
  newLongArray->InsertNextValue("something1");
  newLongArray->InsertNextValue("something2");
  newLongArray->InsertNextValue("something3");
  newLongArray->InsertNextValue("something4");
  newLongArray->InsertNextValue("something5");
  newLongArray->InsertNextValue("something6");
  newLongArray->InsertNextValue("something7");
  newLongArray->InsertNextValue("something8");
  newLongArray->InsertNextValue("something9");
  newLongArray->InsertNextValue("something10");
  if (node2->AddColumn(newLongArray)==NULL)
    {
    std::cerr << "AddColumn test 4a failed" << std::endl;
    return EXIT_FAILURE;
    }
  if (table->GetNumberOfRows()!=10)
    {
    std::cerr << "AddColumn test 4b failed" << std::endl;
    return EXIT_FAILURE;
    }
  if (table->GetNumberOfColumns()!=3)
    {
    std::cerr << "AddColumn test 4c failed" << std::endl;
    return EXIT_FAILURE;
    }
  if (newLongArray->GetNumberOfTuples()!=table->GetNumberOfRows())
    {
    std::cerr << "AddColumn test 4d failed" << std::endl;
    return EXIT_FAILURE;
    }

  if (node2->GetCellText(2,2)!="something3")
    {
    std::cerr << "GetCellText test 1 failed" << std::endl;
    return EXIT_FAILURE;
    }
  // Test out of range cases:
  if (errorWarningObserver->GetError() || errorWarningObserver->GetWarning())
    {
    std::cerr << "Error cases test failed: unexpected errors" << std::endl;
    errorWarningObserver->Clear();
    }
  if (node2->GetCellText(20,2)!="")
    {
    std::cerr << "GetCellText test 2a failed" << std::endl;
    return EXIT_FAILURE;
    }
  // error log is expected
  if (!errorWarningObserver->GetError())
    {
    std::cerr << "GetCellText test 2b failed" << std::endl;
    return EXIT_FAILURE;
    }
  errorWarningObserver->Clear();
  if (node2->GetCellText(2,20)!="")
    {
    std::cerr << "GetCellText test 3a failed" << std::endl;
    return EXIT_FAILURE;
    }
  // error log is expected
  if (!errorWarningObserver->GetError())
    {
    std::cerr << "GetCellText test 3b failed" << std::endl;
    return EXIT_FAILURE;
    }
  errorWarningObserver->Clear();
  if (!node2->SetCellText(2,2,"ModifiedText"))
    {
    std::cerr << "SetCellText test 1a failed" << std::endl;
    return EXIT_FAILURE;
    }
  if (node2->GetCellText(2,2)!="ModifiedText")
    {
    std::cerr << "SetCellText test 1b failed" << std::endl;
    return EXIT_FAILURE;
    }
  // Test out of range cases:
  if (node2->SetCellText(20,2,"invalid"))
    {
    std::cerr << "SetCellText test 2 failed" << std::endl;
    return EXIT_FAILURE;
    }
  if (node2->SetCellText(2,20,"invalid"))
    {
    std::cerr << "SetCellText test 3 failed" << std::endl;
    return EXIT_FAILURE;
    }

  // Verify that Copy method creates a true independent copy
  vtkSmartPointer< vtkMRMLTableNode > node2copy = vtkSmartPointer< vtkMRMLTableNode >::New();
  node2copy->Copy(node2);
  // After copying the contents of the tables should be the same
  if (node2->GetCellText(0,0) != node2copy->GetCellText(0,0))
    {
    std::cerr << "Copy test 1a failed" << std::endl;
    return EXIT_FAILURE;
    }
  // After modifying the copied version, the tables should be different
  // (if there was a shallow copy only, the original table would have been changed, too)
  node2copy->SetCellText(0,0,"someModifiedText");
  if (node2->GetCellText(0,0) == node2copy->GetCellText(0,0))
    {
    std::cerr << "Copy test 1b failed" << std::endl;
    return EXIT_FAILURE;
    }

  std::cout << "vtkMRMLTableNodeTest1 completed successfully" << std::endl;
  return EXIT_SUCCESS;
}
