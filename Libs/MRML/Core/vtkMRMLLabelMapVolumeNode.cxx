/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Andras Lasso, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// MRML includes
#include "vtkMRMLLabelMapVolumeNode.h"
#include "vtkMRMLLabelMapVolumeDisplayNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtk/Common/Core/vtkDataArray.h>
#include <vtk/Common/DataModel/vtkImageData.h>
#include <vtk/Common/Core/vtkNew.h>
#include <vtk/Common/Core/vtkObjectFactory.h>
#include <vtk/Common/DataModel/vtkPointData.h>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLLabelMapVolumeNode);

//----------------------------------------------------------------------------
vtkMRMLLabelMapVolumeNode::vtkMRMLLabelMapVolumeNode()
{
}

//----------------------------------------------------------------------------
vtkMRMLLabelMapVolumeNode::~vtkMRMLLabelMapVolumeNode()
{
}

//-----------------------------------------------------------
void vtkMRMLLabelMapVolumeNode::CreateNoneNode(vtkMRMLScene *scene)
{
  vtkNew<vtkImageData> id;
  id->SetDimensions(1, 1, 1);
  id->AllocateScalars(VTK_SHORT, 1);
  id->GetPointData()->GetScalars()->FillComponent(0, 0);

  vtkNew<vtkMRMLLabelMapVolumeNode> n;
  n->SetName("None");
  // the scene will set the id
  n->SetAndObserveImageData(id.GetPointer());
  scene->AddNode(n.GetPointer());
}

//----------------------------------------------------------------------------
void vtkMRMLLabelMapVolumeNode::CreateDefaultDisplayNodes()
{
  if (vtkMRMLLabelMapVolumeDisplayNode::SafeDownCast(this->GetDisplayNode())!=NULL)
    {
    // display node already exists
    return;
    }
  if (this->GetScene()==NULL)
    {
    vtkErrorMacro("vtkMRMLLabelMapVolumeNode::CreateDefaultDisplayNodes failed: scene is invalid");
    return;
    }
  vtkNew<vtkMRMLLabelMapVolumeDisplayNode> dispNode;
  this->GetScene()->AddNode(dispNode.GetPointer());
  dispNode->SetDefaultColorMap();
  this->SetAndObserveDisplayNodeID(dispNode->GetID());
}
