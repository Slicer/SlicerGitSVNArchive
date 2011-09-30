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

// SlicerQt includes

// Slices QTModule includes
#include "qSlicerSliceControllerModule.h"
#include "qSlicerSliceControllerModuleWidget.h"

//-----------------------------------------------------------------------------
class qSlicerSliceControllerModulePrivate
{
public:
};

//-----------------------------------------------------------------------------
qSlicerSliceControllerModule::qSlicerSliceControllerModule(QObject* _parent)
  :Superclass(_parent)
  , d_ptr(new qSlicerSliceControllerModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerSliceControllerModule::~qSlicerSliceControllerModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerSliceControllerModule::acknowledgementText()const
{
  return "This module was developed by Jean-Christophe Fillion-Robin, Kitware Inc. "
         "This work was supported by NIH grant 3P41RR013218-12S1, "
         "NA-MIC, NAC and Slicer community.";
}

//-----------------------------------------------------------------------------
void qSlicerSliceControllerModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerSliceControllerModule::createWidgetRepresentation()
{
  return new qSlicerSliceControllerModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerSliceControllerModule::createLogic()
{
  return 0;
}
