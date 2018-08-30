/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// Planner Logic includes
#include "vtkSlicerPlanesLogic.h"

// MRML includes
#include <vtkMRMLMarkupsPlanesNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSelectionNode.h>

// VTK includes
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerPlanesLogic);

//----------------------------------------------------------------------------
vtkSlicerPlanesLogic::vtkSlicerPlanesLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerPlanesLogic::~vtkSlicerPlanesLogic()
{
}

//---------------------------------------------------------------------------
vtkMRMLNode* vtkSlicerPlanesLogic::GetSelectionNode() const
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
void vtkSlicerPlanesLogic::RegisterNodes()
{
  vtkMRMLScene* scene = this->GetMRMLScene();
  assert(scene);

  vtkNew<vtkMRMLMarkupsPlanesNode> planes;
  scene->RegisterNodeClass(planes.GetPointer());
}

//---------------------------------------------------------------------------
void vtkSlicerPlanesLogic::ObserveMRMLScene()
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
      "vtkMRMLMarkupsPlanesNode", ":/Icons/MarkupsPlanesMouseModePlace.png", "Planes");

    // trigger an upate on the mouse mode toolbar
    this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState);
    }

 this->Superclass::ObserveMRMLScene();
}

//----------------------------------------------------------------------------
void vtkSlicerPlanesLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
