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
#include "vtkMRMLMarkupsDisplayNode.h"
#include "vtkMRMLMarkupsAngleNode.h"
#include "vtkMRMLMarkupsFiducialStorageNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkBoundingBox.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// STD includes
#include <sstream>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLMarkupsAngleNode);


//----------------------------------------------------------------------------
vtkMRMLMarkupsAngleNode::vtkMRMLMarkupsAngleNode()
{
  // maximum number of control points
  // 4 so we can have one point to use for the follow cursor option
  this->SetMaximumNumberOfControlPoints(4);
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsAngleNode::~vtkMRMLMarkupsAngleNode()
{

}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsAngleNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of,nIndent);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsAngleNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();
  this->RemoveAllControlPoints();

  Superclass::ReadXMLAttributes(atts);
  const char* attName;
  const char* attValue;

  int fidID = 0;
  while (*atts != nullptr)
    {
    attName = *(atts++);
    attValue = *(atts++);
    // backward compatibility reading of annotation Points
    if (!strcmp(attName, "ctrlPtsCoord"))
      {
      std::string valStr(attValue);
      std::stringstream ss;
      double x, y, z;
      ss << valStr;
      ss >> x;
      ss >> y;
      ss >> z;
      fidID = this->AddPoint(x,y,z);
      }
    else if (!strcmp(attName, "ctrlPtsSelected"))
      {
      std::stringstream ss;
      int selected;
      ss << attValue;
      ss >> selected;
      this->SetNthPointSelected(fidID, (selected == 1 ? true : false));
      }
    else if (!strcmp(attName, "ctrlPtsVisible"))
      {
      std::stringstream ss;
      int visible;
      ss << attValue;
      ss >> visible;
      this->SetNthPointVisibility(fidID, (visible == 1 ? true : false));
      }
    else if (!strcmp(attName, "ctrlPtsNumberingScheme"))
      {
      // ignore
      }
    }
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsAngleNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
}


//-----------------------------------------------------------
void vtkMRMLMarkupsAngleNode::UpdateScene(vtkMRMLScene *scene)
{
  Superclass::UpdateScene(scene);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsAngleNode::ProcessMRMLEvents (vtkObject *caller,
                                           unsigned long event,
                                           void *callData)
{
  Superclass::ProcessMRMLEvents(caller, event, callData);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsAngleNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}

//-------------------------------------------------------------------------
vtkMRMLStorageNode* vtkMRMLMarkupsAngleNode::CreateDefaultStorageNode()
{
  vtkMRMLScene* scene = this->GetScene();
  if (scene == nullptr)
    {
    vtkErrorMacro("CreateDefaultStorageNode failed: scene is invalid");
    return nullptr;
    }
  return vtkMRMLStorageNode::SafeDownCast(
    scene->CreateNodeByClass("vtkMRMLMarkupsFiducialStorageNode"));
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsAngleNode::CreateDefaultDisplayNodes()
{
  if (this->GetDisplayNode() != nullptr &&
      vtkMRMLMarkupsDisplayNode::SafeDownCast(this->GetDisplayNode()) != nullptr)
    {
    // display node already exists
    return;
    }
  if (this->GetScene()==nullptr)
    {
    vtkErrorMacro("vtkMRMLMarkupsAngleNode::CreateDefaultDisplayNodes failed: scene is invalid");
    return;
    }
  vtkMRMLMarkupsDisplayNode* dispNode = vtkMRMLMarkupsDisplayNode::SafeDownCast
    (this->GetScene()->AddNewNodeByClass("vtkMRMLMarkupsDisplayNode"));
  this->SetAndObserveDisplayNodeID(dispNode->GetID());
}

//-------------------------------------------------------------------------
int vtkMRMLMarkupsAngleNode::AddPoint(double x, double y, double z)
{
  return this->AddPoint(x, y, z, std::string());
}

//-------------------------------------------------------------------------
int vtkMRMLMarkupsAngleNode::AddPoint(double x, double y, double z, std::string label)
{
  if (this->GetNumberOfPoints() > 2)
    {
    return -1;
    }
  vtkVector3d point;
  point.Set(x, y, z);
  return this->AddControlPoint(point, label);
}

//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
int vtkMRMLMarkupsAngleNode::AddPointFromArray(double pos[3], std::string label)
{
  return this->AddPoint(pos[0], pos[1], pos[2], label);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsAngleNode::GetNthPointPosition(int n, double pos[3])
{
  vtkVector3d point= this->GetNthControlPointPositionVector(n);
  pos[0] = point.GetX();
  pos[1] = point.GetY();
  pos[2] = point.GetZ();
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsAngleNode::SetNthPointPositionFromArray(int n, double pos[3])
{
  this->SetNthControlPointPositionFromArray(n, pos);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsAngleNode::SetNthPointPosition(int n, double x, double y, double z)
{
  this->SetNthControlPointPosition(n, x, y, z);
}

//-------------------------------------------------------------------------
bool vtkMRMLMarkupsAngleNode::GetNthPointSelected(int n)
{
  return this->GetNthControlPointSelected(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsAngleNode::SetNthPointSelected(int n, bool flag)
{
  this->SetNthControlPointSelected(n, flag);
}

//-------------------------------------------------------------------------
bool vtkMRMLMarkupsAngleNode::GetNthPointLocked(int n)
{
  return this->GetNthControlPointLocked(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsAngleNode::SetNthPointLocked(int n, bool flag)
{
  this->SetNthControlPointLocked(n, flag);
}

//-------------------------------------------------------------------------
bool vtkMRMLMarkupsAngleNode::GetNthPointVisibility(int n)
{
  return this->GetNthControlPointVisibility(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsAngleNode::SetNthPointVisibility(int n, bool flag)
{
  this->SetNthControlPointVisibility(n, flag);
}

//-------------------------------------------------------------------------
std::string vtkMRMLMarkupsAngleNode::GetNthPointLabel(int n)
{
  return this->GetNthControlPointLabel(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsAngleNode::SetNthPointLabel(int n, std::string label)
{
  this->SetNthControlPointLabel(n, label);
}

//-------------------------------------------------------------------------
std::string vtkMRMLMarkupsAngleNode::GetNthPointAssociatedNodeID(int n)
{
  return this->GetNthControlPointAssociatedNodeID(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsAngleNode::SetNthPointAssociatedNodeID(int n, const char* id)
{
  this->SetNthControlPointAssociatedNodeID(n, std::string(id));
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsAngleNode::SetNthPointWorldCoordinates(int n, double coords[4])
{
  this->SetNthControlPointPositionWorld(n, coords[0], coords[1], coords[2]);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsAngleNode::GetNthPointWorldCoordinates(int n, double coords[4])
{
  this->GetNthControlPointPositionWorld(n, coords);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsAngleNode::GetRASBounds(double bounds[6])
{
  vtkBoundingBox box;
  box.GetBounds(bounds);

  int numberOfControlPoints = this->GetNumberOfControlPoints();
  if (numberOfControlPoints == 0)
    {
    return;
    }
  double markup_RAS[4] = { 0, 0, 0, 1 };

  for (int i = 0; i < numberOfControlPoints; i++)
    {
    this->GetNthPointWorldCoordinates(i, markup_RAS);
    box.AddPoint(markup_RAS);
    }
  box.GetBounds(bounds);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsAngleNode::GetBounds(double bounds[6])
{
   vtkBoundingBox box;
  box.GetBounds(bounds);

  int numberOfControlPoints = this->GetNumberOfControlPoints();
  if (numberOfControlPoints == 0)
    {
    return;
    }
  double markupPos[4] = { 0, 0, 0 };

  for (int i = 0; i < numberOfControlPoints; i++)
    {
    this->GetNthPointPosition(i, markupPos);
    box.AddPoint(markupPos);
    }
  box.GetBounds(bounds);
}
