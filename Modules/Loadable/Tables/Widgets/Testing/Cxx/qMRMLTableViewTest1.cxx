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

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// QT includes
#include <QApplication>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

// qMRML includes
#include "qSlicerCoreApplication.h"
#include "qMRMLTableModel.h"
#include "qMRMLTableView.h"

// MRML includes
#include "vtkMRMLScene.h"
#include "vtkMRMLTableNode.h"

// VTK includes
#include <vtkDoubleArray.h>
#include <vtkNew.h>
#include <vtkTable.h>

int qMRMLTableViewTest1( int argc, char * argv [] )
{
  qSlicerCoreApplication app(argc, argv);

  // Create a table with some points in it...
  vtkNew<vtkTable> table;
  vtkNew<vtkDoubleArray> arrX;
  arrX->SetName("X Axis");
  table->AddColumn(arrX.GetPointer());
  vtkNew<vtkDoubleArray> arrY;
  arrY->SetName("Y Axis");
  table->AddColumn(arrY.GetPointer());
  vtkNew<vtkDoubleArray> arrSum;
  arrSum->SetName("Sum");
  table->AddColumn(arrSum.GetPointer());
  int numPoints = 15;
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
    {
    table->SetValue(i, 0, i*0.5-10 );
    table->SetValue(i, 1, i*1.2+12);
    table->SetValue(i, 2, table->GetValue(i,0).ToDouble()+table->GetValue(i,1).ToDouble());
    }

  vtkNew<vtkMRMLTableNode> tableNode;
  tableNode->SetAndObserveTable(table.GetPointer());

  //
  // Create a simple gui with non-tranposed and transposed table view
  //
  QWidget parentWidget;
  parentWidget.setWindowTitle("qMRMLTableViewTest1");
  QVBoxLayout vbox;
  parentWidget.setLayout(&vbox);

  qMRMLTableView* tableView = new qMRMLTableView();
  tableView->setParent(&parentWidget);
  tableView->setMRMLTableNode(tableNode.GetPointer());
  vbox.addWidget(tableView);

  qMRMLTableView* tableViewTransposed = new qMRMLTableView();
  tableViewTransposed->setParent(&parentWidget);
  tableViewTransposed->setTransposed(true);
  tableViewTransposed->setMRMLTableNode(tableNode.GetPointer());
  vbox.addWidget(tableViewTransposed);

  parentWidget.show();
  parentWidget.raise();

  if (argc < 2 || QString(argv[1]) != "-I")
    {
    QTimer::singleShot(200, &app, SLOT(quit()));
    }

  return app.exec();
}
