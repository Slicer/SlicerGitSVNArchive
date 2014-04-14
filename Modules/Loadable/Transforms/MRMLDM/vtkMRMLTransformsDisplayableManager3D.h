/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Andras Lasso and Franklin King at
  PerkLab, Queen's University and was supported through the Applied Cancer
  Research Unit program of Cancer Care Ontario with funds provided by the
  Ontario Ministry of Health and Long-Term Care.

==============================================================================*/

#ifndef __vtkMRMLTransformsDisplayableManager3D_h
#define __vtkMRMLTransformsDisplayableManager3D_h

// MRMLDisplayableManager includes
#include "vtkMRMLAbstractThreeDViewDisplayableManager.h"

#include "vtkSlicerTransformsModuleMRMLDisplayableManagerExport.h"

class vtkMRMLTransformNode;
class vtkMRMLTransformDisplayNode;
class vtkActor;

/// \brief Display transforms in 3D views
///
/// Creates corresponding actor for each transform display node in the scene.
/// Adapted from the model display node.
///
class VTK_SLICER_TRANSFORMS_MODULE_MRMLDISPLAYABLEMANAGER_EXPORT vtkMRMLTransformsDisplayableManager3D
  : public vtkMRMLAbstractThreeDViewDisplayableManager
{
public:
  static vtkMRMLTransformsDisplayableManager3D* New();
  vtkTypeMacro(vtkMRMLTransformsDisplayableManager3D,vtkMRMLAbstractThreeDViewDisplayableManager);
  void PrintSelf(ostream& os, vtkIndent indent);

  // DisplayableNode handling customizations
  void AddDisplayableNode(vtkMRMLTransformNode* displayableNode);
  void RemoveDisplayableNode(vtkMRMLTransformNode* displayableNode);

protected:

  vtkMRMLTransformsDisplayableManager3D();
  virtual ~vtkMRMLTransformsDisplayableManager3D();

  virtual void UnobserveMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void ProcessMRMLNodesEvents(vtkObject* caller, unsigned long event, void* callData);

  /// Update Actors based on models in the scene
  virtual void UpdateFromMRML();

  virtual void OnMRMLSceneStartClose();
  virtual void OnMRMLSceneEndClose();

  virtual void OnMRMLSceneEndBatchProcess();

  virtual void SetModelDisplayProperty(vtkMRMLTransformDisplayNode *displayNode, vtkActor* actor);
/*

  virtual void UpdateFromMRMLScene();

  /// Return true if the node can be represented as a model
  bool IsTransformDisplayable(vtkMRMLDisplayableNode* node)const;
  /// Return true if the display node is a model
  bool IsTransformDisplayable(vtkMRMLDisplayNode* node)const;

  /// Returns true if something visible in transformNode has changed and would
  /// require a refresh.
  bool OnMRMLDisplayableNodeModifiedEvent(vtkMRMLDisplayableNode * transformNode);

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

  int GetDisplayedModelsVisibility(vtkMRMLDisplayNode *model);

  void RemoveDispalyedID(std::string &id);

*/

  bool AddingDisplayableNode;

private:

  vtkMRMLTransformsDisplayableManager3D(const vtkMRMLTransformsDisplayableManager3D&); // Not implemented
  void operator=(const vtkMRMLTransformsDisplayableManager3D&);                 // Not Implemented

  class vtkInternal;
  vtkInternal* Internal;
  friend class vtkInternal;
};

#endif
