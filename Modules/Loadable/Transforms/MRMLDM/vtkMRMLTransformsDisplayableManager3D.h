/*=========================================================================

  Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   SlicerViewerWidget
  Module:    $HeadURL: http://svn.slicer.org/Slicer4/trunk/Base/GUI/vtkMRMLTransformsDisplayableManager3D.h $
  Date:      $Date: 2010-05-12 08:34:19 -0400 (Wed, 12 May 2010) $
  Version:   $Revision: 13332 $

==========================================================================*/

#ifndef __vtkMRMLTransformsDisplayableManager3D_h
#define __vtkMRMLTransformsDisplayableManager3D_h

// MRMLDisplayableManager includes
#include "vtkMRMLAbstractThreeDViewDisplayableManager.h"

#include "vtkSlicerTransformsModuleMRMLDisplayableManagerExport.h"

// MRML includes
class vtkMRMLDisplayNode;
class vtkMRMLDisplayableNode;

// VTK includes
#include "vtkRenderWindow.h"
class vtkActor;
class vtkActorText;
class vtkBoundingBox;
class vtkCellArray;
class vtkCellPicker;
class vtkClipPolyData;
class vtkFollower;
class vtkImplicitBoolean;
class vtkMatrix4x4;
class vtkPMatrix4x4;
class vtkPlane;
class vtkPlane;
class vtkPointPicker;
class vtkPolyData;
class vtkProp3D;
class vtkPropPicker;
class vtkWorldPointPicker;

/// \brief Display transforms in 3D views
///
/// Creates corresponding actor for each transform display node in the scene.
/// Adapted from the model display node.
class VTK_SLICER_TRANSFORMS_MODULE_MRMLDISPLAYABLEMANAGER_EXPORT vtkMRMLTransformsDisplayableManager3D
  : public vtkMRMLAbstractThreeDViewDisplayableManager
{
public:
  static vtkMRMLTransformsDisplayableManager3D* New();
  vtkTypeMacro(vtkMRMLTransformsDisplayableManager3D,vtkMRMLAbstractThreeDViewDisplayableManager);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Return the current model actor corresponding to a give MRML ID
  vtkProp3D *GetActorByID(const char *id);

  /// Return the current node ID corresponding to a given vtkProp3D
  const char *GetIDByActor(vtkProp3D *actor);

  /// Return true if the node can be represented as a model
  bool IsTransformDisplayable(vtkMRMLDisplayableNode* node)const;
  /// Return true if the display node is a model
  bool IsTransformDisplayable(vtkMRMLDisplayNode* node)const;

protected:

  vtkMRMLTransformsDisplayableManager3D();
  virtual ~vtkMRMLTransformsDisplayableManager3D();

  virtual void UnobserveMRMLScene();

  virtual void OnMRMLSceneStartClose();
  virtual void OnMRMLSceneEndClose();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);

  virtual void ProcessMRMLNodesEvents(vtkObject *caller, unsigned long event, void *callData);

  /// Returns true if something visible in transformNode has changed and would
  /// require a refresh.
  bool OnMRMLDisplayableNodeModifiedEvent(vtkMRMLDisplayableNode * transformNode);

  /// Updates Actors based on models in the scene
  void UpdateFromMRML();

  virtual void RemoveMRMLObservers();

  friend class vtkThreeDViewInteractorStyle; // Access to RequestRender();

  void RemoveProps();
  void RemoveDisplayableObservers(int clearCache);
  void RemoveDisplayable(vtkMRMLDisplayableNode* model);
  void RemoveDisplayableNodeObservers(vtkMRMLDisplayableNode *model);

  void UpdateModelsFromMRML();
  void UpdateModel(vtkMRMLDisplayableNode *model);
  void UpdateModelPolyData(vtkMRMLDisplayableNode *model);
  void UpdateModifiedModel(vtkMRMLDisplayableNode *model);

  void SetModelDisplayProperty(vtkMRMLDisplayableNode *model);
  int GetDisplayedModelsVisibility(vtkMRMLDisplayNode *model);

  void RemoveDispalyedID(std::string &id);

private:

  vtkMRMLTransformsDisplayableManager3D(const vtkMRMLTransformsDisplayableManager3D&); // Not implemented
  void operator=(const vtkMRMLTransformsDisplayableManager3D&);                 // Not Implemented

  class vtkInternal;
  vtkInternal* Internal;

};

#endif
