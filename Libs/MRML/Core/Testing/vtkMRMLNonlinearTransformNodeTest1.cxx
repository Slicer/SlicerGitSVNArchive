/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH)
  All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer

=========================================================================auto=*/

// ITK includes
#include <itkConfigure.h>
#if ITK_VERSION_MAJOR > 3
#include <itkFactoryRegistration.h>
#endif

#include "vtkITKBSplineTransform.h"
#include "vtkGridTransform.h"
#include "vtkMRMLTransformNode.h"
#include "vtkMRMLBSplineTransformNode.h"
#include "vtkMRMLGridTransformNode.h"
#include "vtkMRMLScene.h"

#include <vtkGeneralTransform.h>

#include "vtkMRMLCoreTestingMacros.h"

bool TestBSplineTransform(const char *filename);
bool TestGridTransform(const char *filename);

int vtkMRMLNonlinearTransformNodeTest1(int argc, char * argv[] )
{
#if ITK_VERSION_MAJOR > 3
  itk::itkFactoryRegistration();
#endif

  bool res = true;
  const char *filename = 0;
  if (argc > 1)
    {
    filename = argv[1];
    }
  res = TestBSplineTransform(filename) && res;
  res = TestGridTransform(filename) && res;

  return res ? EXIT_SUCCESS : EXIT_FAILURE;
}

//---------------------------------------------------------------------------
bool TestBSplineTransform(const char *filename)
{
  vtkMRMLScene *scene = vtkMRMLScene::New();

  scene->SetURL(filename);
  scene->Import();

  vtkMRMLBSplineTransformNode *bsplineTransformNode = vtkMRMLBSplineTransformNode::SafeDownCast(
    scene->GetNodeByID("vtkMRMLBSplineTransformNode1"));

  if (bsplineTransformNode == 0)
    {
    std::cout << __LINE__ << ": TestBSplineTransform failed" << std::endl;
    return false;
    }

  vtkITKBSplineTransform *xfp = vtkITKBSplineTransform::SafeDownCast(
    bsplineTransformNode->GetTransformFromParentAs("vtkITKBSplineTransform"));
  vtkITKBSplineTransform *xtp = vtkITKBSplineTransform::SafeDownCast(
    bsplineTransformNode->GetTransformToParentAs("vtkITKBSplineTransform"));

  if (xfp == 0 || xtp == 0)
    {
    std::cout << __LINE__ << ": TestBSplineTransform failed" << std::endl;
    return false;
    }

  double inp[] = {0,0,0};
  double outp[3];
  xfp->TransformPoint(inp, outp);
  if (fabs(outp[0]) < 0.1 || fabs(outp[1]) < 0.1 || fabs(outp[2]) < 0.1)
    {
    std::cout << __LINE__ << ": TestBSplineTransform failed" << std::endl;
    return false;
    }

  xtp->TransformPoint(outp, inp);
  if (fabs(inp[0]) > 0.1 || fabs(inp[1]) > 0.1 || fabs(inp[2]) > 0.1)
    {
    std::cout << __LINE__ << ": TestBSplineTransform failed" << std::endl;
    return false;
    }

  scene->Clear(1);
  scene->Delete();
  return true;
}

//---------------------------------------------------------------------------
bool TestGridTransform(const char *filename)
{
  vtkMRMLScene *scene = vtkMRMLScene::New();

  scene->SetURL(filename);
  scene->Import();

  vtkMRMLGridTransformNode *gridTransformNode = vtkMRMLGridTransformNode::SafeDownCast(
    scene->GetNodeByID("vtkMRMLGridTransformNode1"));

  if (gridTransformNode == 0)
    {
    std::cout << __LINE__ << ": TestBSplineTransform failed" << std::endl;
    return false;
    }

  vtkGridTransform *xfp = vtkGridTransform::SafeDownCast(
    gridTransformNode->GetTransformFromParentAs("vtkGridTransform"));
  vtkGridTransform *xtp = vtkGridTransform::SafeDownCast(
    gridTransformNode->GetTransformToParentAs("vtkGridTransform"));

  if (xfp == 0 || xtp == 0)
    {
    std::cout << __LINE__ << ": TestBSplineTransform failed" << std::endl;
    return false;
    }

  double inp[] = {0,0,0};
  double outp[3];
  xfp->TransformPoint(inp, outp);
  if (fabs(outp[0]) < 0.1 || fabs(outp[1]) < 0.1 || fabs(outp[2]) < 0.1)
    {
    std::cout << __LINE__ << ": TestBSplineTransform failed" << std::endl;
    return false;
    }

  xtp->TransformPoint(outp, inp);
  if (fabs(inp[0]) > 0.1 || fabs(inp[1]) > 0.1 || fabs(inp[2]) > 0.1)
    {
    std::cout << __LINE__ << ": TestBSplineTransform failed" << std::endl;
    return false;
    }

  scene->Clear(1);
  scene->Delete();
  return true;
}
