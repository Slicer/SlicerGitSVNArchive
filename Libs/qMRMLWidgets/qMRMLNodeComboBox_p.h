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

#ifndef __qMRMLNodeComboBox_p_h
#define __qMRMLNodeComboBox_p_h

// CTK includes
#include <ctkPimpl.h>

// qMRML includes
#include "qMRMLNodeComboBox.h"
class QComboBox;
class qMRMLNodeFactory;
class qMRMLSceneModel;

// -----------------------------------------------------------------------------
class qMRMLNodeComboBoxPrivate
{
  Q_DECLARE_PUBLIC(qMRMLNodeComboBox);
protected:
  qMRMLNodeComboBox* const q_ptr;
  virtual void setModel(QAbstractItemModel* model);
public:
  qMRMLNodeComboBoxPrivate(qMRMLNodeComboBox& object);
  virtual void init(QAbstractItemModel* model);

  vtkMRMLNode* mrmlNode(int row)const;
  vtkMRMLNode* mrmlNodeFromIndex(const QModelIndex& index)const;
  QModelIndexList indexesFromMRMLNodeID(const QString& nodeID)const;

  void updateDefaultText();
  void updateNoneItem(bool resetRootIndex = true);
  void updateActionItems(bool resetRootIndex = true);
  void updateDelegate(bool force = false);
  QString nodeTypeLabel()const;

  QComboBox*        ComboBox;
  qMRMLNodeFactory* MRMLNodeFactory;
  qMRMLSceneModel*  MRMLSceneModel;
  bool              SelectNodeUponCreation;
  bool              NoneEnabled;
  bool              AddEnabled;
  bool              RemoveEnabled;
  bool              EditEnabled;
  bool              RenameEnabled;
  
  bool              AutoDefaultText;
};

#endif
