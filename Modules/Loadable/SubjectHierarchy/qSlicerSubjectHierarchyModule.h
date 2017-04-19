/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#ifndef __qSlicerSubjectHierarchyModule_h
#define __qSlicerSubjectHierarchyModule_h

// CTK includes
#include "ctkVTKObject.h"

// SlicerQt includes
#include "qSlicerLoadableModule.h"

#include "qSlicerSubjectHierarchyModuleExport.h"

class vtkObject;
class qSlicerSubjectHierarchyModulePrivate;

/// \ingroup Slicer_QtModules_SubjectHierarchy
class Q_SLICER_QTMODULES_SUBJECTHIERARCHY_EXPORT qSlicerSubjectHierarchyModule :
  public qSlicerLoadableModule
{
  Q_OBJECT
  Q_INTERFACES(qSlicerLoadableModule);
  QVTK_OBJECT

public:
  typedef qSlicerLoadableModule Superclass;
  explicit qSlicerSubjectHierarchyModule(QObject *parent=0);
  virtual ~qSlicerSubjectHierarchyModule();

  qSlicerGetTitleMacro(QTMODULE_TITLE);

  /// Help to use the module
  virtual QString helpText()const;

  /// Return acknowledgments
  virtual QString acknowledgementText()const;

  /// Return the authors of the module
  virtual QStringList contributors()const;

  /// Return a custom icon for the module
  virtual QIcon icon()const;

  /// Return the categories for the module
  virtual QStringList categories()const;

  /// Make this module hidden
  virtual bool isHidden()const { return true; };

protected:
  /// Initialize the module. Register the volumes reader/writer
  virtual void setup();

  /// Create and return the logic associated to this module
  virtual vtkMRMLAbstractLogic* createLogic();

  /// Create and return the widget representation associated to this module
  virtual qSlicerAbstractModuleRepresentation* createWidgetRepresentation();

protected slots:
  /// Handles logic modified event, which happens on major changes in the
  /// scene (new scene, batch processing, import etc.)
  void onLogicModified();

protected:
  QScopedPointer<qSlicerSubjectHierarchyModulePrivate> d_ptr; 

private:
  Q_DECLARE_PRIVATE(qSlicerSubjectHierarchyModule);
  Q_DISABLE_COPY(qSlicerSubjectHierarchyModule);
};

#endif
