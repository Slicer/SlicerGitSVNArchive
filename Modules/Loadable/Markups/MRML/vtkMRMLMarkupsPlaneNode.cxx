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

  This file was originally developed by Kyle Sunderland, PerkLab, Queen's University
  and was supported through CANARIE's Research Software Program, Cancer
  Care Ontario, OpenAnatomy, and Brigham and Women’s Hospital through NIH grant R01MH112748.

==============================================================================*/

// MRML includes
#include "vtkMRMLMarkupsDisplayNode.h"
#include "vtkMRMLMarkupsPlaneNode.h"
#include "vtkMRMLMarkupsFiducialStorageNode.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLTransformNode.h"

// VTK includes
#include <vtkGeneralTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkTransform.h>
#include <vtkTriangle.h>

// STD includes
#include <sstream>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLMarkupsPlaneNode);

//----------------------------------------------------------------------------
vtkMRMLMarkupsPlaneNode::vtkMRMLMarkupsPlaneNode()
{
  this->MaximumNumberOfControlPoints = 3;
  this->RequiredNumberOfControlPoints = 3;
  this->SizeMode = SizeModeAuto;
  this->AutoSizeScaling = 1.0;
  this->Offset = vtkMatrix4x4::New();
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsPlaneNode::~vtkMRMLMarkupsPlaneNode()
{
  this->SetOffset(nullptr);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of,nIndent);
  vtkMRMLWriteXMLBeginMacro(of);
  vtkMRMLWriteXMLEnumMacro(sizeMode, SizeMode);
  of << " offset=\"";
  for (int row = 0; row < 4; row++)
    {
    for (int col = 0; col < 4; col++)
      {
      of << this->Offset->GetElement(row, col);
      if (!(row == 3 && col == 3))
        {
        of << " ";
        }
      }
    if (row != 3)
      {
      of << " ";
      }
    }
  of << "\"";
  vtkMRMLWriteXMLEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::ReadXMLAttributes(const char** atts)
{
  MRMLNodeModifyBlocker blocker(this);
  Superclass::ReadXMLAttributes(atts);
  vtkMRMLReadXMLBeginMacro(atts);
  vtkMRMLReadXMLEnumMacro(sizeMode, SizeMode);
  if (strcmp(xmlReadAttName, "offset") == 0)
    {
    vtkNew<vtkMatrix4x4> matrix;
    std::stringstream ss;
    double val;
    ss << xmlReadAttValue;
    for (int row=0; row<4; row++)
      {
      for (int col=0; col<4; col++)
        {
        ss >> val;
        matrix->SetElement(row, col, val);
        }
      }
    this->SetOffset(matrix);
    }
  vtkMRMLReadXMLEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::Copy(vtkMRMLNode *anode)
{
  MRMLNodeModifyBlocker blocker(this);
  Superclass::Copy(anode);
  vtkMRMLCopyBeginMacro(anode);
  vtkMRMLCopyEnumMacro(SizeMode);
  if (this->SafeDownCast(copySourceNode) &&
    this->SafeDownCast(copySourceNode)->GetOffset())
    {
    vtkNew<vtkMatrix4x4> offset;
    offset->DeepCopy(this->SafeDownCast(copySourceNode)->GetOffset());
    this->SetOffset(offset);
    }
  vtkMRMLCopyEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
  vtkMRMLPrintBeginMacro(os, indent);
  vtkMRMLPrintEnumMacro(SizeMode);
  vtkMRMLPrintEndMacro();
}

//----------------------------------------------------------------------------
const char* vtkMRMLMarkupsPlaneNode::GetSizeModeAsString(int sizeMode)
{
  switch (sizeMode)
    {
  case SizeModeAuto:
    return "auto";
  case SizeModeAbsolute:
    return "absolute";
  default:
    break;
    }
  return "unknown";
}

//----------------------------------------------------------------------------
int vtkMRMLMarkupsPlaneNode::GetSizeModeFromString(const char* sizeMode)
{
  for (int i = 0; i < SizeModeLast; ++i)
    {
    if (strcmp(this->GetSizeModeAsString(i), sizeMode) == 0)
      {
      return i;
      }
    }
  return -1;
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::GetNormal(double normal[3])
{
  if (!normal)
    {
    return;
    }

  if (this->GetNumberOfControlPoints() < 3)
    {
    return;
    }

  double x[3] = { 0 };
  double y[3] = { 0 };
  this->GetPlaneAxes(x, y, normal);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::GetNormalWorld(double normal[3])
{
  if (!normal)
    {
    return;
    }

  if (this->GetNumberOfControlPoints() < 3)
    {
    return;
    }

  this->GetNormal(normal);

  vtkMRMLTransformNode* transformNode = this->GetParentTransformNode();
  if (!transformNode)
    {
    return;
    }

  // Get transform
  vtkNew<vtkGeneralTransform> transformToWorld;
  transformNode->GetTransformToWorld(transformToWorld.GetPointer());

  // Convert coordinates
  double origin[3] = { 0 };
  this->GetOrigin(origin);
  transformToWorld->TransformVectorAtPoint(origin, normal, normal);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetNormal(const double normal[3])
{
  if (!normal)
    {
    return;
    }

  MRMLNodeModifyBlocker blocker(this);
  this->CreateValidPlane();

  double newNormal[3] = { normal[0], normal[1], normal[2] };
  vtkMath::Normalize(newNormal);

  double currentNormal[3] = { 0 };
  this->GetNormal(currentNormal);

  double epsilon = 0.0001;
  if (vtkMath::Dot(newNormal, currentNormal) >= 1.0 - epsilon)
    {
    // Normal vectors are equivalent, no change required.
    return;
    }

  double angleRadians = vtkMath::AngleBetweenVectors(newNormal, currentNormal);
  double rotationAxis[3] = { 0 };
  vtkMath::Cross(currentNormal, newNormal, rotationAxis);
  if (vtkMath::Norm(rotationAxis) < epsilon)
    {
    // New + old normals are facing opposite directions.
    // Find a perpendicular axis to flip around.
    vtkMath::Perpendiculars(currentNormal, rotationAxis, nullptr, 0);
    }

  vtkNew<vtkTransform> transform;
  double point0[3] = { 0 };
  this->GetNthControlPointPosition(0, point0);
  transform->Translate(point0);
  transform->RotateWXYZ(vtkMath::DegreesFromRadians(angleRadians), rotationAxis);
  for (int i = 0; i < 3; ++i)
    {
    point0[i] = -1 * point0[i];
    }
  transform->Translate(point0);

  double point1[3] = { 0 };
  this->GetNthControlPointPosition(1, point1);
  transform->TransformPoint(point1, point1);
  this->SetNthControlPointPosition(1, point1[0], point1[1], point1[2]);

  double point2[3] = { 0 };
  this->GetNthControlPointPosition(2, point2);
  transform->TransformPoint(point2, point2);
  this->SetNthControlPointPosition(2, point2[0], point2[1], point2[2]);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetNormalWorld(const double inNormal[3])
{
  double normal[3] = { inNormal[0], inNormal[1], inNormal[2] };

  vtkMRMLTransformNode* transformNode = this->GetParentTransformNode();
  if (transformNode)
    {
    // Get transform
    vtkNew<vtkGeneralTransform> transformToWorld;
    transformNode->GetTransformFromWorld(transformToWorld.GetPointer());

    // Convert coordinates
    double origin[3] = { 0 };
    this->GetOriginWorld(origin);
    transformToWorld->TransformVectorAtPoint(origin, normal, normal);
    }
  this->SetNormal(normal);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::GetOrigin(double outOrigin[3])
{
  if (!outOrigin)
    {
    return;
    }

  if (this->GetNumberOfControlPoints() < 1)
    {
    return;
    }

  outOrigin[0] = 0;
  outOrigin[1] = 0;
  outOrigin[2] = 0;

  vtkNew<vtkTransform> localToOffsetTransform;
  localToOffsetTransform->SetMatrix(this->Offset);
  localToOffsetTransform->TransformPoint(outOrigin, outOrigin);

  double origin[3] = { 0 };
  this->GetNthControlPointPosition(0, origin);
  vtkMath::Add(origin, outOrigin, outOrigin);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::GetOriginWorld(double originWorld[3])
{
  if (!originWorld)
    {
    return;
    }

  if (this->GetNumberOfControlPoints() < 1)
    {
    return;
    }

  this->GetOrigin(originWorld);
  this->TransformPointToWorld(originWorld, originWorld);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetOrigin(const double origin[3])
{
  if (!origin)
    {
    return;
    }

  MRMLNodeModifyBlocker blocker(this);
  if (this->GetNumberOfControlPoints() < 1)
    {
    this->AddNControlPoints(1);
    }

  double newControlPointPos[3] = { 0, 0, 0 };
  vtkNew<vtkTransform> offsetToLocalTransform;
  offsetToLocalTransform->SetMatrix(this->Offset);
  offsetToLocalTransform->Inverse();
  offsetToLocalTransform->TransformPoint(origin, newControlPointPos);

  double previousControlPointPos[3] = { 0.0 };
  this->GetNthControlPointPosition(0, previousControlPointPos);

  this->SetNthControlPointPosition(0,
    newControlPointPos[0], newControlPointPos[1], newControlPointPos[2]);

  if (this->GetNumberOfControlPoints() < 3)
    {
    return;
    }

  double displacementVector[3] = { 0.0 };
  vtkMath::Subtract(newControlPointPos, previousControlPointPos, displacementVector);

  for (int i = 1; i < this->GetNumberOfControlPoints(); ++i)
    {
    double currentControlPointPos[3] = { 0.0 };
    this->GetNthControlPointPosition(i, currentControlPointPos);
    vtkMath::Add(currentControlPointPos, displacementVector, currentControlPointPos);
    this->SetNthControlPointPosition(i,
      currentControlPointPos[0], currentControlPointPos[1], currentControlPointPos[2]);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetOriginWorld(const double originWorld[3])
{
  double originLocal[3] = { 0 };
  this->TransformPointFromWorld(originWorld, originLocal);
  this->SetOrigin(originLocal);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::GetPlaneAxes(double x[3], double y[3], double z[3])
{
  if (!x || !y || !z)
    {
    return;
    }

  if (this->GetNumberOfControlPoints() < 3)
    {
    return;
    }

  double point0[3] = { 0 };
  this->GetNthControlPointPosition(0, point0);
  double point1[3] = { 0 };
  this->GetNthControlPointPosition(1, point1);
  double point2[3] = { 0 };
  this->GetNthControlPointPosition(2, point2);

  vtkMath::Subtract(point1, point0, x);
  vtkMath::Normalize(x);

  double tempVector[3] = { 0 };
  vtkMath::Subtract(point2, point0, tempVector);
  vtkMath::Cross(x, tempVector, z);
  vtkMath::Normalize(z);

  vtkMath::Cross(z, x, y);
  vtkMath::Normalize(y);

  vtkNew<vtkTransform> localToOffsetTransform;
  localToOffsetTransform->SetMatrix(this->Offset);
  localToOffsetTransform->TransformVector(x, x);
  localToOffsetTransform->TransformVector(y, y);
  localToOffsetTransform->TransformVector(z, z);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::GetPlaneAxesWorld(double x[3], double y[3], double z[3])
{
  this->GetPlaneAxes(x, y, z);

  vtkMRMLTransformNode* transformNode = this->GetParentTransformNode();
  if (!transformNode)
    {
    return;
    }

  // Get transform
  vtkNew<vtkGeneralTransform> transformToWorld;
  transformNode->GetTransformToWorld(transformToWorld.GetPointer());

  // Convert coordinates
  double origin[3] = { 0 };
  this->GetOrigin(origin);
  transformToWorld->TransformVectorAtPoint(origin, x, x);
  transformToWorld->TransformVectorAtPoint(origin, y, y);
  transformToWorld->TransformVectorAtPoint(origin, z, z);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetPlaneAxes(const double inX[3], const double inY[3], const double inZ[3])
{
  if (!inX || !inY || !inZ)
    {
    return;
    }

  double x[3], y[3], z[3] = { 0 };
  vtkNew<vtkTransform> offsetToLocalTransform;
  offsetToLocalTransform->SetMatrix(this->Offset);
  offsetToLocalTransform->Inverse();
  offsetToLocalTransform->TransformPoint(inX, x);
  offsetToLocalTransform->TransformPoint(inY, y);
  offsetToLocalTransform->TransformPoint(inZ, z);

  double epsilon = 0.0001;

  double tempX[3], tempY[3], tempZ[3] = { 0 };
  vtkMath::Cross(y, z, tempX);
  vtkMath::Cross(z, x, tempY);
  vtkMath::Cross(x, y, tempZ);
  if (vtkMath::Dot(tempX, x) <= 1 - epsilon ||
      vtkMath::Dot(tempY, y) <= 1 - epsilon ||
      vtkMath::Dot(tempZ, z) <= 1 - epsilon)
    {
    vtkErrorMacro("Invalid direction vectors!")
    return;
    }

  if (vtkMath::Dot(x, y) >= epsilon || vtkMath::Dot(y, z) >= epsilon || vtkMath::Dot(z, x) >= epsilon)
    {
    vtkErrorMacro("Invalid vectors.");
    }

  MRMLNodeModifyBlocker blocker(this);
  this->CreateValidPlane();

  double oldX[3], oldY[3], oldZ[3] = { 0 };
  this->GetPlaneAxes(oldX, oldY, oldZ);

  vtkNew<vtkMatrix4x4> oldVectorsToIdentity;
  for (int i = 0; i < 3; ++i)
    {
    oldVectorsToIdentity->SetElement(i, 0, oldX[i]);
    oldVectorsToIdentity->SetElement(i, 1, oldY[i]);
    oldVectorsToIdentity->SetElement(i, 2, oldZ[i]);
    }
  oldVectorsToIdentity->Invert();

  vtkNew<vtkMatrix4x4> identityToNewVectors;
  for (int i = 0; i < 3; ++i)
    {
    identityToNewVectors->SetElement(i, 0, x[i]);
    identityToNewVectors->SetElement(i, 1, y[i]);
    identityToNewVectors->SetElement(i, 2, z[i]);
    }

  double point0[3] = { 0 };
  this->GetNthControlPointPosition(0, point0);

  vtkNew<vtkTransform> transform;
  transform->Translate(point0);
  transform->Concatenate(oldVectorsToIdentity);
  transform->Concatenate(identityToNewVectors);
  for (int i = 0; i < 3; ++i)
    {
    point0[i] = -1 * point0[i];
    }
  transform->Translate(point0);

  for (int i = 0; i < 3; ++i)
    {
    double controlPoint[4] = { 0, 0, 0, 1 };
    this->GetNthControlPointPosition(i, controlPoint);
    transform->MultiplyPoint(controlPoint, controlPoint);
    this->SetNthControlPointPosition(i, controlPoint[0], controlPoint[1], controlPoint[2]);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetPlaneAxesWorld(const double inX[3], const double inY[3], const double inZ[3])
{
  double x[3] = { inX[0], inX[1], inX[2] };
  double y[3] = { inY[0], inY[1], inY[2] };
  double z[3] = { inZ[0], inZ[1], inZ[2] };

  vtkMRMLTransformNode* transformNode = this->GetParentTransformNode();
  if (transformNode)
    {
    // Get transform
    vtkNew<vtkGeneralTransform> transformToWorld;
    transformNode->GetTransformFromWorld(transformToWorld.GetPointer());

    // Convert coordinates
    double origin[3] = { 0 };
    this->GetOriginWorld(origin);
    transformToWorld->TransformVectorAtPoint(origin, x, x);
    transformToWorld->TransformVectorAtPoint(origin, y, y);
    transformToWorld->TransformVectorAtPoint(origin, z, z);
    }
  this->SetPlaneAxes(x, y, z);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::GetSize(double size[3])
{
  if (this->GetNumberOfControlPoints() < 3)
    {
    size[0] = 0.0;
    size[1] = 0.0;
    size[2] = 0.0;
    return;
    }

  // Size mode auto means we need to recalculate the diameter of the plane from the control points.
  if (this->SizeMode == vtkMRMLMarkupsPlaneNode::SizeModeAuto)
    {
    double point0[3] = { 0.0 };
    double point1[3] = { 0.0 };
    double point2[3] = { 0.0 };
    this->GetNthControlPointPosition(0, point0);
    this->GetNthControlPointPosition(1, point1);
    this->GetNthControlPointPosition(2, point2);

    vtkNew<vtkTransform> localToOffsetTransform;
    localToOffsetTransform->SetMatrix(this->Offset);
    localToOffsetTransform->TransformPoint(point0, point0);
    localToOffsetTransform->TransformPoint(point1, point1);
    localToOffsetTransform->TransformPoint(point2, point2);

    double x[3], y[3], z[3] = { 0 };
    this->GetPlaneAxes(x, y, z);

    // Update the plane
    double vector1[3] = { 0 };
    vtkMath::Subtract(point1, point0, vector1);

    double vector2[3] = { 0 };
    vtkMath::Subtract(point2, point0, vector2);

    double point1X = std::abs(vtkMath::Dot(vector1, x));
    double point2X = std::abs(vtkMath::Dot(vector2, x));
    double xMax = std::max({ 0.0, point1X, point2X });

    double point1Y = std::abs(vtkMath::Dot(vector1, y));
    double point2Y = std::abs(vtkMath::Dot(vector2, y));
    double yMax = std::max({ 0.0, point1Y, point2Y });

    this->Size[0] = 2 * xMax * this->AutoSizeScaling;
    this->Size[1] = 2 * yMax * this->AutoSizeScaling;
    this->Size[2] = 0.0;
    }

  for (int i = 0; i < 3; ++i)
    {
    size[i] = this->Size[i];
    }
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::CreateValidPlane()
{
  if (this->GetNumberOfControlPoints() < 3)
    {
    this->AddNControlPoints(3 - this->GetNumberOfControlPoints());
    }

  double point0[3], point1[3], point2[3] = { 0 };
  this->GetNthControlPointPosition(0, point0);
  this->GetNthControlPointPosition(1, point1);
  this->GetNthControlPointPosition(2, point2);

  // Check if existing vectors are unique.
  double vector1[3], vector2[3] = { 0 };
  vtkMath::Subtract(point1, point0, vector1);
  vtkMath::Subtract(point2, point0, vector2);

  bool pointChanged = false;
  double epsilon = 0.0001;
  if (vtkMath::Norm(vector1) <= epsilon)
    {
    // Point1 is at same position as point0.
    // Move point1 away in x axis.
    double xVector[3] = { 1,0,0 };
    vtkMath::Add(point1, xVector, point1);
    pointChanged = true;
    }

  if (vtkMath::Norm(vector2) <= epsilon)
    {
    // Point2 is at same position as point0.
    // Move point2 away in y axis.
    double yVector[3] = { 0,1,0 };
    vtkMath::Add(point2, yVector, point2);
    pointChanged = true;
    }

  vtkMath::Subtract(point1, point0, vector1);
  vtkMath::Subtract(point2, point0, vector2);
  if (vtkMath::Dot(vector1, vector2) >= 1 - epsilon)
    {
    // Point1 and point2 are along the same vector from point0.
    // Find a perpendicular vector and move point2.
    double vector[3] = { 0,0,0 };
    vtkMath::Perpendiculars(vector2, vector, nullptr, 0.0);
    vtkMath::Add(point0, vector, point2);
    }

  if (pointChanged)
    {
    this->SetNthControlPointPosition(1, point1[0], point1[1], point1[2]);
    this->SetNthControlPointPosition(2, point2[0], point2[1], point2[2]);
    }
}