/*==============================================================================

  Copyright (c) Kapteyn Astronomical Institute
  University of Groningen, Groningen, Netherlands. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Davide Punzo, Kapteyn Astronomical Institute,
  and was supported through the European Research Consil grant nr. 291531.

==============================================================================*/

// QT includes
#include <QApplication>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

// Slicer includes
#include "vtkSlicerConfigure.h"

// qMRML includes
#include "qMRMLPlotView.h"
#include "qMRMLPlotWidget.h"
#include "qMRMLPlotViewControllerWidget.h"

// MRML includes
#include "vtkMRMLPlotNode.h"
#include "vtkMRMLPlotLayoutNode.h"
#include "vtkMRMLPlotViewNode.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLTableNode.h"

// VTK includes
#include <vtkFloatArray.h>
#include <vtkNew.h>
#include <vtkTable.h>
#ifdef Slicer_VTK_USE_QVTKOPENGLWIDGET
#include <QVTKOpenGLWidget.h>
#endif

int qMRMLPlotViewTest1( int argc, char * argv [] )
{
#ifdef Slicer_VTK_USE_QVTKOPENGLWIDGET
  // Set default surface format for QVTKOpenGLWidget
  QSurfaceFormat format = QVTKOpenGLWidget::defaultFormat();
  format.setSamples(0);
  QSurfaceFormat::setDefaultFormat(format);
#endif

  QApplication app(argc, argv);

  vtkNew<vtkMRMLScene> scene;

  // Create a vtkTable
  vtkNew<vtkTable> table;

  vtkNew<vtkFloatArray> arrX;
  arrX->SetName("X Axis");
  table->AddColumn(arrX);

  vtkNew<vtkFloatArray> arrC;
  arrC->SetName("Cosine");
  table->AddColumn(arrC);

  vtkNew<vtkFloatArray> arrS;
  arrS->SetName("Sine");
  table->AddColumn(arrS);

  // Fill in the table with some example values
  int numPoints = 69;
  float inc = 7.5 / (numPoints-1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
    {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc));
    table->SetValue(i, 2, sin(i * inc));
    }

  // Create a MRMLTableNode
  vtkNew<vtkMRMLTableNode> TableNode;
  scene->AddNode(TableNode);
  TableNode->SetAndObserveTable(table.GetPointer());

  // Create two plotNodes
  vtkNew<vtkMRMLPlotNode> plotNode1;
  vtkNew<vtkMRMLPlotNode> plotNode2;
  scene->AddNode(plotNode1);
  scene->AddNode(plotNode2);

  // Set and Observe the MRMLTableNode
  plotNode1->SetAndObserveTableNodeID(TableNode->GetID());
  plotNode2->SetAndObserveTableNodeID(TableNode->GetID());
  plotNode2->SetYColumnIndex(2);

  // Create a PlotLayout node
  vtkNew<vtkMRMLPlotLayoutNode> plotLayoutNode;
  scene->AddNode(plotLayoutNode);
  // Add and Observe plots IDs in PlotLayout
  plotNode1->SetName(arrC->GetName());
  plotLayoutNode->AddAndObservePlotNodeID(plotNode1->GetID());
  plotNode2->SetName(arrS->GetName());
  plotLayoutNode->AddAndObservePlotNodeID(plotNode2->GetID());

  // Create PlotView node
  vtkNew<vtkMRMLPlotViewNode> plotViewNode;
  scene->AddNode(plotViewNode);
  // Set plotLayout ID in PlotView
  plotViewNode->SetPlotLayoutNodeID(plotLayoutNode->GetID());

  //
  // Create a simple gui with non-tranposed and transposed table view
  //
  QWidget parentWidget;
  parentWidget.setWindowTitle("qMRMLPlotViewTest1");
  QVBoxLayout vbox;
  parentWidget.setLayout(&vbox);

  qMRMLPlotWidget* plotWidget = new qMRMLPlotWidget();
  plotWidget->setParent(&parentWidget);
  plotWidget->setMRMLScene(scene);
  plotWidget->setMRMLPlotViewNode(plotViewNode);
  vbox.addWidget(plotWidget);
  parentWidget.show();
  parentWidget.raise();

  if (argc < 2 || QString(argv[1]) != "-I")
    {
    QTimer::singleShot(200, &app, SLOT(quit()));
    }

  return app.exec();
}
