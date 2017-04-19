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

// SlicerQt includes
#include "qSlicerDataModuleWidget.h"
#include "ui_qSlicerDataModuleWidget.h"
#include "qSlicerApplication.h"
#include "qSlicerIOManager.h"

// Data Logic includes
#include "vtkSlicerDataModuleLogic.h"

// Subject Hierarchy includes
#include "qMRMLSubjectHierarchyModel.h"
#include "qMRMLSortFilterSubjectHierarchyProxyModel.h"
#include "qSlicerSubjectHierarchyAbstractPlugin.h"
#include "qSlicerSubjectHierarchyPluginHandler.h"

// MRMLWidgets includes
#include <qMRMLSceneModel.h>

// MRML includes
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLSubjectHierarchyNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkCallbackCommand.h>

// Qt includes
#include <QSettings>
#include <QDebug>

//-----------------------------------------------------------------------------
const int qSlicerDataModuleWidget::TAB_INDEX_SUBJECT = 0;
const int qSlicerDataModuleWidget::TAB_INDEX_TRANSFORM = 1;

//-----------------------------------------------------------------------------
class qSlicerDataModuleWidgetPrivate: public Ui_qSlicerDataModuleWidget
{
  Q_DECLARE_PUBLIC(qSlicerDataModuleWidget);
protected:
  qSlicerDataModuleWidget* const q_ptr;
public:
  qSlicerDataModuleWidgetPrivate(qSlicerDataModuleWidget& object);
  vtkSlicerDataModuleLogic* logic() const;
  void showContextMenuHint();
public:
  QAction* HardenTransformAction;

  /// Callback object to get notified about item modified events
  vtkSmartPointer<vtkCallbackCommand> CallBack;
};

//-----------------------------------------------------------------------------
// qSlicerDataModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerDataModuleWidgetPrivate::qSlicerDataModuleWidgetPrivate(qSlicerDataModuleWidget& object)
  : q_ptr(&object)
  , HardenTransformAction(NULL)
{
  this->CallBack = vtkSmartPointer<vtkCallbackCommand>::New();
  this->CallBack->SetClientData(q_ptr);
  this->CallBack->SetCallback(qSlicerDataModuleWidget::onSubjectHierarchyItemEvent);
}

//-----------------------------------------------------------------------------
vtkSlicerDataModuleLogic*
qSlicerDataModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerDataModuleWidget);
  return vtkSlicerDataModuleLogic::SafeDownCast(q->logic());
}

//-----------------------------------------------------------------------------
void qSlicerDataModuleWidgetPrivate::showContextMenuHint()
{
  // Show hint to user about context menus if has not been shown before
  QSettings* settings = qSlicerApplication::application()->settingsDialog()->settings();
  if ( !settings->contains("SubjectHierarchy/ContextMenusHintShown")
    || settings->value("SubjectHierarchy/ContextMenusHintShown").toInt() < 2 )
    {
    if (this->SubjectHierarchyTreeView->showContextMenuHint(settings->contains("SubjectHierarchy/ContextMenusHintShown")))
      {
      settings->setValue("SubjectHierarchy/ContextMenusHintShown", settings->value("SubjectHierarchy/ContextMenusHintShown").toInt() + 1);
      }
    }
}

//-----------------------------------------------------------------------------
// qSlicerDataModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerDataModuleWidget::qSlicerDataModuleWidget(QWidget* parentWidget)
  :qSlicerAbstractModuleWidget(parentWidget)
  , d_ptr( new qSlicerDataModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerDataModuleWidget::~qSlicerDataModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerDataModuleWidget::enter()
{
  Q_D(qSlicerDataModuleWidget);

  // Trigger showing the subject hierarchy context menu hint
  this->onCurrentTabChanged(d->ViewTabWidget->currentIndex());

  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerDataModuleWidget::setup()
{
  Q_D(qSlicerDataModuleWidget);

  d->setupUi(this);

  // Tab widget
  d->ViewTabWidget->widget(TAB_INDEX_SUBJECT)->layout()->setContentsMargins(2,2,2,2);
  d->ViewTabWidget->widget(TAB_INDEX_SUBJECT)->layout()->setSpacing(4);

  d->ViewTabWidget->widget(TAB_INDEX_TRANSFORM)->layout()->setContentsMargins(2,2,2,2);
  d->ViewTabWidget->widget(TAB_INDEX_TRANSFORM)->layout()->setSpacing(4);

  connect( d->ViewTabWidget, SIGNAL(currentChanged(int)),
          this, SLOT(onCurrentTabChanged(int)) );

  //
  // Subject tab
  // Make connections for the checkboxes and buttons
  connect( d->DisplayDataNodeIDsCheckBox, SIGNAL(toggled(bool)),
    this, SLOT(setMRMLIDsVisible(bool)) );
  connect( d->DisplayTransformsCheckBox, SIGNAL(toggled(bool)),
    this, SLOT(setTransformsVisible(bool)) );

  // Set up tree view
  qMRMLSubjectHierarchyModel* sceneModel = (qMRMLSubjectHierarchyModel*)d->SubjectHierarchyTreeView->model();
  d->SubjectHierarchyTreeView->expandToDepth(4);
  d->SubjectHierarchyTreeView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
  d->SubjectHierarchyTreeView->header()->resizeSection(sceneModel->transformColumn(), 60);
  // Make subject hierarchy item info label text selectable
  d->SubjectHierarchyItemInfoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

  connect(d->SubjectHierarchyTreeView, SIGNAL(currentItemChanged(vtkIdType)),
    this, SLOT(setDataNodeFromSubjectHierarchyItem(vtkIdType)) );
  connect(d->SubjectHierarchyTreeView, SIGNAL(currentItemChanged(vtkIdType)),
    this, SLOT(setInfoLabelFromSubjectHierarchyItem(vtkIdType)) );

  // Connect name filter
  connect( d->FilterLineEdit, SIGNAL(textChanged(QString)),
    d->SubjectHierarchyTreeView->sortFilterProxyModel(), SLOT(setNameFilter(QString)) );

  // Assemble help text for question mark tooltip
  QString aggregatedHelpText(
    "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">"
    "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\"> p, li { white-space: pre-wrap; }"
    "</style></head><body style=\" font-family:'MS Shell Dlg 2'; font-size:8.25pt; font-weight:400; font-style:normal;\">");
  foreach (qSlicerSubjectHierarchyAbstractPlugin* plugin, qSlicerSubjectHierarchyPluginHandler::instance()->allPlugins())
    {
    // Add help text from each plugin
    QString pluginHelpText = plugin->helpText();
    if (!pluginHelpText.isEmpty())
      {
      aggregatedHelpText.append(QString("\n") + pluginHelpText);
      }
    }
  aggregatedHelpText.append(QString("</body></html>"));
  d->label_Help->setToolTip(aggregatedHelpText);

  //
  // Transform tab

  // Edit properties...
  connect( d->MRMLTreeView, SIGNAL(editNodeRequested(vtkMRMLNode*)),
           qSlicerApplication::application(), SLOT(openNodeModule(vtkMRMLNode*)) );
  // Insert transform
  QAction* insertTransformAction = new QAction(tr("Insert transform"),this);
  d->MRMLTreeView->prependNodeMenuAction(insertTransformAction);
  d->MRMLTreeView->prependSceneMenuAction(insertTransformAction);
  connect( insertTransformAction, SIGNAL(triggered()),
    this, SLOT(insertTransformNode()) );
  // Harden transform
  connect( d->MRMLTreeView, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(onCurrentNodeChanged(vtkMRMLNode*)) );
  d->HardenTransformAction = new QAction(tr("Harden transform"), this);
  connect( d->HardenTransformAction, SIGNAL(triggered()),
           this, SLOT(hardenTransformOnCurrentNode()) );

  connect( d->DisplayMRMLIDsCheckBox, SIGNAL(toggled(bool)),
          this, SLOT(setMRMLIDsVisible(bool)) );
  connect( d->ShowHiddenCheckBox, SIGNAL(toggled(bool)),
          d->MRMLTreeView->sortFilterProxyModel(), SLOT(setShowHidden(bool)) );

  // Filter on all the columns
  d->MRMLTreeView->sortFilterProxyModel()->setFilterKeyColumn(-1);
  connect( d->FilterLineEdit, SIGNAL(textChanged(QString)),
          d->MRMLTreeView->sortFilterProxyModel(), SLOT(setFilterWildcard(QString)) );

  // Make connections for the attribute table widget
  connect( d->MRMLTreeView, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
          d->MRMLNodeAttributeTableWidget, SLOT(setMRMLNode(vtkMRMLNode*)) );
}

//-----------------------------------------------------------------------------
void qSlicerDataModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerDataModuleWidget);

  Superclass::setMRMLScene(scene);

  this->setMRMLIDsVisible(d->DisplayMRMLIDsCheckBox->isChecked());
  this->setTransformsVisible(d->DisplayTransformsCheckBox->isChecked());
}

//-----------------------------------------------------------------------------
void qSlicerDataModuleWidget::setMRMLIDsVisible(bool visible)
{
  Q_D(qSlicerDataModuleWidget);

  // Subject view
  d->SubjectHierarchyTreeView->setColumnHidden(d->SubjectHierarchyTreeView->model()->idColumn(), !visible);

  // Transform view
  d->MRMLTreeView->setColumnHidden(d->MRMLTreeView->sceneModel()->idColumn(), !visible);
  const int columnCount = d->MRMLTreeView->header()->count();
  for(int i = 0; i < columnCount; ++i)
    {
    d->MRMLTreeView->resizeColumnToContents(i);
    }

  // Update both checkboxes that represent the same thing
  bool wereSignalsBlocked = d->DisplayMRMLIDsCheckBox->blockSignals(true);
  d->DisplayMRMLIDsCheckBox->setChecked(visible);
  d->DisplayMRMLIDsCheckBox->blockSignals(wereSignalsBlocked);

  wereSignalsBlocked = d->DisplayDataNodeIDsCheckBox->blockSignals(true);
  d->DisplayDataNodeIDsCheckBox->setChecked(visible);
  d->DisplayDataNodeIDsCheckBox->blockSignals(wereSignalsBlocked);

}

//-----------------------------------------------------------------------------
void qSlicerDataModuleWidget::onCurrentTabChanged(int tabIndex)
{
  Q_D(qSlicerDataModuleWidget);

  if (tabIndex == TAB_INDEX_SUBJECT)
    {
    // Prevent the taller widget affect the size of the other
    d->MRMLTreeView->setVisible(false);
    d->SubjectHierarchyTreeView->setVisible(true);

    // Make sure MRML node attribute widget is updated
    this->setDataNodeFromSubjectHierarchyItem(d->SubjectHierarchyTreeView->currentItem());

    // Show context menu hint if applicable
    d->showContextMenuHint();
    }
  else if (tabIndex == TAB_INDEX_TRANSFORM)
    {
    // Prevent the taller widget affect the size of the other
    d->MRMLTreeView->setVisible(true);
    d->SubjectHierarchyTreeView->setVisible(false);

    // MRML node attribute widget always enabled in transform mode
    d->MRMLNodeAttributeTableWidget->setEnabled(true);
    // Make sure MRML node attribute widget is updated
    d->MRMLNodeAttributeTableWidget->setMRMLNode(d->MRMLTreeView->currentNode());
    }
}

//-----------------------------------------------------------------------------
void qSlicerDataModuleWidget::onCurrentNodeChanged(vtkMRMLNode* newCurrentNode)
{
  Q_D(qSlicerDataModuleWidget);
  vtkMRMLTransformableNode* transformableNode =
    vtkMRMLTransformableNode::SafeDownCast(newCurrentNode);
  vtkMRMLTransformNode* transformNode =
    transformableNode ? transformableNode->GetParentTransformNode() : 0;
  if (transformNode &&
      (transformNode->CanApplyNonLinearTransforms() ||
      transformNode->IsTransformToWorldLinear()))
    {
    d->MRMLTreeView->prependNodeMenuAction(d->HardenTransformAction);
    }
  else
    {
    d->MRMLTreeView->removeNodeMenuAction(d->HardenTransformAction);
    }
}

//-----------------------------------------------------------------------------
void qSlicerDataModuleWidget::insertTransformNode()
{
  Q_D(qSlicerDataModuleWidget);
  vtkNew<vtkMRMLLinearTransformNode> linearTransform;
  this->mrmlScene()->AddNode(linearTransform.GetPointer());

  vtkMRMLNode* parent = vtkMRMLTransformNode::SafeDownCast(
    d->MRMLTreeView->currentNode());
  if (parent)
    {
    linearTransform->SetAndObserveTransformNodeID( parent->GetID() );
    }
}

//-----------------------------------------------------------------------------
void qSlicerDataModuleWidget::hardenTransformOnCurrentNode()
{
  Q_D(qSlicerDataModuleWidget);
  vtkMRMLNode* node = d->MRMLTreeView->currentNode();
  vtkMRMLTransformableNode* transformableNode = vtkMRMLTransformableNode::SafeDownCast(node);
  if (transformableNode)
    {
    transformableNode->HardenTransform();
    }
}

//-----------------------------------------------------------------------------
void qSlicerDataModuleWidget::setTransformsVisible(bool visible)
{
  Q_D(qSlicerDataModuleWidget);

  qMRMLSubjectHierarchyModel* model = qobject_cast<qMRMLSubjectHierarchyModel*>(d->SubjectHierarchyTreeView->model());
  d->SubjectHierarchyTreeView->setColumnHidden(model->transformColumn(), !visible);
  d->SubjectHierarchyTreeView->header()->resizeSection(model->transformColumn(), 60);

  bool wereSignalsBlocked = d->DisplayTransformsCheckBox->blockSignals(true);
  d->DisplayTransformsCheckBox->setChecked(visible);
  d->DisplayTransformsCheckBox->blockSignals(wereSignalsBlocked);
}

//-----------------------------------------------------------------------------
void qSlicerDataModuleWidget::setDataNodeFromSubjectHierarchyItem(vtkIdType itemID)
{
  Q_D(qSlicerDataModuleWidget);

  vtkMRMLSubjectHierarchyNode* shNode = d->SubjectHierarchyTreeView->subjectHierarchyNode();
  if (!shNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid subject hierarchy";
    return;
    }

  vtkMRMLNode* dataNode = NULL;
  if (itemID != vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    dataNode = shNode->GetItemDataNode(itemID);
    }
  d->MRMLNodeAttributeTableWidget->setEnabled(dataNode);
  d->MRMLNodeAttributeTableWidget->setMRMLNode(dataNode);
}

//-----------------------------------------------------------------------------
void qSlicerDataModuleWidget::setInfoLabelFromSubjectHierarchyItem(vtkIdType itemID)
{
  Q_D(qSlicerDataModuleWidget);

  vtkMRMLSubjectHierarchyNode* shNode = d->SubjectHierarchyTreeView->subjectHierarchyNode();
  if (!shNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid subject hierarchy";
    return;
    }

  if (itemID != vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    std::stringstream infoStream;
    shNode->PrintItem(itemID, infoStream, vtkIndent(0));
    d->SubjectHierarchyItemInfoLabel->setText(QLatin1String(infoStream.str().c_str()));

    // Connect node for updating info label
    if (!shNode->HasObserver(vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemModifiedEvent, d->CallBack))
      {
      shNode->AddObserver(vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemModifiedEvent, d->CallBack, -10.0);
      }
    }
  else
    {
    d->SubjectHierarchyItemInfoLabel->setText("No item selected");
    }

  // Store item ID in the label object
  d->SubjectHierarchyItemInfoLabel->setProperty("itemID", itemID);
}

//-----------------------------------------------------------------------------
void qSlicerDataModuleWidget::onSubjectHierarchyItemEvent(
  vtkObject* caller, unsigned long event, void* clientData, void* callData )
{
  vtkMRMLSubjectHierarchyNode* shNode = reinterpret_cast<vtkMRMLSubjectHierarchyNode*>(caller);
  qSlicerDataModuleWidget* widget = reinterpret_cast<qSlicerDataModuleWidget*>(clientData);
  if (!widget || !shNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid event parameters";
    return;
    }

  // Get item ID
  vtkIdType itemID = vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID;
  if (callData)
    {
    vtkIdType* itemIdPtr = reinterpret_cast<vtkIdType*>(callData);
    if (itemIdPtr)
      {
      itemID = *itemIdPtr;
      }
    }

  switch (event)
    {
    case vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemModifiedEvent:
      widget->onSubjectHierarchyItemModified(itemID);
      break;
    }
}

//------------------------------------------------------------------------------
void qSlicerDataModuleWidget::onSubjectHierarchyItemModified(vtkIdType itemID)
{
  Q_D(qSlicerDataModuleWidget);

  // Get displayed item's ID from label object
  vtkIdType displayedItemID = d->SubjectHierarchyItemInfoLabel->property("itemID").toLongLong();

  // Update label if the displayed item is the one that changed
  if (displayedItemID == itemID && displayedItemID != vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    vtkMRMLSubjectHierarchyNode* shNode = d->SubjectHierarchyTreeView->subjectHierarchyNode();
    if (!shNode)
      {
      qCritical() << Q_FUNC_INFO << ": Invalid subject hierarchy";
      return;
      }

    std::stringstream infoStream;
    shNode->PrintItem(itemID, infoStream, vtkIndent(0));
    d->SubjectHierarchyItemInfoLabel->setText(QLatin1String(infoStream.str().c_str()));
    }
}

//-----------------------------------------------------------------------------
qMRMLSubjectHierarchyModel* qSlicerDataModuleWidget::subjectHierarchySceneModel()const
{
  Q_D(const qSlicerDataModuleWidget);

  qMRMLSubjectHierarchyModel* model = qobject_cast<qMRMLSubjectHierarchyModel*>(d->SubjectHierarchyTreeView->model());
  return model;
}
