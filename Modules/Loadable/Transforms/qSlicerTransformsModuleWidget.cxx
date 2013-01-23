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

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// Qt includes
#include <QFileDialog>

// SlicerQt includes
#include "qSlicerTransformsModuleWidget.h"
#include "ui_qSlicerTransformsModuleWidget.h"
//#include "qSlicerApplication.h"
//#include "qSlicerIOManager.h"

// vtkSlicerLogic includes
#include "vtkSlicerTransformLogic.h"

// MRMLWidgets includes
#include <qMRMLUtils.h>

// MRML includes
#include "vtkMRMLLinearTransformNode.h"

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkTransform.h>

//-----------------------------------------------------------------------------
class qSlicerTransformsModuleWidgetPrivate: public Ui_qSlicerTransformsModuleWidget
{
  Q_DECLARE_PUBLIC(qSlicerTransformsModuleWidget);
protected:
  qSlicerTransformsModuleWidget* const q_ptr;
public:
  qSlicerTransformsModuleWidgetPrivate(qSlicerTransformsModuleWidget& object);
  vtkSlicerTransformLogic*      logic()const;
  QButtonGroup*                 CoordinateReferenceButtonGroup;
  vtkMRMLLinearTransformNode*   MRMLTransformNode;
};

//-----------------------------------------------------------------------------
qSlicerTransformsModuleWidgetPrivate::qSlicerTransformsModuleWidgetPrivate(qSlicerTransformsModuleWidget& object)
  : q_ptr(&object)
{
  this->CoordinateReferenceButtonGroup = 0;
  this->MRMLTransformNode = 0;
}
//-----------------------------------------------------------------------------
vtkSlicerTransformLogic* qSlicerTransformsModuleWidgetPrivate::logic()const
{
  Q_Q(const qSlicerTransformsModuleWidget);
  return vtkSlicerTransformLogic::SafeDownCast(q->logic());
}

//-----------------------------------------------------------------------------
qSlicerTransformsModuleWidget::qSlicerTransformsModuleWidget(QWidget* _parentWidget)
  : Superclass(_parentWidget)
  , d_ptr(new qSlicerTransformsModuleWidgetPrivate(*this))
{
}

//-----------------------------------------------------------------------------
qSlicerTransformsModuleWidget::~qSlicerTransformsModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerTransformsModuleWidget::setup()
{
  Q_D(qSlicerTransformsModuleWidget);
  d->setupUi(this);

  // Add coordinate reference button to a button group
  d->CoordinateReferenceButtonGroup =
    new QButtonGroup(d->CoordinateReferenceGroupBox);
  d->CoordinateReferenceButtonGroup->addButton(
    d->GlobalRadioButton, qMRMLTransformSliders::GLOBAL);
  d->CoordinateReferenceButtonGroup->addButton(
    d->LocalRadioButton, qMRMLTransformSliders::LOCAL);

  // Connect button group
  this->connect(d->CoordinateReferenceButtonGroup,
                SIGNAL(buttonPressed(int)),
                SLOT(onCoordinateReferenceButtonPressed(int)));

  // Connect identity button
  this->connect(d->IdentityPushButton,
                SIGNAL(clicked()),
                SLOT(identity()));

  // Connect revert button
  this->connect(d->InvertPushButton,
                SIGNAL(clicked()),
                SLOT(invert()));

  // Connect node selector with module itself
  this->connect(d->TransformNodeSelector,
                SIGNAL(currentNodeChanged(vtkMRMLNode*)),
                SLOT(onNodeSelected(vtkMRMLNode*)));

  // Connect minimum and maximum from the translation sliders to the matrix
  this->connect(d->TranslationSliders,
               SIGNAL(rangeChanged(double,double)),
               SLOT(onTranslationRangeChanged(double,double)));

  // Notify the matrix of the current translation min/max values
  this->onTranslationRangeChanged(d->TranslationSliders->minimum(),
                                  d->TranslationSliders->maximum());

  // Transform nodes connection
  this->connect(d->TransformToolButton, SIGNAL(clicked()),
                SLOT(transformSelectedNodes()));
  this->connect(d->UntransformToolButton, SIGNAL(clicked()),
                SLOT(untransformSelectedNodes()));

  // Icons
  QIcon rightIcon =
    QApplication::style()->standardIcon(QStyle::SP_ArrowRight);
  d->TransformToolButton->setIcon(rightIcon);

  QIcon leftIcon =
    QApplication::style()->standardIcon(QStyle::SP_ArrowLeft);
  d->UntransformToolButton->setIcon(leftIcon);

  this->onNodeSelected(0);
}

//-----------------------------------------------------------------------------
void qSlicerTransformsModuleWidget::onCoordinateReferenceButtonPressed(int id)
{
  Q_D(qSlicerTransformsModuleWidget);
  
  qMRMLTransformSliders::CoordinateReferenceType ref =
    (id == qMRMLTransformSliders::GLOBAL) ? qMRMLTransformSliders::GLOBAL : qMRMLTransformSliders::LOCAL;
  d->TranslationSliders->setCoordinateReference(ref);
  d->RotationSliders->setCoordinateReference(ref);
}

//-----------------------------------------------------------------------------
void qSlicerTransformsModuleWidget::onNodeSelected(vtkMRMLNode* node)
{
  Q_D(qSlicerTransformsModuleWidget);
  
  vtkMRMLLinearTransformNode* transformNode = vtkMRMLLinearTransformNode::SafeDownCast(node);

  // Enable/Disable CoordinateReference, identity buttons, MatrixViewGroupBox,
  // Min/Max translation inputs
  d->CoordinateReferenceGroupBox->setEnabled(transformNode != 0);
  d->IdentityPushButton->setEnabled(transformNode != 0);
  d->InvertPushButton->setEnabled(transformNode != 0);
  d->MatrixViewGroupBox->setEnabled(transformNode != 0);

  QStringList nodeTypes;
  // If no transform node, it would show the entire scene, lets shown none
  // instead.
  if (transformNode == 0)
    {
    nodeTypes << QString("vtkMRMLNotANode");
    }
  d->TransformedTreeView->setNodeTypes(nodeTypes);

  // Filter the current node in the transformed tree view
  d->TransformedTreeView->setRootNode(transformNode);

  // Hide the current node in the transformable tree view
  QStringList hiddenNodeIDs;
  if (transformNode)
    {
    hiddenNodeIDs << QString(transformNode->GetID());
    }
  d->TransformableTreeView->sortFilterProxyModel()
    ->setHiddenNodeIDs(hiddenNodeIDs);
  d->MRMLTransformNode = transformNode;
}

//-----------------------------------------------------------------------------
void qSlicerTransformsModuleWidget::identity()
{
  Q_D(qSlicerTransformsModuleWidget);

  if (!d->MRMLTransformNode)
    {
    return;
    }

  d->RotationSliders->resetUnactiveSliders();
  d->MRMLTransformNode->GetMatrixTransformToParent()->Identity();
}

//-----------------------------------------------------------------------------
void qSlicerTransformsModuleWidget::invert()
{
  Q_D(qSlicerTransformsModuleWidget);
  
  if (!d->MRMLTransformNode) { return; }

  d->RotationSliders->resetUnactiveSliders();
  d->MRMLTransformNode->GetMatrixTransformToParent()->Invert();
}

//-----------------------------------------------------------------------------
void qSlicerTransformsModuleWidget::onTranslationRangeChanged(double newMin,
                                                              double newMax)
{
  Q_D(qSlicerTransformsModuleWidget);
  d->MatrixWidget->setRange(newMin, newMax);
}

//-----------------------------------------------------------------------------
int qSlicerTransformsModuleWidget::coordinateReference()const
{
  Q_D(const qSlicerTransformsModuleWidget);
  return d->CoordinateReferenceButtonGroup->checkedId();
}

//-----------------------------------------------------------------------------
void qSlicerTransformsModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerTransformsModuleWidget);
  this->Superclass::setMRMLScene(scene);
  // If the root index is set before the scene, it will show the scene as
  // top-level item. Setting the root index to be the scene makes the nodes
  // top-level, and this can only be done once the scene is set.
  d->TransformableTreeView->setRootIndex(
    d->TransformableTreeView->sortFilterProxyModel()->mrmlSceneIndex());
}

//-----------------------------------------------------------------------------
void qSlicerTransformsModuleWidget::transformSelectedNodes()
{
  Q_D(qSlicerTransformsModuleWidget);
  QModelIndexList selectedIndexes =
    d->TransformableTreeView->selectionModel()->selectedRows();
  selectedIndexes = qMRMLTreeView::removeChildren(selectedIndexes);
  // Applying the transform can't be done in the model index loop because
  // setting the transform invalidates the model indexes.
  QList<vtkSmartPointer<vtkMRMLTransformableNode> > nodesToTransform;
  foreach(QModelIndex selectedIndex, selectedIndexes)
    {
    vtkMRMLTransformableNode* node = vtkMRMLTransformableNode::SafeDownCast(
    d->TransformableTreeView->sortFilterProxyModel()->
      mrmlNodeFromIndex( selectedIndex ));
    Q_ASSERT(node);
    nodesToTransform << node;
    }
  foreach(vtkSmartPointer<vtkMRMLTransformableNode> node, nodesToTransform)
    {
    node->SetAndObserveTransformNodeID(d->MRMLTransformNode->GetID());
    }
}

//-----------------------------------------------------------------------------
void qSlicerTransformsModuleWidget::untransformSelectedNodes()
{
  Q_D(qSlicerTransformsModuleWidget);
  QModelIndexList selectedIndexes =
    d->TransformedTreeView->selectionModel()->selectedRows();
  selectedIndexes = qMRMLTreeView::removeChildren(selectedIndexes);
  foreach(QModelIndex selectedIndex, selectedIndexes)
    {
    vtkMRMLTransformableNode* node = vtkMRMLTransformableNode::SafeDownCast(
    d->TransformedTreeView->sortFilterProxyModel()->
      mrmlNodeFromIndex( selectedIndex ));
    Q_ASSERT(node);
    node->SetAndObserveTransformNodeID(0);
    }
}
