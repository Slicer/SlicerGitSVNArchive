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
  /// Important! TransformToParent and TransformFromParent members are not copied
  /// because the derived classes usually store the specific transforms in variables
  /// that have to be copied anyway, so it would result in duplicate copying (which
  /// is an expensive operation for grid transforms).
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
  /// The returned pointer remains valid throughout the lifetime of the object.
  virtual vtkGeneralTransform* GetTransformToParent();

  ///
  /// vtkGeneral transform of this node from parent
  /// The returned pointer remains valid throughout the lifetime of the object.
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

  ///
  /// When TransformToParent or TransformFromParent is modified then the
  /// inverse is not updated automatically because it may take a long time.
  /// Instead, when a transform is requested, this method tells if the transform
  /// has to be computed from its inverse.
  virtual bool NeedToComputeTransformToParentFromInverse();
  virtual bool NeedToComputeTransformFromParentFromInverse();

  ///
  /// These generic transforms always exist (they are created in the constructor and deleted
  /// in the destructor), but not always up-to-date. The NeedToComputeTransform...FromInverse()
  /// methods specify if it is already up-to-date or need to be computed from the inverse.
  vtkGeneralTransform* TransformToParent;
  vtkGeneralTransform* TransformFromParent;

  ///
  /// These variables store the MTime of the TransformToParent or TransformFromParent that was used
  /// to compute a particular transform.
  /// These are used in NeedToComputeTransformToParentFromInverse to determine if a transform has to be
  /// computed from its inverse (to avoid updating transform A from its inverse, if its inverse was
  /// computed from transform A; because then it means that transform A is already up-to-date).
  /// If the Transform...ParentComputedFromInverseMTime is 0 then it means that it is not computed
  /// from an inverse but set explicitly.
  unsigned long TransformToParentComputedFromInverseMTime;
  unsigned long TransformFromParentComputedFromInverseMTime;

  int ReadWriteAsTransformToParent;

  int DisableTransformModifiedEvent;
  int TransformModifiedEventPending;
};

#endif
