/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLVolumeNode.h,v $
  Date:      $Date: 2006/03/19 17:12:29 $
  Version:   $Revision: 1.13 $

=========================================================================auto=*/

#ifndef __vtkMRMLScalarVolumeNode_h
#define __vtkMRMLScalarVolumeNode_h

// MRML includes
#include "vtkMRMLVolumeNode.h"
class vtkMRMLScalarVolumeDisplayNode;
class vtkCodedEntry;

/// \brief MRML node for representing a volume (image stack).
///
/// Volume nodes describe data sets that can be thought of as stacks of 2D
/// images that form a 3D volume. Volume nodes contain only the image data,
/// where it is store on disk and how to read the files is controlled by
/// the volume storage node, how to render the data (window and level) is
/// controlled by the volume display nodes. Image information is extracted
/// from the image headers (if they exist) at the time the MRML file is
/// generated.
/// Consequently, MRML files isolate MRML browsers from understanding how
/// to read the myriad of file formats for medical data.
class VTK_MRML_EXPORT vtkMRMLScalarVolumeNode : public vtkMRMLVolumeNode
{
  public:
  static vtkMRMLScalarVolumeNode *New();
  vtkTypeMacro(vtkMRMLScalarVolumeNode,vtkMRMLVolumeNode);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  virtual vtkMRMLNode* CreateNodeInstance() VTK_OVERRIDE;

  ///
  /// Set node attributes
  virtual void ReadXMLAttributes( const char** atts) VTK_OVERRIDE;

  ///
  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent) VTK_OVERRIDE;

  ///
  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node) VTK_OVERRIDE;

  ///
  /// Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() VTK_OVERRIDE {return "Volume";}

  ///
  /// Make a 'None' volume node with blank image data
  static void CreateNoneNode(vtkMRMLScene *scene);

  ///
  /// Associated display MRML node
  virtual vtkMRMLScalarVolumeDisplayNode* GetScalarVolumeDisplayNode();

  ///
  /// Create default storage node or NULL if does not have one
  virtual vtkMRMLStorageNode* CreateDefaultStorageNode() VTK_OVERRIDE;

  ///
  /// Create and observe default display node
  virtual void CreateDefaultDisplayNodes() VTK_OVERRIDE;

  /// Measured quantity of voxel values, specified as a standard coded entry.
  /// For example: (DCM, 112031, "Attenuation Coefficient")
  void SetVoxelValueQuantity(vtkCodedEntry*);
  vtkGetObjectMacro(VoxelValueQuantity, vtkCodedEntry);

  /// Measurement unit of voxel value quantity, specified as a standard coded entry.
  /// For example: (UCUM, [hnsf'U], "Hounsfield unit")
  /// It stores a single unit. Plural (units) name is chosen to be consistent
  /// with nomenclature in the DICOM standard.
  void SetVoxelValueUnits(vtkCodedEntry*);
  vtkGetObjectMacro(VoxelValueUnits, vtkCodedEntry);

protected:
  vtkMRMLScalarVolumeNode();
  ~vtkMRMLScalarVolumeNode();
  vtkMRMLScalarVolumeNode(const vtkMRMLScalarVolumeNode&);
  void operator=(const vtkMRMLScalarVolumeNode&);

  vtkCodedEntry* VoxelValueQuantity;
  vtkCodedEntry* VoxelValueUnits;
};

#endif
