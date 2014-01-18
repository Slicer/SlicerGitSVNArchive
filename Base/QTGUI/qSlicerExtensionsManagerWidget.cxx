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
#include <QToolButton>
#include <QWebView>

// QtGUI includes
#include "qSlicerExtensionsManagerWidget.h"
#include "qSlicerExtensionsManagerModel.h"
#include "ui_qSlicerExtensionsManagerWidget.h"

//-----------------------------------------------------------------------------
class qSlicerExtensionsManagerWidgetPrivate: public Ui_qSlicerExtensionsManagerWidget
{
  Q_DECLARE_PUBLIC(qSlicerExtensionsManagerWidget);
protected:
  qSlicerExtensionsManagerWidget* const q_ptr;

public:
  qSlicerExtensionsManagerWidgetPrivate(qSlicerExtensionsManagerWidget& object);
  void init();
};

// --------------------------------------------------------------------------
qSlicerExtensionsManagerWidgetPrivate::qSlicerExtensionsManagerWidgetPrivate(qSlicerExtensionsManagerWidget& object)
  :q_ptr(&object)
{
}

// --------------------------------------------------------------------------
void qSlicerExtensionsManagerWidgetPrivate::init()
{
  Q_Q(qSlicerExtensionsManagerWidget);

  this->setupUi(q);

  // Back and forward buttons
  QWidget * actionsWidget = new QWidget;
  QHBoxLayout * hLayout = new QHBoxLayout(actionsWidget);
  hLayout->setContentsMargins(0, 0, 0, 0);
  QToolButton * backButton = new QToolButton;
  backButton->setDefaultAction(this->ExtensionsInstallWidget->webView()->pageAction(QWebPage::Back));
  hLayout->addWidget(backButton);
  QToolButton * forwardButton = new QToolButton;
  forwardButton->setDefaultAction(this->ExtensionsInstallWidget->webView()->pageAction(QWebPage::Forward));
  hLayout->addWidget(forwardButton);

  int size = this->tabWidget->height();
  backButton->setIconSize(QSize(size, size));
  forwardButton->setIconSize(QSize(size, size));

  actionsWidget->setEnabled(false);

  this->tabWidget->setCornerWidget(actionsWidget, Qt::TopLeftCorner);

  QObject::connect(this->tabWidget, SIGNAL(currentChanged(int)),
                   q, SLOT(onCurrentTabChanged(int)));
}

// --------------------------------------------------------------------------
// qSlicerExtensionsManagerWidget methods

// --------------------------------------------------------------------------
qSlicerExtensionsManagerWidget::qSlicerExtensionsManagerWidget(QWidget* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerExtensionsManagerWidgetPrivate(*this))
{
  Q_D(qSlicerExtensionsManagerWidget);
  d->init();
}

// --------------------------------------------------------------------------
qSlicerExtensionsManagerWidget::~qSlicerExtensionsManagerWidget()
{
}

// --------------------------------------------------------------------------
qSlicerExtensionsManagerModel* qSlicerExtensionsManagerWidget::extensionsManagerModel()const
{
  Q_D(const qSlicerExtensionsManagerWidget);
  return d->ExtensionsManageWidget->extensionsManagerModel();
}

// --------------------------------------------------------------------------
void qSlicerExtensionsManagerWidget::setExtensionsManagerModel(qSlicerExtensionsManagerModel* model)
{
  Q_D(qSlicerExtensionsManagerWidget);

  if (this->extensionsManagerModel() == model)
    {
    return;
    }

  disconnect(this, SLOT(onModelUpdated()));

  d->ExtensionsManageWidget->setExtensionsManagerModel(model);
  d->ExtensionsInstallWidget->setExtensionsManagerModel(model);

  if (model)
    {
    this->onModelUpdated();
    connect(model, SIGNAL(modelUpdated()),
            this, SLOT(onModelUpdated()));
    connect(model, SIGNAL(extensionInstalled(QString)),
            this, SLOT(onModelUpdated()));
    connect(model, SIGNAL(extensionUninstalled(QString)),
            this, SLOT(onModelUpdated()));
    }
}

// --------------------------------------------------------------------------
void qSlicerExtensionsManagerWidget::refreshInstallWidget()
{
  Q_D(qSlicerExtensionsManagerWidget);
  d->ExtensionsInstallWidget->refresh();
}

// --------------------------------------------------------------------------
void qSlicerExtensionsManagerWidget::onModelUpdated()
{
  Q_D(qSlicerExtensionsManagerWidget);

  int manageExtensionsTabIndex = d->tabWidget->indexOf(d->ManageExtensionsTab);
  int numberOfInstalledExtensions = this->extensionsManagerModel()->numberOfInstalledExtensions();

  d->tabWidget->setTabText(manageExtensionsTabIndex,
                           QString("Manage Extensions (%1)").arg(numberOfInstalledExtensions));

  if (numberOfInstalledExtensions == 0)
    {
    d->tabWidget->setTabEnabled(manageExtensionsTabIndex, false);
    d->tabWidget->setCurrentWidget(d->InstallExtensionsTab);
    }
  else
    {
    d->tabWidget->setTabEnabled(manageExtensionsTabIndex, true);
    }
}

// --------------------------------------------------------------------------
void qSlicerExtensionsManagerWidget::onCurrentTabChanged(int index)
{
  Q_D(qSlicerExtensionsManagerWidget);
  d->tabWidget->cornerWidget(Qt::TopLeftCorner)->setEnabled(
        d->tabWidget->widget(index) == d->InstallExtensionsTab);
}
