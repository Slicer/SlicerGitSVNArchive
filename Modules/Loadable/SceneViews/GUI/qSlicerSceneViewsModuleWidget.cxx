#include "GUI/qSlicerSceneViewsModuleWidget.h"
#include "ui_qSlicerSceneViewsModuleWidget.h"

// CTK includes
#include "ctkCollapsibleButton.h"

// QT includes
#include <QButtonGroup>
#include <QList>
#include <QFontMetrics>
#include <QMessageBox>
#include <QTextBrowser>
#include <QFile>
#include <QLineEdit>
#include <QFileDialog>
#include <QPrintDialog>
#include <QPrinter>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QBuffer>
#include <QImageWriter>
#include <QMainWindow>
#include <QStatusBar>

#include "vtkCollection.h"
#include "vtkSmartPointer.h"

// GUI includes
#include "GUI/qSlicerSceneViewsModuleDialog.h"

#include "qSlicerApplication.h"

#include "qMRMLSceneModel.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_SceneViews
class qSlicerSceneViewsModuleWidgetPrivate: public Ui_qSlicerSceneViewsModuleWidget
{
  Q_DECLARE_PUBLIC(qSlicerSceneViewsModuleWidget);
protected:
  qSlicerSceneViewsModuleWidget* const q_ptr;
public:

  qSlicerSceneViewsModuleWidgetPrivate(qSlicerSceneViewsModuleWidget& object);
  ~qSlicerSceneViewsModuleWidgetPrivate();
  void setupUi(qSlicerWidget* widget);

  vtkSlicerSceneViewsModuleLogic* logic() const;

  qSlicerSceneViewsModuleDialog* sceneViewDialog();

  QPointer<qSlicerSceneViewsModuleDialog> SceneViewDialog;
};

//-----------------------------------------------------------------------------
// qSlicerSceneViewsModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
vtkSlicerSceneViewsModuleLogic*
qSlicerSceneViewsModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerSceneViewsModuleWidget);
  return vtkSlicerSceneViewsModuleLogic::SafeDownCast(q->logic());
}

//-----------------------------------------------------------------------------
qSlicerSceneViewsModuleDialog* qSlicerSceneViewsModuleWidgetPrivate::sceneViewDialog()
{
  if (!this->SceneViewDialog)
    {
    this->SceneViewDialog = new qSlicerSceneViewsModuleDialog();

    // pass a pointer to the logic class
    this->SceneViewDialog->setLogic(this->logic());

    // create slots which listen to events fired by the OK and CANCEL button on the dialog
    //this->connect(this->SceneViewDialog, SIGNAL(rejected()),
    //              this->SceneViewDialog, SLOT(hide()));
    //this->connect(this->SceneViewDialog, SIGNAL(accepted()),
    //              this->SceneViewDialog, SLOT(hide()));
    }
  return this->SceneViewDialog;
}

//-----------------------------------------------------------------------------
qSlicerSceneViewsModuleWidgetPrivate::qSlicerSceneViewsModuleWidgetPrivate(qSlicerSceneViewsModuleWidget& object)
  : q_ptr(&object)
{
  this->SceneViewDialog = 0;
}

//-----------------------------------------------------------------------------
qSlicerSceneViewsModuleWidgetPrivate::~qSlicerSceneViewsModuleWidgetPrivate()
{
  if (this->SceneViewDialog)
    {
    this->SceneViewDialog->close();
    delete this->SceneViewDialog.data();
    }
}

//-----------------------------------------------------------------------------
void qSlicerSceneViewsModuleWidgetPrivate::setupUi(qSlicerWidget* widget)
{
  Q_Q(qSlicerSceneViewsModuleWidget);
  this->Ui_qSlicerSceneViewsModuleWidget::setupUi(widget);

  QObject::connect(this->hierarchyTreeView,
                   SIGNAL(restoreSceneViewRequested(QString)),
                   q, SLOT(restoreSceneView(QString)));
  QObject::connect(this->hierarchyTreeView,
                   SIGNAL(editSceneViewRequested(QString)),
                   q, SLOT(editSceneView(QString)));

  // setup the hierarchy treeWidget
  this->hierarchyTreeView->setLogic(this->logic());
  this->hierarchyTreeView->setMRMLScene(this->logic()->GetMRMLScene());
  // setMRMLScene calls setRoot

  q->connect(this->moveDownSelectedButton, SIGNAL(clicked()),
             q, SLOT(moveDownSelected()));
  q->connect(this->moveUpSelectedButton, SIGNAL(clicked()),
             q, SLOT(moveUpSelected()));

  QObject::connect(this->deleteSelectedButton, SIGNAL(clicked()),
                   this->hierarchyTreeView, SLOT(deleteSelected()));

  QObject::connect(this->sceneView, SIGNAL(clicked()),
                   q, SLOT(showSceneViewDialog()));

  q->connect(q, SIGNAL(mrmlSceneChanged(vtkMRMLScene*)),
             this->hierarchyTreeView, SLOT(setMRMLScene(vtkMRMLScene*)));

  // listen to some mrml events
  q->qvtkConnect(this->logic()->GetMRMLScene(), vtkMRMLScene::EndImportEvent,
                 this->hierarchyTreeView, SLOT(onSceneEndImportEvent()));
  q->qvtkConnect(this->logic()->GetMRMLScene(), vtkMRMLScene::EndRestoreEvent,
                 this->hierarchyTreeView, SLOT(onSceneEndRestoreEvent()));
  q->qvtkConnect(this->logic()->GetMRMLScene(), vtkMRMLScene::EndBatchProcessEvent,
                 this->hierarchyTreeView, SLOT(onSceneEndBatchProcessEvent()));

}

//-----------------------------------------------------------------------------
// qSlicerSceneViewsModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerSceneViewsModuleWidget::qSlicerSceneViewsModuleWidget(QWidget* parent) :
  qSlicerAbstractModuleWidget(parent)
  , d_ptr(new qSlicerSceneViewsModuleWidgetPrivate(*this))
{
}

//-----------------------------------------------------------------------------
qSlicerSceneViewsModuleWidget::~qSlicerSceneViewsModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerSceneViewsModuleWidget::setup()
{
  Q_D(qSlicerSceneViewsModuleWidget);
  this->Superclass::setup();
  d->setupUi(this);

  this->updateTreeViewModel();
}

//-----------------------------------------------------------------------------
void qSlicerSceneViewsModuleWidget::moveDownSelected()
{
  Q_D(qSlicerSceneViewsModuleWidget);

  const char* id = d->logic()->MoveSceneViewDown(
    d->hierarchyTreeView->firstSelectedNode().toLatin1());

  d->hierarchyTreeView->clearSelection();
  d->hierarchyTreeView->setSelectedNode(id);
}

//-----------------------------------------------------------------------------
void qSlicerSceneViewsModuleWidget::moveUpSelected()
{
  Q_D(qSlicerSceneViewsModuleWidget);

  const char* id = d->logic()->MoveSceneViewUp(
    d->hierarchyTreeView->firstSelectedNode().toLatin1());

  d->hierarchyTreeView->clearSelection();
  d->hierarchyTreeView->setSelectedNode(id);
}

//-----------------------------------------------------------------------------
void qSlicerSceneViewsModuleWidget::restoreSceneView(const QString& mrmlId)
{
  Q_D(qSlicerSceneViewsModuleWidget);

  d->logic()->RestoreSceneView(mrmlId.toLatin1());

  qSlicerApplication::application()->mainWindow()->statusBar()->showMessage("The SceneView was restored including the attached scene.", 2000);
}

//-----------------------------------------------------------------------------
void qSlicerSceneViewsModuleWidget::editSceneView(const QString& mrmlId)
{
  Q_D(qSlicerSceneViewsModuleWidget);
  d->sceneViewDialog()->loadNode(mrmlId);
  d->sceneViewDialog()->exec();
}

//-----------------------------------------------------------------------------
void qSlicerSceneViewsModuleWidget::updateFromMRMLScene()
{
  // logic will be listening for this event as well and filling in missing
  // hierarchy nodes, so just refresh the tree
  this->updateTreeViewModel();
}

//-----------------------------------------------------------------------------
// Refresh the hierarchy tree after an sceneView was added or modified.
//-----------------------------------------------------------------------------
void qSlicerSceneViewsModuleWidget::updateTreeViewModel()
{
  Q_D(qSlicerSceneViewsModuleWidget);

  if (d->logic() && d->logic()->GetMRMLScene() &&
      d->logic()->GetMRMLScene()->IsBatchProcessing())
    {
    // scene is updating, return
    return;
    }
  //qDebug("updateTreeViewModel");

  // use lazy update to trigger a refresh after batch processing
  d->hierarchyTreeView->sceneModel()->setLazyUpdate(true);

  // set mrml scene calls expand all and set root
  d->hierarchyTreeView->setMRMLScene(d->logic()->GetMRMLScene());
}

//-----------------------------------------------------------------------------
// SceneView functionality
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void qSlicerSceneViewsModuleWidget::showSceneViewDialog()
{
  Q_D(qSlicerSceneViewsModuleWidget);
  // show the dialog
  d->sceneViewDialog()->reset();
  d->sceneViewDialog()->exec();
}

