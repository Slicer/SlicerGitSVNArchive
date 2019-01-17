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
 * @class   vtkSlicerAngleRepresentation
 * @brief   Default representation for the angle widget
 *
 * This class provides the default concrete representation for the
 * vtkSlicerAngleWidget. It works in conjunction with the
 * vtkLinearSlicerLineInterpolator and vtkPointPlacer. See vtkSlicerAngleWidget
 * for details.
 * @sa
 * vtkSlicerAbstractRepresentation vtkSlicerAngleWidget vtkPointPlacer
 * vtkLinearSlicerLineInterpolator
*/

#ifndef vtkSlicerAngleRepresentation_h
#define vtkSlicerAngleRepresentation_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkSlicerAbstractRepresentation.h"

class vtkProperty;
class vtkActor;
class vtkArcSource;
class vtkPolyDataMapper;
class vtkPolyData;
class vtkGlyph3D;
class vtkFollower;
class vtkPoints;
class vtkVectorText;
class vtkVolumePicker;

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkSlicerAngleRepresentation : public vtkSlicerAbstractRepresentation
{
public:
  /**
   * Instantiate this class.
   */
  static vtkSlicerAngleRepresentation *New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkSlicerAngleRepresentation,vtkSlicerAbstractRepresentation);
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
  vtkGetObjectMacro(ArcActor,vtkActor);
  vtkGetObjectMacro(TextActor,vtkFollower);
  //@}

  //@{
  /**
   * Angle returned is in radians.
   */
  double GetAngle();
  //@}

  //@{
  /**
   * Scale text.
   */
  virtual void SetTextActorScale( double scale[3] );
  virtual double * GetTextActorScale();
  //@}

  //@{
  /**
   * Specify the format to use for labeling the angle. Note that an empty
   * string results in no label, or a format string without a "%" character
   * will not print the angle value.
   */
  vtkSetStringMacro(LabelFormat);
  vtkGetStringMacro(LabelFormat);
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
  vtkSlicerAngleRepresentation();
  ~vtkSlicerAngleRepresentation() override;

  vtkPolyData          *Lines;
  vtkPolyDataMapper    *LinesMapper;
  vtkActor             *LinesActor;

  vtkArcSource         *ArcSource;
  vtkPolyDataMapper    *ArcMapper;
  vtkActor             *ArcActor;

  vtkFollower          *TextActor;
  vtkPolyDataMapper    *TextMapper;
  vtkVectorText        *TextInput;

  double               Angle;
  bool                 ScaleInitialized;
  double               TextPosition[3];

  // Format for the label
  char                 *LabelFormat;

  vtkProperty          *LinesProperty;
  vtkProperty          *ActiveLinesProperty;
  virtual void         CreateDefaultProperties();

  virtual void BuildLines() override;

  // Methods to manipulate the cursor
  void RotateWidget(double eventPos[2]);

private:
  vtkSlicerAngleRepresentation(const vtkSlicerAngleRepresentation&) = delete;
  void operator=(const vtkSlicerAngleRepresentation&) = delete;
};

#endif
