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

#include "vtkMRMLScene.h"
#include "vtkMRMLPlotLayoutNode.h"
#include "vtkMRMLPlotViewNode.h"

#include "vtkMRMLCoreTestingMacros.h"

int vtkMRMLPlotViewNodeTest1(int , char * [] )
{
  vtkNew<vtkMRMLPlotViewNode> node1;
  EXERCISE_ALL_BASIC_MRML_METHODS(node1.GetPointer());

  // Check if modified eventes are only fired if
  // and only if PlotLayout node ID is changed

  vtkNew<vtkMRMLScene> scene;
  vtkNew<vtkMRMLPlotLayoutNode> plotLayoutNode1;
  vtkNew<vtkMRMLPlotLayoutNode> plotLayoutNode2;
  scene->AddNode(plotLayoutNode1.GetPointer());
  scene->AddNode(plotLayoutNode2.GetPointer());

  vtkNew<vtkMRMLCoreTestingUtilities::vtkMRMLNodeCallback> callback;
  node1->AddObserver(vtkCommand::AnyEvent, callback.GetPointer());

  callback->ResetNumberOfEvents();
  node1->SetPlotLayoutNodeID(plotLayoutNode1->GetID());
  CHECK_INT(callback->GetNumberOfModified(),1);

  callback->ResetNumberOfEvents();
  node1->SetPlotLayoutNodeID(plotLayoutNode2->GetID());
  CHECK_INT(callback->GetNumberOfModified(),1);

  callback->ResetNumberOfEvents();
  node1->SetPlotLayoutNodeID(plotLayoutNode2->GetID());
  CHECK_INT(callback->GetNumberOfModified(),0);

  callback->ResetNumberOfEvents();
  node1->SetPlotLayoutNodeID(NULL);
  CHECK_INT(callback->GetNumberOfModified(),1);

  callback->ResetNumberOfEvents();
  node1->SetPlotLayoutNodeID(NULL);
  CHECK_INT(callback->GetNumberOfModified(),0);

  std::cout << "vtkMRMLPlotViewNodeTest1 completed successfully" << std::endl;
  return EXIT_SUCCESS;
}
