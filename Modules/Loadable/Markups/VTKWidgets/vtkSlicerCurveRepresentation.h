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
 * @class   vtkSlicerCurveRepresentation
 * @brief   Default representation for the curve widget
 *
 * This class provides the default concrete representation for the
 * vtkSlicerCurveWidget. It works in conjunction with the
 * vtkBezierSlicerLineInterpolator and vtkPointPlacer. See vtkSlicerCurveWidget
 * for details.
 * @sa
 * vtkSlicerAbstractRepresentation vtkSlicerCurveWidget vtkPointPlacer
 * vtkBezierSlicerLineInterpolator
*/

#ifndef vtkSlicerCurveRepresentation_h
#define vtkSlicerCurveRepresentation_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkSlicerAbstractRepresentation.h"

class vtkProperty;
class vtkActor;
class vtkPolyDataMapper;
class vtkPolyData;
class vtkGlyph3D;
class vtkPoints;
class vtkVolumePicker;

//----------------------------------------------------------------------
class vtkSlicerAbstractRepresentationCentroidPoint
{
public:
  double        WorldPosition[3] = {0};
  double        DisplayPosition[2] = {0};
  vtkTypeBool   Visibility = false;
};

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkSlicerCurveRepresentation : public vtkSlicerAbstractRepresentation
{
public:
  /**
   * Instantiate this class.
   */
  static vtkSlicerCurveRepresentation *New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkSlicerCurveRepresentation,vtkSlicerAbstractRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * This is the property used by the line is not active
   * (the mouse is not near the line)
   */
  vtkGetObjectMacro(LinesProperty,vtkProperty);
  //@}

  //@{
  /**
   * This is the property used when the user is interacting
   * with the line.
   */
  vtkGetObjectMacro(ActiveLinesProperty,vtkProperty);
  //@}

  //@{
  /**
   * Utility method to add a centroid point.
   */
  virtual void UpdateCentroidPoint();
  //@}

  //@{
  /**
   * Given a display position, activate a node. The closest
   * node within tolerance will be activated. If a node is
   * activated, 1 will be returned, otherwise 0 will be
   * returned.
   * Note, the variable ActiveNode has 4 status:
   * 1) -3 widget is not selected.
   * 2) -2 the centroid has been selected
   * 3) -1 the line has been selected
   * 4) >=0 the index of the selected node (Point).
   */
  virtual int ActivateNode( double displayPos[2] );
  virtual int ActivateNode( int displayPos[2] );
  virtual int ActivateNode( int X, int Y );
  //@}

  //@{
  /**
   * These are the methods that the widget and its representation use to
   * communicate with each other.
   */
  void BuildRepresentation() override;
  void WidgetInteraction(double eventPos[2]) override;
  void Highlight(int highlight) override;
  //@}

  //@{
  /**
   * Methods to make this class behave as a vtkProp.
   */
  void GetActors(vtkPropCollection *) override;
  void ReleaseGraphicsResources(vtkWindow *) override;
  int RenderOverlay(vtkViewport *viewport) override;
  int RenderOpaqueGeometry(vtkViewport *viewport) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport *viewport) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  //@{
  /**
   * Set/Get the three leaders used to create this representation.
   * By obtaining these leaders the user can set the appropriate
   * properties, etc.
   */
  vtkGetObjectMacro(LinesActor,vtkActor);
  //@}

  //@{
  /**
   * Get the lines in this widget as a vtkPolyData.
   */
  vtkPolyData *GetWidgetRepresentationAsPolyData() override;
  //@}

  //@{
  /**
   * Convenience method to set the line color.
   * Ideally one should use GetLinesProperty()->SetColor().
   */
  void SetLineColor(double r, double g, double b);
  //@}

  //@{
  /**
   * Return the bounds of the representation
   */
  double *GetBounds() override;
  //@}

protected:
  vtkSlicerCurveRepresentation();
  ~vtkSlicerCurveRepresentation() override;

  vtkPolyData          *Lines;
  vtkPolyDataMapper    *LinesMapper;
  vtkActor             *LinesActor;
  virtual void         CreateDefaultProperties();

  vtkSlicerAbstractRepresentationCentroidPoint *centroidPoint;

  vtkProperty   *LinesProperty;
  vtkProperty   *ActiveLinesProperty;

  virtual void BuildLines() override;

  // Compute the centroid by sampling the points along the polyline of the widget at equal distances
  virtual void ComputeCentroid(double* ioCentroid);

private:
  vtkSlicerCurveRepresentation(const vtkSlicerCurveRepresentation&) = delete;
  void operator=(const vtkSlicerCurveRepresentation&) = delete;
};

#endif
