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

#ifndef __vtkMRMLMarkupsFiducialDisplayableManager2D_h
#define __vtkMRMLMarkupsFiducialDisplayableManager2D_h

// MarkupsModule includes
#include "vtkSlicerMarkupsModuleMRMLDisplayableManagerExport.h"

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsDisplayableManager2D.h"

class vtkMRMLMarkupsFiducialNode;
class vtkMRMLMarkupsDisplayNode;
class vtkSlicerAbstractWidget;
class vtkTextWidget;

/// \ingroup Slicer_QtModules_Markups
class VTK_SLICER_MARKUPS_MODULE_MRMLDISPLAYABLEMANAGER_EXPORT vtkMRMLMarkupsFiducialDisplayableManager2D :
    public vtkMRMLMarkupsDisplayableManager2D
{
public:

  static vtkMRMLMarkupsFiducialDisplayableManager2D *New();
  vtkTypeMacro(vtkMRMLMarkupsFiducialDisplayableManager2D, vtkMRMLMarkupsDisplayableManager2D);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:

  vtkMRMLMarkupsFiducialDisplayableManager2D(){this->Focus="vtkMRMLMarkupsFiducialNode";}
  virtual ~vtkMRMLMarkupsFiducialDisplayableManager2D(){}

  /// Callback for click in RenderWindow
  virtual void OnClickInRenderWindow(double x, double y,
                                     const char *associatedNodeID,
                                     int action = vtkMRMLMarkupsDisplayableManager2D::AddPoint) VTK_OVERRIDE;
  /// Create a widget.
  virtual vtkSlicerAbstractWidget* CreateWidget(vtkMRMLMarkupsNode* node);
  /// the nth point in the active Markup has been modified, check if it is on the slice
  virtual void OnMRMLMarkupsNthPointModifiedEvent(vtkMRMLNode *node, int n) VTK_OVERRIDE;
  /// a nth point in the active Markup has been added, check if it is on the slice
  virtual void OnMRMLMarkupsPointAddedEvent(vtkMRMLNode *node, int n) VTK_OVERRIDE;
  /// Called after the corresponding MRML Markups node was modified
  virtual void OnMRMLMarkupsNodeModifiedEvent(vtkMRMLNode *node) VTK_OVERRIDE;
  /// Gets called when widget was created
  virtual void OnWidgetCreated(vtkSlicerAbstractWidget * widget, vtkMRMLMarkupsNode * node) VTK_OVERRIDE;
  /// Set up an observer on the interactor style to watch for key press events
  virtual void AdditionnalInitializeStep();
  /// Respond to the interactor style event
  virtual void OnInteractorStyleEvent(int eventid) VTK_OVERRIDE;
  /// Clean up when scene closes
  virtual void OnMRMLSceneEndClose() VTK_OVERRIDE;
  /// Add Control Point
  virtual int AddControlPoint(vtkMRMLMarkupsFiducialNode *markupsNode, double worldCoordinates[4]);
  /// Check, if the point is displayable in the current slice geometry
  virtual bool IsPointDisplayableOnSlice(vtkMRMLMarkupsNode* node, int pointIndex = 0);

private:

  vtkMRMLMarkupsFiducialDisplayableManager2D(const vtkMRMLMarkupsFiducialDisplayableManager2D&); /// Not implemented
  void operator=(const vtkMRMLMarkupsFiducialDisplayableManager2D&); /// Not Implemented

};

#endif
