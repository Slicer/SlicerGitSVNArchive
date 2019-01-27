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
#include "vtkMRMLMarkupsCurveNode.h"
#include "vtkMRMLMarkupsFiducialStorageNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkBoundingBox.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// STD includes
#include <sstream>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLMarkupsCurveNode);


//----------------------------------------------------------------------------
vtkMRMLMarkupsCurveNode::vtkMRMLMarkupsCurveNode()
{
  // maximum number of control points
  // 0 is unlimited
  this->SetMaximumNumberOfControlPoints(0);
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsCurveNode::~vtkMRMLMarkupsCurveNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of,nIndent);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::ReadXMLAttributes(const char** atts)
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
void vtkMRMLMarkupsCurveNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
}


//-----------------------------------------------------------
void vtkMRMLMarkupsCurveNode::UpdateScene(vtkMRMLScene *scene)
{
  Superclass::UpdateScene(scene);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::ProcessMRMLEvents (vtkObject *caller,
                                           unsigned long event,
                                           void *callData)
{
  Superclass::ProcessMRMLEvents(caller, event, callData);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}

//-------------------------------------------------------------------------
vtkMRMLStorageNode* vtkMRMLMarkupsCurveNode::CreateDefaultStorageNode()
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
void vtkMRMLMarkupsCurveNode::CreateDefaultDisplayNodes()
{
  if (this->GetDisplayNode() != nullptr &&
      vtkMRMLMarkupsDisplayNode::SafeDownCast(this->GetDisplayNode()) != nullptr)
    {
    // display node already exists
    return;
    }
  if (this->GetScene()==nullptr)
    {
    vtkErrorMacro("vtkMRMLMarkupsCurveNode::CreateDefaultDisplayNodes failed: scene is invalid");
    return;
    }
  vtkMRMLMarkupsDisplayNode* dispNode = vtkMRMLMarkupsDisplayNode::SafeDownCast
    (this->GetScene()->AddNewNodeByClass("vtkMRMLMarkupsDisplayNode"));
  this->SetAndObserveDisplayNodeID(dispNode->GetID());
}

//-------------------------------------------------------------------------
int vtkMRMLMarkupsCurveNode::AddPoint(double x, double y, double z)
{
  return this->AddPoint(x, y, z, std::string());
}

//-------------------------------------------------------------------------
int vtkMRMLMarkupsCurveNode::AddPoint(double x, double y, double z, std::string label)
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
int vtkMRMLMarkupsCurveNode::AddPointFromArray(double pos[3], std::string label)
{
  return this->AddPoint(pos[0], pos[1], pos[2], label);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::GetNthPointPosition(int n, double pos[3])
{
  vtkVector3d point= this->GetNthControlPointPositionVector(n);
  pos[0] = point.GetX();
  pos[1] = point.GetY();
  pos[2] = point.GetZ();
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::SetNthPointPositionFromArray(int n, double pos[3])
{
  this->SetNthControlPointPositionFromArray(n, pos);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::SetNthPointPosition(int n, double x, double y, double z)
{
  this->SetNthControlPointPosition(n, x, y, z);
}

//-------------------------------------------------------------------------
bool vtkMRMLMarkupsCurveNode::GetNthPointSelected(int n)
{
  return this->GetNthControlPointSelected(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::SetNthPointSelected(int n, bool flag)
{
  this->SetNthControlPointSelected(n, flag);
}

//-------------------------------------------------------------------------
bool vtkMRMLMarkupsCurveNode::GetNthPointLocked(int n)
{
  return this->GetNthControlPointLocked(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::SetNthPointLocked(int n, bool flag)
{
  this->SetNthControlPointLocked(n, flag);
}

//-------------------------------------------------------------------------
bool vtkMRMLMarkupsCurveNode::GetNthPointVisibility(int n)
{
  return this->GetNthControlPointVisibility(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::SetNthPointVisibility(int n, bool flag)
{
  this->SetNthControlPointVisibility(n, flag);
}

//-------------------------------------------------------------------------
std::string vtkMRMLMarkupsCurveNode::GetNthPointLabel(int n)
{
  return this->GetNthControlPointLabel(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::SetNthPointLabel(int n, std::string label)
{
  this->SetNthControlPointLabel(n, label);
}

//-------------------------------------------------------------------------
std::string vtkMRMLMarkupsCurveNode::GetNthPointAssociatedNodeID(int n)
{
  return this->GetNthControlPointAssociatedNodeID(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::SetNthPointAssociatedNodeID(int n, const char* id)
{
  this->SetNthControlPointAssociatedNodeID(n, std::string(id));
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::SetNthPointWorldCoordinates(int n, double coords[4])
{
  this->SetNthControlPointPositionWorld(n, coords[0], coords[1], coords[2]);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::GetNthPointWorldCoordinates(int n, double coords[4])
{
  this->GetNthControlPointPositionWorld(n, coords);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::GetRASBounds(double bounds[6])
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
void vtkMRMLMarkupsCurveNode::GetBounds(double bounds[6])
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
