/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH)
  All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer

=========================================================================auto=*/

// MRML includes
#include "vtkMRMLCoreTestingMacros.h"
#include "vtkMRMLDoubleArrayNode.h"
#include "vtkMRMLDoubleArrayStorageNode.h"

// VTK includes
#include <vtkNew.h>

// STD includes
#include <vector>
#include <sstream>

//---------------------------------------------------------------------------
bool TestSetValues();
bool TestSetValue();
bool TestSetXYValue();
bool TestSetXYerrValue();
bool TestAddValues();
bool TestAddValue();
bool TestAddXYValue();
bool TestAddXYerrValue();

//---------------------------------------------------------------------------
int vtkMRMLDoubleArrayNodeTest1(int, char * [])
{
  vtkNew<vtkMRMLDoubleArrayNode> node1;

  EXERCISE_BASIC_OBJECT_METHODS(node1.GetPointer());

  EXERCISE_BASIC_MRML_METHODS(vtkMRMLDoubleArrayNode, node1.GetPointer());

  bool res = true;
  res = res && TestSetValues();
  res = res && TestSetValue();
  res = res && TestSetXYValue();
  res = res && TestSetXYerrValue();
  res = res && TestAddValues();
  res = res && TestAddValue();
  res = res && TestAddXYValue();
  res = res && TestAddXYerrValue();

  return res ? EXIT_SUCCESS : EXIT_FAILURE;
}

//---------------------------------------------------------------------------
bool doubleArrayMatch(int line, double array1[], double array2[],
                      unsigned int sizeOfArray)
{
  for(unsigned int i = 0; i < sizeOfArray; i++)
    {
    std::ostringstream s1;
    std::ostringstream s2;
    s1 << array1[i];
    s2 << array2[i];
    if( s1.str() != s2.str() )
      {
      std::cerr << "Line " << line
                << " - Array do not match !\n"
                << array1[i] << " not equal to " << array2[i]
                << std::endl;
      return false;
      }
    }
  return true;
}

//---------------------------------------------------------------------------
bool TestSetValues()
{
  vtkNew<vtkMRMLDoubleArrayNode> doubleArray;
  doubleArray->SetSize(2);
  double testArray0[3] = { 12.1, 425.576817, -454};
  double testArray1[3] = { 8796325425.01, 0.0, -1};
  double testArray2[3];
  double testArray3[3];

  bool res = true;
  res = res && doubleArray->SetValues(0, testArray0);
  res = res && doubleArray->SetValues(1, testArray1);
  res = res && doubleArray->GetValues(0, testArray2);
  res = res && doubleArray->GetValues(1, testArray3);
  res = res && doubleArrayMatch(__LINE__, testArray0, testArray2, 3);
  res = res && doubleArrayMatch(__LINE__, testArray1, testArray3, 3);

  return res;
}

//---------------------------------------------------------------------------
bool TestSetValue()
{
  vtkNew<vtkMRMLDoubleArrayNode> doubleArray;
  doubleArray->SetSize(2);
  double testSet[3] = { 12.1, 4255444465.576817, -454};
  double testGet[3];

  bool res = true;
  res = res && doubleArray->SetValue(0, 0, testSet[0]);
  res = res && doubleArray->SetValue(0, 1, testSet[1]);
  res = res && doubleArray->SetValue(1, 1, testSet[2]);
  int success0;
  testGet[0] = doubleArray->GetValue(0, 0, success0);
  int success1;
  testGet[1] = doubleArray->GetValue(0, 1, success1);
  int success2;
  testGet[2] = doubleArray->GetValue(1, 1, success2);
  res = res && doubleArrayMatch(__LINE__, testSet, testGet, 3);

  return (res && success0 && success1 && success2);
}

//---------------------------------------------------------------------------
bool TestSetXYValue()
{
  vtkNew<vtkMRMLDoubleArrayNode> doubleArray;
  doubleArray->SetSize(2);
  double testArray0[2] = { 12.1, -454};
  double testArray1[2] = { 8796325425.01, -1};
  double testArray2[2];
  double testArray3[2];

  bool res = true;
  res = res && doubleArray->SetXYValue(0, testArray0[0], testArray0[1]);
  res = res && doubleArray->SetXYValue(1, testArray1[0], testArray1[1]);
  res = res && doubleArray->GetXYValue(0, &testArray2[0], &testArray2[1]);
  res = res && doubleArray->GetXYValue(1, &testArray3[0], &testArray3[1]);
  res = res && doubleArrayMatch(__LINE__, testArray0, testArray2, 2);
  res = res && doubleArrayMatch(__LINE__, testArray1, testArray3, 2);

  return res;
}

//---------------------------------------------------------------------------
bool TestSetXYerrValue()
{
  vtkNew<vtkMRMLDoubleArrayNode> doubleArray;
  doubleArray->SetSize(2);
  double testArray0[3] = { 12.1, 425.576817, -454};
  double testArray1[3] = { 8796325425.01, 0.0, -1};
  double testArray2[3];
  double testArray3[3];

  bool res = true;
  res = res && doubleArray->SetXYValue(0, testArray0[0], testArray0[1], testArray0[2]);
  res = res && doubleArray->SetXYValue(1, testArray1[0], testArray1[1], testArray1[2]);
  res = res && doubleArray->GetXYValue(0, &testArray2[0], &testArray2[1], &testArray2[2]);
  res = res && doubleArray->GetXYValue(1, &testArray3[0], &testArray3[1], &testArray3[2]);
  res = res && doubleArrayMatch(__LINE__, testArray0, testArray2, 3);
  res = res && doubleArrayMatch(__LINE__, testArray1, testArray3, 3);

  return res;
}

//---------------------------------------------------------------------------
bool TestAddValues()
{
  vtkNew<vtkMRMLDoubleArrayNode> doubleArray;
  double testArray0[3] = { 12.1, 425.576817, -454};
  double testArray1[3] = { 8796325425.01, 0.0, -1};
  double testArray2[3];
  double testArray3[3];

  bool res = true;
  res = res && doubleArray->AddValues(testArray0);
  res = res && doubleArray->AddValues(testArray1);
  res = res && doubleArray->GetValues(0, testArray2);
  res = res && doubleArray->GetValues(1, testArray3);
  res = res && doubleArrayMatch(__LINE__, testArray0, testArray2, 3);
  res = res && doubleArrayMatch(__LINE__, testArray1, testArray3, 3);

  return res;
}

//---------------------------------------------------------------------------
bool TestAddValue()
{
  vtkNew<vtkMRMLDoubleArrayNode> doubleArray;
  double testSet[3] = { 12.1, 4255444465.576817, -454};
  double testGet[3];

  bool res = true;
  res = res && doubleArray->AddValue(0, testSet[0]);
  res = res && doubleArray->AddValue(2, testSet[1]);
  res = res && doubleArray->AddValue(1, testSet[2]);

  int success0;
  testGet[0] = doubleArray->GetValue(0, 0, success0);
  int success1;
  testGet[1] = doubleArray->GetValue(1, 2, success1);
  int success2;
  testGet[2] = doubleArray->GetValue(2, 1, success2);
  res = res && doubleArrayMatch(__LINE__, testSet, testGet, 3);

  return (res && success0 && success1 && success2);
}

//---------------------------------------------------------------------------
bool TestAddXYValue()
{
  vtkNew<vtkMRMLDoubleArrayNode> doubleArray;
  double testArray0[2] = { 12.1, -454};
  double testArray1[2] = { 8796325425.01, -1};
  double testArray2[2];
  double testArray3[2];

  bool res = true;
  res = res && doubleArray->AddXYValue(testArray0[0], testArray0[1]);
  res = res && doubleArray->AddXYValue(testArray1[0], testArray1[1]);
  res = res && doubleArray->GetXYValue(0, &testArray2[0], &testArray2[1]);
  res = res && doubleArray->GetXYValue(1, &testArray3[0], &testArray3[1]);
  res = res && doubleArrayMatch(__LINE__, testArray0, testArray2, 2);
  res = res && doubleArrayMatch(__LINE__, testArray1, testArray3, 2);

  return res;
}

//---------------------------------------------------------------------------
bool TestAddXYerrValue()
{
  vtkNew<vtkMRMLDoubleArrayNode> doubleArray;
  double testArray0[3] = { 12.1, 425.576817, -454};
  double testArray1[3] = { 8796325425.01, 0.0, -1};
  double testArray2[3];
  double testArray3[3];

  bool res = true;
  res = res && doubleArray->AddXYValue(testArray0[0], testArray0[1], testArray0[2]);
  res = res && doubleArray->AddXYValue(testArray1[0], testArray1[1], testArray1[2]);
  res = res && doubleArray->GetXYValue(0, &testArray2[0], &testArray2[1], &testArray2[2]);
  res = res && doubleArray->GetXYValue(1, &testArray3[0], &testArray3[1], &testArray3[2]);
  res = res && doubleArrayMatch(__LINE__, testArray0, testArray2, 3);
  res = res && doubleArrayMatch(__LINE__, testArray1, testArray3, 3);

  return res;
}
