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
#include <QStyledItemDelegate>
#include <QPainter>
#include <QEvent>
#include <QApplication>

// QtGUI includes
#include "qSlicerExtensionsRestoreWidget.h"
#include "qSlicerExtensionsManagerModel.h"

// --------------------------------------------------------------------------
class qSlicerRestoreExtensionsItemDelegate : public QStyledItemDelegate
{
public:
  qSlicerRestoreExtensionsItemDelegate(QObject * parent = 0)
    : QStyledItemDelegate(parent) {};

  bool editorEvent(QEvent *event,
    QAbstractItemModel *model,
    const QStyleOptionViewItem &option,
    const QModelIndex &index)
  {
    bool isEnabled = index.data(Qt::UserRole + 3).toBool();

    if (event->type() == QEvent::MouseButtonRelease && isEnabled)
    {
      bool value = index.data(Qt::UserRole + 1).toBool();
      model->setData(index, !value, Qt::UserRole + 1);
      return true;
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
  }

  // --------------------------------------------------------------------------
  void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const{
    QRect r = option.rect;

    QPen enabledPen(QColor::fromRgb(0, 0, 0), 1, Qt::SolidLine);
    QPen disabledPen(QColor::fromRgb(125, 125, 125), 1, Qt::SolidLine);
    QPen candidatePen(QColor::fromRgb(0, 200, 50), 1, Qt::SolidLine);

    //GET DATA
    QString title           = index.data(Qt::DisplayRole).toString();
    bool isChecked          = index.data(Qt::UserRole + 1).toBool();
    QString description     = index.data(Qt::UserRole + 2).toString();
    bool isEnabled          = index.data(Qt::UserRole + 3).toBool();
    bool isRestoreCandidate = index.data(Qt::UserRole + 4).toBool();

    //TITLE
    painter->setPen((isEnabled ? enabledPen : disabledPen));
    r = option.rect.adjusted(55, 10, 0, 0);
    painter->setFont(QFont("Arial", 13, QFont::Bold));
    painter->drawText(r.left(), r.top(), r.width(), r.height(), Qt::AlignTop | Qt::AlignLeft, title, &r);
    //DESCRIPTION
    painter->setPen((isEnabled ? ( isRestoreCandidate ? candidatePen : enabledPen ) : disabledPen));
    r = option.rect.adjusted(55, 30, -10, 0);
    painter->setFont(QFont("Arial", 9, QFont::Normal));
    painter->drawText(r.left(), r.top(), r.width(), r.height(), Qt::AlignLeft, description, &r);
    //CHECKBOX
    QStyleOptionButton cbOpt;
    cbOpt.rect = option.rect.adjusted(20, 0, 0, 0);

    if (isChecked)
    {
      cbOpt.state |= QStyle::State_On;
    }
    else
    {
      cbOpt.state |= QStyle::State_Off;
    }

    QApplication::style()->drawControl(QStyle::CE_CheckBox, &cbOpt, painter);
  }
  QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const{
    return QSize(200, 60); // very dumb value
  }

};


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
  QStringList extractInstallationCandidates(QVariantMap extensionHistoryInformation);
  void processExtensionsHistoryInformationOnStartup(QVariantMap extensionHistoryInformation);

  qSlicerExtensionsManagerModel *ExtensionsManagerModel;
  QProgressBar *progressBar;
  QProgressDialog *progressDialog;
  QListWidget *extensionList;
  QStringList extensionsToInstall;
  QVariantMap extensionRestoreInformation;
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
  extensionList->setAlternatingRowColors(true);
  extensionList->setItemDelegate(new qSlicerRestoreExtensionsItemDelegate(q));
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
QStringList qSlicerExtensionsRestoreWidgetPrivate
::extractInstallationCandidates(QVariantMap extensionHistoryInformation)
{
  QStringList candidateIds;
  for (unsigned int i = 0; i < extensionHistoryInformation.size(); i++)
  {
    QVariantMap currentInfo = extensionHistoryInformation.value(extensionHistoryInformation.keys().at(i)).toMap();
    if (currentInfo.value("WasInstalledInLastRevision").toBool() && currentInfo.value("IsCompatible").toBool() && !currentInfo.value("IsInstalled").toBool())
    {
      candidateIds.append(extensionHistoryInformation.keys().at(i));
    }
  }
  return candidateIds;
}

// --------------------------------------------------------------------------
void qSlicerExtensionsRestoreWidgetPrivate
::processExtensionsHistoryInformationOnStartup(QVariantMap extensionHistoryInformation)
{
  QStringList candidateIds = extractInstallationCandidates(extensionHistoryInformation);
  if (candidateIds.length() > 0)
  {
    QMessageBox msgBox;
    msgBox.setText("Previously installed extensions identified.");
    QString text = QString("%1 compatible extension(s) from a previous Slicer installation found. Do you want to install?"
    "(For details see: Extension Manager > Restore Extensions)").arg(candidateIds.length());
    msgBox.setInformativeText(text);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if (msgBox.exec() == QMessageBox::Yes)
    {
      this->headlessMode = true;
      this->progressDialog->show();
      this->startDownloadAndInstallExtensions(candidateIds);
    }
  }



}

// --------------------------------------------------------------------------
void qSlicerExtensionsRestoreWidgetPrivate
::setupList()
{
  Q_Q(qSlicerExtensionsRestoreWidget);

  extensionList->clear();
  QVariantMap extensionInfo = q->extensionsManagerModel()->getExtensionHistoryInformation();


  foreach(QString extensionId, extensionInfo.keys())
  {
    QListWidgetItem* extensionItem = new QListWidgetItem;

    QVariantMap currentInfo = extensionInfo.value(extensionId).toMap();

    QString title                   = currentInfo.value("Name").toString();
    bool isCompatible               = currentInfo.value("IsCompatible").toBool();
    bool isInstalled                = currentInfo.value("IsInstalled").toBool();
    QString usedLastInRevision      = currentInfo.value("UsedLastInRevision").toString();
    bool wasInstalledInLastRevision = currentInfo.value("WasInstalledInLastRevision").toBool();
    bool isItemEnabled              = isCompatible && !isInstalled;
    bool isItemChecked              = isItemEnabled && wasInstalledInLastRevision;
    QString description =
      (isInstalled ? "currently installed" :
      (isCompatible ? ( wasInstalledInLastRevision ? "was used in previously installed Slicer version (" + usedLastInRevision + ") " :
      "was last used in Slicer version " + usedLastInRevision) :
      "not compatible with current Slicer version (was last used in Slicer version " + usedLastInRevision + ")"));

    extensionItem->setData(Qt::UserRole, extensionId);
    extensionItem->setData(Qt::UserRole + 1, isItemChecked);
    extensionItem->setData(Qt::UserRole + 2, description);
    extensionItem->setData(Qt::UserRole + 3, isItemEnabled);
    extensionItem->setData(Qt::UserRole + 4, wasInstalledInLastRevision);
    extensionItem->setText(title);

    extensionList->addItem(extensionItem);
  }
}

// --------------------------------------------------------------------------
QStringList qSlicerExtensionsRestoreWidgetPrivate
::getSelectedExtensions()
{
  QStringList selectedExtensions;
  for (int i = 0; i < extensionList->count(); i++)
  {
    QListWidgetItem* currentItem = extensionList->item(i);
    if (currentItem->data(Qt::UserRole + 1).toBool())
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
    q->extensionsManagerModel()->downloadAndInstallExtension(extensionsToInstall.at(currentExtensionToInstall));
  }
  else {
	  if (headlessMode) {
		  progressDialog->close();
      headlessMode = false;
      QMessageBox msgBox;
      msgBox.setText("Restart required.");
      msgBox.setInformativeText("All extensions restored. Please restart Slicer.");
      msgBox.setStandardButtons(QMessageBox::Ok);
      msgBox.exec();
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
  if (headlessMode) {
	  progressDialog->setValue(value);
    progressDialog->setLabelText("Installing " + extensionName + " (" + QString::number(received) + "/" + QString::number(total) + ")");
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
  disconnect(this, SLOT(onExtensionHistoryGatheredOnStartup(QVariantMap)));
  d->ExtensionsManagerModel = model;
  d->setupList();

  if (model)
  {
    connect(model, SIGNAL(installDownloadProgress(QString, qint64, qint64)),
      this, SLOT(onProgressChanged(QString, qint64, qint64)));
    connect(model, SIGNAL(extensionInstalled(QString)),
      this, SLOT(onInstallationFinished(QString)));
    connect(model, SIGNAL(extensionHistoryGatheredOnStartup(QVariantMap)),
      this, SLOT(onExtensionHistoryGatheredOnStartup(QVariantMap)));
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
::onExtensionHistoryGatheredOnStartup(const QVariantMap& extensionInfo)
{
	Q_D(qSlicerExtensionsRestoreWidget);

  qDebug() << "got triggered:" << extensionInfo;
  d->processExtensionsHistoryInformationOnStartup(extensionInfo);




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
