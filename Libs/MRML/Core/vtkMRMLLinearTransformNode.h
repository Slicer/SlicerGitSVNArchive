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

#define TRANSFORM_NODE_MATRIX_COPY_REQUIRED

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
  /// Get the vtkMatrix4x4 transform of this node to parent node
  /// Returns 0 if the transform is undefined or there is an error.
  virtual int GetMatrixTransformToParent(vtkMatrix4x4* matrix);

  ///
  /// Get the vtkMatrix4x4 transform of this node from parent node
  /// Returns 0 if the transform is undefined or there is an error.
  virtual int GetMatrixTransformFromParent(vtkMatrix4x4* matrix);

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

    ///
  /// Set a new matrix transform of this node to parent node.
  /// Deprecated! Use SetMatrixTransformToParent instead.
  void SetAndObserveMatrixTransformToParent(vtkMatrix4x4 *matrix);

  ///
  /// Set a new matrix transform of this node from parent node.
  /// Deprecated! Use SetMatrixTransformToParent instead.
  void SetAndObserveMatrixTransformFromParent(vtkMatrix4x4 *matrix);

  ///
  /// Set a new matrix transform of this node to parent node.
  /// Deprecated! Use GetMatrixTransformToParent(vtkMatrix4x4*) instead.
  /// The method returns a cached copy of the transform, so modification
  /// of the matrix does not alter the transform node.
  vtkMatrix4x4* GetMatrixTransformToParent();

  ///
  /// Set a new matrix transform of this node from parent node.
  /// Deprecated! Use GetMatrixTransformFromParent(vtkMatrix4x4*) instead.
  /// The method returns a cached copy of the transform, so modification
  /// of the matrix does not alter the transform node.
  vtkMatrix4x4* GetMatrixTransformFromParent();

protected:
  vtkMRMLLinearTransformNode();
  ~vtkMRMLLinearTransformNode();
  vtkMRMLLinearTransformNode(const vtkMRMLLinearTransformNode&);
  void operator=(const vtkMRMLLinearTransformNode&);

  /// These variables are only for supporting the deprecated
  /// GetMatrixTransformToParent and GetMatrixFromParent methods
  vtkMatrix4x4* CachedMatrixTransformToParent;
  vtkMatrix4x4* CachedMatrixTransformFromParent;
};

#endif
