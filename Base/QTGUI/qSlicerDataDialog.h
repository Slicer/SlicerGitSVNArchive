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

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

#ifndef __qSlicerDataDialog_h
#define __qSlicerDataDialog_h

// CTK includes
#include <ctkPimpl.h>

// SlicerQt includes
#include "qSlicerFileDialog.h"
#include "qSlicerBaseQTGUIExport.h"

/// Forward declarations
class qSlicerDataDialogPrivate;
class QDropEvent;

//------------------------------------------------------------------------------
class Q_SLICER_BASE_QTGUI_EXPORT qSlicerDataDialog : public qSlicerFileDialog
{
  Q_OBJECT
public:
  typedef qSlicerFileDialog Superclass;
  qSlicerDataDialog(QObject* parent =0);
  virtual ~qSlicerDataDialog();

  virtual qSlicerIO::IOFileType fileType()const;
  virtual QString description()const;
  virtual qSlicerFileDialog::IOAction action()const;

  virtual bool isMimeDataAccepted(const QMimeData* mimeData)const;
  virtual void dropEvent(QDropEvent *event);

  /// run the dialog to select the file/files/directory
  virtual bool exec(const qSlicerIO::IOProperties& readerProperties =
                    qSlicerIO::IOProperties());

  /// for programmatic population of dialog
  Q_INVOKABLE virtual void addFile(const QString filePath);
  Q_INVOKABLE virtual void addDirectory(const QString directoryPath);

protected:
  QScopedPointer<qSlicerDataDialogPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerDataDialog);
  Q_DISABLE_COPY(qSlicerDataDialog);
};

#endif
