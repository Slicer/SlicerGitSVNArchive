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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// MRML includes
#include "vtkMRMLPatientHierarchyNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLPatientHierarchyNode);

//----------------------------------------------------------------------------
vtkMRMLPatientHierarchyNode::vtkMRMLPatientHierarchyNode()
{
  this->InstanceUid = NULL;
  this->DicomDatabaseFileName = NULL;
  this->Level = Unset;
}

//----------------------------------------------------------------------------
vtkMRMLPatientHierarchyNode::~vtkMRMLPatientHierarchyNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLPatientHierarchyNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << " InstanceUid=\""
    << (this->InstanceUid ? this->InstanceUid : "NULL" ) << "\n";
  os << indent << " DicomDatabaseFileName=\""
    << (this->DicomDatabaseFileName ? this->DicomDatabaseFileName : "NULL" ) << "\n";
  os << indent << " Level=\"" << (int)(this->Level) << "\"";
}

//----------------------------------------------------------------------------
const char* vtkMRMLPatientHierarchyNode::GetNodeTagName()
{
  return "PatientHierarchy";
}
//----------------------------------------------------------------------------
void vtkMRMLPatientHierarchyNode::ReadXMLAttributes( const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "InstanceUid")) 
      {
      this->SetInstanceUid(attValue);
      }
    else if (!strcmp(attName, "DicomDatabaseFileName"))
      {
      this->SetDicomDatabaseFileName(attValue);
      }
    else if (!strcmp(attName, "Level")) 
      {
      std::stringstream ss;
      ss << attValue;
      int intAttValue;
      ss >> intAttValue;
      this->Level = (PatientHierarchyLevel)intAttValue;
      }
    }

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLPatientHierarchyNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of,nIndent);

  vtkIndent indent(nIndent);

  of << indent << " InstanceUid=\""
    << (this->InstanceUid ? this->InstanceUid : "NULL" ) << "\"";

  of << indent << " DicomDatabaseFileName=\""
    << (this->DicomDatabaseFileName ? this->DicomDatabaseFileName : "NULL" ) << "\"";

  of << indent << " Level=\"" << (int)(this->Level) << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLPatientHierarchyNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);
  vtkMRMLPatientHierarchyNode *node = (vtkMRMLPatientHierarchyNode*) anode;

  this->SetInstanceUid(node->InstanceUid);
  this->SetDicomDatabaseFileName(node->DicomDatabaseFileName);
  this->SetLevel(node->Level);

  this->EndModify(disabledModify);
}

//---------------------------------------------------------------------------
vtkMRMLPatientHierarchyNode*
vtkMRMLPatientHierarchyNode::GetPatientHierarchyNodeByInstanceUid(
  vtkMRMLScene *scene, const char* dicomDatabaseFileName, const char* instanceUid )
{
  if (!scene || !dicomDatabaseFileName || !instanceUid)
    {
    return NULL;
    }

  std::vector<vtkMRMLNode *> patientHierarchyNodes;
  unsigned int numberOfNodes = scene->GetNodesByClass("vtkMRMLPatientHierarchyNode", patientHierarchyNodes);

  for (unsigned int i=0; i<numberOfNodes; i++)
    {
    vtkMRMLPatientHierarchyNode *node = vtkMRMLPatientHierarchyNode::SafeDownCast(patientHierarchyNodes[i]);
    if (node && node->GetInstanceUid()
             && !strcmp(dicomDatabaseFileName, node->GetDicomDatabaseFileName())
             && !strcmp(instanceUid, node->GetInstanceUid()) )
      {
      return node;
      }
    }

  return NULL;
}

//---------------------------------------------------------------------------
void vtkMRMLPatientHierarchyNode::InsertSeriesInHierarchy(
  vtkMRMLScene *scene, const char* dicomDatabaseFileName, 
  const char* patientId, const char* studyInstanceUid, const char* seriesInstanceUid )
{
  if ( !scene || !dicomDatabaseFileName
    || !patientId || !studyInstanceUid || !seriesInstanceUid )
    {
    return;
    }

  vtkMRMLPatientHierarchyNode* patientNode = NULL;
  vtkMRMLPatientHierarchyNode* studyNode = NULL;
  vtkMRMLPatientHierarchyNode* seriesNode = NULL;

  std::vector<vtkMRMLNode *> patientHierarchyNodes;
  unsigned int numberOfNodes = scene->GetNodesByClass("vtkMRMLPatientHierarchyNode", patientHierarchyNodes);

  // Find referenced nodes
  for (unsigned int i=0; i<numberOfNodes; i++)
    {
    vtkMRMLPatientHierarchyNode *node = vtkMRMLPatientHierarchyNode::SafeDownCast(patientHierarchyNodes[i]);
    if ( node && node->GetInstanceUid() && !strcmp(dicomDatabaseFileName, node->GetDicomDatabaseFileName()) )
      {
      if (!strcmp(patientId, node->GetInstanceUid()))
        {
        patientNode = node;
        }
      else if (!strcmp(studyInstanceUid, node->GetInstanceUid()))
        {
        studyNode = node;
        }
      else if (!strcmp(seriesInstanceUid, node->GetInstanceUid()))
        {
        seriesNode = node;
        }
      }
    }

  if (!seriesNode)
    {
    vtkErrorWithObjectMacro(scene,
      "vtkMRMLPatientHierarchyNode::InsertSeriesInHierarchy: Patient hierarchy node with ID="
      << patientId << " cannot be found!");
    return;
    }

  // Create patient and study nodes if they do not exist yet
  if (!patientNode)
    {
    patientNode = vtkMRMLPatientHierarchyNode::New();
    patientNode->AllowMultipleChildrenOn();
    patientNode->HideFromEditorsOff();
    patientNode->SetInstanceUid(patientId);
    patientNode->SetDicomDatabaseFileName(dicomDatabaseFileName);
    patientNode->SetLevel(Patient);
    scene->AddNode(patientNode);
    }

  if (!studyNode)
    {
    studyNode = vtkMRMLPatientHierarchyNode::New();
    studyNode->AllowMultipleChildrenOn();
    studyNode->HideFromEditorsOff();
    studyNode->SetInstanceUid(studyInstanceUid);
    studyNode->SetDicomDatabaseFileName(dicomDatabaseFileName);
    studyNode->SetLevel(Study);
    studyNode->SetParentNodeID(patientNode->GetID());
    scene->AddNode(studyNode);
    }

  seriesNode->SetParentNodeID(studyNode->GetID());
}

//---------------------------------------------------------------------------
bool vtkMRMLPatientHierarchyNode::AreNodesInSameBranch( vtkMRMLScene *scene,
                                                       const char* nodeId1, const char* nodeId2,
                                                       PatientHierarchyLevel lowestCommonLevel )
{
  if ( !scene || !nodeId1 || !nodeId2 )
    {
    return false;
    }

  if (lowestCommonLevel < Patient)
    {
    vtkErrorWithObjectMacro(scene,
      "vtkMRMLPatientHierarchyNode::AreNodesInSameBranch: Invalid lowest common level!");
    return false;
    }

  if (lowestCommonLevel == Subseries)
    {
    vtkErrorWithObjectMacro(scene,
      "vtkMRMLPatientHierarchyNode::AreNodesInSameBranch: Lowest common level must be Series or above!");
    return false;
    }

  // Get input nodes
  vtkMRMLPatientHierarchyNode* node1 = vtkMRMLPatientHierarchyNode::SafeDownCast(scene->GetNodeByID(nodeId1));
  vtkMRMLPatientHierarchyNode* node2 = vtkMRMLPatientHierarchyNode::SafeDownCast(scene->GetNodeByID(nodeId2));

  // If not hierarchy nodes, check if they have an associated patient hierarchy node
  if (!node1)
    {
    node1 = vtkMRMLPatientHierarchyNode::SafeDownCast(
      vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(scene, nodeId1) );
    }
  if (!node2)
    {
    node2 = vtkMRMLPatientHierarchyNode::SafeDownCast(
      vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(scene, nodeId2) );
    }

  // Check if valid nodes are found
  if (!node1 || !node2)
    {
    return false;
    }

  // If either input node is already higher level than the lowest common level then return with false
  if (node1->GetLevel() < lowestCommonLevel || node2->GetLevel() < lowestCommonLevel)
    {
    return false;
    }

  // Walk the hierarchy up until we reach the lowest common level
  while (node1->GetLevel() > lowestCommonLevel)
    {
    node1 = vtkMRMLPatientHierarchyNode::SafeDownCast(node1->GetParentNode());
    }
  while (node2->GetLevel() > lowestCommonLevel)
    {
    node2 = vtkMRMLPatientHierarchyNode::SafeDownCast(node2->GetParentNode());
    }

  // Sanity check
  if (!node1 || !node2)
    {
    vtkErrorWithObjectMacro(scene,
      "vtkMRMLPatientHierarchyNode::AreNodesInSameBranch: Invalid patient hierarchy tree - level inconsistency!");
    return false;
    }
  if ( !node1->GetInstanceUid() || !node1->GetDicomDatabaseFileName()
    || !node2->GetInstanceUid() || !node2->GetDicomDatabaseFileName() )
    {
    vtkErrorWithObjectMacro(scene,
      "vtkMRMLPatientHierarchyNode::AreNodesInSameBranch: Found ancestor node contains empty Instance UID or DICOM database file name!");
    return false;
    }

  // Check if the found nodes match
  // (handling the case when two patient hierarchy nodes point to the same DICOM object)
  if ( !strcmp(node1->GetInstanceUid(), node2->GetInstanceUid())
    && !strcmp(node1->GetDicomDatabaseFileName(), node2->GetDicomDatabaseFileName()) )
    {
    return true;
    }

  return false;
}

//---------------------------------------------------------------------------
bool vtkMRMLPatientHierarchyNode::AreNodesInSameBranch( vtkMRMLScene *scene,
                                                       const char* nodeId1, const char* nodeId2,
                                                       int lowestCommonLevel )
{
  return vtkMRMLPatientHierarchyNode::AreNodesInSameBranch(
                                      scene, nodeId1, nodeId2,
                                      (PatientHierarchyLevel)lowestCommonLevel );
}
