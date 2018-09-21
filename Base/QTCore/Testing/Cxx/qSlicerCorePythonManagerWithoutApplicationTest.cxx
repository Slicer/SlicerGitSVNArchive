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

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// CTK includes
#include <ctkTest.h>

// Slicer includes
#include "qSlicerCorePythonManager.h"

// ----------------------------------------------------------------------------
class qSlicerCorePythonManagerWithoutApplicationTester: public QObject
{
  Q_OBJECT

private:
  qSlicerCorePythonManager PythonManager;

private slots:
  void testInitialize();
  void toPythonStringLiteral();
};

// ----------------------------------------------------------------------------
void qSlicerCorePythonManagerWithoutApplicationTester::testInitialize()
{
  this->PythonManager.initialize();
}

// ----------------------------------------------------------------------------
void qSlicerCorePythonManagerWithoutApplicationTester::toPythonStringLiteral()
{
  QCOMPARE(this->PythonManager.toPythonStringLiteral("simple string"), "'simple string'");
  QCOMPARE(this->PythonManager.toPythonStringLiteral("C:\\folder1\\folder2"), "'C:\\\\folder1\\\\folder2'");
  QCOMPARE(this->PythonManager.toPythonStringLiteral("C:/folder1/folder2"), "'C:/folder1/folder2'");
  QCOMPARE(this->PythonManager.toPythonStringLiteral("this \"special\" string contains double-quotes"), "'this \"special\" string contains double-quotes'");
  QCOMPARE(this->PythonManager.toPythonStringLiteral("this name O'Neil contains a single-quote"), "'this name O\\'Neil contains a single-quote'");
  QCOMPARE(this->PythonManager.toPythonStringLiteral("'single-quoted string'"), "'\\\'single-quoted string\\\''");
  QCOMPARE(this->PythonManager.toPythonStringLiteral("\"double-quoted string\""), "'\"double-quoted string\"'");
}

// ----------------------------------------------------------------------------
CTK_TEST_MAIN(qSlicerCorePythonManagerWithoutApplicationTest)
#include "moc_qSlicerCorePythonManagerWithoutApplicationTest.cxx"

