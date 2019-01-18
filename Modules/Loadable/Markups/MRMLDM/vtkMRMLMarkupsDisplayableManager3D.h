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

==============================================================================*/

#ifndef __vtkMRMLMarkupsDisplayableManager3D_h
#define __vtkMRMLMarkupsDisplayableManager3D_h

// MarkupsModule includes
#include "vtkSlicerMarkupsModuleMRMLDisplayableManagerExport.h"

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsDisplayableManagerHelper.h"

// MRMLDisplayableManager includes
#include <vtkMRMLAbstractThreeDViewDisplayableManager.h>

// VTK includes
#include <vtkSlicerPointsWidget.h>

class vtkMRMLMarkupsNode;
class vtkSlicerViewerWidget;
class vtkMRMLMarkupsDisplayNode;
class vtkSlicerAbstractWidget;

/// \ingroup Slicer_QtModules_Markups
class  VTK_SLICER_MARKUPS_MODULE_MRMLDISPLAYABLEMANAGER_EXPORT vtkMRMLMarkupsDisplayableManager3D :
    public vtkMRMLAbstractThreeDViewDisplayableManager
{
public:

  static vtkMRMLMarkupsDisplayableManager3D *New();
  vtkTypeMacro(vtkMRMLMarkupsDisplayableManager3D, vtkMRMLAbstractThreeDViewDisplayableManager);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /// Check if there are real changes between two sets of displayCoordinates
  bool GetDisplayCoordinatesChanged(double * displayCoordinates1, double * displayCoordinates2);

  /// Check if there are real changes between two sets of worldCoordinates
  bool GetWorldCoordinatesChanged(double * worldCoordinates1, double * worldCoordinates2);

  /// Convert display to world coordinates
  void GetDisplayToWorldCoordinates(double x, double y, double * worldCoordinates);
  void GetDisplayToWorldCoordinates(double * displayCoordinates, double * worldCoordinates);

  /// Convert world coordinates to local using mrml parent transform
  virtual void GetWorldToLocalCoordinates(vtkMRMLMarkupsNode *node,
                                  double *worldCoordinates, double *localCoordinates);

  /// Set mrml parent transform to widgets
  virtual void SetParentTransformToWidget(vtkMRMLMarkupsNode *vtkNotUsed(node), vtkSlicerAbstractWidget *vtkNotUsed(widget)){};

  /// Create a new widget for this markups node and save it to the helper.
  /// Returns widget on success, null on failure.
  vtkSlicerAbstractWidget *AddWidget(vtkMRMLMarkupsNode *markupsNode);

  vtkGetObjectMacro(Helper, vtkMRMLMarkupsDisplayableManagerHelper);

protected:
  vtkMRMLMarkupsDisplayableManager3D();
  virtual ~vtkMRMLMarkupsDisplayableManager3D();

  virtual void ProcessMRMLNodesEvents(vtkObject *caller, unsigned long event, void *callData) VTK_OVERRIDE;

  /// Wrap the superclass render request in a check for batch processing
  virtual void RequestRender();

  /// Remove MRML observers
  virtual void RemoveMRMLObservers() VTK_OVERRIDE;

  /// Called from RequestRender method if UpdateFromMRMLRequested is true
  /// \sa RequestRender() SetUpdateFromMRMLRequested()
  virtual void UpdateFromMRML() VTK_OVERRIDE;

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene) VTK_OVERRIDE;

  /// Called after the corresponding MRML event is triggered, from AbstractDisplayableManager
  /// \sa ProcessMRMLSceneEvents
  virtual void UpdateFromMRMLScene() VTK_OVERRIDE;
  virtual void OnMRMLSceneEndClose() VTK_OVERRIDE;
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node) VTK_OVERRIDE;
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node) VTK_OVERRIDE;

  /// Called after the corresponding MRML View container was modified
  virtual void OnMRMLDisplayableNodeModifiedEvent(vtkObject* caller) VTK_OVERRIDE;

  /// Observe one node
  void SetAndObserveNode(vtkMRMLMarkupsNode *markupsNode);
  /// Observe all associated nodes.
  void SetAndObserveNodes();

  /// Observe the interaction node.
  void AddObserversToInteractionNode();
  void RemoveObserversFromInteractionNode();

  /// Preset functions for certain events.
  virtual void OnMRMLMarkupsNodeModifiedEvent(vtkMRMLNode* node);
  virtual void OnMRMLMarkupsNodeTransformModifiedEvent(vtkMRMLNode* node);
  virtual void OnMRMLMarkupsNodeLockModifiedEvent(vtkMRMLNode* node);
  virtual void OnMRMLMarkupsDisplayNodeModifiedEvent(vtkMRMLNode *node);
  virtual void OnMRMLMarkupsNthPointModifiedEvent(vtkMRMLNode *node, int n);
  virtual void OnMRMLMarkupsPointAddedEvent(vtkMRMLNode *node, int n);
  virtual void OnMRMLMarkupsPointRemovedEvent(vtkMRMLNode *node, int n);
  virtual void OnMRMLMarkupsAllPointsRemovedEvent(vtkMRMLNode *node);

  /// Get the coordinates of a click in the RenderWindow
  void OnClickInRenderWindowGetCoordinates();
  /// Callback for click in RenderWindow
  virtual void OnClickInRenderWindow(double x, double y, const char *associatedNodeID = NULL);

  /// Convert display to world coordinates
  void GetWorldToDisplayCoordinates(double r, double a, double s, double * displayCoordinates);
  void GetWorldToDisplayCoordinates(double * worldCoordinates, double * displayCoordinates);

  /// Convert display to viewport coordinates
  void GetDisplayToViewportCoordinates(double x, double y, double * viewportCoordinates);
  void GetDisplayToViewportCoordinates(double *displayCoordinates, double * viewportCoordinates);

  /// Create a widget.
  virtual vtkSlicerAbstractWidget * CreateWidget(vtkMRMLMarkupsNode* node);
  /// Gets called when widget was created
  virtual void OnWidgetCreated(vtkSlicerAbstractWidget * widget, vtkMRMLMarkupsNode * node);
  /// Get the widget of a node.
  vtkSlicerAbstractWidget * GetWidget(vtkMRMLMarkupsNode * node);

  /// Check if it is the right displayManager
  virtual bool IsCorrectDisplayableManager();

  /// Return true if this displayable manager supports(can manage) that node,
  /// false otherwise.
  /// Can be reimplemented to add more conditions.
  /// \sa IsManageable(const char*), IsCorrectDisplayableManager()
  virtual bool IsManageable(vtkMRMLNode* node);
  /// Return true if this displayable manager supports(can manage) that node class,
  /// false otherwise.
  /// Can be reimplemented to add more conditions.
  /// \sa IsManageable(vtkMRMLNode*), IsCorrectDisplayableManager()
  virtual bool IsManageable(const char* nodeClassName);

  /// Focus of this displayableManager is set to a specific markups type when inherited
  const char* Focus;

  /// Respond to interactor style events
  virtual void OnInteractorStyleEvent(int eventid) VTK_OVERRIDE;

  /// Accessor for internal flag that disables interactor style event processing
  vtkGetMacro(DisableInteractorStyleEventsProcessing, int);

  vtkMRMLMarkupsDisplayableManagerHelper * Helper;

  double LastClickWorldCoordinates[4];

private:
  vtkMRMLMarkupsDisplayableManager3D(const vtkMRMLMarkupsDisplayableManager3D&); /// Not implemented
  void operator=(const vtkMRMLMarkupsDisplayableManager3D&); /// Not Implemented

  int DisableInteractorStyleEventsProcessing;
};

#endif
