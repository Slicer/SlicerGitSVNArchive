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

#include "qSlicerDiffusionTensorVolumeDisplayWidget.h"
#include "ui_qSlicerDiffusionTensorVolumeDisplayWidget.h"

// Qt includes

// MRML includes
#include "vtkMRMLDiffusionTensorVolumeNode.h"
#include "vtkMRMLDiffusionTensorVolumeDisplayNode.h"
#include "vtkMRMLGlyphableVolumeSliceDisplayNode.h"

// VTK includes

// STD includes

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Volumes
class qSlicerDiffusionTensorVolumeDisplayWidgetPrivate
  : public Ui_qSlicerDiffusionTensorVolumeDisplayWidget
{
  Q_DECLARE_PUBLIC(qSlicerDiffusionTensorVolumeDisplayWidget);
protected:
  qSlicerDiffusionTensorVolumeDisplayWidget* const q_ptr;
public:
  qSlicerDiffusionTensorVolumeDisplayWidgetPrivate(qSlicerDiffusionTensorVolumeDisplayWidget& object);
  ~qSlicerDiffusionTensorVolumeDisplayWidgetPrivate();
  void init();
  vtkMRMLDiffusionTensorVolumeNode* VolumeNode;
};

//-----------------------------------------------------------------------------
qSlicerDiffusionTensorVolumeDisplayWidgetPrivate
::qSlicerDiffusionTensorVolumeDisplayWidgetPrivate(
  qSlicerDiffusionTensorVolumeDisplayWidget& object)
  : q_ptr(&object)
{
  this->VolumeNode = 0;
}

//-----------------------------------------------------------------------------
qSlicerDiffusionTensorVolumeDisplayWidgetPrivate
::~qSlicerDiffusionTensorVolumeDisplayWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
void qSlicerDiffusionTensorVolumeDisplayWidgetPrivate::init()
{
  Q_Q(qSlicerDiffusionTensorVolumeDisplayWidget);

  this->setupUi(q);
  this->DTISliceDisplayWidget->setVisibilityHidden(true);

  QObject::connect(this->ScalarInvariantComboBox, SIGNAL(scalarInvariantChanged(int)),
                   q, SLOT(setVolumeScalarInvariant(int)));
  QObject::connect(this->RedSliceCheckBox, SIGNAL(toggled(bool)),
                   q, SLOT(setRedSliceVisible(bool)));
  QObject::connect(this->YellowSliceCheckBox, SIGNAL(toggled(bool)),
                   q, SLOT(setYellowSliceVisible(bool)));
  QObject::connect(this->GreenSliceCheckBox, SIGNAL(toggled(bool)),
                   q, SLOT(setGreenSliceVisible(bool)));
}

// --------------------------------------------------------------------------
qSlicerDiffusionTensorVolumeDisplayWidget
::qSlicerDiffusionTensorVolumeDisplayWidget(QWidget* parentWidget)
  : Superclass(parentWidget)
  , d_ptr(new qSlicerDiffusionTensorVolumeDisplayWidgetPrivate(*this))
{
  Q_D(qSlicerDiffusionTensorVolumeDisplayWidget);
  d->init();

  // disable as there is not MRML Node associated with the widget
  this->setEnabled(false);
}

// --------------------------------------------------------------------------
qSlicerDiffusionTensorVolumeDisplayWidget
::~qSlicerDiffusionTensorVolumeDisplayWidget()
{
}

// --------------------------------------------------------------------------
vtkMRMLDiffusionTensorVolumeNode* qSlicerDiffusionTensorVolumeDisplayWidget
::volumeNode()const
{
  Q_D(const qSlicerDiffusionTensorVolumeDisplayWidget);
  return d->VolumeNode;
}

// --------------------------------------------------------------------------
vtkMRMLDiffusionTensorVolumeDisplayNode* qSlicerDiffusionTensorVolumeDisplayWidget::volumeDisplayNode()const
{
  vtkMRMLDiffusionTensorVolumeNode* volumeNode = this->volumeNode();
  return volumeNode ? vtkMRMLDiffusionTensorVolumeDisplayNode::SafeDownCast(
    volumeNode->GetDisplayNode()) : 0;
}

// --------------------------------------------------------------------------
QList<vtkMRMLGlyphableVolumeSliceDisplayNode*> qSlicerDiffusionTensorVolumeDisplayWidget::sliceDisplayNodes()const
{
  Q_D(const qSlicerDiffusionTensorVolumeDisplayWidget);
  vtkMRMLDiffusionTensorVolumeDisplayNode* displayNode = this->volumeDisplayNode();
  if (!displayNode)
    {
    return QList<vtkMRMLGlyphableVolumeSliceDisplayNode*>();
    }
  QList<vtkMRMLGlyphableVolumeSliceDisplayNode*> res
    = QList<vtkMRMLGlyphableVolumeSliceDisplayNode*>::fromVector(
      QVector<vtkMRMLGlyphableVolumeSliceDisplayNode*>::fromStdVector(
        displayNode->GetSliceGlyphDisplayNodes(d->VolumeNode)));
  return res;
}

// --------------------------------------------------------------------------
void qSlicerDiffusionTensorVolumeDisplayWidget::setMRMLVolumeNode(vtkMRMLNode* node)
{
  this->setMRMLVolumeNode(vtkMRMLDiffusionTensorVolumeNode::SafeDownCast(node));
}

// --------------------------------------------------------------------------
void qSlicerDiffusionTensorVolumeDisplayWidget::setMRMLVolumeNode(vtkMRMLDiffusionTensorVolumeNode* volumeNode)
{
  Q_D(qSlicerDiffusionTensorVolumeDisplayWidget);

  vtkMRMLDiffusionTensorVolumeDisplayNode* oldVolumeDisplayNode = this->volumeDisplayNode();

  qvtkReconnect(oldVolumeDisplayNode, volumeNode ? volumeNode->GetDisplayNode() :0,
                vtkCommand::ModifiedEvent,
                this, SLOT(updateWidgetFromMRML()));
  d->VolumeNode = volumeNode;
  d->ScalarVolumeDisplayWidget->setMRMLVolumeNode(volumeNode);
  vtkMRMLDiffusionTensorVolumeDisplayNode* newVolumeDisplayNode = this->volumeDisplayNode();
  if (newVolumeDisplayNode)
    {
    std::vector< vtkMRMLGlyphableVolumeSliceDisplayNode*> dtiSliceDisplayNodes =
      newVolumeDisplayNode->GetSliceGlyphDisplayNodes(d->VolumeNode);
    if (dtiSliceDisplayNodes.size() == 0)
      {
      newVolumeDisplayNode->AddSliceGlyphDisplayNodes(d->VolumeNode);
      dtiSliceDisplayNodes =
        newVolumeDisplayNode->GetSliceGlyphDisplayNodes(d->VolumeNode);
      }
    Q_ASSERT(dtiSliceDisplayNodes.size());
    d->DTISliceDisplayWidget->setMRMLDTISliceDisplayNode(dtiSliceDisplayNodes[0]);
    qvtkDisconnect(0, vtkCommand::ModifiedEvent, this, SLOT(synchronizeSliceDisplayNodes()));
    qvtkConnect(dtiSliceDisplayNodes[0], vtkCommand::ModifiedEvent,
                this, SLOT(synchronizeSliceDisplayNodes()));
    this->synchronizeSliceDisplayNodes();
    }
  this->updateWidgetFromMRML();
}

// --------------------------------------------------------------------------
void qSlicerDiffusionTensorVolumeDisplayWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerDiffusionTensorVolumeDisplayWidget);
  this->setEnabled(d->VolumeNode != 0);
  vtkMRMLDiffusionTensorVolumeDisplayNode* displayNode =
    this->volumeDisplayNode();
  if (!displayNode)
    {
    return;
    }
  d->ScalarInvariantComboBox->setScalarInvariant(displayNode->GetScalarInvariant());
  d->ScalarVolumeDisplayWidget->setColorTableComboBoxEnabled(
    displayNode->GetScalarInvariant() != vtkMRMLDiffusionTensorDisplayPropertiesNode::ColorOrientation &&
    displayNode->GetScalarInvariant() != vtkMRMLDiffusionTensorDisplayPropertiesNode::ColorOrientationMiddleEigenvector &&
    displayNode->GetScalarInvariant() != vtkMRMLDiffusionTensorDisplayPropertiesNode::ColorOrientationMinEigenvector);
}

// --------------------------------------------------------------------------
void qSlicerDiffusionTensorVolumeDisplayWidget::synchronizeSliceDisplayNodes()
{
  QList<vtkMRMLGlyphableVolumeSliceDisplayNode*> sliceDisplayNodes = this->sliceDisplayNodes();
  if (sliceDisplayNodes.count() != 3)
    {
    return;
    }
  sliceDisplayNodes[1]->SetColorMode(sliceDisplayNodes[0]->GetColorMode());
  sliceDisplayNodes[1]->SetOpacity(sliceDisplayNodes[0]->GetOpacity());
  sliceDisplayNodes[1]->SetAndObserveColorNodeID(sliceDisplayNodes[0]->GetColorNodeID());
  sliceDisplayNodes[1]->SetAutoScalarRange(sliceDisplayNodes[0]->GetAutoScalarRange());
  sliceDisplayNodes[1]->SetScalarRange(sliceDisplayNodes[0]->GetScalarRange()[0],
                                       sliceDisplayNodes[0]->GetScalarRange()[1]);
  sliceDisplayNodes[2]->SetColorMode(sliceDisplayNodes[0]->GetColorMode());
  sliceDisplayNodes[2]->SetOpacity(sliceDisplayNodes[0]->GetOpacity());
  sliceDisplayNodes[2]->SetAndObserveColorNodeID(sliceDisplayNodes[0]->GetColorNodeID());
  sliceDisplayNodes[2]->SetAutoScalarRange(sliceDisplayNodes[0]->GetAutoScalarRange());
  sliceDisplayNodes[2]->SetScalarRange(sliceDisplayNodes[0]->GetScalarRange()[0],
                                       sliceDisplayNodes[0]->GetScalarRange()[1]);
}

//----------------------------------------------------------------------------
void qSlicerDiffusionTensorVolumeDisplayWidget::setVolumeScalarInvariant(int scalarInvariant)
{
  vtkMRMLDiffusionTensorVolumeDisplayNode* volumeDisplayNode = this->volumeDisplayNode();
  if (!volumeDisplayNode)
    {
    return;
    }
  volumeDisplayNode->SetScalarInvariant(scalarInvariant);
}

//----------------------------------------------------------------------------
void qSlicerDiffusionTensorVolumeDisplayWidget::setRedSliceVisible(bool visible)
{
  QList<vtkMRMLGlyphableVolumeSliceDisplayNode*> sliceDisplayNodes = this->sliceDisplayNodes();
  if (sliceDisplayNodes.count() != 3)
    {
    return;
    }
  sliceDisplayNodes[0]->SetVisibility(visible);
}

//----------------------------------------------------------------------------
void qSlicerDiffusionTensorVolumeDisplayWidget::setYellowSliceVisible(bool visible)
{
  QList<vtkMRMLGlyphableVolumeSliceDisplayNode*> sliceDisplayNodes = this->sliceDisplayNodes();
  if (sliceDisplayNodes.count() != 3)
    {
    return;
    }
  sliceDisplayNodes[1]->SetVisibility(visible);
}

//----------------------------------------------------------------------------
void qSlicerDiffusionTensorVolumeDisplayWidget::setGreenSliceVisible(bool visible)
{
  QList<vtkMRMLGlyphableVolumeSliceDisplayNode*> sliceDisplayNodes = this->sliceDisplayNodes();
  if (sliceDisplayNodes.count() != 3)
    {
    return;
    }
  sliceDisplayNodes[2]->SetVisibility(visible);
}
