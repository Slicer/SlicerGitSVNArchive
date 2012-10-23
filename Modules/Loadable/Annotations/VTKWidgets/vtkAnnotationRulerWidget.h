/*=auto=========================================================================

 Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

 See COPYRIGHT.txt
 or http://www.slicer.org/copyright/copyright.txt for details.

 Program:   3D Slicer
 Module:    $RCSfile: vtkAnnotationRulerWidget,v $
 Date:      $Date: Aug 4, 2010 10:44:52 AM $
 Version:   $Revision: 1.0 $

 =========================================================================auto=*/

#ifndef __vtkAnnotationRulerWidget_h
#define __vtkAnnotationRulerWidget_h

// Annotations includes
#include "vtkSlicerAnnotationsModuleVTKWidgetsExport.h"

// VTK includes
#include <vtkDistanceWidget.h>

/// \ingroup Slicer_QtModules_Annotation
class VTK_SLICER_ANNOTATIONS_MODULE_VTKWIDGETS_EXPORT vtkAnnotationRulerWidget
  : public vtkDistanceWidget
{
public:

  static vtkAnnotationRulerWidget *New();
  vtkTypeRevisionMacro(vtkAnnotationRulerWidget, vtkDistanceWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void CreateDefaultRepresentation();

  /// Return True if the widget will build its 2D representation
  bool GetIs2DWidget();

  /// Set the widget mode. By default, the widget will build its 2D representation
  void SetIs2DWidget(int value);

protected:

  vtkAnnotationRulerWidget();
  virtual ~vtkAnnotationRulerWidget();

  bool Is2DWidget;

private:

  vtkAnnotationRulerWidget(const vtkAnnotationRulerWidget&); /// Not implemented
  void operator=(const vtkAnnotationRulerWidget&); /// Not Implemented

};

#endif /* __vtkAnnotationRulerWidget_h */
