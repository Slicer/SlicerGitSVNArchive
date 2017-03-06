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

#ifndef __qMRMLTransformItemDelegate_h
#define __qMRMLTransformItemDelegate_h

// Qt includes
#include <QStyledItemDelegate>

// SubjectHierarchy includes
#include "qSlicerSubjectHierarchyModuleWidgetsExport.h"

class vtkMRMLScene;

/// \brief Item Delegate for MRML parent transform property
class Q_SLICER_MODULE_SUBJECTHIERARCHY_WIDGETS_EXPORT qMRMLTransformItemDelegate: public QStyledItemDelegate
{
  Q_OBJECT
public:
  qMRMLTransformItemDelegate(QObject *parent = 0);
  virtual ~qMRMLTransformItemDelegate();

  void setMRMLScene(vtkMRMLScene* scene);

  /// Determine if the current index contains a transform
  bool isTransform(const QModelIndex& index)const;

  virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
    const QModelIndex &index) const;

  virtual void setEditorData(QWidget *editor, const QModelIndex &index) const;
  virtual void setModelData(QWidget *editor, QAbstractItemModel *model,
                    const QModelIndex &index) const;

  virtual QSize sizeHint(const QStyleOptionViewItem &option,
                         const QModelIndex &index) const;

  void updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &index) const;

  virtual bool eventFilter(QObject *object, QEvent *event);

  /// Set a fixed row height. Useful if uniform row heights is turned on, but the
  /// desired row height is different than that of the first row (often scene).
  /// Set value to -1 to disable fixed row height (this is the default)
  void setFixedRowHeight(int height);

  // We make initStyleOption public so it can be used by the tree view
  using QStyledItemDelegate::initStyleOption;

signals:
  void removeTransformsFromBranchOfCurrentItem();
  void hardenTransformOnBranchOfCurrentItem();

protected slots:
  void commitAndClose();

protected:
  vtkMRMLScene* MRMLScene;
  QAction* RemoveTransformAction;
  QAction* HardenAction;
  int FixedRowHeight;
};

#endif
