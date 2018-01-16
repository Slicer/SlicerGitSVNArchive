
#ifndef __qSlicerExtensionsRestoreWidget_h
#define __qSlicerExtensionsRestoreWidget_h

// CTK includes
#include <ctkErrorLogLevel.h>

// Qt includes
#include <QWidget>
#include <QVariant>

// QtGUI includes
#include "qSlicerBaseQTGUIExport.h"

class qSlicerExtensionsRestoreWidgetPrivate;
class qSlicerExtensionsManagerModel;

class Q_SLICER_BASE_QTGUI_EXPORT qSlicerExtensionsRestoreWidget
  : public QWidget
{
  Q_OBJECT
public:
  /// Superclass typedef
  typedef QWidget Superclass;

  /// Constructor
  explicit qSlicerExtensionsRestoreWidget(QWidget* parent = 0);

  /// Destructor
  virtual ~qSlicerExtensionsRestoreWidget();

  Q_INVOKABLE qSlicerExtensionsManagerModel* extensionsManagerModel()const;
  Q_INVOKABLE void setExtensionsManagerModel(qSlicerExtensionsManagerModel* model);

  // Events
  void showEvent(QShowEvent* event);

  protected slots :
  void onInstallSelectedExtensionsTriggered();
  void onCheckOnStartupChanged(int state);
  void onSilentInstallOnStartupChanged(int state);
  void onProgressChanged(const QString& extensionName, qint64 received, qint64 total);
  void onInstallationFinished(QString extensionName);
  void onExtensionHistoryGatheredOnStartup(const QVariantMap&);

protected:
  QScopedPointer<qSlicerExtensionsRestoreWidgetPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerExtensionsRestoreWidget);
  Q_DISABLE_COPY(qSlicerExtensionsRestoreWidget);
};

#endif
