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

#ifndef __qMRMLColorPickerWidget_h
#define __qMRMLColorPickerWidget_h

//QT include
#include <QModelIndex>

// qMRML includes
#include "qMRMLWidget.h"
#include "qMRMLWidgetsExport.h"

// CTK includes
#include <ctkVTKObject.h>

class qMRMLColorPickerWidgetPrivate;
class vtkMRMLNode;
class vtkMRMLColorNode;

/// Given a mrml scene, qMRMLColorPickerWidget allows the selection of
/// a color/label from all the vtkMRMLColorNode in the scene.
class QMRML_WIDGETS_EXPORT qMRMLColorPickerWidget : public qMRMLWidget
{
  Q_OBJECT
  QVTK_OBJECT
public:
  qMRMLColorPickerWidget(QWidget *parent=0);
  virtual ~qMRMLColorPickerWidget();
  
  vtkMRMLColorNode* currentColorNode()const;
  virtual void setMRMLScene(vtkMRMLScene* scene);
  virtual bool eventFilter(QObject* target, QEvent* event);

public slots:
  void setCurrentColorNode(vtkMRMLNode* node);
  void setCurrentColorNodeToDefault();

signals:
  /// Fired wen the current color table node is selected
  void currentColorNodeChanged(vtkMRMLNode* node);

  /// Fired when the user selects a color in the list. index is the selected
  /// color node entry.
  void colorEntrySelected(int index);

  /// Fired when the user selects a color in the list
  void colorSelected(const QColor& color);
protected slots:
  void onNodeAdded(vtkObject*, vtkObject*);
  void onCurrentColorNodeChanged(vtkMRMLNode* node);
  void onTextChanged(const QString& colorText);

protected:
  QScopedPointer<qMRMLColorPickerWidgetPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qMRMLColorPickerWidget);
  Q_DISABLE_COPY(qMRMLColorPickerWidget);
};

#endif
