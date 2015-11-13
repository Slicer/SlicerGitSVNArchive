/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// Qt includes
#include <QtPlugin>

// Slice includes
#include <qSlicerCoreApplication.h>
#include <qSlicerCoreIOManager.h>
#include <qSlicerNodeWriter.h>

// Tables Logic includes
#include <vtkSlicerTablesLogic.h>

// Tables includes
#include "qSlicerTablesModule.h"
#include "qSlicerTablesReader.h"
#include "qSlicerTablesModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerTablesModule, qSlicerTablesModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerTablesModulePrivate
{
public:
  qSlicerTablesModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerTablesModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerTablesModulePrivate::qSlicerTablesModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerTablesModule methods

//-----------------------------------------------------------------------------
qSlicerTablesModule::qSlicerTablesModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerTablesModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerTablesModule::~qSlicerTablesModule()
{
}

//-----------------------------------------------------------------------------
QIcon qSlicerTablesModule::icon()const
{
  return QIcon(":/Icons/Tables.png");
}

//-----------------------------------------------------------------------------
QString qSlicerTablesModule::helpText()const
{
  return "This module provides support for data table nodes";
}

//-----------------------------------------------------------------------------
QString qSlicerTablesModule::acknowledgementText()const
{
  return "This work was was partially funded by NIH grant 3P41RR013218-12S1";
}

//-----------------------------------------------------------------------------
QStringList qSlicerTablesModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Kevin Wang (PMH)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QStringList qSlicerTablesModule::categories() const
{
  return QStringList() << "Informatics";
}


//-----------------------------------------------------------------------------
QStringList qSlicerTablesModule::dependencies() const
{
  return QStringList();
}

//-----------------------------------------------------------------------------
void qSlicerTablesModule::setup()
{
  this->Superclass::setup();
  vtkSlicerTablesLogic* TablesLogic =
    vtkSlicerTablesLogic::SafeDownCast(this->logic());

  qSlicerCoreIOManager* ioManager =
    qSlicerCoreApplication::application()->coreIOManager();
  ioManager->registerIO(new qSlicerTablesReader(TablesLogic,this));
  ioManager->registerIO(new qSlicerNodeWriter(
    "Table", QString("TableFile"),
    QStringList() << "vtkMRMLTableNode", false, this));
}


//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerTablesModule::createWidgetRepresentation()
{
  return new qSlicerTablesModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerTablesModule::createLogic()
{
  return vtkSlicerTablesLogic::New();
}
