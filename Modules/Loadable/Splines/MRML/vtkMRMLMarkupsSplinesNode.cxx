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

// MRML includes
#include <vtkMRMLMarkupsDisplayNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkBoundingBox.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// Planner includes
#include "vtkMRMLMarkupsSplinesNode.h"

// STD includes
#include <sstream>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLMarkupsSplinesNode);

//----------------------------------------------------------------------------
vtkMRMLMarkupsSplinesNode::vtkMRMLMarkupsSplinesNode()
{
  this->CurrentSpline = -1;
  this->DefaultClosed = true;
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsSplinesNode::~vtkMRMLMarkupsSplinesNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsSplinesNode::SetCurrentSpline(int i)
{
  if (this->CurrentSpline == i)
  {
    return;
  }
  this->CurrentSpline = i;
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkMRMLMarkupsSplinesNode::GetCurrentSpline() const
{
  return this->CurrentSpline;
}

//----------------------------------------------------------------------------
int vtkMRMLMarkupsSplinesNode::AddSpline(vtkVector3d point)
{
  this->CurrentSpline = this->AddPointToNewMarkup(point);
  this->Closed.push_back(this->DefaultClosed);
  return this->CurrentSpline;
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsSplinesNode::CreateDefaultDisplayNodes()
{
  if (this->GetDisplayNode() != NULL &&
    vtkMRMLMarkupsDisplayNode::SafeDownCast(this->GetDisplayNode()) != NULL)
  {
    // display node already exists
    return;
  }
  if (this->GetScene() == NULL)
  {
    vtkErrorMacro(
      "vtkMRMLMarkupsSplinesNode::CreateDefaultDisplayNodes failed: scene is invalid");
    return;
  }
  vtkNew<vtkMRMLMarkupsDisplayNode> dispNode;
  this->GetScene()->AddNode(dispNode.GetPointer());
  this->SetAndObserveDisplayNodeID(dispNode->GetID());
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsSplinesNode::SetNthSplineClosed(int n, bool closed)
{
  if (n >= this->Closed.size() || this->Closed[n] == closed)
  {
    return;
  }

  this->Closed[n] = closed;
  this->Modified();
  this->InvokeCustomModifiedEvent(
    vtkMRMLMarkupsNode::NthMarkupModifiedEvent, (void*)&n);
}

//----------------------------------------------------------------------------
bool vtkMRMLMarkupsSplinesNode::GetNthSplineClosed(int n)
{
  if (n >= this->Closed.size())
  {
    vtkErrorMacro("The " << n << "th spline doesn't exist");
    return this->DefaultClosed;
  }
  return this->Closed[n];
}
