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

#ifndef __vtkMRMLMarkupsFiducialDisplayableManager3D_h
#define __vtkMRMLMarkupsFiducialDisplayableManager3D_h

// MarkupsModule includes
#include "vtkSlicerMarkupsModuleMRMLDisplayableManagerExport.h"

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsDisplayableManager3D.h"

class vtkMRMLMarkupsFiducialNode;
class vtkSlicerViewerWidget;
class vtkMRMLMarkupsDisplayNode;
class vtkTextWidget;

/// \ingroup Slicer_QtModules_Markups
class VTK_SLICER_MARKUPS_MODULE_MRMLDISPLAYABLEMANAGER_EXPORT vtkMRMLMarkupsFiducialDisplayableManager3D :
    public vtkMRMLMarkupsDisplayableManager3D
{
public:

  static vtkMRMLMarkupsFiducialDisplayableManager3D *New();
  vtkTypeMacro(vtkMRMLMarkupsFiducialDisplayableManager3D, vtkMRMLMarkupsDisplayableManager3D);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:

  vtkMRMLMarkupsFiducialDisplayableManager3D(){this->Focus="vtkMRMLMarkupsFiducialNode";}
  virtual ~vtkMRMLMarkupsFiducialDisplayableManager3D(){}

  /// Callback for click in RenderWindow
  virtual void OnClickInRenderWindow(double x, double y,
                                     const char *associatedNodeID,
                                     int action = vtkMRMLMarkupsFiducialDisplayableManager3D::AddPoint) VTK_OVERRIDE;
  /// Create a widget.
  virtual vtkSlicerAbstractWidget * CreateWidget(vtkMRMLMarkupsNode* node) VTK_OVERRIDE;
  /// Gets called when widget was created
  virtual void OnWidgetCreated(vtkSlicerAbstractWidget * widget, vtkMRMLMarkupsNode * node) VTK_OVERRIDE;
  /// Clean up when scene closes
  virtual void OnMRMLSceneEndClose() VTK_OVERRIDE;

private:

  vtkMRMLMarkupsFiducialDisplayableManager3D(const vtkMRMLMarkupsFiducialDisplayableManager3D&); /// Not implemented
  void operator=(const vtkMRMLMarkupsFiducialDisplayableManager3D&); /// Not Implemented
};

#endif
