/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// std includes
#include <sstream>

// vtkAddon includes
#include "vtkAddonMathUtilities.h"
#include "vtkAddonTestingMacros.h"

// vtk includes
#include <vtkMatrix3x3.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>


using namespace vtkAddonTestingUtilities;

//----------------------------------------------------------------------------
int AreMatrixEqual_4x4_4x4_Test();
int AreMatrixEqual_4x4_3x3_Test();
int AreMatrixEqual_3x3_4x4_Test();
int AreMatrixEqual_3x3_3x3_Test();
int GetOrientationMatrixTest();
int ToString_Test();
int FromString_Test();

//----------------------------------------------------------------------------
int vtkAddonMathUtilitiesTest1(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  CHECK_INT(AreMatrixEqual_4x4_4x4_Test(), EXIT_SUCCESS);
  CHECK_INT(AreMatrixEqual_4x4_3x3_Test(), EXIT_SUCCESS);
  CHECK_INT(AreMatrixEqual_3x3_4x4_Test(), EXIT_SUCCESS);
  CHECK_INT(AreMatrixEqual_3x3_3x3_Test(), EXIT_SUCCESS);
  CHECK_INT(GetOrientationMatrixTest(), EXIT_SUCCESS);
  CHECK_INT(ToString_Test(), EXIT_SUCCESS);
  CHECK_INT(FromString_Test(), EXIT_SUCCESS);
  return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
template< typename M1_TYPE, typename M2_TYPE>
int AreMatrixEqual_Test(int size_m1, int size_m2)
{
  vtkNew<M1_TYPE> m1;
  vtkNew<M2_TYPE> m2;
  vtkNew<M1_TYPE> m3;
  for (int i = 0; i < size_m1; i++)
    {
    for (int j = 0; j < size_m1; j++)
      {
      m1->SetElement(i, j, i);
      m3->SetElement(i, j, i * j);
      }
    }

  for (int i = 0; i < size_m2; i++)
    {
    for (int j = 0; j < size_m2; j++)
      {
      m2->SetElement(i, j, i);
      }
    }

  CHECK_BOOL( vtkAddonMathUtilities::MatrixAreEqual(m1.GetPointer(), m2.GetPointer()), true);
  CHECK_BOOL( vtkAddonMathUtilities::MatrixAreEqual(m1.GetPointer(), m3.GetPointer()), false);

  double tolerance = 1e-3;
  CHECK_BOOL( vtkAddonMathUtilities::MatrixAreEqual(m1.GetPointer(), m2.GetPointer(), tolerance), true);

  m1->SetElement(0, 0 , 1e-4);
  CHECK_BOOL( vtkAddonMathUtilities::MatrixAreEqual(m1.GetPointer(), m2.GetPointer(), tolerance), true);

  m1->SetElement(0, 0 , -1e-4);
  CHECK_BOOL( vtkAddonMathUtilities::MatrixAreEqual(m1.GetPointer(), m2.GetPointer(), tolerance), true);

  m1->SetElement(0, 0 , 5e-3);
  CHECK_BOOL( vtkAddonMathUtilities::MatrixAreEqual(m1.GetPointer(), m2.GetPointer(), tolerance), false);

  m1->SetElement(0, 0 , -5e-3);
  CHECK_BOOL( vtkAddonMathUtilities::MatrixAreEqual(m1.GetPointer(), m2.GetPointer(), tolerance), false);

  return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
int AreMatrixEqual_4x4_4x4_Test()
{
  return AreMatrixEqual_Test<vtkMatrix4x4, vtkMatrix4x4>(4, 4);
}

//----------------------------------------------------------------------------
int AreMatrixEqual_4x4_3x3_Test()
{
  return AreMatrixEqual_Test<vtkMatrix4x4, vtkMatrix3x3>(4, 3);
}

//----------------------------------------------------------------------------
int AreMatrixEqual_3x3_4x4_Test()
{
  return AreMatrixEqual_Test<vtkMatrix3x3, vtkMatrix4x4>(3, 4);
}

//----------------------------------------------------------------------------
int AreMatrixEqual_3x3_3x3_Test()
{
  return AreMatrixEqual_Test<vtkMatrix3x3, vtkMatrix3x3>(3, 3);
}

//----------------------------------------------------------------------------
int GetOrientationMatrixTest()
{
  vtkNew<vtkMatrix4x4> m44;
  for (int ii = 0; ii < 4; ii++)
    {
    for (int jj = 0; jj < 4; jj++)
      {
      m44->SetElement(ii, jj, (1 + ii)*(1 + jj));
      }
    }

  vtkNew<vtkMatrix3x3> m33;

  vtkAddonMathUtilities::GetOrientationMatrix(m44.GetPointer(), m33.GetPointer());

  for (int ii = 0; ii < 3; ii++)
    {
    for (int jj = 0; jj < 3; jj++)
      {
      CHECK_DOUBLE(m33->GetElement(ii, jj), (1 + ii)*(1 + jj));
      }
    }

  return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
int ToString_Test()
{
  vtkNew<vtkMatrix4x4> mat;
  std::stringstream ss;
  std::string delimiter = ",";
  std::string rowDelimiter = "\n";
  for (int ii = 0; ii < 4; ii++)
    {
    for (int jj = 0; jj < 4; jj++)
      {
      double val = (1 + ii)*(1 + jj);
      mat->SetElement(ii, jj, val);
      ss << val << delimiter;
      }
    ss << rowDelimiter;
    }

  std::string resultStr = vtkAddonMathUtilities::ToString(mat.GetPointer(), delimiter, rowDelimiter);

  CHECK_INT(resultStr.compare(ss.str()), 0);

  return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
int FromString_Test()
{
  vtkNew<vtkMatrix4x4> mat;
  std::stringstream ss;
  std::string delimiter = ",";
  for (int ii = 0; ii < 4; ii++)
    {
    for (int jj = 0; jj < 4; jj++)
      {
      double val = (1 + ii)*(1 + jj);
      mat->SetElement(ii, jj, val);
      ss << val << delimiter;
      }
    }

  vtkNew<vtkMatrix4x4> resMat;
  vtkAddonMathUtilities::FromString(resMat.GetPointer(),ss.str());

  for (int ii = 0; ii < 4; ii++)
    {
    for (int jj = 0; jj < 4; jj++)
      {
      CHECK_DOUBLE(resMat->GetElement(ii, jj), mat->GetElement(ii, jj));
      }
    }

  return EXIT_SUCCESS;
}