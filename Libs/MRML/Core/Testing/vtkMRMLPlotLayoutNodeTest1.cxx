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

#include "vtkMRMLPlotNode.h"
#include "vtkMRMLPlotLayoutNode.h"
#include "vtkMRMLTableNode.h"

#include "vtkFloatArray.h"
#include "vtkPlot.h"
#include "vtkTable.h"
#include "vtkTestErrorObserver.h"

#include "vtkMRMLCoreTestingMacros.h"

int vtkMRMLPlotLayoutNodeTest1(int , char * [] )
{
  // Create a PlotLayout node
  vtkNew<vtkMRMLPlotLayoutNode> node;
  EXERCISE_ALL_BASIC_MRML_METHODS(node.GetPointer());

  // Create two plotNodes
  vtkNew<vtkMRMLPlotNode> plotNode1;
  vtkNew<vtkMRMLPlotNode> plotNode2;

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
  TableNode->SetAndObserveTable(table.GetPointer());

  // Set and Observe the MRMLTableNode
  plotNode1->SetAndObserveTableNodeID(TableNode->GetID());
  plotNode2->SetAndObserveTableNodeID(TableNode->GetID());
  plotNode2->SetYColumnIndex(2);

  // Add and Observe plots IDs in PlotLayout
  node->AddAndObservePlotNodeID(plotNode1->GetID());
  node->AddAndObservePlotNodeID(plotNode2->GetID());

  // Test The references
  CHECK_POINTER(node->GetPlotNode(), plotNode1);
  CHECK_POINTER(node->GetNthPlotNode(1), plotNode2);

  node->RemovePlotNodeID(plotNode1->GetID());
  CHECK_POINTER(node->GetPlotNode(), plotNode2);

  // Verify that Copy method creates a true independent copy
  vtkSmartPointer< vtkMRMLPlotLayoutNode > nodeCopy = vtkSmartPointer< vtkMRMLPlotLayoutNode >::New();
  nodeCopy->Copy(node.GetPointer());

  CHECK_STD_STRING(node->GetName(), nodeCopy->GetName());

  std::cout << "vtkMRMLPlotLayoutNodeTest1 completed successfully" << std::endl;
  return EXIT_SUCCESS;
}
