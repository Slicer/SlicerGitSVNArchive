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

// MRML includes
#include <vtkMRMLMarkupsDisplayNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkBoundingBox.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// Planner includes
#include "vtkMRMLMarkupsPlanesNode.h"

// STD includes
#include <sstream>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLMarkupsPlanesNode);


//----------------------------------------------------------------------------
vtkMRMLMarkupsPlanesNode::vtkMRMLMarkupsPlanesNode()
{

}

//----------------------------------------------------------------------------
vtkMRMLMarkupsPlanesNode::~vtkMRMLMarkupsPlanesNode()
{

}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of,nIndent);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::ReadXMLAttributes(const char** atts)
{
  Superclass::ReadXMLAttributes(atts);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
}

//-----------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::UpdateScene(vtkMRMLScene *scene)
{
  Superclass::UpdateScene(scene);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::ProcessMRMLEvents ( vtkObject *caller,
                                           unsigned long event,
                                           void *callData )
{
  Superclass::ProcessMRMLEvents(caller, event, callData);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}

//-------------------------------------------------------------------------
vtkMRMLStorageNode* vtkMRMLMarkupsPlanesNode::CreateDefaultStorageNode()
{
  return Superclass::CreateDefaultStorageNode();
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::CreateDefaultDisplayNodes()
{
  if (this->GetDisplayNode() != NULL &&
      vtkMRMLMarkupsDisplayNode::SafeDownCast(this->GetDisplayNode()) != NULL)
    {
    // display node already exists
    return;
    }
  if (this->GetScene()==NULL)
    {
    vtkErrorMacro(
      "vtkMRMLMarkupsPlanesNode::CreateDefaultDisplayNodes failed: scene is invalid");
    return;
    }
  vtkNew<vtkMRMLMarkupsDisplayNode> dispNode;
  this->GetScene()->AddNode(dispNode.GetPointer());
  this->SetAndObserveDisplayNodeID(dispNode->GetID());
}

//-------------------------------------------------------------------------
vtkMRMLMarkupsDisplayNode *vtkMRMLMarkupsPlanesNode::GetMarkupsDisplayNode()
{
  vtkMRMLDisplayNode *displayNode = this->GetDisplayNode();
  if (displayNode &&
      displayNode->IsA("vtkMRMLMarkupsDisplayNode"))
    {
    return vtkMRMLMarkupsDisplayNode::SafeDownCast(displayNode);
    }
  return NULL;
}

//-------------------------------------------------------------------------
int vtkMRMLMarkupsPlanesNode::AddPlane(double x, double y, double z,
                                      double nx, double ny, double nz,
                                      double xmin, double ymin, double zmin,
                                      double xmax, double ymax, double zmax,
                                      std::string label)
{
  Markup markup;
  markup.Label = label;
  this->InitMarkup(&markup);

  markup.points.resize(4);
  markup.points[vtkMRMLMarkupsPlanesNode::ORIGIN_INDEX] = vtkVector3d(x,y,z);
  markup.points[vtkMRMLMarkupsPlanesNode::NORMAL_INDEX] = vtkVector3d(nx,ny,nz);
  markup.points[vtkMRMLMarkupsPlanesNode::BOUND_MIN_INDEX] = vtkVector3d(xmin,ymin,zmin);
  markup.points[vtkMRMLMarkupsPlanesNode::BOUND_MAX_INDEX] = vtkVector3d(xmax,ymax,zmax);
  return this->AddMarkup(markup);
}

//-------------------------------------------------------------------------
int vtkMRMLMarkupsPlanesNode::AddPlaneFromArray(double origin[3],
                                               double normal[3],
                                               double min[3],
                                               double max[3],
                                               std::string label)
{
  return this->AddPlane(
    origin[0], origin[1], origin[2],
    normal[0], normal[1], normal[2],
    min[0], min[1], min[2],
    max[0], max[1], max[2],
    label);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::GetNthPlaneOrigin(int n, double pos[3])
{
  this->GetMarkupPoint(n, vtkMRMLMarkupsPlanesNode::ORIGIN_INDEX, pos);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::GetNthPlaneNormal(int n, double pos[3])
{
  this->GetMarkupPoint(n, vtkMRMLMarkupsPlanesNode::NORMAL_INDEX, pos);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::GetNthPlaneBoundMinimum(int n, double pos[3])
{
  this->GetMarkupPoint(n, vtkMRMLMarkupsPlanesNode::BOUND_MIN_INDEX, pos);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::GetNthPlaneBoundMaximum(int n, double pos[3])
{
  this->GetMarkupPoint(n, vtkMRMLMarkupsPlanesNode::BOUND_MAX_INDEX, pos);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::SetNthPlaneOrigin(int n, double x, double y, double z)
{
  this->SetMarkupPoint(n, vtkMRMLMarkupsPlanesNode::ORIGIN_INDEX, x, y, z);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::SetNthPlaneNormal(int n, double x, double y, double z)
{
  this->SetMarkupPoint(n, vtkMRMLMarkupsPlanesNode::NORMAL_INDEX, x, y, z);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::SetNthPlaneBoundMinimum(int n, double x, double y, double z)
{
  this->SetMarkupPoint(n, vtkMRMLMarkupsPlanesNode::BOUND_MIN_INDEX, x, y, z);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::SetNthPlaneBoundMaximum(int n, double x, double y, double z)
{
  this->SetMarkupPoint(n, vtkMRMLMarkupsPlanesNode::BOUND_MAX_INDEX, x, y, z);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::SetNthPlaneOriginFromArray(int n, double pos[3])
{
  this->SetNthPlaneOrigin(n, pos[0], pos[1], pos[2]);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::SetNthPlaneNormalFromArray(int n, double pos[3])
{
  this->SetNthPlaneNormal(n, pos[0], pos[1], pos[2]);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode
::SetNthPlaneBoundMinimumFromArray(int n, double pos[3])
{
  this->SetNthPlaneBoundMinimum(n, pos[0], pos[1], pos[2]);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode
::SetNthPlaneBoundMaximumFromArray(int n, double pos[3])
{
  this->SetNthPlaneBoundMaximum(n, pos[0], pos[1], pos[2]);
}

//-------------------------------------------------------------------------
bool vtkMRMLMarkupsPlanesNode::GetNthPlaneSelected(int n)
{
  return this->GetNthMarkupSelected(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::SetNthPlaneSelected(int n, bool flag)
{
  this->SetNthMarkupSelected(n, flag);
}

//-------------------------------------------------------------------------
bool vtkMRMLMarkupsPlanesNode::GetNthPlaneVisibility(int n)
{
  return this->GetNthMarkupVisibility(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::SetNthPlaneVisibility(int n, bool flag)
{
  this->SetNthMarkupVisibility(n, flag);
}

//-------------------------------------------------------------------------
std::string vtkMRMLMarkupsPlanesNode::GetNthPlaneLabel(int n)
{
  return this->GetNthMarkupLabel(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::SetNthPlaneLabel(int n, std::string label)
{
  this->SetNthMarkupLabel(n, label);
}

//-------------------------------------------------------------------------
std::string vtkMRMLMarkupsPlanesNode::GetNthPlaneAssociatedNodeID(int n)
{
  return this->GetNthMarkupAssociatedNodeID(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::SetNthPlaneAssociatedNodeID(int n, const char* id)
{
  this->SetNthMarkupAssociatedNodeID(n, std::string(id));
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::SetNthPlaneOriginWorldCoordinates(int n, double coords[4])
{
  this->SetMarkupPointWorld(
    n, vtkMRMLMarkupsPlanesNode::ORIGIN_INDEX, coords[0], coords[1], coords[2]);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::GetNthPlaneOriginWorldCoordinates(int n, double coords[4])
{
  this->GetMarkupPointWorld(n, vtkMRMLMarkupsPlanesNode::ORIGIN_INDEX, coords);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::SetNthPlaneBoundMinimumWorldCoordinates(int n, double coords[4])
{
  this->SetMarkupPointWorld(
    n, vtkMRMLMarkupsPlanesNode::BOUND_MIN_INDEX, coords[0], coords[1], coords[2]);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::GetNthPlaneBoundMinimumWorldCoordinates(int n, double coords[4])
{
  this->GetMarkupPointWorld(n, vtkMRMLMarkupsPlanesNode::BOUND_MIN_INDEX, coords);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::GetNthPlaneBoundMaximumWorldCoordinates(int n, double coords[4])
{
  this->GetMarkupPointWorld(n, vtkMRMLMarkupsPlanesNode::BOUND_MAX_INDEX, coords);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::SetNthPlaneBoundMaximumWorldCoordinates(int n, double coords[4])
{
  this->SetMarkupPointWorld(
    n, vtkMRMLMarkupsPlanesNode::BOUND_MAX_INDEX, coords[0], coords[1], coords[2]);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::GetRASBounds(double bounds[6])
{
  vtkBoundingBox box;
  box.GetBounds(bounds);

  int numberOfMarkups = this->GetNumberOfMarkups();
  if (numberOfMarkups == 0)
    {
    return;
    }

  for (int i = 0; i < numberOfMarkups; i++)
    {
    double tmp[4];
    this->GetNthPlaneBoundMinimumWorldCoordinates(i, tmp);
    box.AddPoint(tmp);
    this->GetNthPlaneBoundMaximumWorldCoordinates(i, tmp);
    box.AddPoint(tmp);
    }
  box.GetBounds(bounds);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlanesNode::GetBounds(double bounds[6])
{
   vtkBoundingBox box;
  box.GetBounds(bounds);

  int numberOfMarkups = this->GetNumberOfMarkups();
  if (numberOfMarkups == 0)
    {
    return;
    }

  for (int i = 0; i < numberOfMarkups; i++)
    {
    double tmp[3];
    this->GetNthPlaneBoundMinimum(i, tmp);
    box.AddPoint(tmp);
    this->GetNthPlaneBoundMaximum(i, tmp);
    box.AddPoint(tmp);
    }
  box.GetBounds(bounds);
}
