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

// QtCore includes
#include "qSlicerSceneReader.h"
#include "qSlicerSceneIOOptionsWidget.h"

// Logic includes
#include <vtkSlicerCamerasModuleLogic.h>

// MRML includes
#include <vtkMRMLScene.h>

class qSlicerSceneReaderPrivate
{
public:
  vtkSmartPointer<vtkSlicerCamerasModuleLogic> CamerasLogic;
};

//-----------------------------------------------------------------------------
qSlicerSceneReader::qSlicerSceneReader(vtkSlicerCamerasModuleLogic* camerasLogic,
                               QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerSceneReaderPrivate)
{
  Q_D(qSlicerSceneReader);
  d->CamerasLogic = camerasLogic;
}

//-----------------------------------------------------------------------------
qSlicerSceneReader::~qSlicerSceneReader()
{
}

//-----------------------------------------------------------------------------
QString qSlicerSceneReader::description()const
{
  return "MRML Scene";
}

//-----------------------------------------------------------------------------
qSlicerIO::IOFileType qSlicerSceneReader::fileType()const
{
  return QString("SceneFile");
}

//-----------------------------------------------------------------------------
QStringList qSlicerSceneReader::extensions()const
{
  return QStringList() << "*.mrml";
}

//-----------------------------------------------------------------------------
qSlicerIOOptions* qSlicerSceneReader::options()const
{
  return new qSlicerSceneIOOptionsWidget;
}

//-----------------------------------------------------------------------------
bool qSlicerSceneReader::load(const qSlicerIO::IOProperties& properties)
{
  Q_D(qSlicerSceneReader);
  Q_ASSERT(properties.contains("fileName"));
  QString file = properties["fileName"].toString();
  this->mrmlScene()->SetURL(file.toLatin1());
  bool clear = properties.value("clear", false).toBool();
  int res = 0;
  if (clear)
    {
    res = this->mrmlScene()->Connect();
    }
  else
    {
    bool wasCopying = d->CamerasLogic->GetCopyImportedCameras();
    bool copyCameras = properties.value("copyCameras", wasCopying).toBool();
    d->CamerasLogic->SetCopyImportedCameras(copyCameras);
    res = this->mrmlScene()->Import();
    d->CamerasLogic->SetCopyImportedCameras(wasCopying);
    }
  return res;
}
