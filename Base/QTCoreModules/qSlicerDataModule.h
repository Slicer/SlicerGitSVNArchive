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
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

#ifndef __qSlicerDataModule_h
#define __qSlicerDataModule_h

/// CTK includes
#include <ctkPimpl.h>

/// SlicerQt includes
#include "qSlicerCoreModule.h"

#include "qSlicerBaseQTCoreModulesExport.h"

class qSlicerAbstractModuleWidget;
class qSlicerDataModulePrivate;

class Q_SLICER_BASE_QTCOREMODULES_EXPORT qSlicerDataModule :
  public qSlicerCoreModule
{
  Q_OBJECT
public:
  typedef qSlicerCoreModule Superclass;
  qSlicerDataModule(QObject *parent=0);
  virtual ~qSlicerDataModule();

  virtual void setup();

  virtual QIcon icon()const;
  qSlicerGetTitleMacro("Data");

  /// Return help/acknowledgement text
  virtual QString helpText()const;
  virtual QString acknowledgementText()const;

protected:

  ///
  /// Create and return the widget representation associated to this module
  virtual qSlicerAbstractModuleRepresentation * createWidgetRepresentation();

  ///
  /// Create and return the logic associated to this module
  virtual vtkMRMLAbstractLogic* createLogic();

protected:
  QScopedPointer<qSlicerDataModulePrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerDataModule);
  Q_DISABLE_COPY(qSlicerDataModule);
};

#endif
