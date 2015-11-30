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

  This file was originally developed by Kevin Wang, PMH.
  and was partially funded by OCAIRO and Sparkit.

==============================================================================*/

#ifndef __vtkSlicerTablesLogic_h
#define __vtkSlicerTablesLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes

// Tables includes
#include "vtkSlicerTablesModuleLogicExport.h"

class vtkAbstractArray;
class vtkMRMLTableNode;

/// \ingroup Slicer_QtModules_ExtensionTemplate
/// \brief Slicer logic class for double array manipulation
/// This class manages the logic associated with reading, saving,
/// and changing propertied of the double array nodes
class VTK_SLICER_TABLES_MODULE_LOGIC_EXPORT vtkSlicerTablesLogic
  : public vtkSlicerModuleLogic
{
public:

  static vtkSlicerTablesLogic *New();
  vtkTypeMacro(vtkSlicerTablesLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkMRMLTableNode* AddTable(const char* fileName, const char* name = 0);

protected:
  vtkSlicerTablesLogic();
  virtual ~vtkSlicerTablesLogic();

private:
  vtkSlicerTablesLogic(const vtkSlicerTablesLogic&); // Not implemented
  void operator=(const vtkSlicerTablesLogic&);               // Not implemented
};

#endif
