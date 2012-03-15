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

#ifndef __qSlicerSettingsExtensionsPanel_h
#define __qSlicerSettingsExtensionsPanel_h

// Qt includes
#include <QWidget>

// CTK includes
#include <ctkSettingsPanel.h>

// QtGUI includes
#include "qSlicerBaseQTGUIExport.h"

class QSettings;
class qSlicerSettingsExtensionsPanelPrivate;

class Q_SLICER_BASE_QTGUI_EXPORT qSlicerSettingsExtensionsPanel
  : public ctkSettingsPanel
{
  Q_OBJECT
  Q_PROPERTY(bool restartRequested READ restartRequested WRITE setRestartRequested)
public:
  /// Superclass typedef
  typedef ctkSettingsPanel Superclass;

  /// Constructor
  explicit qSlicerSettingsExtensionsPanel(QWidget* parent = 0);

  /// Destructor
  virtual ~qSlicerSettingsExtensionsPanel();

  /// Return True if the application is expected to be restarted.
  bool restartRequested()const;

  /// \sa restartRequested()
  void setRestartRequested(bool value);

public slots:
  virtual void resetSettings();

protected slots:
  /// \todo This slot does nothing.
  void onExensionsServerUrlChanged(const QString& url);
  void onExensionsPathChanged(const QString& path);

protected:
  QScopedPointer<qSlicerSettingsExtensionsPanelPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerSettingsExtensionsPanel);
  Q_DISABLE_COPY(qSlicerSettingsExtensionsPanel);
};

#endif
