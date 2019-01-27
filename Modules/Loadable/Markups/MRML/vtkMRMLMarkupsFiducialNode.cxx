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
#include "vtkMRMLMarkupsFiducialNode.h"
#include "vtkMRMLMarkupsFiducialStorageNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkBoundingBox.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// STD includes
#include <sstream>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLMarkupsFiducialNode);


//----------------------------------------------------------------------------
vtkMRMLMarkupsFiducialNode::vtkMRMLMarkupsFiducialNode()
{
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsFiducialNode::~vtkMRMLMarkupsFiducialNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of,nIndent);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();
  this->RemoveAllControlPoints();

  Superclass::ReadXMLAttributes(atts);
  const char* attName;
  const char* attValue;

  int fidID = 0;
  while (*atts != NULL)
    {
    attName = *(atts++);
    attValue = *(atts++);
    // backward compatibility reading of annotation fiducials
    if (!strcmp(attName, "ctrlPtsCoord"))
      {
      std::string valStr(attValue);
      std::stringstream ss;
      double x, y, z;
      ss << valStr;
      ss >> x;
      ss >> y;
      ss >> z;
      fidID = this->AddFiducial(x,y,z);
      }
    else if (!strcmp(attName, "ctrlPtsSelected"))
      {
      std::stringstream ss;
      int selected;
      ss << attValue;
      ss >> selected;
      this->SetNthFiducialSelected(fidID, (selected == 1 ? true : false));
      }
    else if (!strcmp(attName, "ctrlPtsVisible"))
      {
      std::stringstream ss;
      int visible;
      ss << attValue;
      ss >> visible;
      this->SetNthFiducialVisibility(fidID, (visible == 1 ? true : false));
      }
    else if (!strcmp(attName, "ctrlPtsNumberingScheme"))
      {
      // ignore
      }
    }
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
}


//-----------------------------------------------------------
void vtkMRMLMarkupsFiducialNode::UpdateScene(vtkMRMLScene *scene)
{
  Superclass::UpdateScene(scene);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialNode::ProcessMRMLEvents(vtkObject *caller,
                                                   unsigned long event,
                                                   void *callData)
{
  Superclass::ProcessMRMLEvents(caller, event, callData);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}

//-------------------------------------------------------------------------
vtkMRMLStorageNode* vtkMRMLMarkupsFiducialNode::CreateDefaultStorageNode()
{
  vtkMRMLScene* scene = this->GetScene();
  if (scene == NULL)
    {
    vtkErrorMacro("CreateDefaultStorageNode failed: scene is invalid");
    return NULL;
    }
  return vtkMRMLStorageNode::SafeDownCast(
    scene->CreateNodeByClass("vtkMRMLMarkupsFiducialStorageNode"));
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialNode::CreateDefaultDisplayNodes()
{
  if (this->GetDisplayNode() != NULL &&
      vtkMRMLMarkupsDisplayNode::SafeDownCast(this->GetDisplayNode()) != NULL)
    {
    // display node already exists
    return;
    }
  if (this->GetScene()==NULL)
    {
    vtkErrorMacro("vtkMRMLMarkupsFiducialNode::CreateDefaultDisplayNodes failed: scene is invalid");
    return;
    }
  vtkMRMLMarkupsDisplayNode* dispNode = vtkMRMLMarkupsDisplayNode::SafeDownCast(
    this->GetScene()->AddNewNodeByClass("vtkMRMLMarkupsDisplayNode"));
  this->SetAndObserveDisplayNodeID(dispNode->GetID());
}

//-------------------------------------------------------------------------
int vtkMRMLMarkupsFiducialNode::AddFiducial(double x, double y, double z)
{
  return this->AddFiducial(x, y, z, std::string());
}

//-------------------------------------------------------------------------
int vtkMRMLMarkupsFiducialNode::AddFiducial(double x, double y, double z,
                                            std::string label)
{
  vtkVector3d point;
  point.Set(x, y, z);
  return this->AddControlPoint(point, label);
}

//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
int vtkMRMLMarkupsFiducialNode::AddFiducialFromArray(double pos[3], std::string label)
{
  return this->AddFiducial(pos[0], pos[1], pos[2], label);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialNode::GetNthFiducialPosition(int n, double pos[3])
{
  vtkVector3d point= this->GetNthControlPointPositionVector(n);
  pos[0] = point.GetX();
  pos[1] = point.GetY();
  pos[2] = point.GetZ();
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialNode::SetNthFiducialPositionFromArray(int n, double pos[3])
{
  this->SetNthControlPointPositionFromArray(n, pos);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialNode::SetNthFiducialPosition(int n, double x, double y, double z)
{
  this->SetNthControlPointPosition(n, x, y, z);
}

//-------------------------------------------------------------------------
bool vtkMRMLMarkupsFiducialNode::GetNthFiducialSelected(int n)
{
  return this->GetNthControlPointSelected(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialNode::SetNthFiducialSelected(int n, bool flag)
{
  this->SetNthControlPointSelected(n, flag);
}

//-------------------------------------------------------------------------
bool vtkMRMLMarkupsFiducialNode::GetNthFiducialLocked(int n)
{
  return this->GetNthControlPointLocked(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialNode::SetNthFiducialLocked(int n, bool flag)
{
  this->SetNthControlPointLocked(n, flag);
}

//-------------------------------------------------------------------------
bool vtkMRMLMarkupsFiducialNode::GetNthFiducialVisibility(int n)
{
  return this->GetNthControlPointVisibility(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialNode::SetNthFiducialVisibility(int n, bool flag)
{
  this->SetNthControlPointVisibility(n, flag);
}

//-------------------------------------------------------------------------
std::string vtkMRMLMarkupsFiducialNode::GetNthFiducialLabel(int n)
{
  return this->GetNthControlPointLabel(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialNode::SetNthFiducialLabel(int n, std::string label)
{
  this->SetNthControlPointLabel(n, label);
}

//-------------------------------------------------------------------------
std::string vtkMRMLMarkupsFiducialNode::GetNthFiducialAssociatedNodeID(int n)
{
  return this->GetNthControlPointAssociatedNodeID(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialNode::SetNthFiducialAssociatedNodeID(int n, const char* id)
{
  this->SetNthControlPointAssociatedNodeID(n, std::string(id));
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialNode::SetNthFiducialWorldCoordinates(int n, double coords[4])
{
  this->SetNthControlPointPositionWorld(n, coords[0], coords[1], coords[2]);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialNode::GetNthFiducialWorldCoordinates(int n, double coords[4])
{
  this->GetNthControlPointPositionWorld(n, coords);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialNode::GetRASBounds(double bounds[6])
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
    this->GetNthFiducialWorldCoordinates(i, markup_RAS);
    box.AddPoint(markup_RAS);
    }
  box.GetBounds(bounds);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialNode::GetBounds(double bounds[6])
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
    this->GetNthFiducialPosition(i, markupPos);
    box.AddPoint(markupPos);
    }
  box.GetBounds(bounds);
}
