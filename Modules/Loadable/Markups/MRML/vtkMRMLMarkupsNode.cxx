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
#include "vtkMRMLMarkupsFiducialStorageNode.h"
#include "vtkMRMLMarkupsDisplayNode.h"
#include "vtkMRMLMarkupsNode.h"
#include "vtkMRMLMarkupsStorageNode.h"
#include "vtkMRMLTransformNode.h"

// Slicer MRML includes
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkAbstractTransform.h>
#include <vtkBitArray.h>
#include <vtkCommand.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkStringArray.h>

// STD includes
#include <sstream>
#include <algorithm>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLMarkupsNode);

//----------------------------------------------------------------------------
vtkMRMLMarkupsNode::vtkMRMLMarkupsNode()
{
  this->TextList = vtkStringArray::New();
  this->Locked = 0;

  this->MaximumNumberOfControlPoints = 0;
  this->MarkupLabelFormat = std::string("%N-%d");
  this->LastUsedControlPointNumber = 0;
  this->ActiveControlPoint = -1;
  this->PlacingEnded = false;
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsNode::~vtkMRMLMarkupsNode()
{
  this->TextList->Delete();
  this->RemoveAllControlPoints();
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of,nIndent);

  of << " locked=\"" << this->Locked << "\"";

  if (this->MaximumNumberOfControlPoints>0)
    {
    of << " MaximumNumberOfControlPoints=\"" << this->MaximumNumberOfControlPoints << "\"";
    }
  of << " ActiveControlPoint=\"" << this->ActiveControlPoint << "\"";
  of << " PlacingEnded=\"" << this->PlacingEnded << "\"";

  of << " markupLabelFormat=\"" << this->MarkupLabelFormat.c_str() << "\"";

  int textLength = static_cast<int>(this->TextList->GetNumberOfValues());

  for (int i = 0 ; i < textLength; i++)
    {
    of << " textList" << i << "=\"" << this->TextList->GetValue(i) << "\"";
    }
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();
  this->RemoveAllControlPoints();
  this->RemoveAllTexts();

  bool maximumNumberOfControlPointsSpecified = false;

  Superclass::ReadXMLAttributes(atts);
  const char* attName;
  const char* attValue;

  while (*atts != nullptr)
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strncmp(attName, "textList", 9))
      {
      this->AddText(attValue);
      }
    else if (!strcmp(attName, "locked"))
      {
      this->SetLocked(atoi(attValue));
      }
    else if (!strcmp(attName, "MaximumNumberOfControlPoints"))
      {
      this->SetMaximumNumberOfControlPoints(atoi(attValue));
      maximumNumberOfControlPointsSpecified = true;
      }
    else if (!strcmp(attName, "ActiveControlPoint"))
      {
      this->SetActiveControlPoint(atoi(attValue));
      }
    else if (!strcmp(attName, "PlacingEnded"))
      {
      this->SetPlacingEnded(atoi(attValue));
      }
    else if (!strcmp(attName, "markupLabelFormat"))
      {
      this->SetMarkupLabelFormat(attValue);
      }
    }

  // If maximumNumberOfControlPointsSpecified is not specified in XML then it means
  // there is no limit.
  if (!maximumNumberOfControlPointsSpecified)
    {
    this->SetMaximumNumberOfControlPoints(0);
    }

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);

  vtkMRMLMarkupsNode *node = vtkMRMLMarkupsNode::SafeDownCast(anode);
  if (!node)
    {
    return;
    }

  this->SetLocked(node->GetLocked());
  this->SetMaximumNumberOfControlPoints(node->GetMaximumNumberOfControlPoints());
  this->TextList->DeepCopy(node->TextList);
  this->SetActiveControlPoint(node->GetActiveControlPoint());
  this->SetPlacingEnded(node->GetPlacingEnded());

  // BUG: When fiducial nodes appear in scene views as of Slicer 4.1 the per
  // fiducial information (visibility, position etc) is saved to the file on
  // disk and not read, so the scene view copy of a fiducial node doesn't have
  // any fiducials in it. This work around prevents the main scene fiducial
  // list from being cleared of points and then not repopulated.
  // TBD: if scene view node reading xml triggers reading the data from
  // storage nodes, this should no longer be necessary.
  if (this->Scene &&
      this->Scene->IsRestoring())
    {
    if (this->GetNumberOfControlPoints() != 0 &&
        node->GetNumberOfControlPoints() == 0)
      {
      // just return for now
      vtkWarningMacro("MarkupsNode Copy: Scene view is restoring and list to restore is empty, skipping copy of points");
      return;
      }
    }

  this->RemoveAllControlPoints();
  int numMarkups = node->GetNumberOfControlPoints();
  for (int n = 0; n < numMarkups; n++)
    {
    ControlPoint *controlPoint = node->GetNthControlPoint(n);
    int controlPointIndex = this->AddControlPoint(controlPoint);
    this->CopyControlPoint(controlPoint, this->GetNthControlPoint(controlPointIndex));
    }

  // set max number of markups after adding the new ones
  this->LastUsedControlPointNumber = node->LastUsedControlPointNumber;
}


//-----------------------------------------------------------
void vtkMRMLMarkupsNode::UpdateScene(vtkMRMLScene *scene)
{
  Superclass::UpdateScene(scene);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsNode::ProcessMRMLEvents(vtkObject *caller,
                                           unsigned long event,
                                           void *callData)
{
  Superclass::ProcessMRMLEvents(caller, event, callData);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << "Locked: " << this->Locked << "\n";
  os << indent << "MaximumNumberOfControlPoints: ";
  if (this->MaximumNumberOfControlPoints>0)
    {
    os << this->MaximumNumberOfControlPoints << "\n";
    }
  else
    {
    os << "unlimited\n";
    }
  os << indent << "MarkupLabelFormat: " << this->MarkupLabelFormat.c_str() << "\n";
  os << indent << "NumberOfControlPoints: " << this->GetNumberOfControlPoints() << "\n";
  os << indent << "ActiveControlPoint: " << this->ActiveControlPoint << "\n";
  os << indent << "PlacingEnded: " << this->PlacingEnded << "\n";

  for (int controlPointIndex = 0; controlPointIndex < this->GetNumberOfControlPoints(); controlPointIndex++)
    {
    ControlPoint* controlPoint = this->GetNthControlPoint(controlPointIndex);
    if (!controlPoint)
      {
      continue;
      }
    os << indent << "Control Point " << controlPointIndex << ":\n";
    os << indent << "ID = " << controlPoint->ID.c_str() << "\n";
    os << indent << "Label = " << controlPoint->Label.c_str() << "\n";
    os << indent << "Description = " << controlPoint->Description.c_str() << "\n";
    os << indent << "Associated node id = " << controlPoint->AssociatedNodeID.c_str() << "\n";
    os << indent << "Selected = " << controlPoint->Selected << "\n";
    os << indent << "Locked = " << controlPoint->Locked << "\n";
    os << indent << "Visibility = " << controlPoint->Visibility << "\n";
    os << indent << "Position : " << controlPoint->WorldPosition.GetX() << ", " <<
          controlPoint->WorldPosition.GetY() << ", " << controlPoint->WorldPosition.GetZ() << "\n";
    os << indent << "Orientation = "
       << controlPoint->OrientationWXYZ.GetW() << ","
       << controlPoint->OrientationWXYZ.GetX() << ","
       << controlPoint->OrientationWXYZ.GetY() << ","
       << controlPoint->OrientationWXYZ.GetZ() << "\n";

    }

  os << indent << "textList: ";
  if  (!this->TextList || !this->GetNumberOfTexts())
    {
    os << indent << "None"  << endl;
    }
  else
    {
    os << endl;
    for (int i = 0 ; i < this->GetNumberOfTexts() ; i++)
      {
      os << indent << "  " << i <<": " <<  (TextList->GetValue(i) ? TextList->GetValue(i) : "(none)") << endl;
      }
    }

}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsNode::RemoveAllControlPoints()
{
  for(unsigned int i = 0; i < this->ControlPoints.size(); i++)
    {
    this->ControlPoints[i]->intermadiatePoints.clear();
    delete this->ControlPoints[i];
    }

  this->ControlPoints.clear();

  this->InvokeCustomModifiedEvent(vtkMRMLMarkupsNode::AllPointsRemovedEvent);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsNode::SetText(int id, const char *newText)
{
  if (id < 0)
    {
    vtkErrorMacro("SetText: Invalid ID");
    return;
    }
  if (!this->TextList)
    {
    vtkErrorMacro("SetText: TextList is NULL");
    return;
    }

  vtkStdString newString;
  if (newText)
    {
    newString = vtkStdString(newText);
    }

  // check if the same as before
  if (((this->TextList->GetNumberOfValues() == 0) && (newText == nullptr || newString == "")) ||
      ((this->TextList->GetNumberOfValues() > id) &&
       (this->TextList->GetValue(id) == newString)))
    {
    return;
    }

  this->TextList->InsertValue(id,newString);

  // invoke a modified event
  this->Modified();
}

//-------------------------------------------------------------------------
int vtkMRMLMarkupsNode::AddText(const char *newText)
{
  if (!this->TextList)
    {
    vtkErrorMacro("Markups: For node " << this->GetName() << " text is not defined");
    return -1 ;
    }

  int n = this->GetNumberOfTexts();
  this->SetText(n,newText);

  return n;
}

//-------------------------------------------------------------------------
vtkStdString vtkMRMLMarkupsNode::GetText(int n)
{
  if ((this->GetNumberOfTexts() <= n) || n < 0)
    {
    return vtkStdString();
    }

  return this->TextList->GetValue(n);
}

//-------------------------------------------------------------------------
int  vtkMRMLMarkupsNode::DeleteText(int id)
{
  if (!this->TextList)
    {
    return -1;
    }

  int n = this->GetNumberOfTexts();
  if (id < 0 || id >= n)
    {
    return -1;
    }

  for (int i = id; i < n-1; i++)
    {
    this->TextList->SetValue(i, this->GetText(i+1));
    }

  this->TextList->Resize(n-1);

  return 1;
}


//-------------------------------------------------------------------------
int vtkMRMLMarkupsNode::GetNumberOfTexts()
{
  if (!this->TextList)
    {
    return -1;
    }
  return static_cast<int>(this->TextList->GetNumberOfValues());
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsNode::RemoveAllTexts()
{
  this->TextList->Initialize();
}

//-------------------------------------------------------------------------
vtkMRMLStorageNode* vtkMRMLMarkupsNode::CreateDefaultStorageNode()
{
  vtkMRMLScene* scene = this->GetScene();
  if (scene == nullptr)
    {
    vtkErrorMacro("CreateDefaultStorageNode failed: scene is invalid");
    return nullptr;
    }
  return vtkMRMLStorageNode::SafeDownCast(
    scene->CreateNodeByClass("vtkMRMLMarkupsStorageNode"));
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsNode::SetLocked(int locked)
{
  if (this->Locked == locked)
    {
    return;
    }
  this->Locked = locked;

  this->InvokeCustomModifiedEvent(vtkMRMLMarkupsNode::LockModifiedEvent);
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsNode::ControlPointExists(int n)
{
  if (n < 0 || n >= this->GetNumberOfControlPoints())
    {
    return false;
    }

  if (this->ControlPoints[static_cast<size_t>(n)] != nullptr)
    {
    return true;
    }

  return false;
}

//---------------------------------------------------------------------------
int vtkMRMLMarkupsNode::GetNumberOfControlPoints()
{
  return static_cast<int> (this->ControlPoints.size());
}

//---------------------------------------------------------------------------
ControlPoint *vtkMRMLMarkupsNode::GetNthControlPoint(int n)
{
  if (!this->ControlPointExists(n))
    {
    return nullptr;
    }

  return this->ControlPoints[static_cast<unsigned int> (n)];
}

//-----------------------------------------------------------
std::vector<ControlPoint *>* vtkMRMLMarkupsNode::GetControlPoints()
{
  return &this->ControlPoints;
}

//-----------------------------------------------------------
void vtkMRMLMarkupsNode::InitControlPoint(ControlPoint *controlPoint)
{
  if (!controlPoint)
    {
    vtkErrorMacro("InitMarkup: null controlPoint!");
    return;
    }

  // generate a unique id based on list policy
  std::string id = this->GenerateUniqueControlPointID();
  controlPoint->ID = id;

  if (!controlPoint->Label.compare(""))
    {
    std::string formatString = this->ReplaceListNameInMarkupLabelFormat();
    std::string str = formatString.substr(0, formatString.size()-2);
    controlPoint->Label = str + id;
    }

  // use an empty description
  controlPoint->Description = std::string("");
  // use an empty associated node id
  controlPoint->AssociatedNodeID = std::string("");

  // position is 0
  controlPoint->WorldPosition.Set(0, 0, 0);

  // orientatation is 0 around the z axis
  controlPoint->OrientationWXYZ.Set(0, 0, 0, 1);

  // set the flags
  controlPoint->Selected = true;
  controlPoint->Locked = false;
  controlPoint->Visibility = true;

  controlPoint->intermadiatePoints.clear();
}

//-----------------------------------------------------------
int vtkMRMLMarkupsNode::AddControlPoint(ControlPoint *controlPoint)
{  
  if (this->MaximumNumberOfControlPoints != 0 &&
      this->GetNumberOfControlPoints() + 1 > this->MaximumNumberOfControlPoints)
    {
    vtkErrorMacro("AddNControlPoints: number of points major than maximum number of control points allowed.");
    return -1;
    }

  this->ControlPoints.push_back(controlPoint);
  this->LastUsedControlPointNumber++;

  int controlPointIndex = this->GetNumberOfControlPoints() - 1;
  this->ActiveControlPoint = controlPointIndex;

  this->InvokeCustomModifiedEvent(vtkMRMLMarkupsNode::PointAddedEvent,  static_cast<void*>(&controlPointIndex));
  return controlPointIndex;
}

//-----------------------------------------------------------
int vtkMRMLMarkupsNode::AddNControlPoints(int n, std::string label /*=std::string()*/, vtkVector3d* point /*=NULL*/)
{
  int controlPointIndex = -1;
  if (n < 0)
    {
    vtkErrorMacro("AddNControlPoints: invalid number of points " << n);
    return controlPointIndex;
    }

  if (this->MaximumNumberOfControlPoints != 0 &&  n > this->MaximumNumberOfControlPoints)
    {
    vtkErrorMacro("AddNControlPoints: number of points " << n <<
                  " major than maximum number of control points allowed : " << this->MaximumNumberOfControlPoints);
    return controlPointIndex;
    }

  for (int i = 0; i < n; i++)
    {
    ControlPoint *controlPoint = new ControlPoint;
    controlPoint->Label = label;
    this->InitControlPoint(controlPoint);
    if (point != nullptr)
      {
      controlPoint->WorldPosition.Set(point->GetX(), point->GetY(), point->GetZ());
      }
    controlPointIndex = this->AddControlPoint(controlPoint);
    }

  return controlPointIndex;
}

//-----------------------------------------------------------
int vtkMRMLMarkupsNode::AddControlPointWorld(vtkVector3d pointWorld, std::string label /*=std::string()*/)
{
  vtkVector3d point;
  this->TransformPointFromWorld(pointWorld, point);
  return this->AddNControlPoints(1, label, &point);
}

//-----------------------------------------------------------
int vtkMRMLMarkupsNode::AddControlPoint(vtkVector3d point, std::string label /*=std::string()*/)
{
  return this->AddNControlPoints(1, label, &point);
}

//-----------------------------------------------------------
vtkVector3d vtkMRMLMarkupsNode::GetNthControlPointPositionVector(int pointIndex)
{
  if (!this->ControlPointExists(pointIndex))
    {
    vtkVector3d point;
    point.Set(0, 0, 0);
    return point;
    }

  return this->GetNthControlPoint(pointIndex)->WorldPosition;
}

//-----------------------------------------------------------
void vtkMRMLMarkupsNode::GetNthControlPointPosition(int pointIndex, double point[3])
{
  vtkVector3d vectorPoint = this->GetNthControlPointPositionVector(pointIndex);
  point[0] = vectorPoint.GetX();
  point[1] = vectorPoint.GetY();
  point[2] = vectorPoint.GetZ();
}

//-----------------------------------------------------------
void vtkMRMLMarkupsNode::GetNthControlPointPositionLPS(int pointIndex, double point[3])
{
  vtkVector3d vectorPoint = this->GetNthControlPointPositionVector(pointIndex);
  point[0] = -1.0 * vectorPoint.GetX();
  point[1] = -1.0 * vectorPoint.GetY();
  point[2] = vectorPoint.GetZ();
}

//-----------------------------------------------------------
int vtkMRMLMarkupsNode::GetNthControlPointPositionWorld(int pointIndex, double worldxyz[4])
{
  vtkVector3d world;
  this->TransformPointToWorld(this->GetNthControlPointPositionVector(pointIndex), world);
  worldxyz[0] = world[0];
  worldxyz[1] = world[1];
  worldxyz[2] = world[2];
  worldxyz[3] = 1;
  return 1;
}

//-----------------------------------------------------------
void vtkMRMLMarkupsNode::RemoveNthControlPoint(int pointIndex)
{
  if (!this->ControlPointExists(pointIndex))
    {
    return;
    }

  this->ControlPoints[static_cast<unsigned int> (pointIndex)]->intermadiatePoints.clear();
  delete this->ControlPoints[static_cast<unsigned int> (pointIndex)];
  this->ControlPoints.erase(this->ControlPoints.begin() + pointIndex);

  this->InvokeCustomModifiedEvent(vtkMRMLMarkupsNode::PointRemovedEvent, static_cast<void*>(&pointIndex));
}

//-----------------------------------------------------------
void vtkMRMLMarkupsNode::RemoveLastControlPoint()
{
  int pointIndex = this->GetNumberOfControlPoints() - 1;
  if (!this->ControlPointExists(pointIndex))
    {
    return;
    }

  this->ControlPoints[static_cast<unsigned int> (pointIndex)]->intermadiatePoints.clear();
  delete this->ControlPoints[static_cast<unsigned int> (pointIndex)];
  this->ControlPoints.erase(this->ControlPoints.begin() + pointIndex);
  this->LastUsedControlPointNumber--;

  this->InvokeCustomModifiedEvent(vtkMRMLMarkupsNode::PointRemovedEvent, static_cast<void*>(&pointIndex));
}

//-----------------------------------------------------------
bool vtkMRMLMarkupsNode::InsertControlPoint(ControlPoint *controlPoint, int targetIndex)
{
  int listSize = this->GetNumberOfControlPoints();

  int destIndex = targetIndex;
  if (targetIndex < 0)
    {
    destIndex = 0;
    }
  else if (targetIndex > listSize)
    {
    destIndex = listSize;
    }
  vtkDebugMacro("InsertControlPoint: list size = " << listSize
                << ", input target index = " << targetIndex
                << ", adjusted destination index = " << destIndex);

  std::vector < ControlPoint* >::iterator pos;
  pos = this->ControlPoints.begin() + destIndex;

  std::vector < ControlPoint* >::iterator result;
  result = this->ControlPoints.insert(pos, controlPoint);

  // sanity check
  if ((*result)->Label.compare(controlPoint->Label) != 0)
    {
    vtkErrorMacro("InsertMarkup: failed to insert a control point at index " << destIndex
                  << ", expected label on that control point to be " << controlPoint->Label.c_str()
                  << " but got " << (*result)->Label.c_str());
    return false;
    }

  // let observers know that a markup was added
  this->InvokeCustomModifiedEvent(vtkMRMLMarkupsNode::PointAddedEvent, static_cast<void*>(&targetIndex));

  return true;
}

//-----------------------------------------------------------
void vtkMRMLMarkupsNode::CopyControlPoint(ControlPoint *source, ControlPoint *target)
{
  if (source == nullptr || target == nullptr)
    {
    return;
    }

  target->ID = source->ID;
  target->Label = source->Label;
  target->Description = source->Description;
  target->AssociatedNodeID = source->AssociatedNodeID;
  target->Selected = source->Selected;
  target->Locked = source->Locked;
  target->Visibility = source->Visibility;

  target->WorldPosition = source->WorldPosition;
  for (int i = 0; i < 4; ++i)
    {
    target->OrientationWXYZ[i] = source->OrientationWXYZ[i];
    }

  target->intermadiatePoints.clear();
  for (unsigned int m = 0; m < source->intermadiatePoints.size(); m++)
    {
    vtkVector3d point = source->intermadiatePoints[m];
    target->intermadiatePoints.push_back(point);
    }
}

//-----------------------------------------------------------
void vtkMRMLMarkupsNode::SwapControlPoints(int m1, int m2)
{
  if (!this->ControlPointExists(m1))
    {
    vtkErrorMacro("SwapMarkups: first control point index is out of range 0-" <<
                  this->GetNumberOfControlPoints() -1 << ", m1 = " << m1);
    return;
    }
  if (!this->ControlPointExists(m2))
    {
    vtkErrorMacro("SwapMarkups: second control point index is out of range 0-" <<
                  this->GetNumberOfControlPoints() -1 << ", m2 = " << m2);
    return;
    }

  ControlPoint *m1Markup = this->GetNthControlPoint(m1);
  ControlPoint m1MarkupBackup;
  // make a copy of the first control point
  this->CopyControlPoint(m1Markup, &m1MarkupBackup);
  // copy the second control point into the first
  this->CopyControlPoint(this->GetNthControlPoint(m2), m1Markup);
  // and copy the backup of the first one into the second
  this->CopyControlPoint(&m1MarkupBackup, this->GetNthControlPoint(m2));

  // and let listeners know that two control points have changed
  this->InvokeCustomModifiedEvent(vtkMRMLMarkupsNode::PointModifiedEvent, static_cast<void*>(&m1));
  this->InvokeCustomModifiedEvent(vtkMRMLMarkupsNode::PointModifiedEvent, static_cast<void*>(&m2));
}

//-----------------------------------------------------------
void vtkMRMLMarkupsNode::SetNthControlPointPositionFromPointer(const int pointIndex,
                                                               const double * pos)
{
  if (!pos)
    {
    vtkErrorMacro("SetNthControlPointFromPointer: invalid position pointer!");
    return;
    }

  this->SetNthControlPointPosition(pointIndex, pos[0], pos[1], pos[2]);
}

//-----------------------------------------------------------
void vtkMRMLMarkupsNode::SetNthControlPointPositionFromArray(const int pointIndex,
                                                             const double pos[3])
{
  this->SetNthControlPointPosition(pointIndex, pos[0], pos[1], pos[2]);
}

//-----------------------------------------------------------
void vtkMRMLMarkupsNode::SetNthControlPointPosition(const int pointIndex,
                                                    const double x, const double y, const double z)
{
  if (!this->ControlPointExists(pointIndex))
    {
    return;
    }

  this->GetNthControlPoint(pointIndex)->WorldPosition.Set(x, y, z);

  // throw an event to let listeners know the position has changed
  int n = pointIndex;
  this->InvokeCustomModifiedEvent(vtkMRMLMarkupsNode::PointModifiedEvent, static_cast<void*>(&n));
}

//-----------------------------------------------------------
void vtkMRMLMarkupsNode::SetNthControlPointPositionLPS(const int pointIndex,
                                                       const double x, const double y, const double z)
{
  double r, a, s;
  r = -x;
  a = -y;
  s = z;
  this->SetNthControlPointPosition(pointIndex, r, a, s);
}

//-----------------------------------------------------------
void vtkMRMLMarkupsNode::SetNthControlPointPositionWorld(const int pointIndex,
                                                         const double x, const double y, const double z)
{
  if (!this->ControlPointExists(pointIndex))
    {
    return;
    }

  vtkVector3d markupxyz;
  TransformPointFromWorld(vtkVector3d(x,y,z), markupxyz);
  this->SetNthControlPointPosition(pointIndex, markupxyz[0], markupxyz[1], markupxyz[2]);
}

//-----------------------------------------------------------
void vtkMRMLMarkupsNode::SetNthControlPointOrientationFromPointer(int n, const double *orientation)
{
  if (!orientation)
    {
    vtkErrorMacro("Invalid orientation pointer!");
    return;
    }
  this->SetNthControlPointOrientation(n, orientation[0], orientation[1], orientation[2], orientation[3]);
}

//-----------------------------------------------------------
void vtkMRMLMarkupsNode::SetNthControlPointOrientationFromArray(int n, const double orientation[4])
{
  this->SetNthControlPointOrientation(n, orientation[0], orientation[1], orientation[2], orientation[3]);
}

//-----------------------------------------------------------
void vtkMRMLMarkupsNode::SetNthControlPointOrientation(int n, double w, double x, double y, double z)
{
  if (!this->ControlPointExists(n))
    {
    return;
    }

  ControlPoint *controlPoint = this->GetNthControlPoint(n);
  controlPoint->OrientationWXYZ.Set(w, x, y, z);

  this->InvokeCustomModifiedEvent(vtkMRMLMarkupsNode::PointModifiedEvent, static_cast<void*>(&n));
}

//-----------------------------------------------------------
void vtkMRMLMarkupsNode::GetNthControlPointOrientation(int n, double orientation[4])
{
  if (!this->ControlPointExists(n))
    {
    return;
    }

  ControlPoint *controlPoint = this->GetNthControlPoint(n);
  orientation[0] = controlPoint->OrientationWXYZ.GetW();
  orientation[1] = controlPoint->OrientationWXYZ.GetX();
  orientation[2] = controlPoint->OrientationWXYZ.GetY();
  orientation[3] = controlPoint->OrientationWXYZ.GetZ();
}

//-----------------------------------------------------------
vtkVector4d vtkMRMLMarkupsNode::GetNthControlPointOrientationVector(int pointIndex)
{
  if (!this->ControlPointExists(pointIndex))
    {
    vtkVector4d orientation;
    orientation.Set(0, 0, 0, 0);
    return orientation;
    }

  return this->GetNthControlPoint(pointIndex)->OrientationWXYZ;
}

//-----------------------------------------------------------
std::string vtkMRMLMarkupsNode::GetNthControlPointAssociatedNodeID(int n)
{
  if (!this->ControlPointExists(n))
    {
    return std::string("");
    }

  return this->GetNthControlPoint(n)->AssociatedNodeID;
}

//-----------------------------------------------------------
void vtkMRMLMarkupsNode::SetNthControlPointAssociatedNodeID(int n, std::string id)
{
  vtkDebugMacro("SetNthMarkupAssociatedNodeID: n = " << n << ", id = '" << id.c_str() << "'");
  if (!this->ControlPointExists(n))
    {
    vtkErrorMacro("SetNthMarkupAssociatedNodeID: control point " << n << " doesn't exist, can't set id to " << id);
    return;
    }

  vtkDebugMacro("Changing markup " << n << " associated node id from " <<
                this->GetNthControlPoint(n)->AssociatedNodeID.c_str() << " to " << id.c_str());
  this->GetNthControlPoint(n)->AssociatedNodeID = std::string(id.c_str());

  this->InvokeCustomModifiedEvent(vtkMRMLMarkupsNode::PointModifiedEvent, static_cast<void*>(&n));
}

//-----------------------------------------------------------
std::string vtkMRMLMarkupsNode::GetNthControlPointID(int n)
{
  if (!this->ControlPointExists(n))
    {
    return std::string("");
    }

  return this->GetNthControlPoint(n)->ID;
}

//-------------------------------------------------------------------------
int vtkMRMLMarkupsNode::GetNthControlPointIndexByID(const char* controlPointID)
{
  if (!controlPointID)
    {
    return -1;
    }

  for (int controlPointIndex = 0; controlPointIndex < this->GetNumberOfControlPoints(); controlPointIndex++)
    {
    ControlPoint *compareControlPoint = this->GetNthControlPoint(controlPointIndex);
    if (compareControlPoint &&
        strcmp(compareControlPoint->ID.c_str(), controlPointID) == 0)
      {
      return controlPointIndex;
      }
    }
  return -1;
}

//-------------------------------------------------------------------------
ControlPoint* vtkMRMLMarkupsNode::GetNthControlPointByID(const char* controlPointID)
{
  if (!controlPointID)
    {
    return nullptr;
    }

  int controlPointIndex = this->GetNthControlPointIndexByID(controlPointID);
  if (controlPointIndex >= 0 && controlPointIndex < this->GetNumberOfControlPoints())
    {
    return this->GetNthControlPoint(controlPointIndex);
    }
  return nullptr;
}

//-----------------------------------------------------------
void vtkMRMLMarkupsNode::SetActiveControlPoint(int index)
{
  if (this->ActiveControlPoint == index)
    {
    return;
    }

  this->ActiveControlPoint = index;
  this->Modified();
}

//-----------------------------------------------------------
void vtkMRMLMarkupsNode::SetNthControlPointID(int n, std::string id)
{
  vtkDebugMacro("SetNthControlPointID: n = " << n << ", id = '" << id.c_str() << "'");
  if (!this->ControlPointExists(n))
    {
    vtkWarningMacro("SetNthControlPointID: control point " << n << " doesn't exist, can't set id to " << id);
    return;
    }

  ControlPoint *controlPoint = this->GetNthControlPoint(n);
  if (controlPoint->ID.compare(id) != 0)
    {
    vtkDebugMacro("Changing control point " << n << " associated node id from " <<
                  controlPoint->ID.c_str() << " to " << id.c_str());
    controlPoint->ID = std::string(id.c_str());
    }
  else
    {
    vtkDebugMacro("SetNthControlPointID: not changing, was the same: " << controlPoint->ID);
    }
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsNode::GetNthControlPointSelected(int n)
{
 if (!this->ControlPointExists(n))
   {
   return false;
   }

 return this->GetNthControlPoint(n)->Selected;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsNode::SetNthControlPointSelected(int n, bool flag)
{
  if (!this->ControlPointExists(n))
    {
    return;
    }

  ControlPoint *controlPoint = this->GetNthControlPoint(n);
  if (controlPoint->Selected != flag)
    {
    controlPoint->Selected = flag;
    this->InvokeCustomModifiedEvent(vtkMRMLMarkupsNode::PointModifiedEvent, static_cast<void*>(&n));
    }
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsNode::GetNthControlPointLocked(int n)
{
  if (!this->ControlPointExists(n))
    {
    return false;
    }

  return this->GetNthControlPoint(n)->Locked;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsNode::SetNthControlPointLocked(int n, bool flag)
{
  if (!this->ControlPointExists(n))
    {
    return;
    }

  ControlPoint *controlPoint = this->GetNthControlPoint(n);
  if (controlPoint->Locked != flag)
    {
    controlPoint->Locked = flag;
    this->InvokeCustomModifiedEvent(vtkMRMLMarkupsNode::PointModifiedEvent, static_cast<void*>(&n));
    }
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsNode::GetNthControlPointVisibility(int n)
{
  if (!this->ControlPointExists(n))
    {
    return false;
    }

  return this->GetNthControlPoint(n)->Visibility;
}


//---------------------------------------------------------------------------
void vtkMRMLMarkupsNode::SetNthControlPointVisibility(int n, bool flag)
{
  if (!this->ControlPointExists(n))
    {
    return;
    }

  ControlPoint *controlPoint = this->GetNthControlPoint(n);
  if (controlPoint->Visibility != flag)
    {
    controlPoint->Visibility = flag;
    this->InvokeCustomModifiedEvent(vtkMRMLMarkupsNode::PointModifiedEvent, static_cast<void*>(&n));
    }
}

//---------------------------------------------------------------------------
std::string vtkMRMLMarkupsNode::GetNthControlPointLabel(int n)
{
  if (!this->ControlPointExists(n))
    {
    return std::string("");
    }

  return this->GetNthControlPoint(n)->Label;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsNode::SetNthControlPointLabel(int n, std::string label)
{
  if (!this->ControlPointExists(n))
    {
    return ;
    }

  ControlPoint *controlPoint = this->GetNthControlPoint(n);
  if (controlPoint->Label.compare(label))
    {
    controlPoint->Label = label;
    this->InvokeCustomModifiedEvent(vtkMRMLMarkupsNode::PointModifiedEvent, static_cast<void*>(&n));
    }
}

//---------------------------------------------------------------------------
std::string vtkMRMLMarkupsNode::GetNthControlPointDescription(int n)
{
  if (!this->ControlPointExists(n))
    {
    return std::string("");
    }

  return this->GetNthControlPoint(n)->Description;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsNode::SetNthControlPointDescription(int n, std::string description)
{
  if (!this->ControlPointExists(n))
    {
    return ;
    }

  ControlPoint *controlPoint = this->GetNthControlPoint(n);
  if (controlPoint->Description.compare(description))
    {
    controlPoint->Description = description;
    this->InvokeCustomModifiedEvent(vtkMRMLMarkupsNode::PointModifiedEvent, static_cast<void*>(&n));
    }
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsNode::CanApplyNonLinearTransforms()const
{
  return true;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsNode::ApplyTransform(vtkAbstractTransform* transform)
{
  int numControlPoints = this->GetNumberOfControlPoints();
  double xyzIn[3];
  double xyzOut[3];
  for (int controlpointIndex = 0; controlpointIndex < numControlPoints; controlpointIndex++)
    {
    this->GetNthControlPointPosition(controlpointIndex, xyzIn);
    transform->TransformPoint(xyzIn,xyzOut);
    this->SetNthControlPointPositionFromArray(controlpointIndex, xyzOut);
    }
  this->StorableModifiedTime.Modified();
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsNode::
WriteCLI(std::vector<std::string>& commandLine, std::string prefix,
         int coordinateSystem, int multipleFlag)
{
  Superclass::WriteCLI(commandLine, prefix, coordinateSystem, multipleFlag);

  int numControlPoints = this->GetNumberOfControlPoints();

  // check if the coordinate system flag is set to LPS, otherwise assume RAS
  bool useLPS = false;
  if (coordinateSystem == 1)
    {
    useLPS = true;
    }

  // loop over the control points
  for (int m = 0; m < numControlPoints; m++)
    {
    // only use selected markups
    if (this->GetNthControlPointSelected(m))
      {
      std::stringstream ss;
      double point[3];
      if (useLPS)
        {
        this->GetNthControlPointPositionLPS(m, point);
        }
      else
        {
        this->GetNthControlPointPosition(m, point);
        }
      // write
      if (prefix.compare("") != 0)
        {
        commandLine.push_back(prefix);
        }
      // avoid scientific notation
      //ss.precision(5);
      //ss << std::fixed << point[0] << "," <<  point[1] << "," <<  point[2] ;
      ss << point[0] << "," <<  point[1] << "," <<  point[2];
      commandLine.push_back(ss.str());
      if (multipleFlag == 0)
        {
        // only print out one markup, but print out all the points in that markup
        // (if have a ruler, need to do 2 points)
        break;
        }
      }
    }
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsNode::GetModifiedSinceRead()
{
  return this->Superclass::GetModifiedSinceRead() ||
    (this->GetMTime() > this->GetStoredTime());
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsNode::ResetNthControlPointID(int n)
{
  if (!this->ControlPointExists(n))
    {
    return false;
    }

  this->SetNthControlPointID(n, this->GenerateUniqueControlPointID());

  return true;
}

//---------------------------------------------------------------------------
std::string vtkMRMLMarkupsNode::GenerateUniqueControlPointID()
{
  std::string id;
  int controlPointNumber = this->LastUsedControlPointNumber;
  // increment by one so as not to start with 0
  controlPointNumber++;
  // put the number in a string
  return std::to_string(controlPointNumber);
}

//---------------------------------------------------------------------------
std::string vtkMRMLMarkupsNode::GetMarkupLabelFormat()
{
  return this->MarkupLabelFormat;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsNode::SetMarkupLabelFormat(std::string format)
{
  if (this->MarkupLabelFormat.compare(format) == 0)
    {
    return;
    }
  this->MarkupLabelFormat = format;

  this->InvokeCustomModifiedEvent(vtkMRMLMarkupsNode::LabelFormatModifiedEvent);
}

//---------------------------------------------------------------------------
std::string vtkMRMLMarkupsNode::ReplaceListNameInMarkupLabelFormat()
{
  std::string newFormatString = this->MarkupLabelFormat;
  size_t replacePos = newFormatString.find("%N");
  if (replacePos != std::string::npos)
    {
    // replace the special character with the list name, or an empty string if
    // no list name is set
    std::string name;
    if (this->GetName() != nullptr)
      {
      name = std::string(this->GetName());
      }
    newFormatString.replace(replacePos, 2, name);
    }
  return newFormatString;
}
