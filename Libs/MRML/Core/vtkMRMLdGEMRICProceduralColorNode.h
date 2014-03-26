/*=auto=========================================================================

  Portions (c) Copyright 2006 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLdGEMRICProceduralColorNode.h,v $
  Date:      $Date: 2006/03/19 17:12:28 $
  Version:   $Revision: 1.0 $

=========================================================================auto=*/

#ifndef __vtkMRMLdGEMRICProceduralColorNode_h
#define __vtkMRMLdGEMRICProceduralColorNode_h

#include "vtkMRMLProceduralColorNode.h"

/// \brief MRML node to represent procedurally defined color information.
///
/// Procedural nodes define methods that are used to map colours to scalar
/// values. Usually they will incorporate a custom subclass of a
/// vtkLookupTable, or a vtkColorTransferFunction.
class VTK_MRML_EXPORT vtkMRMLdGEMRICProceduralColorNode : public vtkMRMLProceduralColorNode
{
public:
  static vtkMRMLdGEMRICProceduralColorNode *New();
  vtkTypeMacro(vtkMRMLdGEMRICProceduralColorNode,vtkMRMLProceduralColorNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  //--------------------------------------------------------------------------
  /// MRMLNode methods
  //--------------------------------------------------------------------------

  virtual vtkMRMLNode* CreateNodeInstance();

  ///
  /// Set node attributes
  virtual void ReadXMLAttributes( const char** atts);

  ///
  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent);

  ///
  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node);

  ///
  /// Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() {return "dGEMRICProceduralColor";};

  ///
  ///
  virtual void UpdateScene(vtkMRMLScene *scene);

  ///
  /// Get/Set for Type. In SetType, set up the custom colour options for this
  /// set of colours
  virtual void SetType(int type);

  void ProcessMRMLEvents ( vtkObject *caller, unsigned long event, void *callData );

  /// The list of valid procedural types
  //enum
  //{
  ///
  //};

  /// DisplayModifiedEvent is generated when display node parameters is changed
  enum
    {
      DisplayModifiedEvent = 20000
    };

  ///
  /// Create default storage node or NULL if does not have one
  virtual vtkMRMLStorageNode* CreateDefaultStorageNode()
    {
    return Superclass::CreateDefaultStorageNode();
    };

  /// The list of valid types
  /// dGEMRIC-15T to display 1.5T dGEMRIC scans
  /// dGEMRIC-3T to display 3T dGEMRIC scans
  enum
  {
    dGEMRIC15T = 0,
    dGEMRIC3T = 1
  };

  /// Return the lowest and the highest integers, for use in looping
  int GetFirstType() { return this->dGEMRIC15T; };
  int GetLastType() { return this->dGEMRIC3T; };

  const char *GetTypeAsString();
  void SetTypeTo15T();
  void SetTypeTo3T();

protected:
  vtkMRMLdGEMRICProceduralColorNode();
  ~vtkMRMLdGEMRICProceduralColorNode();
  vtkMRMLdGEMRICProceduralColorNode(const vtkMRMLdGEMRICProceduralColorNode&);
  void operator=(const vtkMRMLdGEMRICProceduralColorNode&);
};

#endif
