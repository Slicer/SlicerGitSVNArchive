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

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

#ifndef __vtkSlicerScriptedLoadableModuleLogic_h
#define __vtkSlicerScriptedLoadableModuleLogic_h


// Slicer includes
#include "vtkSlicerModuleLogic.h"

// VTK includes
#include "vtkObject.h"
#include "vtkObjectFactory.h"

class VTK_SLICER_BASE_LOGIC_EXPORT vtkSlicerScriptedLoadableModuleLogic :
  public vtkSlicerModuleLogic
{
public:

  static vtkSlicerScriptedLoadableModuleLogic *New();
  vtkTypeMacro(vtkSlicerScriptedLoadableModuleLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  bool SetPythonSource(const std::string& pythonSource);

protected:

  vtkSlicerScriptedLoadableModuleLogic();
  virtual ~vtkSlicerScriptedLoadableModuleLogic();

//  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);

private:

  vtkSlicerScriptedLoadableModuleLogic(const vtkSlicerScriptedLoadableModuleLogic&); // Not implemented
  void operator=(const vtkSlicerScriptedLoadableModuleLogic&);       // Not implemented

  class vtkInternal;
  vtkInternal * Internal;
};

#endif

