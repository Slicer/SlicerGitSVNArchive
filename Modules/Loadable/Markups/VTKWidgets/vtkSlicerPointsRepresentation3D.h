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
 * @class   vtkSlicerPointsRepresentation3D
 * @brief   Default representation for the points widget
 *
 * This class provides the default concrete representation for the
 * vtkSlicerPointsWidget. It works in conjunction with the
 * vtkPointPlacer. See vtkSlicerPointsWidget for details.
 * @sa
 * vtkSlicerAbstractRepresentation3D vtkSlicerPointsWidget vtkPointPlacer
*/

#ifndef vtkSlicerPointsRepresentation3D_h
#define vtkSlicerPointsRepresentation3D_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkSlicerAbstractRepresentation3D.h"

class vtkAppendPolyData;

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkSlicerPointsRepresentation3D : public vtkSlicerAbstractRepresentation3D
{
public:
  /// Instantiate this class.
  static vtkSlicerPointsRepresentation3D *New();

  /// Standard methods for instances of this class.
  vtkTypeMacro(vtkSlicerPointsRepresentation3D,vtkSlicerAbstractRepresentation3D);

  /// These are the methods that the widget and its representation use to
  /// communicate with each other.
  void Highlight(int highlight) VTK_OVERRIDE;

  /// Get the points in this widget as a vtkPolyData.
  vtkPolyData *GetWidgetRepresentationAsPolyData() VTK_OVERRIDE;

  /// Return the bounds of the representation
  double *GetBounds() VTK_OVERRIDE;

protected:
  vtkSlicerPointsRepresentation3D();
  ~vtkSlicerPointsRepresentation3D() VTK_OVERRIDE;

  virtual void BuildLines() VTK_OVERRIDE;

  vtkAppendPolyData *appendActors;

private:
  vtkSlicerPointsRepresentation3D(const vtkSlicerPointsRepresentation3D&) = delete;
  void operator=(const vtkSlicerPointsRepresentation3D&) = delete;
};

#endif
