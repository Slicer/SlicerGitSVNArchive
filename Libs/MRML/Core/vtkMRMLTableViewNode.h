/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

=========================================================================auto=*/

#ifndef __vtkMRMLTableViewNode_h
#define __vtkMRMLTableViewNode_h

#include "vtkMRMLAbstractViewNode.h"

class vtkMRMLTableNode;

/// \brief MRML node to represent table view parameters.
///
/// TableViewNodes are associated one to one with a TableWidget.
class VTK_MRML_EXPORT vtkMRMLTableViewNode : public vtkMRMLAbstractViewNode
{
public:
  static vtkMRMLTableViewNode *New();
  vtkTypeMacro(vtkMRMLTableViewNode, vtkMRMLAbstractViewNode);
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
  virtual const char* GetNodeTagName();

  ///
  /// Set the Table node id displayed in this Table View
  void SetTableNodeID(const char *);

  ///
  /// Get the Table node id displayed in this Table View
  const char * GetTableNodeID();

  ///
  /// Get the Table node displayed in this Table View
  vtkMRMLTableNode* GetTableNode();

  ///
  /// configures the behavior of PropagateTableSelection():
  /// if set to false then this view will not be affected by
  /// PropagateTableSelection. Default value is true
  vtkSetMacro (DoPropagateTableSelection, bool );
  vtkGetMacro (DoPropagateTableSelection, bool );

  virtual const char* GetTableNodeReferenceRole();

  /*
  ///
  /// Events
  enum
  {
    TableNodeChangedEvent = 16001
  };
  */

protected:
  vtkMRMLTableViewNode();
  ~vtkMRMLTableViewNode();
  vtkMRMLTableViewNode(const vtkMRMLTableViewNode&);
  void operator=(const vtkMRMLTableViewNode&);

    ///
  /// Called when a node reference ID is added (list size increased).
  virtual void OnNodeReferenceAdded(vtkMRMLNodeReference *reference);

  ///
  /// Called when a node reference ID is modified.
  virtual void OnNodeReferenceModified(vtkMRMLNodeReference *reference);

  ///
  /// Called after a node reference ID is removed (list size decreased).
  virtual void OnNodeReferenceRemoved(vtkMRMLNodeReference *reference);

  virtual const char* GetTableNodeReferenceMRMLAttributeName();

  static const char* TableNodeReferenceRole;
  static const char* TableNodeReferenceMRMLAttributeName;

  bool DoPropagateTableSelection;
};

#endif
