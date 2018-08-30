/*==============================================================================

Program: 3D Slicer

Copyright (c) Kitware Inc.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

This file was originally developed by Julien Finet, Kitware Inc.
and was partially funded by Allen Institute

==============================================================================*/

#ifndef __vtkMRMLMarkupsSplinesDisplayableManager2D_h
#define __vtkMRMLMarkupsSplinesDisplayableManager2D_h

// MarkupsModule includes
#include "vtkSlicerSplinesModuleMRMLDisplayableManagerExport.h"

// MarkupsModule/MRMLDisplayableManager includes
#include <vtkMRMLAbstractSliceViewDisplayableManager.h>

class vtkMRMLMarkupsPlanesNode;
class vtkSlicerViewerWidget;
class vtkMRMLMarkupsDisplayNode;

/// \ingroup Slicer_QtModules_Markups
class VTK_SLICER_SPLINES_MODULE_MRMLDISPLAYABLEMANAGER_EXPORT vtkMRMLMarkupsSplinesDisplayableManager2D :
  public vtkMRMLAbstractSliceViewDisplayableManager
{
public:

  static vtkMRMLMarkupsSplinesDisplayableManager2D *New();
  vtkTypeMacro(
    vtkMRMLMarkupsSplinesDisplayableManager2D,
    vtkMRMLAbstractSliceViewDisplayableManager);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkMRMLMarkupsSplinesDisplayableManager2D();
  virtual ~vtkMRMLMarkupsSplinesDisplayableManager2D();

  virtual void UnobserveMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void ProcessMRMLNodesEvents(
    vtkObject* caller, unsigned long event, void* callData);

  virtual void SetMRMLSceneInternal(vtkMRMLScene* scene);
  virtual void OnInteractorStyleEvent(int eventid);

  /// Update Actors based on transforms in the scene
  virtual void UpdateFromMRML();

  virtual void OnMRMLSceneStartClose();
  virtual void OnMRMLSceneEndClose();

  virtual void OnMRMLSceneEndBatchProcess();

  /// Initialize the displayable manager
  virtual void Create();

  virtual void ProcessWidgetsEvents(
    vtkObject* caller, unsigned long event, void* callData);

  virtual void OnMRMLSliceNodeModifiedEvent();

private:
  vtkMRMLMarkupsSplinesDisplayableManager2D(
    const vtkMRMLMarkupsSplinesDisplayableManager2D&); // Not implemented
  void operator=(const vtkMRMLMarkupsSplinesDisplayableManager2D&); // Not Implemented

  class vtkInternal;
  vtkInternal* Internal;
  friend class vtkInternal;
};

#endif
