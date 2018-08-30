/*==============================================================================

Program: 3D Slicer

Copyright (c) Kitware Inc.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

This file was originally developed by Julien Finet, Kitware Inc.
and was partially funded by Allen Institute

==============================================================================*/

// .NAME vtkSlicerSplinesLogic - slicer logic class splines manipulation
// .SECTION Description

#ifndef __vtkSlicerSplinesLogic_h
#define __vtkSlicerSplinesLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes
class vtkMRMLMarkupsSplinesNode;
class vtkMRMLSelectionNode;

// STD includes
#include <cstdlib>

#include "vtkSlicerSplinesModuleLogicExport.h"

class VTK_SLICER_SPLINES_MODULE_LOGIC_EXPORT vtkSlicerSplinesLogic :
  public vtkSlicerModuleLogic
{
public:

  static vtkSlicerSplinesLogic *New();
  vtkTypeMacro(vtkSlicerSplinesLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkMRMLSelectionNode* GetSelectionNode() const;

  static bool GetCentroid(vtkMRMLMarkupsSplinesNode* splinesNode, int n, double centroid[3]);

protected:
  vtkSlicerSplinesLogic();
  virtual ~vtkSlicerSplinesLogic();

  virtual void RegisterNodes();
  virtual void ObserveMRMLScene();

private:

  vtkSlicerSplinesLogic(const vtkSlicerSplinesLogic&); // Not implemented
  void operator=(const vtkSlicerSplinesLogic&); // Not implemented
};

#endif
