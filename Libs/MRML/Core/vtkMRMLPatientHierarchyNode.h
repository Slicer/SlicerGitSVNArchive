// .NAME vtkMRMLPatientHierarchyNode - MRML node to represent hierarchy of Contours.
// .SECTION Description
// n/a
//

#ifndef __vtkMRMLPatientHierarchyNode_h
#define __vtkMRMLPatientHierarchyNode_h

#include "vtkMRMLDisplayableHierarchyNode.h"

/// \brief MRML node to represent the patient hierarchy of DICOM objects
///        (Patient / Study / Series)
class VTK_MRML_EXPORT vtkMRMLPatientHierarchyNode : public vtkMRMLDisplayableHierarchyNode
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
  vtkTypeMacro(vtkMRMLPatientHierarchyNode,vtkMRMLDisplayableHierarchyNode);
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
