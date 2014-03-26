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

// Qt includes
#include <QDebug>
#include <QMap>
#include <QMimeData>
#include <QSharedPointer>
#include <QStack>
#include <QStringList>
#include <QVector>

// qMRML includes
#include "qMRMLSceneViewsModel.h"
#include "qMRMLSceneModel.h"
#include "qMRMLUtils.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLNode.h>
#include <vtkMRMLSceneViewNode.h>

// VTK includes
#include <vtkVariantArray.h>
#include <typeinfo>
#include <vtkStdString.h>

//------------------------------------------------------------------------------
qMRMLSceneViewsModel::qMRMLSceneViewsModel(QObject *vparent)
  : Superclass(vparent)
{
  this->setNameColumn(2);
  this->setColumnCount(4);
  this->setHorizontalHeaderLabels(
      QStringList()  << "Preview/Edit" << "Restore" << "Name" << "Description");
}

//------------------------------------------------------------------------------
qMRMLSceneViewsModel::~qMRMLSceneViewsModel()
{
}

//------------------------------------------------------------------------------
void qMRMLSceneViewsModel::updateNodeFromItemData(vtkMRMLNode* node, QStandardItem* item)
{
  this->Superclass::updateNodeFromItemData(node, item);
  if (item->column() == qMRMLSceneViewsModel::DescriptionColumn)
    {
    vtkMRMLSceneViewNode* viewNode = vtkMRMLSceneViewNode::SafeDownCast(node);

    if (viewNode)
      {
      // if we have a snapshot node, the name can be changed by editing the textcolumn
      viewNode->SetSceneViewDescription(item->text().toStdString());
      }
    }
}

//------------------------------------------------------------------------------
void qMRMLSceneViewsModel::updateItemDataFromNode(QStandardItem* item, vtkMRMLNode* node, int column)
{
  this->Superclass::updateItemDataFromNode(item, node, column);

  vtkMRMLSceneViewNode* viewNode = vtkMRMLSceneViewNode::SafeDownCast(node);
  if (!viewNode)
    {
    return;
    }
  switch (column)
    {
    case qMRMLSceneViewsModel::ThumbnailColumn:
      if (viewNode)
        {
        QImage qimage;
        if (viewNode->GetScreenShot())
          {
          qMRMLUtils::vtkImageDataToQImage(viewNode->GetScreenShot(),qimage);
          }
        else
          {
          // std::cout << "view node's screen shot is null" << std::endl;
          qimage = QImage(80,80, QImage::Format_RGB32);
          qimage.fill(0);
          }
        QPixmap screenshot;
        screenshot = QPixmap::fromImage(qimage, Qt::AutoColor);
        item->setData(screenshot.scaled(80,80,Qt::KeepAspectRatio,Qt::SmoothTransformation),Qt::DecorationRole);
        item->setData(QSize(80,80),Qt::SizeHintRole);
        }
      break;
    case qMRMLSceneViewsModel::RestoreColumn:
      item->setData(QPixmap(":/Icons/Restore.png"),Qt::DecorationRole);
      break;
    case qMRMLSceneViewsModel::DescriptionColumn:
      if (viewNode)
        {
        item->setText(QString(viewNode->GetSceneViewDescription()));
        item->setData(QSize(80,80),Qt::SizeHintRole);
        }
      break;
    }
}

//------------------------------------------------------------------------------
QFlags<Qt::ItemFlag> qMRMLSceneViewsModel::nodeFlags(vtkMRMLNode* node, int column)const
{
  QFlags<Qt::ItemFlag> flags = this->Superclass::nodeFlags(node, column);
  switch(column)
    {
    case qMRMLSceneViewsModel::DescriptionColumn:
      flags = flags | Qt::ItemIsEditable;
      break;
    default:
      break;
    }
  return flags;
}
