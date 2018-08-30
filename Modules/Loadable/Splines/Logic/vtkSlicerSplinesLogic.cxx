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
and was partially funded by Allen Institute

==============================================================================*/

// Splines Logic includes
#include "vtkSlicerSplinesLogic.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLSelectionNode.h>

// VTK includes
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// Splines includes
#include "vtkMRMLMarkupsSplinesNode.h"

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerSplinesLogic);

//----------------------------------------------------------------------------
vtkSlicerSplinesLogic::vtkSlicerSplinesLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerSplinesLogic::~vtkSlicerSplinesLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerSplinesLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
vtkMRMLSelectionNode* vtkSlicerSplinesLogic::GetSelectionNode() const
{
  if (!this->GetMRMLScene())
  {
    return NULL;
  }

  // try the application logic first
  vtkMRMLApplicationLogic *mrmlAppLogic = this->GetMRMLApplicationLogic();
  return mrmlAppLogic ?
    (mrmlAppLogic->GetSelectionNode() ?
      mrmlAppLogic->GetSelectionNode() : NULL) : NULL;
}

//---------------------------------------------------------------------------
bool vtkSlicerSplinesLogic
::GetCentroid(vtkMRMLMarkupsSplinesNode* splinesNode, int n, double centroid[3])
{
  if (!splinesNode || !splinesNode->MarkupExists(n))
  {
    return false;
  }
  int numberOfPoints = splinesNode->GetNumberOfPointsInNthMarkup(n);
  if (numberOfPoints <= 0)
  {
    return false;
  }

  centroid[0] = 0.0;
  centroid[1] = 0.0;
  centroid[2] = 0.0;
  for (int i = 0; i < numberOfPoints; ++i)
  {
    double point[4];
    splinesNode->GetMarkupPointWorld(n, i, point);
    vtkMath::Add(point, centroid, centroid);
  }
  vtkMath::MultiplyScalar(centroid, 1.0/numberOfPoints);
  return true;
}

//---------------------------------------------------------------------------
void vtkSlicerSplinesLogic::ObserveMRMLScene()
{
  if (!this->GetMRMLScene())
  {
    return;
  }

  // add known markup types to the selection node
  vtkMRMLSelectionNode *selectionNode =
    vtkMRMLSelectionNode::SafeDownCast(this->GetSelectionNode());
  if (selectionNode)
  {
    // got into batch process mode so that an update on the mouse mode tool
    // bar is triggered when leave it
    this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState);

    selectionNode->AddNewPlaceNodeClassNameToList(
      "vtkMRMLMarkupsSplinesNode", ":/Icons/SplinesMouseModePlace.png", "Splines");

    // trigger an upate on the mouse mode toolbar
    this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState);
  }

  this->Superclass::ObserveMRMLScene();
}

//-----------------------------------------------------------------------------
void vtkSlicerSplinesLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene();
  assert(scene != 0);
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLMarkupsSplinesNode>::New());
}
