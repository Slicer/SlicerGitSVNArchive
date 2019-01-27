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
#include "vtkMRMLMarkupsClosedCurveNode.h"
#include "vtkMRMLMarkupsFiducialStorageNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkBoundingBox.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// STD includes
#include <sstream>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLMarkupsClosedCurveNode);


//----------------------------------------------------------------------------
vtkMRMLMarkupsClosedCurveNode::vtkMRMLMarkupsClosedCurveNode()
{
  // maximum number of control points
  // 0 is unlimited
  this->SetMaximumNumberOfControlPoints(0);
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsClosedCurveNode::~vtkMRMLMarkupsClosedCurveNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsClosedCurveNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of,nIndent);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsClosedCurveNode::ReadXMLAttributes(const char** atts)
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
void vtkMRMLMarkupsClosedCurveNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
}


//-----------------------------------------------------------
void vtkMRMLMarkupsClosedCurveNode::UpdateScene(vtkMRMLScene *scene)
{
  Superclass::UpdateScene(scene);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsClosedCurveNode::ProcessMRMLEvents (vtkObject *caller,
                                           unsigned long event,
                                           void *callData)
{
  Superclass::ProcessMRMLEvents(caller, event, callData);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsClosedCurveNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}

//-------------------------------------------------------------------------
vtkMRMLStorageNode* vtkMRMLMarkupsClosedCurveNode::CreateDefaultStorageNode()
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
void vtkMRMLMarkupsClosedCurveNode::CreateDefaultDisplayNodes()
{
  if (this->GetDisplayNode() != nullptr &&
      vtkMRMLMarkupsDisplayNode::SafeDownCast(this->GetDisplayNode()) != nullptr)
    {
    // display node already exists
    return;
    }
  if (this->GetScene()==nullptr)
    {
    vtkErrorMacro("vtkMRMLMarkupsClosedCurveNode::CreateDefaultDisplayNodes failed: scene is invalid");
    return;
    }
  vtkMRMLMarkupsDisplayNode* dispNode = vtkMRMLMarkupsDisplayNode::SafeDownCast
    (this->GetScene()->AddNewNodeByClass("vtkMRMLMarkupsDisplayNode"));
  this->SetAndObserveDisplayNodeID(dispNode->GetID());
}

//-------------------------------------------------------------------------
int vtkMRMLMarkupsClosedCurveNode::AddPoint(double x, double y, double z)
{
  return this->AddPoint(x, y, z, std::string());
}

//-------------------------------------------------------------------------
int vtkMRMLMarkupsClosedCurveNode::AddPoint(double x, double y, double z, std::string label)
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
int vtkMRMLMarkupsClosedCurveNode::AddPointFromArray(double pos[3], std::string label)
{
  return this->AddPoint(pos[0], pos[1], pos[2], label);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsClosedCurveNode::GetNthPointPosition(int n, double pos[3])
{
  vtkVector3d point= this->GetNthControlPointPositionVector(n);
  pos[0] = point.GetX();
  pos[1] = point.GetY();
  pos[2] = point.GetZ();
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsClosedCurveNode::SetNthPointPositionFromArray(int n, double pos[3])
{
  this->SetNthControlPointPositionFromArray(n, pos);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsClosedCurveNode::SetNthPointPosition(int n, double x, double y, double z)
{
  this->SetNthControlPointPosition(n, x, y, z);
}

//-------------------------------------------------------------------------
bool vtkMRMLMarkupsClosedCurveNode::GetNthPointSelected(int n)
{
  return this->GetNthControlPointSelected(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsClosedCurveNode::SetNthPointSelected(int n, bool flag)
{
  this->SetNthControlPointSelected(n, flag);
}

//-------------------------------------------------------------------------
bool vtkMRMLMarkupsClosedCurveNode::GetNthPointLocked(int n)
{
  return this->GetNthControlPointLocked(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsClosedCurveNode::SetNthPointLocked(int n, bool flag)
{
  this->SetNthControlPointLocked(n, flag);
}

//-------------------------------------------------------------------------
bool vtkMRMLMarkupsClosedCurveNode::GetNthPointVisibility(int n)
{
  return this->GetNthControlPointVisibility(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsClosedCurveNode::SetNthPointVisibility(int n, bool flag)
{
  this->SetNthControlPointVisibility(n, flag);
}

//-------------------------------------------------------------------------
std::string vtkMRMLMarkupsClosedCurveNode::GetNthPointLabel(int n)
{
  return this->GetNthControlPointLabel(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsClosedCurveNode::SetNthPointLabel(int n, std::string label)
{
  this->SetNthControlPointLabel(n, label);
}

//-------------------------------------------------------------------------
std::string vtkMRMLMarkupsClosedCurveNode::GetNthPointAssociatedNodeID(int n)
{
  return this->GetNthControlPointAssociatedNodeID(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsClosedCurveNode::SetNthPointAssociatedNodeID(int n, const char* id)
{
  this->SetNthControlPointAssociatedNodeID(n, std::string(id));
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsClosedCurveNode::SetNthPointWorldCoordinates(int n, double coords[4])
{
  this->SetNthControlPointPositionWorld(n, coords[0], coords[1], coords[2]);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsClosedCurveNode::GetNthPointWorldCoordinates(int n, double coords[4])
{
  this->GetNthControlPointPositionWorld(n, coords);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsClosedCurveNode::GetRASBounds(double bounds[6])
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
void vtkMRMLMarkupsClosedCurveNode::GetBounds(double bounds[6])
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
