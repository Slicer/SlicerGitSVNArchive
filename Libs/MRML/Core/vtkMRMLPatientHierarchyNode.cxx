// MRML includes
#include "vtkMRMLPatientHierarchyNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkCollection.h>
#include <vtkObjectFactory.h>

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
}

//----------------------------------------------------------------------------
const char* vtkMRMLPatientHierarchyNode::GetNodeTagName()
{
  return "PatientHierarchy";
}
//----------------------------------------------------------------------------
void vtkMRMLPatientHierarchyNode::ReadXMLAttributes( const char** atts)
{
  Superclass::ReadXMLAttributes(atts);
}

//----------------------------------------------------------------------------
void vtkMRMLPatientHierarchyNode::WriteXML(ostream& of, int indent)
{
  Superclass::WriteXML(of,indent);
}

//---------------------------------------------------------------------------
vtkMRMLPatientHierarchyNode*
vtkMRMLPatientHierarchyNode::GetPatientHierarchyNodeByInstanceUid(
  vtkMRMLScene *scene, const char* instanceUid, const char* dicomDatabaseFileName )
{
  if (!scene)
    {
    return NULL;
    }

  std::vector<vtkMRMLNode *> patientHierarchyNodes;
  unsigned int numberOfNodes = scene->GetNodesByClass("vtkMRMLPatientHierarchyNode", patientHierarchyNodes);

  for (unsigned int i=0; i<numberOfNodes; i++)
    {
    vtkMRMLPatientHierarchyNode *node = vtkMRMLPatientHierarchyNode::SafeDownCast(patientHierarchyNodes[i]);
    if (node && strcmp(dicomDatabaseFileName, node->GetDicomDatabaseFileName())
             && strcmp(instanceUid, node->GetInstanceUid()) )
      {
        return node;
      }
    }

  return NULL;
}
