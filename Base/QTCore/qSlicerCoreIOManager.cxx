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
#include <QDebug>
#include <QFileInfo>

// CTK includes
#include <ctkUtils.h>

// SlicerQt includes
#include "qSlicerCoreApplication.h"
#include "qSlicerCoreIOManager.h"
#include "qSlicerFileReader.h"
#include "qSlicerFileWriter.h"

// MRML includes
#include <vtkMRMLNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLStorableNode.h>
#include <vtkMRMLStorageNode.h>

// VTK includes
#include <vtkCollection.h>
#include <vtkDataFileFormatHelper.h> // for GetFileExtensionFromFormatString()
#include <vtkNew.h>
#include <vtkStringArray.h>

//-----------------------------------------------------------------------------
class qSlicerCoreIOManagerPrivate
{
public:
  qSlicerCoreIOManagerPrivate();
  ~qSlicerCoreIOManagerPrivate();
  vtkMRMLScene* currentScene()const;

  qSlicerFileReader* reader(const QString& fileName)const;
  QList<qSlicerFileReader*> readers(const QString& fileName)const;

  QList<qSlicerFileWriter*> writers(const qSlicerIO::IOFileType &fileType, const qSlicerIO::IOProperties& parameters)const;

  QSettings*        ExtensionFileType;
  QList<qSlicerFileReader*> Readers;
  QList<qSlicerFileWriter*> Writers;
  QMap<qSlicerIO::IOFileType, QStringList> FileTypes;
};

//-----------------------------------------------------------------------------
qSlicerCoreIOManagerPrivate::qSlicerCoreIOManagerPrivate()
{
}

//-----------------------------------------------------------------------------
qSlicerCoreIOManagerPrivate::~qSlicerCoreIOManagerPrivate()
{
}

//-----------------------------------------------------------------------------
vtkMRMLScene* qSlicerCoreIOManagerPrivate::currentScene()const
{
  return qSlicerCoreApplication::application()->mrmlScene();
}

//-----------------------------------------------------------------------------
qSlicerFileReader* qSlicerCoreIOManagerPrivate::reader(const QString& fileName)const
{
  QList<qSlicerFileReader*> matchingReaders = this->readers(fileName);
  return matchingReaders.count() ? matchingReaders[0] : 0;
}

//-----------------------------------------------------------------------------
QList<qSlicerFileReader*> qSlicerCoreIOManagerPrivate::readers(const QString& fileName)const
{
  QList<qSlicerFileReader*> matchingReaders;
  // Some readers ("DICOM (*)" or "Scalar Overlay (*.*))" can support any file,
  // they are called generic readers. They might not be the best choice to read
  // the file as it might exist a more specific reader to read it.
  // So let's add generic readers at the end of the reader list.
  QList<qSlicerFileReader*> genericReaders;
  foreach(qSlicerFileReader* reader, this->Readers)
    {
    QStringList matchingNameFilters = reader->supportedNameFilters(fileName);
    if (matchingNameFilters.count() == 0)
      {
      continue;
      }
    // Generic readers must be added to the end
    foreach(const QString& nameFilter, matchingNameFilters)
      {
      if (nameFilter.contains( "*.*" ) || nameFilter.contains("(*)"))
        {
        genericReaders << reader;
        continue;
        }
      if (!matchingReaders.contains(reader))
        {
        matchingReaders << reader;
        }
      }
    }
  foreach(qSlicerFileReader* reader, genericReaders)
    {
    if (!matchingReaders.contains(reader))
      {
      matchingReaders << reader;
      }
    }
  return matchingReaders;
}

//-----------------------------------------------------------------------------
QList<qSlicerFileWriter*> qSlicerCoreIOManagerPrivate::writers(
    const qSlicerIO::IOFileType& fileType, const qSlicerIO::IOProperties& parameters)const
{
  QString fileName = parameters.value("fileName").toString();
  QString nodeID = parameters.value("nodeID").toString();

  vtkObject * object = this->currentScene()->GetNodeByID(nodeID.toLatin1());
  QFileInfo file(fileName);

  QList<qSlicerFileWriter*> matchingWriters;
  // Some writers ("Slicer Data Bundle (*)" can support any file,
  // they are called generic writers. The following code ensures
  // that writers associated with specific file extension are
  // considered first.
  QList<qSlicerFileWriter*> genericWriters;
  foreach(qSlicerFileWriter* writer, this->Writers)
    {
    if (writer->fileType() != fileType)
      {
      continue;
      }
    QStringList matchingNameFilters;
    foreach(const QString& nameFilter, writer->extensions(object))
      {
      foreach(const QString& extension, ctk::nameFilterToExtensions(nameFilter))
        {
        // HACK - See http://www.na-mic.org/Bug/view.php?id=3322
        QString extensionWithStar(extension);
        if (!extensionWithStar.startsWith("*"))
          {
          extensionWithStar.prepend("*");
          }
        QRegExp regExp(extensionWithStar, Qt::CaseInsensitive, QRegExp::Wildcard);
        Q_ASSERT(regExp.isValid());
        if (regExp.exactMatch(file.absoluteFilePath()))
          {
          matchingNameFilters << nameFilter;
          }
        }
      }
    if (matchingNameFilters.count() == 0)
      {
      continue;
      }
    // Generic readers must be added to the end
    foreach(const QString& nameFilter, matchingNameFilters)
      {
      if (nameFilter.contains( "*.*" ) || nameFilter.contains("(*)"))
        {
        genericWriters << writer;
        continue;
        }
      if (!matchingWriters.contains(writer))
        {
        matchingWriters << writer;
        }
      }
    }
  foreach(qSlicerFileWriter* writer, genericWriters)
    {
    if (!matchingWriters.contains(writer))
      {
      matchingWriters << writer;
      }
    }
  return matchingWriters;
}

//-----------------------------------------------------------------------------
qSlicerCoreIOManager::qSlicerCoreIOManager(QObject* _parent)
  :QObject(_parent)
  , d_ptr(new qSlicerCoreIOManagerPrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerCoreIOManager::~qSlicerCoreIOManager()
{
}

//-----------------------------------------------------------------------------
qSlicerIO::IOFileType qSlicerCoreIOManager::fileType(const QString& fileName)const
{
  QList<qSlicerIO::IOFileType> matchingFileTypes = this->fileTypes(fileName);
  return matchingFileTypes.count() ? matchingFileTypes[0] : QString("NoFile");
}

//-----------------------------------------------------------------------------
qSlicerIO::IOFileType qSlicerCoreIOManager
::fileTypeFromDescription(const QString& fileDescription)const
{
  qSlicerFileReader* reader = this->reader(fileDescription);
  return reader ? reader->fileType() : QString("NoFile");
}

//-----------------------------------------------------------------------------
qSlicerIO::IOFileType qSlicerCoreIOManager
::fileWriterFileType(vtkObject* object)const
{
  Q_D(const qSlicerCoreIOManager);
  QList<qSlicerIO::IOFileType> matchingFileTypes;
  foreach (const qSlicerFileWriter* writer, d->Writers)
    {
    if (writer->canWriteObject(object))
      {
      return writer->fileType();
      }
    }
  return QString("NoFile");
}

//-----------------------------------------------------------------------------
QList<qSlicerIO::IOFileType> qSlicerCoreIOManager::fileTypes(const QString& fileName)const
{
  Q_D(const qSlicerCoreIOManager);
  QList<qSlicerIO::IOFileType> matchingFileTypes;
  foreach (const qSlicerIO* matchingReader, d->readers(fileName))
    {
    matchingFileTypes << matchingReader->fileType();
    }
  return matchingFileTypes;
}

//-----------------------------------------------------------------------------
QStringList qSlicerCoreIOManager::fileDescriptions(const QString& fileName)const
{
  Q_D(const qSlicerCoreIOManager);
  QStringList matchingDescriptions;
  foreach(qSlicerFileReader* reader, d->readers(fileName))
    {
    matchingDescriptions << reader->description();
    }
  return matchingDescriptions;
}

//-----------------------------------------------------------------------------
QStringList qSlicerCoreIOManager::
fileDescriptionsByType(const qSlicerIO::IOFileType fileType)const
{
  QStringList matchingDescriptions;
  foreach(qSlicerFileReader* reader, this->readers())
    {
    if (reader->fileType() == fileType)
      {
      matchingDescriptions << reader->description();
      }
    }
  return matchingDescriptions;
}

//-----------------------------------------------------------------------------
QStringList qSlicerCoreIOManager::fileWriterDescriptions(
  const qSlicerIO::IOFileType& fileType)const
{
  QStringList matchingDescriptions;
  foreach(qSlicerFileWriter* writer, this->writers(fileType))
    {
    matchingDescriptions << writer->description();
    }
  return matchingDescriptions;
}

//-----------------------------------------------------------------------------
QStringList qSlicerCoreIOManager::fileWriterExtensions(
  vtkObject* object)const
{
  Q_D(const qSlicerCoreIOManager);
  QStringList matchingExtensions;
  foreach(qSlicerFileWriter* writer, d->Writers)
    {
    if (writer->canWriteObject(object))
      {
      matchingExtensions << writer->extensions(object);
      }
    }
  matchingExtensions.removeDuplicates();
  return matchingExtensions;
}

//-----------------------------------------------------------------------------
QStringList qSlicerCoreIOManager::allWritableFileExtensions()const
{
  Q_D(const qSlicerCoreIOManager);

  QStringList extensions;

  if (!d->currentScene())
    {
    qWarning() << "allWritableFileExtensions: manager has no scene defined";
    return extensions;
    }
  // check for all extensions that can be used to write storable nodes
  int numRegisteredNodeClasses = d->currentScene()->GetNumberOfRegisteredNodeClasses();
  for (int i = 0; i < numRegisteredNodeClasses; ++i)
    {
    vtkMRMLNode *mrmlNode = d->currentScene()->GetNthRegisteredNodeClass(i);
    if (mrmlNode && mrmlNode->IsA("vtkMRMLStorageNode"))
      {
      vtkMRMLStorageNode* snode = vtkMRMLStorageNode::SafeDownCast(mrmlNode);
      if (snode)
        {
        const int formatCount = snode->GetSupportedWriteFileTypes()->GetNumberOfValues();
        for (int formatIt = 0; formatIt < formatCount; ++formatIt)
          {
          vtkStdString format = snode->GetSupportedWriteFileTypes()->GetValue(formatIt);
          QString extension = QString::fromStdString(
                 vtkDataFileFormatHelper::GetFileExtensionFromFormatString(format));
          extensions << extension;
          }
        }
      }
    }
  extensions.removeDuplicates();
  return extensions;
}

//-----------------------------------------------------------------------------
QStringList qSlicerCoreIOManager::allReadableFileExtensions()const
{
  Q_D(const qSlicerCoreIOManager);

  QStringList extensions;

  if (!d->currentScene())
    {
    qWarning() << "allReadableFileExtensions: manager has no scene defined";
    return extensions;
    }
  // check for all extensions that can be used to read storable nodes
  int numRegisteredNodeClasses = d->currentScene()->GetNumberOfRegisteredNodeClasses();
  for (int i = 0; i < numRegisteredNodeClasses; ++i)
    {
    vtkMRMLNode *mrmlNode = d->currentScene()->GetNthRegisteredNodeClass(i);
    if (mrmlNode && mrmlNode->IsA("vtkMRMLStorageNode"))
      {
      vtkMRMLStorageNode* snode = vtkMRMLStorageNode::SafeDownCast(mrmlNode);
      if (snode)
        {
        const int formatCount = snode->GetSupportedReadFileTypes()->GetNumberOfValues();
        for (int formatIt = 0; formatIt < formatCount; ++formatIt)
          {
          vtkStdString format = snode->GetSupportedReadFileTypes()->GetValue(formatIt);
          QString extension = QString::fromStdString(
                 vtkDataFileFormatHelper::GetFileExtensionFromFormatString(format));
          extensions << extension;
          }
        }
      }
    }
  extensions.removeDuplicates();
  return extensions;
}

//-----------------------------------------------------------------------------
qSlicerIOOptions* qSlicerCoreIOManager::fileOptions(const QString& readerDescription)const
{
  Q_D(const qSlicerCoreIOManager);
  qSlicerFileReader* reader = this->reader(readerDescription);
  if (!reader)
    {
    return 0;
    }
  reader->setMRMLScene(d->currentScene());
  return reader->options();
}

//-----------------------------------------------------------------------------
qSlicerIOOptions* qSlicerCoreIOManager::fileWriterOptions(
  vtkObject* object, const QString& extension)const
{
  Q_D(const qSlicerCoreIOManager);
  qSlicerFileWriter* bestWriter = 0;
  foreach(qSlicerFileWriter* writer, d->Writers)
    {
    if (writer->canWriteObject(object))
      {
      if (writer->extensions(object).contains(extension))
        {
        writer->setMRMLScene(d->currentScene());
        bestWriter = writer;
        }
      }
    }
  return bestWriter ? bestWriter->options() : 0;
}

//-----------------------------------------------------------------------------
QString qSlicerCoreIOManager::completeSlicerWritableFileNameSuffix(const QString &fileName)const
{
  // first get all possible Slicer file extensions
  QStringList allExtensions = qSlicerCoreIOManager::allWritableFileExtensions();
  // then iterate through them to find one that matches
  foreach (QString extension, allExtensions)
    {
    // check if this extension is at the end of the file name
    if (fileName.endsWith(extension))
      {
      return extension;
      }
    }
  if (allExtensions.contains(QString(".*")))
    {
    // if the .* option is in the valid extensions list,
    // use the QFileInfo complete suffix. That will return
    // a string of everything following the first '.', then
    // prepend the dot to match the extensions returned from above
    QString suffix = QString (".") + QFileInfo(fileName).completeSuffix();
    qDebug() << "Slicer extension not found in file name " << fileName
             << ", returning Qt complete suffix as .* case " << suffix;
    return suffix;
    }
  // otherwise return an empty suffix
  return QString (".");
}

//-----------------------------------------------------------------------------
bool qSlicerCoreIOManager::loadScene(const QString& fileName, bool clear)
{
  qSlicerIO::IOProperties properties;
  properties["fileName"] = fileName;
  properties["clear"] = clear;
  return this->loadNodes(QString("SceneFile"), properties);
}

//-----------------------------------------------------------------------------
bool qSlicerCoreIOManager::loadFile(const QString& fileName)
{
  qSlicerIO::IOProperties properties;
  properties["fileName"] = fileName;
  return this->loadNodes(this->fileType(fileName), properties);
}

//-----------------------------------------------------------------------------
bool qSlicerCoreIOManager::loadNodes(const qSlicerIO::IOFileType& fileType,
#if QT_VERSION < 0x040700
                                     const QVariantMap& parameters,
#else
                                     const qSlicerIO::IOProperties& parameters,
#endif
                                     vtkCollection* loadedNodes)
{
  Q_D(qSlicerCoreIOManager);

  Q_ASSERT(parameters.contains("fileName"));
  if (parameters["fileName"].type() == QVariant::StringList)
    {
    bool res = true;
    QStringList fileNames = parameters["fileName"].toStringList();
    QStringList names = parameters["name"].toStringList();
    int nameId = 0;
    foreach(const QString& fileName, fileNames)
      {
      qSlicerIO::IOProperties fileParameters = parameters;
      fileParameters["fileName"] = fileName;
      if (!names.isEmpty())
        {
        fileParameters["name"] = nameId < names.size() ? names[nameId] : names.last();
        ++nameId;
        }
      res &= this->loadNodes(fileType, fileParameters, loadedNodes);
      }
    return res;
    }
  Q_ASSERT(!parameters["fileName"].toString().isEmpty());

  qSlicerIO::IOProperties loadedFileParameters = parameters;
  loadedFileParameters.insert("fileType", fileType);

  const QList<qSlicerFileReader*>& readers = this->readers(fileType);

  // If no readers were able to read and load the file(s), success will remain false
  bool success = false;

  QStringList nodes;
  foreach (qSlicerFileReader* reader, readers)
    {
    reader->setMRMLScene(d->currentScene());
    if (!reader->canLoadFile(parameters["fileName"].toString()))
      {
      continue;
      }
    if (!reader->load(parameters))
      {
      continue;
      }
    qDebug() << reader->description() << "Reader has successfully read the file"
             << parameters["fileName"].toString();
    nodes << reader->loadedNodes();
    success = true;
    break;
    }

  loadedFileParameters.insert("nodeIDs", nodes);

  emit newFileLoaded(loadedFileParameters);

  if (loadedNodes)
    {
    foreach(const QString& node, nodes)
      {
      loadedNodes->AddItem(
        d->currentScene()->GetNodeByID(node.toLatin1()));
      }
    }

  return success;
}

//-----------------------------------------------------------------------------
bool qSlicerCoreIOManager::
loadNodes(const QList<qSlicerIO::IOProperties>& files,
          vtkCollection* loadedNodes)
{
  bool res = true;
  foreach(qSlicerIO::IOProperties fileProperties, files)
    {
    res = this->loadNodes(
      static_cast<qSlicerIO::IOFileType>(fileProperties["fileType"].toString()),
      fileProperties,
      loadedNodes)

      && res;
    }
  return res;
}

//-----------------------------------------------------------------------------
vtkMRMLNode* qSlicerCoreIOManager::loadNodesAndGetFirst(
  qSlicerIO::IOFileType fileType,
  const qSlicerIO::IOProperties& parameters)
{
  vtkNew<vtkCollection> loadedNodes;
  this->loadNodes(fileType, parameters, loadedNodes.GetPointer());

  vtkMRMLNode* node = vtkMRMLNode::SafeDownCast(loadedNodes->GetItemAsObject(0));
  Q_ASSERT(node);

  return node;
}

//-----------------------------------------------------------------------------
vtkMRMLStorageNode* qSlicerCoreIOManager::createAndAddDefaultStorageNode(
    vtkMRMLStorableNode* node)
{
  vtkMRMLStorageNode* snode = node ? node->GetStorageNode() : 0;
  if (snode == 0 && node != 0)
    {
    snode = node->CreateDefaultStorageNode();
    if (snode != 0)
      {
      node->GetScene()->AddNode(snode);
      snode->Delete();
      node->SetAndObserveStorageNodeID(snode->GetID());
      }
    }
  return snode;
}

//-----------------------------------------------------------------------------
void qSlicerCoreIOManager::emitNewFileLoaded(const QVariantMap& loadedFileParameters)
{
  emit this->newFileLoaded(loadedFileParameters);
}

//-----------------------------------------------------------------------------
bool qSlicerCoreIOManager::saveNodes(qSlicerIO::IOFileType fileType,
                                     const qSlicerIO::IOProperties& parameters)
{
  Q_D(qSlicerCoreIOManager);

  Q_ASSERT(parameters.contains("fileName"));

  // HACK - See http://www.na-mic.org/Bug/view.php?id=3322
  //        Sort writers to ensure generic ones are last.
  const QList<qSlicerFileWriter*> writers = d->writers(fileType, parameters);
  if (writers.isEmpty())
    {
    qWarning() << "No writer found to write file" << parameters.value("fileName")
               << "of type" << fileType;
    return false;
    }

  QStringList nodes;
  bool writeSuccess=false;
  foreach (qSlicerFileWriter* writer, writers)
    {
    writer->setMRMLScene(d->currentScene());
    if (!writer->write(parameters))
      {
      continue;
      }
    nodes << writer->writtenNodes();
    writeSuccess = true;
    break;
    }

  if (!writeSuccess)
    {
    // no appropriate writer was found
    return false;
    }

  if (nodes.count() == 0 &&
      fileType != QString("SceneFile"))
    {
    // the writer did not report error
    // but did not report any successfully written nodes either
    return false;
    }

  return true;
}

//-----------------------------------------------------------------------------
bool qSlicerCoreIOManager::saveScene(const QString& fileName, QImage screenShot)
{
  qSlicerIO::IOProperties properties;
  properties["fileName"] = fileName;
  properties["screenShot"] = screenShot;

  return this->saveNodes(QString("SceneFile"), properties);
}

//-----------------------------------------------------------------------------
const QList<qSlicerFileReader*>& qSlicerCoreIOManager::readers()const
{
  Q_D(const qSlicerCoreIOManager);
  return d->Readers;
}

//-----------------------------------------------------------------------------
const QList<qSlicerFileWriter*>& qSlicerCoreIOManager::writers()const
{
  Q_D(const qSlicerCoreIOManager);
  return d->Writers;
}

//-----------------------------------------------------------------------------
QList<qSlicerFileReader*> qSlicerCoreIOManager::readers(const qSlicerIO::IOFileType& fileType)const
{
  Q_D(const qSlicerCoreIOManager);
  QList<qSlicerFileReader*> res;
  foreach(qSlicerFileReader* io, d->Readers)
    {
    if (io->fileType() == fileType)
      {
      res << io;
      }
    }
  return res;
}

//-----------------------------------------------------------------------------
QList<qSlicerFileWriter*> qSlicerCoreIOManager::writers(const qSlicerIO::IOFileType& fileType)const
{
  Q_D(const qSlicerCoreIOManager);
  QList<qSlicerFileWriter*> res;
  foreach(qSlicerFileWriter* io, d->Writers)
    {
    if (io->fileType() == fileType)
      {
      res << io;
      }
    }
  return res;
}

//-----------------------------------------------------------------------------
qSlicerFileReader* qSlicerCoreIOManager::reader(const QString& ioDescription)const
{
  Q_D(const qSlicerCoreIOManager);
  QList<qSlicerFileReader*> res;
  foreach(qSlicerFileReader* io, d->Readers)
    {
    if (io->description() == ioDescription)
      {
      res << io;
      }
    }
  Q_ASSERT(res.count() < 2);
  return res.count() ? res[0] : 0;
}

//-----------------------------------------------------------------------------
void qSlicerCoreIOManager::registerIO(qSlicerIO* io)
{
  Q_ASSERT(io);
  Q_D(qSlicerCoreIOManager);
  qSlicerFileReader* fileReader = qobject_cast<qSlicerFileReader*>(io);
  qSlicerFileWriter* fileWriter = qobject_cast<qSlicerFileWriter*>(io);
  if (fileWriter)
    {
    d->Writers << fileWriter;
    }
  else if (fileReader)
    {
    d->Readers << fileReader;
    }

  // Reparent - this will make sure the object is destroyed properly
  if (io)
    {
    io->setParent(this);
    }
}
