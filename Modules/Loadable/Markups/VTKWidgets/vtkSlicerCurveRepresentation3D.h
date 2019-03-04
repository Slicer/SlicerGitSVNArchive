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
 * @class   vtkSlicerCurveRepresentation3D
 * @brief   Default representation for the curve widget
 *
  * This class provides the default concrete representation for the
 * vtkSlicerAbstractWidget. See vtkSlicerAbstractWidget
 * for details.
 * @sa
 * vtkSlicerAbstractWidgetRepresentation3D vtkSlicerAbstractWidget

*/

#ifndef vtkSlicerCurveRepresentation3D_h
#define vtkSlicerCurveRepresentation3D_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkSlicerAbstractWidgetRepresentation3D.h"

class vtkAppendPolyData;
class vtkCellLocator;
class vtkOpenGLActor;
class vtkOpenGLPolyDataMapper;
class vtkPolyData;
class vtkTubeFilter;
class vtkOpenGLTextActor;
class vtkArcSource;

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkSlicerCurveRepresentation3D : public vtkSlicerAbstractWidgetRepresentation3D
{
public:
  /// Instantiate this class.
  static vtkSlicerCurveRepresentation3D *New();

  /// Standard methods for instances of this class.
  vtkTypeMacro(vtkSlicerCurveRepresentation3D,vtkSlicerAbstractWidgetRepresentation3D);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /// Subclasses of vtkSlicerAbstractWidgetRepresentation must implement these methods. These
  /// are the methods that the widget and its representation use to
  /// communicate with each other.
  void UpdateFromMRML(vtkMRMLNode* caller, unsigned long event, void *callData = NULL) VTK_OVERRIDE;

  /// Methods to make this class behave as a vtkProp.
  void GetActors(vtkPropCollection *) VTK_OVERRIDE;
  void ReleaseGraphicsResources(vtkWindow *) VTK_OVERRIDE;
  int RenderOverlay(vtkViewport *viewport) VTK_OVERRIDE;
  int RenderOpaqueGeometry(vtkViewport *viewport) VTK_OVERRIDE;
  int RenderTranslucentPolygonalGeometry(vtkViewport *viewport) VTK_OVERRIDE;
  vtkTypeBool HasTranslucentPolygonalGeometry() VTK_OVERRIDE;

  /// Return the bounds of the representation
  double *GetBounds() VTK_OVERRIDE;

  void CanInteract(const int displayPosition[2], const double worldPosition[3],
    int &foundComponentType, int &foundComponentIndex, double &closestDistance2) VTK_OVERRIDE;

  void CanInteractWithCurve(const int displayPosition[2], const double worldPosition[3],
    int &foundComponentType, int &componentIndex, double &closestDistance2);

protected:
  vtkSlicerCurveRepresentation3D();
  ~vtkSlicerCurveRepresentation3D() VTK_OVERRIDE;

  void SetMarkupsNode(vtkMRMLMarkupsNode *markupsNode) VTK_OVERRIDE;

  vtkSmartPointer<vtkPolyData> Line;
  vtkSmartPointer<vtkOpenGLPolyDataMapper> LineMapper;
  vtkSmartPointer<vtkOpenGLActor> LineActor;

  vtkSmartPointer<vtkTubeFilter> TubeFilter;

  vtkSmartPointer<vtkCellLocator> CurvePointLocator;

private:
  vtkSlicerCurveRepresentation3D(const vtkSlicerCurveRepresentation3D&) = delete;
  void operator=(const vtkSlicerCurveRepresentation3D&) = delete;
};

#endif
