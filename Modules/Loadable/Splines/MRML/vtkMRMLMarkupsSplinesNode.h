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

#ifndef __vtkMRMLMarkupsSplinesNode_h
#define __vtkMRMLMarkupsSplinesNode_h

// MRML includes
#include <vtkMRMLMarkupsNode.h>

// Markups includes
#include "vtkSlicerSplinesModuleMRMLExport.h"

// VTK includes
#include <vtkSmartPointer.h>

class vtkMRMLMarkupsDisplayNode;

//
//
class  VTK_SLICER_SPLINES_MODULE_MRML_EXPORT vtkMRMLMarkupsSplinesNode
  : public vtkMRMLMarkupsNode
{
public:
  static vtkMRMLMarkupsSplinesNode *New();
  vtkTypeMacro(vtkMRMLMarkupsSplinesNode, vtkMRMLMarkupsNode);

  virtual const char* GetIcon() VTK_OVERRIDE { return ":/Icons/SplinesMouseModePlace.png"; }

  //--------------------------------------------------------------------------
  // MRMLNode methods
  //--------------------------------------------------------------------------

  virtual vtkMRMLNode* CreateNodeInstance() VTK_OVERRIDE;
  /// Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() VTK_OVERRIDE { return "MarkupsSpline"; };

  // Return the current spline index which may be modified.
  // -1 means no spline is selected.
  void SetCurrentSpline(int i);
  int GetCurrentSpline() const;

  // Add an empty spline and return its index. The new spline is automatically
  // the current spline.
  int AddSpline(vtkVector3d point);

  /// Create and observe default display node(s)
  virtual void CreateDefaultDisplayNodes() VTK_OVERRIDE;

  void SetNthSplineClosed(int n, bool closed);
  bool GetNthSplineClosed(int n);

protected:
  vtkMRMLMarkupsSplinesNode();
  ~vtkMRMLMarkupsSplinesNode();
  vtkMRMLMarkupsSplinesNode(const vtkMRMLMarkupsSplinesNode&);
  void operator=(const vtkMRMLMarkupsSplinesNode&);

  int CurrentSpline;
  bool DefaultClosed;
  std::vector<bool> Closed;
};

#endif
