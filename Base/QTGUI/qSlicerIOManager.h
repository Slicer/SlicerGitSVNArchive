#ifndef __qSlicerIOManager_h
#define __qSlicerIOManager_h

// Qt includes
#include <QList>
#include <QString>
#include <QUrl>

// CTK includes
#include <ctkVTKObject.h>

// SlicerQ includes
#include "qSlicerCoreIOManager.h"
#include "qSlicerFileDialog.h"

#include "qSlicerBaseQTGUIExport.h"

/// QT declarations
class QWidget;

class qSlicerIOManagerPrivate;

class Q_SLICER_BASE_QTGUI_EXPORT qSlicerIOManager : public qSlicerCoreIOManager
{
  Q_OBJECT;
  QVTK_OBJECT;
public:
  typedef qSlicerCoreIOManager Superclass;
  qSlicerIOManager(QObject* parent = 0);
  virtual ~qSlicerIOManager();

  /// Search for the most appropriate dialog based on the fileType,
  /// and open it.
  /// If no dialog is registered for a given fileType (e.g.
  /// qSlicerIO::SceneFile), a default dialog (qSlicerStandardFileDialog) is 
  /// used.
  bool openDialog(qSlicerIO::IOFileType fileType,
                  qSlicerFileDialog::IOAction action,
                  const qSlicerIO::IOProperties& ioProperties
                    = qSlicerIO::IOProperties());

  void addHistory(const QString& path);
  const QStringList& history()const;

  void addFavorite(const QUrl& urls);
  const QList<QUrl>& favorites()const;

  ///
  /// Takes ownership. Any previously set dialog corresponding to the same
  /// fileType (only 1 dialog per filetype) is overriden.
  void registerDialog(qSlicerFileDialog* dialog);

  ///
  /// Displays a progress dialog if it takes too long to load
  /// There is no way to know in advance how long the loading will take, so the
  /// progress dialog listens to the scene and increment the progress anytime
  /// a node is added.
  Q_INVOKABLE virtual bool loadNodes(const qSlicerIO::IOFileType& fileType,
                                     const qSlicerIO::IOProperties& parameters,
                                     vtkCollection* loadedNodes = 0);
  ///
  /// If you have a list of nodes to load, it's best to use this function
  /// in order to have a unique progress dialog instead of multiple ones.
  /// It internally calls loadNodes() for each file.
  virtual bool loadNodes(const QList<qSlicerIO::IOProperties>& files,
                         vtkCollection* loadedNodes = 0);
public slots:

  void openScreenshotDialog();
  void openSceneViewsDialog();
  bool openLoadSceneDialog();
  bool openAddSceneDialog();
  inline bool openAddDataDialog();
  inline bool openAddVolumeDialog();
  inline bool openAddModelDialog();
  inline bool openAddScalarOverlayDialog();
  inline bool openAddTransformDialog();
  inline bool openAddColorTableDialog();
  inline bool openAddFiducialDialog();
  inline bool openAddFiberBundleDialog();
  inline bool openSaveDataDialog();

protected slots:
  void updateProgressDialog();

protected:
  friend class qSlicerFileDialog;
  using qSlicerCoreIOManager::ios;
protected:
  QScopedPointer<qSlicerIOManagerPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerIOManager);
  Q_DISABLE_COPY(qSlicerIOManager);
};

//------------------------------------------------------------------------------
bool qSlicerIOManager::openAddDataDialog()
{
  return this->openDialog(qSlicerIO::NoFile, qSlicerFileDialog::Read);
}

//-----------------------------------------------------------------------------
bool qSlicerIOManager::openAddVolumeDialog()
{
  return this->openDialog(qSlicerIO::VolumeFile, qSlicerFileDialog::Read);
}

//-----------------------------------------------------------------------------
bool qSlicerIOManager::openAddModelDialog()
{
  return this->openDialog(qSlicerIO::ModelFile, qSlicerFileDialog::Read);
}

//-----------------------------------------------------------------------------
bool qSlicerIOManager::openAddScalarOverlayDialog()
{
  return this->openDialog(qSlicerIO::ScalarOverlayFile, qSlicerFileDialog::Read);
}

//-----------------------------------------------------------------------------
bool qSlicerIOManager::openAddTransformDialog()
{
  return this->openDialog(qSlicerIO::TransformFile, qSlicerFileDialog::Read);
}

//-----------------------------------------------------------------------------
bool qSlicerIOManager::openAddColorTableDialog()
{
  return this->openDialog(qSlicerIO::ColorTableFile, qSlicerFileDialog::Read);
}

//-----------------------------------------------------------------------------
bool qSlicerIOManager::openAddFiducialDialog()
{
  return this->openDialog(qSlicerIO::FiducialListFile, qSlicerFileDialog::Read);
}

//-----------------------------------------------------------------------------
bool qSlicerIOManager::openAddFiberBundleDialog()
{
  return this->openDialog(qSlicerIO::FiberBundleFile, qSlicerFileDialog::Read);
}

//------------------------------------------------------------------------------
bool qSlicerIOManager::openSaveDataDialog()
{
  return this->openDialog(qSlicerIO::NoFile, qSlicerFileDialog::Write);
}

#endif
