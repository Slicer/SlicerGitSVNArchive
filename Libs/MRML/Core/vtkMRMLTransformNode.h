/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLTransformNode.h,v $
  Date:      $Date: 2006/03/19 17:12:29 $
  Version:   $Revision: 1.13 $

=========================================================================auto=*/

#ifndef __vtkMRMLTransformNode_h
#define __vtkMRMLTransformNode_h

#include "vtkMRMLStorableNode.h"

class vtkGeneralTransform;
class vtkMatrix4x4;

/// \brief MRML node for representing a transformation
/// between this node space and a parent node space.
///
/// General Transformation between this node space and a parent node space.
class VTK_MRML_EXPORT vtkMRMLTransformNode : public vtkMRMLStorableNode
{
public:
  vtkTypeMacro(vtkMRMLTransformNode,vtkMRMLStorableNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkMRMLNode* CreateNodeInstance() = 0;

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
  virtual const char* GetNodeTagName() = 0;

  /// 
  /// Finds the storage node and read the data
  virtual void UpdateScene(vtkMRMLScene *scene)
    {
     Superclass::UpdateScene(scene);
    };

  /// 
  /// 1 if transfrom is linear, 0 otherwise
  virtual int IsLinear() = 0;

  ///
  /// vtkGeneral transform of this node to parent
  virtual vtkGeneralTransform* GetTransformToParent();

  ///
  /// vtkGeneral transform of this node from parent
  virtual vtkGeneralTransform* GetTransformFromParent();

  /// 
  /// 1 if all the transforms to the top are linear, 0 otherwise
  int  IsTransformToWorldLinear() ;

  /// 
  /// 1 if all the transforms between nodes are linear, 0 otherwise
  int  IsTransformToNodeLinear(vtkMRMLTransformNode* node);

  /// 
  /// Get concatinated transforms to the top
  void GetTransformToWorld(vtkGeneralTransform* transformToWorld);

  /// 
  /// Get concatinated transforms from the top
  void GetTransformFromWorld(vtkGeneralTransform* transformToWorld);

  ///
  /// Get concatinated transforms between nodes
  void GetTransformToNode(vtkMRMLTransformNode* node,
                          vtkGeneralTransform* transformToNode);

  /// 
  /// Get concatinated transforms to the top
  virtual int GetMatrixTransformToWorld(vtkMatrix4x4* transformToWorld) = 0;

  /// 
  /// Get concatinated transforms between nodes
  virtual int GetMatrixTransformToNode(vtkMRMLTransformNode* node, 
                                       vtkMatrix4x4* transformToNode) = 0;
  /// 
  /// Returns 1 if this node is one of the node's descendents
  int IsTransformNodeMyParent(vtkMRMLTransformNode* node);

  /// 
  /// Returns 1 if the node is one of the this node's descendents
  int IsTransformNodeMyChild(vtkMRMLTransformNode* node);

  /// Reimplemented from vtkMRMLTransformableNode
  virtual bool CanApplyNonLinearTransforms()const;
  /// Reimplemented from vtkMRMLTransformableNode
  virtual void ApplyTransform(vtkAbstractTransform* transform);

  /// 
  /// Create default storage node or NULL if does not have one
  virtual vtkMRMLStorageNode* CreateDefaultStorageNode();

  /// Get/Set for ReadWriteAsTransformToParent
  vtkGetMacro(ReadWriteAsTransformToParent, int);
  vtkSetMacro(ReadWriteAsTransformToParent, int);
  vtkBooleanMacro(ReadWriteAsTransformToParent, int);

  ///
  /// Start modifying the transform in the node.
  /// Disable vtkMRMLTransformableNode::TransformModifiedEvent events.
  /// Returns the previous state of DisableTransformModifiedEvent flag
  /// that should be passed to EndTransformModify() method
  virtual int StartTransformModify()
    {
    int disabledTransformModify = this->GetDisableTransformModifiedEvent();
    this->DisableTransformModifiedEventOn();
    return disabledTransformModify;
    };

  ///
  /// End modifying the transform in the node.
  /// Enable vtkMRMLTransformableNode::TransformModifiedEvent events if the
  /// previous state of DisableTransformModifiedEvent flag is 0.
  /// Return the number of pending events (even if
  /// InvokePendingTransformModifiedEvent is not called).
  virtual int EndTransformModify(int previousDisableTransformModifiedEventState)
    {
    this->SetDisableTransformModifiedEvent(previousDisableTransformModifiedEventState);
    if (!previousDisableTransformModifiedEventState)
      {
      return this->InvokePendingTransformModifiedEvent();
      }
    return this->TransformModifiedEventPending;
    };

  ///
  /// Turn on/off generating InvokeEvent for transformation changes
  vtkGetMacro(DisableTransformModifiedEvent, int);
  void SetDisableTransformModifiedEvent(int onOff)
    {
    this->DisableTransformModifiedEvent = onOff;
    }
  void DisableTransformModifiedEventOn()
    {
    this->SetDisableTransformModifiedEvent(1);
    }
  void DisableTransformModifiedEventOff()
    {
    this->SetDisableTransformModifiedEvent(0);
    }

  /// Count of pending modified events
  vtkGetMacro(TransformModifiedEventPending, int);

  ///
  /// Indicates that the transform inside the object is modified.
  /// Typical usage would be to disable transform modified events, call a series of operations that change transforms
  /// and then re-enable transform modified events to invoke any pending notifications.
  virtual void TransformModified()
    {
    if (!this->GetDisableTransformModifiedEvent())
      {
      this->InvokeEvent(vtkMRMLTransformableNode::TransformModifiedEvent, NULL);
      }
    else
      {
      ++this->TransformModifiedEventPending;
      }
    }

  ///
  /// Invokes any transform modified events that are 'pending', meaning they were generated
  /// while the DisableTransformModifiedEvent flag was nonzero.
  /// Returns the old flag state.
  virtual int InvokePendingTransformModifiedEvent ()
    {
    if ( this->TransformModifiedEventPending )
      {
      int oldModifiedEventPending = this->TransformModifiedEventPending;
      this->TransformModifiedEventPending = 0;
      this->InvokeEvent(vtkMRMLTransformableNode::TransformModifiedEvent, NULL);
      return oldModifiedEventPending;
      }
    return this->TransformModifiedEventPending;
    }

  virtual bool GetModifiedSinceRead();
protected:
  vtkMRMLTransformNode();
  ~vtkMRMLTransformNode();
  vtkMRMLTransformNode(const vtkMRMLTransformNode&);
  void operator=(const vtkMRMLTransformNode&);

  virtual bool NeedToComputeTransformToParentFromInverse();
  virtual bool NeedToComputeTransformFromParentFromInverse();

  vtkGeneralTransform* TransformToParent;
  vtkGeneralTransform* TransformFromParent;

  /// If a transform is updated the inverse is not computed automatically
  /// (because it may be computationally expensive).
  /// If a transform is set then the corresponding ...MTime member has to set to 0 (to indicate that it is not a computed) transform.
  /// The other (the computed) transform ...MTime member should reflect the time it was actually computed.
  unsigned long TransformToParentComputedFromInverseMTime;
  unsigned long TransformFromParentComputedFromInverseMTime;

  int ReadWriteAsTransformToParent;

  int DisableTransformModifiedEvent;
  int TransformModifiedEventPending;
};

#endif
