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

#ifndef __qMRMLSortFilterSubjectHierarchyProxyModel_h
#define __qMRMLSortFilterSubjectHierarchyProxyModel_h

// qMRML includes
#include "qMRMLWidgetsExport.h"

// Qt includes
#include <QSortFilterProxyModel>

// CTK includes
#include <ctkVTKObject.h>

// MRML includes
#include <vtkMRMLSubjectHierarchyNode.h>

class qMRMLSortFilterSubjectHierarchyProxyModelPrivate;

/// \ingroup Slicer_QtModules_SubjectHierarchy
class QMRML_WIDGETS_EXPORT qMRMLSortFilterSubjectHierarchyProxyModel : public qMRMLSortFilterProxyModel
{
  Q_OBJECT
  QVTK_OBJECT

public:
  typedef qMRMLSortFilterProxyModel Superclass;
  qMRMLSortFilterSubjectHierarchyProxyModel(QObject *parent=0);
  virtual ~qMRMLSortFilterSubjectHierarchyProxyModel();

  Q_INVOKABLE vtkMRMLSubjectHierarchyNode* subjectHierarchyNode()const;
  Q_INVOKABLE vtkMRMLScene* mrmlScene()const;

  void Q_INVOKABLE setNameFilterString(QString nameFilter);

protected:
  /// Returns true if the item in the row indicated by the given sourceRow and
  /// sourceParent should be included in the model; otherwise returns false.
  /// This method test each item via \a filterAcceptsItem
  virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent)const;

  /// Filters items to decide which to display in the view
  virtual bool filterAcceptsItem(vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID itemID)const;

  QStandardItem* sourceItem(const QModelIndex& index)const;

protected:
  QScopedPointer<qMRMLSortFilterSubjectHierarchyProxyModelPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qMRMLSortFilterSubjectHierarchyProxyModel);
  Q_DISABLE_COPY(qMRMLSortFilterSubjectHierarchyProxyModel);
};

#endif
