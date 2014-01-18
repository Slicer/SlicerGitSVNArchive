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
#include <QtPlugin>

// SlicerQt includes
#include "qSlicerApplication.h"
#include "qSlicerIOManager.h"
#include "qSlicerModuleManager.h"

// Data includes
#include "qSlicerDataDialog.h"
#include "qSlicerDataModule.h"
#include "qSlicerDataModuleWidget.h"
#include "qSlicerSaveDataDialog.h"
#include "qSlicerSceneBundleReader.h"
#include "qSlicerSceneReader.h"
#include "qSlicerSceneWriter.h"
#include "qSlicerSlicer2SceneReader.h"
#include "qSlicerXcedeCatalogReader.h"

// SlicerLogic includes
#include <vtkSlicerApplicationLogic.h>

// Data Logic includes
#include "vtkSlicerDataModuleLogic.h"

// Logic includes
#include <vtkMRMLColorLogic.h>
#include <vtkSlicerCamerasModuleLogic.h>

// VTK includes
#include <vtkSmartPointer.h>

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerDataModule, qSlicerDataModule);

//-----------------------------------------------------------------------------
class qSlicerDataModulePrivate
{
public:
};

//-----------------------------------------------------------------------------
qSlicerDataModule::qSlicerDataModule(QObject* parentObject)
  :Superclass(parentObject)
  , d_ptr(new qSlicerDataModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerDataModule::~qSlicerDataModule()
{
}

//-----------------------------------------------------------------------------
QIcon qSlicerDataModule::icon()const
{
  return QIcon(":/Icons/Data.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerDataModule::categories() const
{
  return QStringList() << "" << "Informatics";
}

//-----------------------------------------------------------------------------
QStringList qSlicerDataModule::dependencies() const
{
  QStringList moduleDependencies;
  // Colors: Required to have a valid color logic for XcedeCatalogUI.
  // Cameras: Required in qSlicerSceneReader
  moduleDependencies << "Colors" << "Cameras";
  return moduleDependencies;
}

//-----------------------------------------------------------------------------
void qSlicerDataModule::setup()
{
  this->Superclass::setup();

  qSlicerAbstractCoreModule* colorsModule =
    qSlicerCoreApplication::application()->moduleManager()->module("Colors");
  vtkMRMLColorLogic* colorLogic =
    vtkMRMLColorLogic::SafeDownCast(colorsModule ? colorsModule->logic() : 0);

  qSlicerAbstractCoreModule* camerasModule =
    qSlicerCoreApplication::application()->moduleManager()->module("Cameras");
  vtkSlicerCamerasModuleLogic* camerasLogic =
    vtkSlicerCamerasModuleLogic::SafeDownCast(camerasModule ? camerasModule->logic() : 0);

  qSlicerIOManager* ioManager = qSlicerApplication::application()->ioManager();

  // Readers
  ioManager->registerIO(new qSlicerSceneReader(camerasLogic, this));
  ioManager->registerIO(new qSlicerSceneBundleReader(this));
  ioManager->registerIO(new qSlicerSlicer2SceneReader(this->appLogic(), this));
  ioManager->registerIO(new qSlicerXcedeCatalogReader(colorLogic, this));

  // Writers
  ioManager->registerIO(new qSlicerSceneWriter(this));

  // Dialogs
  ioManager->registerDialog(new qSlicerDataDialog(this));
  ioManager->registerDialog(new qSlicerSaveDataDialog(this));
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerDataModule::createWidgetRepresentation()
{
  qSlicerDataModuleWidget *widget = new qSlicerDataModuleWidget;
  return widget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerDataModule::createLogic()
{
  return vtkSlicerDataModuleLogic::New();
}

//-----------------------------------------------------------------------------
QString qSlicerDataModule::helpText()const
{
  QString help =
    "The Data Module displays and permits operations on the MRML tree, and "
    "creates and edits transformation hierarchies.<br>"
    "The Load panels exposes options for loading data. Helpful comments can be "
    "opened by clicking on the \"information\" icons in each load panel.<br>"
    "<a href=\"%1/Documentation/%2.%3/Modules/Data\">"
    "%1/Documentation/%2.%3/Modules/Data</a>";
  return help.arg(this->slicerWikiUrl()).arg(Slicer_VERSION_MAJOR).arg(Slicer_VERSION_MINOR);
}

//-----------------------------------------------------------------------------
QString qSlicerDataModule::acknowledgementText()const
{
  QString about =
    "<center><table border=\"0\"><tr>"
    "<td><img src=\":Logos/NAMIC.png\" alt\"NA-MIC\"></td>"
    "<td><img src=\":Logos/NAC.png\" alt\"NAC\"></td>"
    "</tr><tr>"
    "<td><img src=\":Logos/BIRN-NoText.png\" alt\"BIRN\"></td>"
    "<td><img src=\":Logos/NCIGT.png\" alt\"NCIGT\"></td>"
    "</tr></table></center>"
    "This work was supported by NA-MIC, NAC, BIRN, NCIGT, CTSC, and the Slicer "
    "Community.<br>"
    "See <a href=\"http://www.slicer.org\">www.slicer.org</a> for details.<br>";
  return about;
}

//-----------------------------------------------------------------------------
QStringList qSlicerDataModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Julien Finet (Kitware)");
  moduleContributors << QString("Alex Yarmarkovich (Isomics)");
  moduleContributors << QString("Nicole Aucoin (SPL, BWH)");
  moduleContributors << QString("Csaba Pinter (Queen's)");
  return moduleContributors;
}
