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

// Qt includes
#include <QDir>

// SlicerQt includes
#include "qSlicerFiberBundleReader.h"

// Logic includes
#include "vtkSlicerFiberBundleLogic.h"

// MRML includes
#include <vtkMRMLFiberBundleNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkSmartPointer.h>

//-----------------------------------------------------------------------------
class qSlicerFiberBundleReaderPrivate
{
public:
  vtkSmartPointer<vtkSlicerFiberBundleLogic> FiberBundleLogic;
};

//-----------------------------------------------------------------------------
qSlicerFiberBundleReader::qSlicerFiberBundleReader(
  vtkSlicerFiberBundleLogic* _fiberBundleLogic, QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerFiberBundleReaderPrivate)
{
  this->setFiberBundleLogic(_fiberBundleLogic);
}

//-----------------------------------------------------------------------------
qSlicerFiberBundleReader::~qSlicerFiberBundleReader()
{
}

//-----------------------------------------------------------------------------
void qSlicerFiberBundleReader::setFiberBundleLogic(vtkSlicerFiberBundleLogic* newFiberBundleLogic)
{
  Q_D(qSlicerFiberBundleReader);
  d->FiberBundleLogic = newFiberBundleLogic;
}

//-----------------------------------------------------------------------------
vtkSlicerFiberBundleLogic* qSlicerFiberBundleReader::fiberBundleLogic()const
{
  Q_D(const qSlicerFiberBundleReader);
  return d->FiberBundleLogic;
}

//-----------------------------------------------------------------------------
QString qSlicerFiberBundleReader::description()const
{
  return "FiberBundle";
}

//-----------------------------------------------------------------------------
qSlicerIO::IOFileType qSlicerFiberBundleReader::fileType()const
{
  return QString("FiberBundleFile");
}

//-----------------------------------------------------------------------------
QStringList qSlicerFiberBundleReader::extensions()const
{
  return QStringList("*.*");
}

//-----------------------------------------------------------------------------
bool qSlicerFiberBundleReader::load(const IOProperties& properties)
{
  Q_D(qSlicerFiberBundleReader);
  Q_ASSERT(properties.contains("fileName"));
  QString fileName = properties["fileName"].toString();

  QStringList fileNames;
  if (properties.contains("suffix"))
    {
    QStringList suffixList = properties["suffix"].toStringList();
    suffixList.removeDuplicates();
    // here filename describes a directory
    Q_ASSERT(QFileInfo(fileName).isDir());
    QDir dir(fileName);
    // suffix should be of style: *.png
    fileNames = dir.entryList(suffixList);
    }
  else
    {
    fileNames << fileName;
    }

  if (d->FiberBundleLogic.GetPointer() == 0)
    {
    return false;
    }

  QStringList nodes;
  foreach(QString file, fileNames)
    {
    vtkMRMLFiberBundleNode* node = d->FiberBundleLogic->AddFiberBundle(file.toLatin1());
    if (node)
      {
      if (properties.contains("name"))
        {
        std::string uname = this->mrmlScene()->GetUniqueNameByString(
          properties["name"].toString().toLatin1());
        node->SetName(uname.c_str());
        }
      nodes << node->GetID();
      }
    }
  this->setLoadedNodes(nodes);

  return nodes.size() > 0;
}
