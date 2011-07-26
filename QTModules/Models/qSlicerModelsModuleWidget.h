/*==============================================================================

  Program: 3D Slicer

  Copyright (c) 2010 Kitware Inc.

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

#ifndef __qSlicerModelsModuleWidget_h
#define __qSlicerModelsModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerModelsModuleExport.h"

class qSlicerModelsModuleWidgetPrivate;
class vtkMRMLNode;
class QModelIndex;

/// \ingroup Slicer_QtModules_Models
class Q_SLICER_QTMODULES_MODELS_EXPORT qSlicerModelsModuleWidget
  : public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:

  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerModelsModuleWidget(QWidget *parent=0);
  virtual ~qSlicerModelsModuleWidget();

public slots:
  void insertHierarchyNode();
  void onCurrentNodeChanged(vtkMRMLNode* newCurrentNode);
  void onExpanded(const QModelIndex &index);
  void onCollapsed(const QModelIndex &index);

protected:
  QScopedPointer<qSlicerModelsModuleWidgetPrivate> d_ptr;
  
  virtual void setup();

private:
  Q_DECLARE_PRIVATE(qSlicerModelsModuleWidget);
  Q_DISABLE_COPY(qSlicerModelsModuleWidget);
};

#endif
