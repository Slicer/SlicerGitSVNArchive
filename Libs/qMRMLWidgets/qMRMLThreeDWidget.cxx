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

// Qt includes
#include <QDebug>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QToolButton>

// CTK includes
#include <ctkLogger.h>
#include <ctkPopupWidget.h>

// qMRML includes
#include "qMRMLThreeDViewControllerWidget.h"
#include "qMRMLThreeDView.h"
#include "qMRMLThreeDWidget.h"

//--------------------------------------------------------------------------
static ctkLogger logger("org.slicer.libs.qmrmlwidgets.qMRMLThreeDWidget");
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// qMRMLSliceViewPrivate
class qMRMLThreeDWidgetPrivate
  : public QObject
{
  Q_DECLARE_PUBLIC(qMRMLThreeDWidget);
protected:
  qMRMLThreeDWidget* const q_ptr;
public:
  qMRMLThreeDWidgetPrivate(qMRMLThreeDWidget& object);
  ~qMRMLThreeDWidgetPrivate();
  
  void init();

  qMRMLThreeDView*       ThreeDView;
  qMRMLThreeDViewControllerWidget* ThreeDController;
};


//---------------------------------------------------------------------------
qMRMLThreeDWidgetPrivate::qMRMLThreeDWidgetPrivate(qMRMLThreeDWidget& object)
  : q_ptr(&object)
{
  this->ThreeDView = 0;
  this->ThreeDController = 0;
}

//---------------------------------------------------------------------------
qMRMLThreeDWidgetPrivate::~qMRMLThreeDWidgetPrivate()
{
}

//---------------------------------------------------------------------------
void qMRMLThreeDWidgetPrivate::init()
{
  Q_Q(qMRMLThreeDWidget);
  
  QVBoxLayout* layout = new QVBoxLayout(q);
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);

  this->ThreeDController = new qMRMLThreeDViewControllerWidget;
  layout->addWidget(this->ThreeDController);

  this->ThreeDView = new qMRMLThreeDView;
  layout->addWidget(this->ThreeDView);
  
  this->ThreeDController->setThreeDView(this->ThreeDView);

  QObject::connect(q, SIGNAL(mrmlSceneChanged(vtkMRMLScene*)),
                   this->ThreeDView, SLOT(setMRMLScene(vtkMRMLScene*)));
}

// --------------------------------------------------------------------------
// qMRMLThreeDWidget methods

// --------------------------------------------------------------------------
qMRMLThreeDWidget::qMRMLThreeDWidget(QWidget* parentWidget)
  : Superclass(parentWidget)
  , d_ptr(new qMRMLThreeDWidgetPrivate(*this))
{
  Q_D(qMRMLThreeDWidget);
  d->init();
}

// --------------------------------------------------------------------------
qMRMLThreeDWidget::~qMRMLThreeDWidget()
{
}

// --------------------------------------------------------------------------
void qMRMLThreeDWidget::addDisplayableManager(const QString& dManager)
{
  Q_D(qMRMLThreeDWidget);
  d->ThreeDView->addDisplayableManager(dManager);
}

// --------------------------------------------------------------------------
void qMRMLThreeDWidget::setMRMLViewNode(vtkMRMLViewNode* newViewNode)
{
  Q_D(qMRMLThreeDWidget);
  d->ThreeDView->setMRMLViewNode(newViewNode);
  d->ThreeDController->setMRMLViewNode(newViewNode);
}

// --------------------------------------------------------------------------
vtkMRMLViewNode* qMRMLThreeDWidget::mrmlViewNode()const
{
  Q_D(const qMRMLThreeDWidget);
  return d->ThreeDView->mrmlViewNode();
}

// --------------------------------------------------------------------------
qMRMLThreeDView* qMRMLThreeDWidget::threeDView()const
{
  Q_D(const qMRMLThreeDWidget);
  return d->ThreeDView;
}
