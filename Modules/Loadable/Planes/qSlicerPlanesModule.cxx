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

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>

// Planes Logic includes
#include <vtkSlicerPlanesLogic.h>
#include <vtkMRMLMarkupsPlanesDisplayableManager3D.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLThreeDViewDisplayableManagerFactory.h>

// Planner includes
#include "qSlicerPlanesModule.h"

// DisplayableManager Initialization
#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkSlicerPlanesModuleMRMLDisplayableManager)

//-----------------------------------------------------------------------------

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QtPlugin>
	Q_EXPORT_PLUGIN2(qSlicerPlanesModule, qSlicerPlanesModule);
#endif
//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerPlanesModulePrivate
{
public:
  qSlicerPlanesModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerPlanesModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerPlanesModulePrivate::qSlicerPlanesModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerPlanesModule methods

//-----------------------------------------------------------------------------
qSlicerPlanesModule::qSlicerPlanesModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerPlanesModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerPlanesModule::~qSlicerPlanesModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerPlanesModule::helpText() const
{
  return "This module helps with planes interaction.";
}

//-----------------------------------------------------------------------------
QString qSlicerPlanesModule::acknowledgementText() const
{
  return "TODO";
}

//-----------------------------------------------------------------------------
QStringList qSlicerPlanesModule::contributors() const
{
  QStringList moduleContributors;
  moduleContributors << QString("Johan Andruejol (Kitware Inc.)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerPlanesModule::icon() const
{
  return QIcon(":/Icons/Planes.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerPlanesModule::categories() const
{
  return QStringList() << "Interaction";
}

//-----------------------------------------------------------------------------
QStringList qSlicerPlanesModule::dependencies() const
{
  return QStringList() << "Markups" << "Models";
}

//-----------------------------------------------------------------------------
void qSlicerPlanesModule::setup()
{
  this->Superclass::setup();

  vtkSmartPointer<vtkMRMLMarkupsPlanesDisplayableManager3D> dm3d=
    vtkSmartPointer<vtkMRMLMarkupsPlanesDisplayableManager3D>::New();

  vtkMRMLThreeDViewDisplayableManagerFactory::GetInstance()
    ->RegisterDisplayableManager("vtkMRMLMarkupsPlanesDisplayableManager3D");
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation* qSlicerPlanesModule
::createWidgetRepresentation()
{
  return NULL;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerPlanesModule::createLogic()
{
  return vtkSlicerPlanesLogic::New();
}
