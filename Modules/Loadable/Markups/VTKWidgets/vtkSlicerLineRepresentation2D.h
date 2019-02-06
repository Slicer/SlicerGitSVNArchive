/*=========================================================================

 Copyright (c) ProxSim ltd., Kwun Tong, Hong Kong. All Rights Reserved.

 See COPYRIGHT.txt
 or http://www.slicer.org/copyright/copyright.txt for details.

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

 This file was originally developed by Davide Punzo, punzodavide@hotmail.it,
 and development was supported by ProxSim ltd.

=========================================================================*/

/**
 * @class   vtkSlicerLineRepresentation2D
 * @brief   Default representation for the line widget
 *
 * This class provides the default concrete representation for the
 * vtkSlicerLineWidget. It works in conjunction with the
 * vtkLinearSlicerLineInterpolator and vtkPointPlacer. See vtkSlicerLineWidget
 * for details.
 * @sa
 * vtkSlicerAbstractRepresentation2D vtkSlicerLineWidget vtkPointPlacer
 * vtkLinearSlicerLineInterpolator
*/

#ifndef vtkSlicerLineRepresentation2D_h
#define vtkSlicerLineRepresentation2D_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkSlicerAbstractRepresentation2D.h"

class vtkActor2D;
class vtkAppendPolyData;
class vtkOpenGLPolyDataMapper2D;
class vtkPolyData;
class vtkProperty2D;
class vtkTubeFilter;
class vtkPropPicker;

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkSlicerLineRepresentation2D : public vtkSlicerAbstractRepresentation2D
{
public:
  /// Instantiate this class.
  static vtkSlicerLineRepresentation2D *New();

  /// Standard methods for instances of this class.
  vtkTypeMacro(vtkSlicerLineRepresentation2D,vtkSlicerAbstractRepresentation2D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Subclasses of vtkContourCurveRepresentation must implement these methods. These
  /// are the methods that the widget and its representation use to
  /// communicate with each other.
  void BuildRepresentation() VTK_OVERRIDE;
  int ComputeInteractionState(int X, int Y, int modified=0) VTK_OVERRIDE;

  /// Methods to make this class behave as a vtkProp.
  void GetActors(vtkPropCollection *) override;
  void ReleaseGraphicsResources(vtkWindow *) override;
  int RenderOverlay(vtkViewport *viewport) override;
  int RenderOpaqueGeometry(vtkViewport *viewport) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport *viewport) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;

  /// Set/Get the three leaders used to create this representation.
  /// By obtaining these leaders the user can set the appropriate
  /// properties, etc.
  vtkGetObjectMacro(LineActor,vtkActor2D);

  /// Register internal Pickers in the Picking Manager.
  /// Must be reimplemented by concrete widget representations to register
  /// their pickers.
  virtual void RegisterPickers() VTK_OVERRIDE;

  /// Return the bounds of the representation
  double *GetBounds() override;

protected:
  vtkSlicerLineRepresentation2D();
  ~vtkSlicerLineRepresentation2D() override;

  // Methods to manipulate the cursor
  virtual void TranslateWidget(double eventPos[2]) VTK_OVERRIDE;
  virtual void ScaleWidget(double eventPos[2]) VTK_OVERRIDE;
  virtual void RotateWidget(double eventPos[2]) VTK_OVERRIDE;

  vtkPolyData                  *Line;
  vtkOpenGLPolyDataMapper2D    *LineMapper;
  vtkActor2D                   *LineActor;

  vtkTubeFilter  *TubeFilter;

  virtual void BuildLines() override;

  vtkAppendPolyData *appendActors;

  // Support picking
  vtkPropPicker *LinePicker;

private:
  vtkSlicerLineRepresentation2D(const vtkSlicerLineRepresentation2D&) = delete;
  void operator=(const vtkSlicerLineRepresentation2D&) = delete;
};

#endif
