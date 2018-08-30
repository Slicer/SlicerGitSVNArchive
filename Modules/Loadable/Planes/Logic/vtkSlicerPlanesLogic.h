/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// .NAME vtkSlicerPlanesLogic -
// .SECTION Description

#ifndef __vtkSlicerPlanesLogic_h
#define __vtkSlicerPlanesLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes

// STD includes
#include <cstdlib>

#include "vtkSlicerPlanesModuleLogicExport.h"

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_PLANES_MODULE_LOGIC_EXPORT vtkSlicerPlanesLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerPlanesLogic *New();
  vtkTypeMacro(vtkSlicerPlanesLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkMRMLNode* GetSelectionNode() const;

protected:
  vtkSlicerPlanesLogic();
  virtual ~vtkSlicerPlanesLogic();

  virtual void RegisterNodes();
  virtual void ObserveMRMLScene();

private:
  vtkSlicerPlanesLogic(const vtkSlicerPlanesLogic&); // Not implemented
  void operator=(const vtkSlicerPlanesLogic&); // Not implemented
};

#endif
