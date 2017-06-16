/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

=========================================================================auto=*/

///  vtkSlicerColorLogic - slicer logic class for color manipulation
///
/// This class manages the logic associated with reading, saving,
/// and changing propertied of the colors

#ifndef __vtkSlicerColorLogic_h
#define __vtkSlicerColorLogic_h

// MRMLLogic includes
#include <vtkMRMLColorLogic.h>
#include "vtkSlicerColorsModuleLogicExport.h"

class VTK_SLICER_COLORS_MODULE_LOGIC_EXPORT vtkSlicerColorLogic
  : public vtkMRMLColorLogic
{
  public:

  /// The Usual vtk class functions
  static vtkSlicerColorLogic *New();
  vtkTypeMacro(vtkSlicerColorLogic,vtkMRMLColorLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Return a default color node id for a label map
  virtual const char * GetDefaultLabelMapColorNodeID();

  /// Return a default color node id for the editor
  virtual const char * GetDefaultEditorColorNodeID();

  /// Return a default color node id for the charts
  virtual const char * GetDefaultChartColorNodeID();

  /// look for color files in the Base/Logic/Resources/ColorFiles directory and
  /// put their names in the ColorFiles list. Look in any user defined color
  /// files paths and put them in the UserColorFiles list.
  virtual std::vector<std::string> FindDefaultColorFiles();
  virtual std::vector<std::string> FindUserColorFiles();

protected:
  vtkSlicerColorLogic();
  ~vtkSlicerColorLogic();
  vtkSlicerColorLogic(const vtkSlicerColorLogic&);
  void operator=(const vtkSlicerColorLogic&);
  std::vector<std::string> FindColorFiles(const std::vector<std::string>& directories);
};

#endif

