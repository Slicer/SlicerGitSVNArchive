/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSlicerLineRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSlicerLineRepresentation
 * @brief   Default representation for the line widget
 *
 * This class provides the default concrete representation for the
 * vtkSlicerLineWidget. It works in conjunction with the
 * vtkLinearSlicerLineInterpolator and vtkPointPlacer. See vtkSlicerLineWidget
 * for details.
 * @sa
 * vtkSlicerAbstractRepresentation vtkSlicerLineWidget vtkPointPlacer
 * vtkLinearSlicerLineInterpolator
*/

#ifndef vtkSlicerLineRepresentation_h
#define vtkSlicerLineRepresentation_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkSlicerAbstractRepresentation.h"

class vtkProperty;
class vtkActor;
class vtkPolyDataMapper;
class vtkPolyData;
class vtkGlyph3D;
class vtkPoints;
class vtkVolumePicker;

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkSlicerLineRepresentation : public vtkSlicerAbstractRepresentation
{
public:
  /**
   * Instantiate this class.
   */
  static vtkSlicerLineRepresentation *New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkSlicerLineRepresentation,vtkSlicerAbstractRepresentation);
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
   * Subclasses of vtkContourCurveRepresentation must implement these methods. These
   * are the methods that the widget and its representation use to
   * communicate with each other.
   */
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
   * Get the points in this contour as a vtkPolyData.
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
  vtkSlicerLineRepresentation();
  ~vtkSlicerLineRepresentation() override;

  vtkPolyData          *Lines;
  vtkPolyDataMapper    *LinesMapper;
  vtkActor             *LinesActor;
  virtual void         CreateDefaultProperties();

  vtkProperty   *LinesProperty;
  vtkProperty   *ActiveLinesProperty;

  virtual void BuildLines() override;

private:
  vtkSlicerLineRepresentation(const vtkSlicerLineRepresentation&) = delete;
  void operator=(const vtkSlicerLineRepresentation&) = delete;
};

#endif
