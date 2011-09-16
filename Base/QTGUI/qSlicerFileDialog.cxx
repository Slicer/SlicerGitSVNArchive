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

// CTK includes
#include <ctkFileDialog.h>
#include <ctkUtils.h>

// SlicerQt includes
#include "qSlicerApplication.h"
#include "qSlicerIOManager.h"
#include "qSlicerIOOptionsWidget.h"

/*
//-----------------------------------------------------------------------------
class qSlicerFileDialogPrivate
{
public:
};
*/

//-----------------------------------------------------------------------------
qSlicerFileDialog::qSlicerFileDialog(QObject* _parent)
  :QObject(_parent)
{
}

//-----------------------------------------------------------------------------
qSlicerFileDialog::~qSlicerFileDialog()
{
}

//-----------------------------------------------------------------------------
QStringList qSlicerFileDialog::nameFilters(qSlicerIO::IOFileType fileType)
{
  QStringList filters;
  QStringList extensions;
  QList<qSlicerIO*> readers = 
    qSlicerApplication::application()->ioManager()->ios(fileType);
  foreach(const qSlicerIO* reader, readers)
    {
    foreach(const QString& filter, reader->extensions())
      {
      QString nameFilter = filter.contains('(') ? filter :
        reader->description() + " (" + filter + ")";
      filters << nameFilter;
      extensions << ctk::nameFilterToExtensions(nameFilter);
      }
    }
  filters.insert(0, QString("All (") + extensions.join(" ") + QString(")"));
  return filters;
}

//-----------------------------------------------------------------------------
class qSlicerStandardFileDialogPrivate
{
public:
  qSlicerStandardFileDialogPrivate();
  qSlicerIO::IOFileType       FileType;
  qSlicerFileDialog::IOAction Action;
};

//-----------------------------------------------------------------------------
qSlicerStandardFileDialogPrivate::qSlicerStandardFileDialogPrivate()
{
  this->FileType = qSlicerIO::NoFile;
  this->Action = qSlicerFileDialog::Read;
}

//-----------------------------------------------------------------------------
qSlicerStandardFileDialog::qSlicerStandardFileDialog(QObject* _parent)
  :qSlicerFileDialog(_parent)
  , d_ptr(new qSlicerStandardFileDialogPrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerStandardFileDialog::~qSlicerStandardFileDialog()
{
}

//-----------------------------------------------------------------------------
void qSlicerStandardFileDialog::setFileType(qSlicerIO::IOFileType _fileType)
{
  Q_D(qSlicerStandardFileDialog);
  d->FileType = _fileType;
}

//-----------------------------------------------------------------------------
qSlicerIO::IOFileType qSlicerStandardFileDialog::fileType()const
{
  Q_D(const qSlicerStandardFileDialog);
  return d->FileType;
}


//-----------------------------------------------------------------------------
void qSlicerStandardFileDialog::setAction(qSlicerFileDialog::IOAction dialogAction)
{
  Q_D(qSlicerStandardFileDialog);
  d->Action = dialogAction;
}

//-----------------------------------------------------------------------------
qSlicerFileDialog::IOAction qSlicerStandardFileDialog::action()const
{
  Q_D(const qSlicerStandardFileDialog);
  return d->Action;
}

//-----------------------------------------------------------------------------
bool qSlicerStandardFileDialog::exec(const qSlicerIO::IOProperties& ioProperties)
{
  Q_D(qSlicerStandardFileDialog);
  Q_ASSERT(!ioProperties.contains("fileName"));
  qSlicerIOManager* ioManager = qSlicerApplication::application()->ioManager();
  ctkFileDialog fileDialog(qobject_cast<QWidget*>(this->parent()));
  // We don't want the odd dialog in Mac: it shows in gray filenames that
  // don't match the filter -> we don't want to show files that don't match
  // the filter.
  fileDialog.setOption(QFileDialog::DontUseNativeDialog);
  fileDialog.setNameFilters(
    qSlicerFileDialog::nameFilters(d->FileType));
  fileDialog.setHistory(ioManager->history());
  if (ioManager->favorites().count())
    {
    fileDialog.setSidebarUrls(ioManager->favorites());
    }
  if (ioProperties["multipleFiles"].toBool())
    {
    fileDialog.setFileMode(QFileDialog::ExistingFiles);
    }
  // warning: we are responsible for the memory of options
  QStringList fileDescriptions = ioManager->fileDescriptions(this->fileType());
  qSlicerIOOptions* options = fileDescriptions.count() ?
    ioManager->fileOptions(fileDescriptions[0]) : 0;
  qSlicerIOOptionsWidget* optionsWidget =
    dynamic_cast<qSlicerIOOptionsWidget*>(options);
  // options is not necessary a qSlicerIOOptionsWidget (for the case of
  // readers/modules with no UI. If there is a UI then add it inside the  file
  // dialog.
  if (optionsWidget)
    {
    // fileDialog will reparent optionsWidget and take care of deleting
    // optionsWidget for us.
    fileDialog.setBottomWidget(optionsWidget, tr("Options:"));
    connect(&fileDialog, SIGNAL(fileSelectionChanged(QStringList)),
            optionsWidget, SLOT(setFileNames(QStringList)));
    connect(optionsWidget, SIGNAL(validChanged(bool)),
            &fileDialog, SLOT(setAcceptButtonEnable(bool)));
    fileDialog.setAcceptButtonEnable(optionsWidget->isValid());
    }
  // we do not delete options now as it is still useful later (even if there is
  // no UI.) they are the options of the reader, UI or not.
  bool res = fileDialog.exec();
  if (res)
    {
    qSlicerIO::IOProperties properties = ioProperties;
    if (options)
      {
      properties.unite(options->properties());
      }
    else
      {
      properties["fileName"] = fileDialog.selectedFiles();
      }
    if (d->Action == qSlicerFileDialog::Read)
      {
      ioManager->loadNodes(this->fileType(), properties);
      }
    else if(d->Action == qSlicerFileDialog::Write)
      {
      ioManager->saveNodes(this->fileType(), properties);
      }
    else
      {
      Q_ASSERT(d->Action == qSlicerFileDialog::Read ||
               d->Action == qSlicerFileDialog::Write);
      }
    }

  // If the favorite list has change, we set it in the ioManager.
  if (fileDialog.sidebarUrls() != ioManager->favorites())
    {
    ioManager->setFavorites(fileDialog.sidebarUrls());
    }

  // If options is not a qSlicerIOOptionsWidget, we are responsible for
  // deleting options. If it is, then fileDialog would have reparent
  // the options and take care of its destruction
  if (!optionsWidget)
    {
    delete options;
    options = 0;
    }
  return res;
}
