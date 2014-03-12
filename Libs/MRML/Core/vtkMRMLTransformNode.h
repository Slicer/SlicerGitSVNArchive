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

class vtkCollection;
class vtkGeneralTransform;
class vtkMatrix4x4;

/// \brief MRML node for representing a transformation
/// between this node space and a parent node space.
///
/// General Transformation between this node space and a parent node space.
/// A vtkMRMLTransformableNode::TransformModifiedEvent is called if the transforms
/// are changed. ModifiedEvent is called if either transforms or other properties
/// of the object are changed.
class VTK_MRML_EXPORT vtkMRMLTransformNode : public vtkMRMLStorableNode
{
public:
  static vtkMRMLTransformNode *New();
  vtkTypeMacro(vtkMRMLTransformNode,vtkMRMLStorableNode);
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
  virtual const char* GetNodeTagName() {return "Transform";};

  /// 
  /// Finds the storage node and read the data
  virtual void UpdateScene(vtkMRMLScene *scene)
    {
     Superclass::UpdateScene(scene);
    };

  /// 
  /// 1 if transfrom is linear, 0 otherwise
  virtual int IsLinear() { return 0; }

  ///
  /// vtkGeneral transform of this node to parent
  /// The returned pointer remains valid throughout the lifetime of the object, therefore.
  /// it is safe to modify transform parameters through the returned pointer.
  virtual vtkGeneralTransform* GetTransformToParent();

  ///
  /// vtkGeneral transform of this node from parent
  /// The returned pointer remains valid throughout the lifetime of the object, therefore.
  /// it is safe to modify transform parameters through the returned pointer.
  virtual vtkGeneralTransform* GetTransformFromParent();

  /// 
  /// 1 if all the transforms to the top are linear, 0 otherwise
  int  IsTransformToWorldLinear() ;

  /// 
  /// 1 if all the transforms between nodes are linear, 0 otherwise
  int  IsTransformToNodeLinear(vtkMRMLTransformNode* node);

  /// 
  /// Get concatenated transforms to the top
  void GetTransformToWorld(vtkGeneralTransform* transformToWorld);

  /// 
  /// Get concatenated transforms from the top
  void GetTransformFromWorld(vtkGeneralTransform* transformToWorld);

  ///
  /// Get concatenated transforms between nodes
  void GetTransformToNode(vtkMRMLTransformNode* node,
                          vtkGeneralTransform* transformToNode);

  /// 
  /// Get concatenated transforms to the top.
  /// This method and probably needs to be moved down a level in the
  /// hierarchy because this node cannot satisfy the call.
  /// Must be overridden in linear transform node classses.
  /// Returns 0 if the transform is not linear (cannot be described by a matrix).
  virtual int GetMatrixTransformToWorld(vtkMatrix4x4* transformToWorld);

  /// 
  /// Get concatenated transforms between nodes
  /// This method and probably needs to be moved down a level in the
  /// hierarchy because this node cannot satisfy the call.
  /// Must be overridden in linear transform node classses.
  /// Returns 0 if the transform is not linear (cannot be described by a matrix).
  virtual int GetMatrixTransformToNode(vtkMRMLTransformNode* node, 
                                       vtkMatrix4x4* transformToNode);
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

  ///
  /// Retrieves the transform as the specified transform class.
  /// Returns NULL if the transform is not a kind of transform that was requested.
  /// Example usage: vtkWarpTransform* warpTransform=vtkWarpTransform::SafeDownCast(GetTransformToParentAs("vtkWarpTransform"));
  vtkAbstractTransform* GetTransformToParentAs(const char* transformType);

  ///
  /// Retrieves the transform as the specified transform class.
  /// Returns NULL if the transform is not a kind of transform that was requested.
  /// Example usage: vtkWarpTransform* warpTransform=vtkWarpTransform::SafeDownCast(GetTransformFromParentAs("vtkWarpTransform"));
  vtkAbstractTransform* GetTransformFromParentAs(const char* transformType);

  /// Set and observe a new transform of this node to parent node.
  /// Each time the transform is modified,
  /// vtkMRMLTransformableNode::TransformModifiedEvent is fired.
  /// ModifiedEvent() and TransformModifiedEvent() are fired after the transform
  /// is set.
  void SetAndObserveTransformToParent(vtkAbstractTransform *transform);

  /// Set and observe a new transform of this node from parent node.
  /// Each time the transform is modified,
  /// vtkMRMLTransformableNode::TransformModifiedEvent is fired.
  /// ModifiedEvent() and TransformModifiedEvent() are fired after the transform
  /// is set.
  void SetAndObserveTransformFromParent(vtkAbstractTransform *transform);

  /// alternative method to propagate events generated in Transform nodes
  virtual void ProcessMRMLEvents ( vtkObject * /*caller*/,
                                   unsigned long /*event*/,
                                   void * /*callData*/ );

protected:
  vtkMRMLTransformNode();
  ~vtkMRMLTransformNode();
  vtkMRMLTransformNode(const vtkMRMLTransformNode&);
  void operator=(const vtkMRMLTransformNode&);

  ///
  /// TransformToParent and TransformFromParent do not emit
  /// modified event if a concatenated transform is modified, therefore we
  /// have to add observers to these transforms.
  virtual void UpdateTransformToParentObservers();
  virtual void UpdateTransformFromParentObservers();

  ///
  /// These generic transforms always exist (they are created in the constructor and deleted
  /// in the destructor), and they are set up so that one is always computed as the inverse of the
  /// other. We use the capability of generic transforms for concatenating and inverting the same
  /// abstract transform in multiple generic transforms, therefore they are automaticall.
  vtkGeneralTransform* TransformToParent;
  vtkGeneralTransform* TransformFromParent;

  int ReadWriteAsTransformToParent;

  int DisableTransformModifiedEvent;
  int TransformModifiedEventPending;

  vtkCollection* ObservedTransformToParentTransforms;
  vtkCollection* ObservedTransformFromParentTransforms;
};

#endif
