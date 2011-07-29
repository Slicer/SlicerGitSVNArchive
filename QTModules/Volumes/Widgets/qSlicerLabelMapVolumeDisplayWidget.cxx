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

#include "qSlicerLabelMapVolumeDisplayWidget.h"
#include "ui_qSlicerLabelMapVolumeDisplayWidget.h"

// Qt includes

// MRML includes
#include "vtkMRMLColorNode.h"
#include "vtkMRMLLabelMapVolumeDisplayNode.h"
#include "vtkMRMLScalarVolumeNode.h"

// VTK includes

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Volumes
class qSlicerLabelMapVolumeDisplayWidgetPrivate:
                                          public Ui_qSlicerLabelMapVolumeDisplayWidget
{
  Q_DECLARE_PUBLIC(qSlicerLabelMapVolumeDisplayWidget);
protected:
  qSlicerLabelMapVolumeDisplayWidget* const q_ptr;
public:
  qSlicerLabelMapVolumeDisplayWidgetPrivate(qSlicerLabelMapVolumeDisplayWidget& object);
  ~qSlicerLabelMapVolumeDisplayWidgetPrivate();
  void init();

  vtkMRMLScalarVolumeNode* VolumeNode;
};

//-----------------------------------------------------------------------------
qSlicerLabelMapVolumeDisplayWidgetPrivate::qSlicerLabelMapVolumeDisplayWidgetPrivate(qSlicerLabelMapVolumeDisplayWidget& object)
  : q_ptr(&object)
{
  this->VolumeNode = 0;
}

//-----------------------------------------------------------------------------
qSlicerLabelMapVolumeDisplayWidgetPrivate::~qSlicerLabelMapVolumeDisplayWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
void qSlicerLabelMapVolumeDisplayWidgetPrivate::init()
{
  Q_Q(qSlicerLabelMapVolumeDisplayWidget);

  this->setupUi(q);
  QObject::connect(this->ColorTableComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
                   q, SLOT(setColorNode(vtkMRMLNode*)));
  // disable as there is not MRML Node associated with the widget
  q->setEnabled(false);
}

// --------------------------------------------------------------------------
qSlicerLabelMapVolumeDisplayWidget::qSlicerLabelMapVolumeDisplayWidget(QWidget* _parent) : Superclass(_parent)
  , d_ptr(new qSlicerLabelMapVolumeDisplayWidgetPrivate(*this))
{
  Q_D(qSlicerLabelMapVolumeDisplayWidget);
  d->init();
}

// --------------------------------------------------------------------------
qSlicerLabelMapVolumeDisplayWidget::~qSlicerLabelMapVolumeDisplayWidget()
{
}

// --------------------------------------------------------------------------
vtkMRMLScalarVolumeNode* qSlicerLabelMapVolumeDisplayWidget::volumeNode()const
{
  Q_D(const qSlicerLabelMapVolumeDisplayWidget);
  return d->VolumeNode;
}

// --------------------------------------------------------------------------
vtkMRMLLabelMapVolumeDisplayNode* qSlicerLabelMapVolumeDisplayWidget::volumeDisplayNode()const
{
  Q_D(const qSlicerLabelMapVolumeDisplayWidget);
  return d->VolumeNode ? vtkMRMLLabelMapVolumeDisplayNode::SafeDownCast(
    d->VolumeNode->GetDisplayNode()) : 0;
}

// --------------------------------------------------------------------------
void qSlicerLabelMapVolumeDisplayWidget::setMRMLVolumeNode(vtkMRMLNode* node)
{
  this->setMRMLVolumeNode(vtkMRMLScalarVolumeNode::SafeDownCast(node));
}

// --------------------------------------------------------------------------
void qSlicerLabelMapVolumeDisplayWidget::setMRMLVolumeNode(vtkMRMLScalarVolumeNode* volumeNode)
{
  Q_D(qSlicerLabelMapVolumeDisplayWidget);
  vtkMRMLLabelMapVolumeDisplayNode* oldVolumeDisplayNode = this->volumeDisplayNode();

  qvtkReconnect(oldVolumeDisplayNode, volumeNode ? volumeNode->GetDisplayNode() : 0,
                vtkCommand::ModifiedEvent,
                this, SLOT(updateWidgetFromMRML()));
  d->VolumeNode = volumeNode;
  this->setEnabled(volumeNode != 0);
  this->updateWidgetFromMRML();
}

// --------------------------------------------------------------------------
void qSlicerLabelMapVolumeDisplayWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerLabelMapVolumeDisplayWidget);
  vtkMRMLLabelMapVolumeDisplayNode* displayNode =
    this->volumeDisplayNode();
  if (displayNode)
    {
    d->ColorTableComboBox->setCurrentNode(displayNode->GetColorNode());
    }
}

// --------------------------------------------------------------------------
void qSlicerLabelMapVolumeDisplayWidget::setColorNode(vtkMRMLNode* colorNode)
{
  vtkMRMLLabelMapVolumeDisplayNode* displayNode =
    this->volumeDisplayNode();
  if (!displayNode || !colorNode)
    {
    return;
    }
  Q_ASSERT(vtkMRMLColorNode::SafeDownCast(colorNode));
  displayNode->SetAndObserveColorNodeID(colorNode->GetID());
}
