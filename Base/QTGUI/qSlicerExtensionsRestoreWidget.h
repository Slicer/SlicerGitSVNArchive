
#ifndef __qSlicerExtensionsRestoreWidget_h
#define __qSlicerExtensionsRestoreWidget_h

// CTK includes
#include <ctkErrorLogLevel.h>

// Qt includes
#include <QWidget>

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

  protected slots :
  void onInstallSelectedExtensionsTriggered();
  void onProgressChanged(const QString& extensionName, qint64 received, qint64 total);
  void onInstallationFinished(QString extensionName);
  void onMessageLogged(const QString& text, ctkErrorLogLevel::LogLevels level);

protected:
  QScopedPointer<qSlicerExtensionsRestoreWidgetPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerExtensionsRestoreWidget);
  Q_DISABLE_COPY(qSlicerExtensionsRestoreWidget);
};

#endif