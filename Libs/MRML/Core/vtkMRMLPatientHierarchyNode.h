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

// .NAME vtkMRMLPatientHierarchyNode - MRML node to represent hierarchy of Contours.
// .SECTION Description
// n/a
//

#ifndef __vtkMRMLPatientHierarchyNode_h
#define __vtkMRMLPatientHierarchyNode_h

#include "vtkMRMLHierarchyNode.h"

/// \brief MRML node to represent the patient hierarchy of DICOM objects
///        (Patient / Study / Series)
class VTK_MRML_EXPORT vtkMRMLPatientHierarchyNode : public vtkMRMLHierarchyNode
{
public:
  enum PatientHierarchyLevel
  {
    Unset = -1,
    Patient = 1,
    Study,
    Series,
    Subseries
  };

public:
  static vtkMRMLPatientHierarchyNode *New();
  vtkTypeMacro(vtkMRMLPatientHierarchyNode,vtkMRMLHierarchyNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkMRMLNode* CreateNodeInstance();

  /// Read node attributes from XML file
  virtual void ReadXMLAttributes(const char** atts);

  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent);

  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node);

  /// Get node XML tag name (like Volume, Contour)
  virtual const char* GetNodeTagName();

public:
  vtkSetStringMacro(InstanceUid);
  vtkGetStringMacro(InstanceUid);

  vtkSetStringMacro(DicomDatabaseFileName);
  vtkGetStringMacro(DicomDatabaseFileName);

  PatientHierarchyLevel GetLevel() { return this->Level; };
  void SetLevel(PatientHierarchyLevel level) { this->Level = level; };
  /// Ensure python compatibility
  void SetLevel(unsigned int level) { this->Level = (PatientHierarchyLevel)level; };

public:
  /// Find patient hierarchy node according to an instance UID and database
  static vtkMRMLPatientHierarchyNode* GetPatientHierarchyNodeByInstanceUid(
    vtkMRMLScene *scene, const char* dicomDatabaseFileName, const char* instanceUid );

  /// Place series in patient hierarchy. Create patient and study node if needed
  static void InsertSeriesInHierarchy(
    vtkMRMLScene *scene, const char* dicomDatabaseFileName, 
    const char* patientId, const char* studyInstanceUid, const char* seriesInstanceUid );

  /// Determine if two patient hierarchy nodes are in the same branch (share the same parent)
  /// \param nodeId1 ID of the first node to check. Can be patient hierarchy node or a node
  ///   associated with one
  /// \param nodeId2 ID of the second node to check
  /// \param lowestCommonLevel Lowest level on which they have to share an ancestor
  /// \return True if the two nodes or their associated hierarchy nodes share a parent on the
  ///   specified level, false otherwise
  static bool AreNodesInSameBranch( vtkMRMLScene *scene,
    const char* nodeId1, const char* nodeId2, PatientHierarchyLevel lowestCommonLevel );

  /// Determine if two patient hierarchy nodes are in the same branch. For python compatibility
  static bool AreNodesInSameBranch( vtkMRMLScene *scene,
    const char* nodeId1, const char* nodeId2, int lowestCommonLevel );

protected:
  /// The instance UID of the corresponding DICOM entity in the database
  char* InstanceUid;

  /// Reference to the corresponding DICOM database
  /// The file name is used because the database pointer gets invalid
  /// (only one database object is present at a time in Slicer)
  char* DicomDatabaseFileName;

  /// Level of this hierarchy instance in the patient hierarchy tree
  PatientHierarchyLevel Level;

protected:
  vtkMRMLPatientHierarchyNode();
  ~vtkMRMLPatientHierarchyNode();
  vtkMRMLPatientHierarchyNode(const vtkMRMLPatientHierarchyNode&);
  void operator=(const vtkMRMLPatientHierarchyNode&);

};

#endif
