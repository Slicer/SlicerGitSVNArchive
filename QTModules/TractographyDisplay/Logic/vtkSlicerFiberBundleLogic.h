/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkSlicerFiberBundleLogic.h,v $
  Date:      $Date: 2006/01/08 04:48:05 $
  Version:   $Revision: 1.45 $

=========================================================================auto=*/

// .NAME vtkSlicerFiberBundleLogic - slicer logic class for fiber bundle manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the fiber bundles


#ifndef __vtkSlicerFiberBundleLogic_h
#define __vtkSlicerFiberBundleLogic_h

#include "vtkSlicerModuleLogic.h"
#include "vtkSlicerTractographyDisplayModuleLogicExport.h"

// STD includes
#include <stdlib.h>

class vtkMRMLFiberBundleNode;

class VTK_SLICER_TRACTOGRAPHY_DISPLAY_MODULE_LOGIC_EXPORT vtkSlicerFiberBundleLogic
  : public vtkSlicerModuleLogic 
{
  public:
  
  // The Usual vtk class functions
  static vtkSlicerFiberBundleLogic *New();
  vtkTypeRevisionMacro(vtkSlicerFiberBundleLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create new mrml fiber bundle node and read its polydata from a specified file.
  // Also create the logic object for its display.
  vtkMRMLFiberBundleNode* AddFiberBundle (const char* filename, int notifyScene);

  // Description:
  // Create fiber bundle nodes and read their polydata from a specified directory.
  // Files matching suffix are read
  // Internally calls AddFiberBundle for each file.
  int AddFiberBundles (const char* dirname, const char* suffix );
//BTX
  // Description:
  // Create fiber bundle nodes and read their polydata from a specified directory.
  // Files matching all suffixes are read
  // Internally calls AddFiberBundle for each file.
  int AddFiberBundles (const char* dirname, std::vector< std::string > suffix );
//ETX
  // Description:
  // Write fiber bundle's polydata  to a specified file.
  int SaveFiberBundle (const char* filename, vtkMRMLFiberBundleNode *fiberBundleNode);

  // Description:
  // Update logic state when MRML scene changes.
  virtual void ProcessMRMLEvents ( vtkObject * caller, 
                                  unsigned long event, 
                                  void * callData );

  // Description:
  // Get the maximum number of fibers to show by default when a new fiber bundle node is set
  vtkGetMacro ( MaxNumberOfFibersToShowByDefault, vtkIdType );

  // Description:
  // Set the maximum number of fibers to show by default when a new fiber bundle node is set
  vtkSetMacro ( MaxNumberOfFibersToShowByDefault, vtkIdType );

protected:
  vtkSlicerFiberBundleLogic();
  ~vtkSlicerFiberBundleLogic();
  vtkSlicerFiberBundleLogic(const vtkSlicerFiberBundleLogic&);
  void operator=(const vtkSlicerFiberBundleLogic&);


  // Description:
  // Create internal logic objects to manage fiber bundles (currently display).
  void InitializeLogicForFiberBundleNode(vtkMRMLFiberBundleNode *node);

  // Description:
  // Collection of pointers to display logic objects for fiber bundle nodes in the scene.
  vtkCollection *DisplayLogicCollection;

  // Description:
  // Maximum number of fibers to show per bundle when it is loaded.
  vtkIdType MaxNumberOfFibersToShowByDefault;
};

#endif

