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
and was partially funded by Allen Institute

==============================================================================*/

// Splines Logic includes
#include <vtkSlicerSplinesLogic.h>

// Splines includes
#include "qSlicerSplinesModule.h"
#include "qSlicerSplinesModuleWidget.h"

#include <vtkMRMLSliceViewDisplayableManagerFactory.h>
#include <vtkMRMLThreeDViewDisplayableManagerFactory.h>

#include <vtkMRMLMarkupsSplinesDisplayableManager2D.h>
#include <vtkMRMLMarkupsSplinesDisplayableManager3D.h>
#include <vtkSmartPointer.h>

// DisplayableManager initialization
#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkSlicerSplinesModuleMRMLDisplayableManager)

//-----------------------------------------------------------------------------
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QtPlugin>
Q_EXPORT_PLUGIN2(qSlicerSplinesModule, qSlicerSplinesModule);
#endif

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerSplinesModulePrivate
{
public:
  qSlicerSplinesModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerSplinesModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerSplinesModulePrivate::qSlicerSplinesModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerSplinesModule methods

//-----------------------------------------------------------------------------
qSlicerSplinesModule::qSlicerSplinesModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerSplinesModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerSplinesModule::~qSlicerSplinesModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerSplinesModule::helpText() const
{
  return "Manages Spliness in 2D and 3D";
}

//-----------------------------------------------------------------------------
QString qSlicerSplinesModule::acknowledgementText() const
{
  return "This work was partially funded by Allen Institute";
}

//-----------------------------------------------------------------------------
QStringList qSlicerSplinesModule::contributors() const
{
  QStringList moduleContributors;
  moduleContributors << QString("Johan Andruejol (Kitware Inc.)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerSplinesModule::icon() const
{
  return QIcon(":/Icons/Splines.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerSplinesModule::categories() const
{
  return QStringList() << "" << "Informatics";
}

//-----------------------------------------------------------------------------
QStringList qSlicerSplinesModule::dependencies() const
{
  return QStringList() << "Markups";
}

//-----------------------------------------------------------------------------
void qSlicerSplinesModule::setup()
{
  this->Superclass::setup();

  // This is just to ensure that the displayable managers can be
  // instantiated by the factories
  vtkSmartPointer<vtkMRMLMarkupsSplinesDisplayableManager2D> dm2D =
    vtkSmartPointer<vtkMRMLMarkupsSplinesDisplayableManager2D>::New();
  vtkSmartPointer<vtkMRMLMarkupsSplinesDisplayableManager3D> dm3D =
    vtkSmartPointer<vtkMRMLMarkupsSplinesDisplayableManager3D>::New();

  // Actually register the displayable manager here
  vtkMRMLSliceViewDisplayableManagerFactory::GetInstance()
    ->RegisterDisplayableManager("vtkMRMLMarkupsSplinesDisplayableManager2D");
  vtkMRMLThreeDViewDisplayableManagerFactory::GetInstance()
    ->RegisterDisplayableManager("vtkMRMLMarkupsSplinesDisplayableManager3D");
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation* qSlicerSplinesModule
::createWidgetRepresentation()
{
  return new qSlicerSplinesModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerSplinesModule::createLogic()
{
  return vtkSlicerSplinesLogic::New();
}
