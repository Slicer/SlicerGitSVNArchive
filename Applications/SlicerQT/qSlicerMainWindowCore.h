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

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

#ifndef __qSlicerMainWindowCore_h
#define __qSlicerMainWindowCore_h 

// Qt includes
#include <QObject>

// CTK includes
#include <ctkPimpl.h>

#include "qSlicerQTExport.h"

class qSlicerMainWindow; 
class qSlicerMainWindowCorePrivate;

class Q_SLICERQT_EXPORT qSlicerMainWindowCore : public QObject
{
  Q_OBJECT
  
public:
  typedef QObject Superclass;
  qSlicerMainWindowCore(qSlicerMainWindow *parent = 0);
  virtual ~qSlicerMainWindowCore();

public slots: 
  /// 
  /// Handle actions - See qSlicerMainWindow::setupMenuActions
  void onFileAddDataActionTriggered();
  void onFileImportSceneActionTriggered();
  void onFileLoadSceneActionTriggered();
  void onFileAddVolumeActionTriggered();
  void onFileAddTransformActionTriggered();
  void onFileSaveSceneActionTriggered();
  void onSDBSaveToDirectoryActionTriggered();
  void onSDBZipDirectoryActionTriggered();
  void onSDBZipToDCMActionTriggered();
  void onFileCloseSceneActionTriggered();
  void onEditUndoActionTriggered();
  void onEditRedoActionTriggered();
  void setLayout(int);
  void onWindowErrorLogActionTriggered();
  void onWindowPythonInteractorActionTriggered();

  void onHelpKeyboardShortcutsActionTriggered();
  void onHelpBrowseTutorialsActionTriggered();
  void onHelpInterfaceDocumentationActionTriggered();
  void onHelpSlicerPublicationsActionTriggered();
  void onHelpVisualBlogActionTriggered();

  void onHelpReportBugOrFeatureRequestActionTriggered();
  void onHelpAboutSlicerQTActionTriggered();

protected:
  qSlicerMainWindow* widget() const;

protected:
  QScopedPointer<qSlicerMainWindowCorePrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerMainWindowCore);
  Q_DISABLE_COPY(qSlicerMainWindowCore);
};

#endif
