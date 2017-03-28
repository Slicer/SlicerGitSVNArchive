//QT includes
#include <QPushButton>
#include <QProgressBar>
#include <QLayout>
#include <iostream>
#include <QFileInfo>
#include <QDir>
#include <QListWidget>
#include <QDebug>
#include <QProgressdialog>
#include <QMessagebox>

// QtGUI includes
#include "qSlicerExtensionsRestoreWidget.h"
#include "qSlicerExtensionsManagerModel.h"

//-----------------------------------------------------------------------------
class qSlicerExtensionsRestoreWidgetPrivate
{
  Q_DECLARE_PUBLIC(qSlicerExtensionsRestoreWidget);
protected:
  qSlicerExtensionsRestoreWidget* const q_ptr;

public:
  qSlicerExtensionsRestoreWidgetPrivate(qSlicerExtensionsRestoreWidget& object);
  void init();
  void setupUi();
  void setupList();
  QStringList getSelectedExtensions();
  void startDownloadAndInstallExtensions(QStringList extensionIds);
  void downloadAndInstallNextExtension();
  void downloadProgress(const QString& extensionName, qint64 received, qint64 total);

  qSlicerExtensionsManagerModel *ExtensionsManagerModel;
  QProgressBar *progressBar;
  QProgressDialog *progressDialog;
  QListWidget *extensionList;
  QStringList extensionsToInstall;
  unsigned int nrOfExtensionsToInstall;
  int currentExtensionToInstall;
  bool headlessMode;
  unsigned int maxProgress;
};

// --------------------------------------------------------------------------
qSlicerExtensionsRestoreWidgetPrivate::qSlicerExtensionsRestoreWidgetPrivate(qSlicerExtensionsRestoreWidget& object)
:q_ptr(&object)
{

}

// --------------------------------------------------------------------------
void qSlicerExtensionsRestoreWidgetPrivate::init()
{
  nrOfExtensionsToInstall = 0;
  headlessMode = false;
  setupUi();
}

// --------------------------------------------------------------------------
void qSlicerExtensionsRestoreWidgetPrivate
::setupUi()
{
  Q_Q(qSlicerExtensionsRestoreWidget);

  QVBoxLayout *mainLayout = new QVBoxLayout;
  QHBoxLayout *layoutForProgressAndButton = new QHBoxLayout;
  QPushButton *installButton = new QPushButton;
  progressDialog = new QProgressDialog;
  extensionList = new QListWidget;
  progressBar = new QProgressBar;
  installButton->setText("Install Selected");
  maxProgress = 1000;
  progressBar->setValue(0);
  progressBar->setMaximum(maxProgress);
  progressDialog->setMinimum(0);
  progressDialog->setMaximum(maxProgress);
  layoutForProgressAndButton->addWidget(progressBar);
  layoutForProgressAndButton->addWidget(installButton);
  mainLayout->addWidget(extensionList);
  mainLayout->addLayout(layoutForProgressAndButton);
  progressDialog->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
  q->setLayout(mainLayout);

  //Setup Handling
  QObject::connect(installButton, SIGNAL(clicked()),
    q, SLOT(onInstallSelectedExtensionsTriggered()));
}

// --------------------------------------------------------------------------
void qSlicerExtensionsRestoreWidgetPrivate
::setupList()
{
  Q_Q(qSlicerExtensionsRestoreWidget);

  /*extensionList->clear();

  QMap<QString, QStringList> extensionRestoreInfo = q->extensionsManagerModel()->getExtensionRestoreInformation();
  foreach(QString extensionId, extensionRestoreInfo.keys())
  {
    QListWidgetItem* extensionItem = new QListWidgetItem;
    extensionItem->setData(Qt::UserRole, extensionId);
    QString itemText = extensionRestoreInfo[extensionId][0];
    bool isItemEnabled = (extensionRestoreInfo[extensionId][2] == "0");
    //&& extensionRestoreInfo[extensionId][3] == "1");

    if (extensionRestoreInfo[extensionId][1] == "1" && isItemEnabled)
    {
      extensionItem->setForeground(QBrush(Qt::darkGreen));
    }
    extensionItem->setText(extensionRestoreInfo[extensionId][0]);
    Qt::ItemFlags flags = isItemEnabled ? Qt::ItemIsUserCheckable | Qt::ItemIsEnabled : Qt::ItemIsUserCheckable;
    extensionItem->setFlags(flags);
    extensionItem->setCheckState((extensionRestoreInfo[extensionId][1] == "0" || !isItemEnabled) ? Qt::Unchecked : Qt::Checked);
    extensionList->addItem(extensionItem);
  }*/
}

// --------------------------------------------------------------------------
QStringList qSlicerExtensionsRestoreWidgetPrivate
::getSelectedExtensions()
{
  QStringList selectedExtensions;
  for (int i = 0; i < extensionList->count(); i++)
  {
    QListWidgetItem* currentItem = extensionList->item(i);
    if (currentItem->checkState() && (currentItem->flags() & Qt::ItemIsEnabled))
    {
      selectedExtensions.append(currentItem->data(Qt::UserRole).toString());
    }
  }
  return selectedExtensions;
}

// --------------------------------------------------------------------------
void qSlicerExtensionsRestoreWidgetPrivate
::startDownloadAndInstallExtensions(QStringList extensionIds)
{
  extensionsToInstall = extensionIds;
  nrOfExtensionsToInstall = extensionsToInstall.size();
  currentExtensionToInstall = -1;
  downloadAndInstallNextExtension();
}

// --------------------------------------------------------------------------
void qSlicerExtensionsRestoreWidgetPrivate
::downloadAndInstallNextExtension()
{
  Q_Q(qSlicerExtensionsRestoreWidget);
  currentExtensionToInstall++;
  if (currentExtensionToInstall < nrOfExtensionsToInstall)
  {
    if (headlessMode) {
      progressDialog->setLabelText("Installing Extension (ID):" + extensionsToInstall.at(currentExtensionToInstall));
    }
    q->extensionsManagerModel()->downloadAndInstallExtension(extensionsToInstall.at(currentExtensionToInstall));
  }
  else {
	  if (headlessMode) {
		  progressDialog->close();
      headlessMode = false;
	  }
    else
    {
      setupList();
    }
  }
}
// --------------------------------------------------------------------------
void qSlicerExtensionsRestoreWidgetPrivate
::downloadProgress(const QString& extensionName, qint64 received, qint64 total)
{
  int value = (((float(maxProgress) / float(nrOfExtensionsToInstall))*float(currentExtensionToInstall)) +
    ((float(received) / float(total)) * (float(maxProgress) / float(nrOfExtensionsToInstall))));
  qDebug() << value;
  if (headlessMode) {
	  progressDialog->setValue(value);
  }
  else
  {
	  progressBar->setValue(value);
  }
}

// qSlicerExtensionsRestoreWidget methods

// --------------------------------------------------------------------------
qSlicerExtensionsRestoreWidget
::qSlicerExtensionsRestoreWidget(QWidget* _parent)
: Superclass(_parent)
, d_ptr(new qSlicerExtensionsRestoreWidgetPrivate(*this))
{
  Q_D(qSlicerExtensionsRestoreWidget);
  d->init();
}

// --------------------------------------------------------------------------
qSlicerExtensionsRestoreWidget
::~qSlicerExtensionsRestoreWidget()
{
}

// --------------------------------------------------------------------------
qSlicerExtensionsManagerModel* qSlicerExtensionsRestoreWidget
::extensionsManagerModel()const
{
  Q_D(const qSlicerExtensionsRestoreWidget);
  return d->ExtensionsManagerModel;
}



// --------------------------------------------------------------------------
void qSlicerExtensionsRestoreWidget
::setExtensionsManagerModel(qSlicerExtensionsManagerModel* model)
{
  Q_D(qSlicerExtensionsRestoreWidget);

  if (this->extensionsManagerModel() == model)
  {
    return;
  }

  disconnect(this, SLOT(onProgressChanged(QString, qint64, qint64)));
  disconnect(this, SLOT(onInstallationFinished(QString)));
  disconnect(this, SLOT(onExtensionRestoreTriggered(QStringList &)));
  d->ExtensionsManagerModel = model;
  d->setupList();

  if (model)
  {
    connect(model, SIGNAL(installDownloadProgress(QString, qint64, qint64)),
      this, SLOT(onProgressChanged(QString, qint64, qint64)));
    connect(model, SIGNAL(extensionInstalled(QString)),
      this, SLOT(onInstallationFinished(QString)));
	  connect(model, SIGNAL(extensionRestoreTriggered(QStringList &)),
	    this, SLOT(onExtensionRestoreTriggered(QStringList &)));
  }
}

// --------------------------------------------------------------------------
void qSlicerExtensionsRestoreWidget
::onInstallSelectedExtensionsTriggered()
{
  Q_D(qSlicerExtensionsRestoreWidget);
  d->startDownloadAndInstallExtensions(d->getSelectedExtensions());
}

// --------------------------------------------------------------------------
void qSlicerExtensionsRestoreWidget::onProgressChanged(const QString& extensionName, qint64 received, qint64 total)
{
  Q_D(qSlicerExtensionsRestoreWidget);
  d->downloadProgress(extensionName, received, total);
};

// --------------------------------------------------------------------------
void qSlicerExtensionsRestoreWidget
::onInstallationFinished(QString extensionName)
{
  Q_D(qSlicerExtensionsRestoreWidget);
  d->downloadAndInstallNextExtension();
}

void qSlicerExtensionsRestoreWidget
::onExtensionRestoreTriggered(QStringList &extensionIds)
{
	Q_D(qSlicerExtensionsRestoreWidget);
	qDebug() << "Hey, I was triggered with following info: " << extensionIds;
	d->headlessMode = true;
	d->extensionsToInstall = extensionIds;
  d->progressDialog->show();
	d->startDownloadAndInstallExtensions(extensionIds);
}

// --------------------------------------------------------------------------
void qSlicerExtensionsRestoreWidget::onMessageLogged(const QString& text, ctkErrorLogLevel::LogLevels level)
{
  QString delay = "2500";
  QString state;
  if (level == ctkErrorLogLevel::Warning)
  {
    delay = "10000";
    state = "warning";
  }
  else if (level == ctkErrorLogLevel::Critical || level == ctkErrorLogLevel::Fatal)
  {
    delay = "10000";
    state = "error";
  }

}
