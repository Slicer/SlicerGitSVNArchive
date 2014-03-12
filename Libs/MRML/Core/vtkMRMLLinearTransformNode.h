/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLLinearTransformNode.h,v $
  Date:      $Date: 2006/03/19 17:12:29 $
  Version:   $Revision: 1.13 $

=========================================================================auto=*/

#ifndef __vtkMRMLLinearTransformNode_h
#define __vtkMRMLLinearTransformNode_h

#include "vtkMRMLTransformNode.h"

class vtkMRMLStorageNode;

/// \brief MRML node for representing a linear transformation to the parent
/// node.
///
/// MRML node for representing
/// a linear transformation to the parent node in the form vtkMatrix4x4
/// MatrixTransformToParent.
class VTK_MRML_EXPORT vtkMRMLLinearTransformNode : public vtkMRMLTransformNode
{
  public:
  static vtkMRMLLinearTransformNode *New();
  vtkTypeMacro(vtkMRMLLinearTransformNode,vtkMRMLTransformNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkMRMLNode* CreateNodeInstance();

  /// 
  /// Read node attributes from XML file
  virtual void ReadXMLAttributes( const char** atts);

  /// 
  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent);

  /// 
  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node);

  /// 
  /// Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() {return "LinearTransform";};

  /// 
  /// 1 if transfrom is linear, 0 otherwise
  virtual int IsLinear() {return 1;};

  ///
  /// Return the vtkMatrix4x4 transform of this node to parent node
  /// The output is a cached output of the transform and therefore should
  /// not be changed (to change the transform call SetMatrixTransformToParent()).
  virtual vtkMatrix4x4* GetMatrixTransformToParent();

  ///
  /// Return the vtkMatrix4x4 transform of this node from parent node
  /// The output is a cached output of the transform and therefore should
  /// not be changed (to change the transform call SetMatrixTransformFromParent()).
  virtual vtkMatrix4x4* GetMatrixTransformFromParent();

  ///
  /// Set a new matrix transform of this node to parent node.
  /// Invokes a TransformModified event (does not invoke Modified).
  void SetMatrixTransformToParent(vtkMatrix4x4 *matrix);

  ///
  /// Set a new matrix transform of this node from parent node.
  /// Invokes a TransformModified event (does not invoke Modified).
  void SetMatrixTransformFromParent(vtkMatrix4x4 *matrix);

  /// 
  /// Get concatenated transforms to the top
  virtual int  GetMatrixTransformToWorld(vtkMatrix4x4* transformToWorld);
  
  /// 
  /// Get concatenated transforms  bwetween nodes
  virtual int  GetMatrixTransformToNode(vtkMRMLTransformNode* node, 
                                        vtkMatrix4x4* transformToNode);

  virtual bool CanApplyNonLinearTransforms()const;
  virtual void ApplyTransformMatrix(vtkMatrix4x4* transformMatrix);
 
  /// 
  /// Create default storage node or NULL if does not have one
  virtual vtkMRMLStorageNode* CreateDefaultStorageNode()
    {
    return Superclass::CreateDefaultStorageNode();
    };

protected:
  vtkMRMLLinearTransformNode();
  ~vtkMRMLLinearTransformNode();
  vtkMRMLLinearTransformNode(const vtkMRMLLinearTransformNode&);
  void operator=(const vtkMRMLLinearTransformNode&);
};

#endif
