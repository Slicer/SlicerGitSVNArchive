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

// ExtensionTemplate Logic includes
#include <vtkSlicerRSSLoadableModuleLogic.h>

// ExtensionTemplate includes
#include "qSlicerRSSLoadableModuleModule.h"
#include "qSlicerRSSLoadableModuleModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerRSSLoadableModuleModule, qSlicerRSSLoadableModuleModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerRSSLoadableModuleModulePrivate
{
public:
  qSlicerRSSLoadableModuleModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerRSSLoadableModuleModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerRSSLoadableModuleModulePrivate::qSlicerRSSLoadableModuleModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerRSSLoadableModuleModule methods

//-----------------------------------------------------------------------------
qSlicerRSSLoadableModuleModule::qSlicerRSSLoadableModuleModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerRSSLoadableModuleModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerRSSLoadableModuleModule::~qSlicerRSSLoadableModuleModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerRSSLoadableModuleModule::helpText()const
{
  return "This is a loadable module bundled in an extension";
}

//-----------------------------------------------------------------------------
QString qSlicerRSSLoadableModuleModule::acknowledgementText()const
{
  return "This work was was partially funded by NIH grant 3P41RR013218-12S1";
}

//-----------------------------------------------------------------------------
QStringList qSlicerRSSLoadableModuleModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Jean-Christophe Fillion-Robin (Kitware)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerRSSLoadableModuleModule::icon()const
{
  return QIcon(":/Icons/RSSLoadableModule.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerRSSLoadableModuleModule::categories() const
{
  return QStringList() << "Examples";
}

//-----------------------------------------------------------------------------
QStringList qSlicerRSSLoadableModuleModule::dependencies() const
{
  return QStringList();
}

//-----------------------------------------------------------------------------
void qSlicerRSSLoadableModuleModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerRSSLoadableModuleModule::createWidgetRepresentation()
{
  return new qSlicerRSSLoadableModuleModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerRSSLoadableModuleModule::createLogic()
{
  return vtkSlicerRSSLoadableModuleLogic::New();
}
