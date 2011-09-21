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

// QT includes
#include <QtPlugin>

// SlicerQt includes
#include <qSlicerCoreApplication.h>
#include <qSlicerCoreIOManager.h>


// Tractography QTModule includes
#include "qSlicerTractographyDisplayModule.h"
#include "qSlicerTractographyDisplayModuleWidget.h"
//#include "qSlicerTractographyDisplayWidget.h"
#include "qSlicerFiberBundleIO.h"

// Tractography Logic includes
#include "vtkSlicerFiberBundleLogic.h"

// Setup PythonQt wrapping if enabled
#ifdef Slicer_USE_PYTHONQT
# include <PythonQt.h>
//void PythonQt_init_org_slicer_libs_qSlicerTractographyDisplayWidgets(PyObject*);
#endif 

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerTractographyDisplayModule, qSlicerTractographyDisplayModule);

//-----------------------------------------------------------------------------
qSlicerTractographyDisplayModule::
qSlicerTractographyDisplayModule(QObject* _parent):Superclass(_parent)
{
}

//-----------------------------------------------------------------------------
void qSlicerTractographyDisplayModule::setup()
{
  this->Superclass::setup();
  qSlicerCoreApplication::application()->coreIOManager()->registerIO(
    new qSlicerFiberBundleIO(this));

  #ifdef Slicer_USE_PYTHONQT
    //PythonQt_init_org_slicer_libs_qSlicerTractographyDisplayWidgets(0);
  #endif
    
}

//-----------------------------------------------------------------------------
QString qSlicerTractographyDisplayModule::category() const
{
  return "Diffusion.Tractography";
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation* qSlicerTractographyDisplayModule::createWidgetRepresentation()
{
  return new qSlicerTractographyDisplayModuleWidget;
}

//-----------------------------------------------------------------------------
//
vtkMRMLAbstractLogic* qSlicerTractographyDisplayModule::createLogic()
{
  return vtkSlicerFiberBundleLogic::New();
}

//-----------------------------------------------------------------------------
QString qSlicerTractographyDisplayModule::helpText()const
{
  QString help = 
    "Load, save and adjust display parameters of fiber bundles. \n"
    "<a>%1/Modules:DTIDisplay-Documentation-3.6</a>";
  return help.arg(this->slicerWikiUrl());
}

//-----------------------------------------------------------------------------
QString qSlicerTractographyDisplayModule::acknowledgementText()const
{
  QString acknowledgement = 
    "This work was supported by NA-MIC, NAC, BIRN, NCIGT, and the Slicer Community. "
    "See <a href=\"http://www.slicer.org\">http://www.slicer.org</a> for details.\n";
  return acknowledgement;
}
