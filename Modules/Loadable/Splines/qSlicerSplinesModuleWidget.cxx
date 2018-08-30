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

// Qt includes
#include <QDebug>

// SlicerQt includes
#include "qSlicerSplinesModuleWidget.h"
#include "ui_qSlicerSplinesModuleWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerSplinesModuleWidgetPrivate: public Ui_qSlicerSplinesModuleWidget
{
public:
  qSlicerSplinesModuleWidgetPrivate();
};

//-----------------------------------------------------------------------------
// qSlicerSplinesModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerSplinesModuleWidgetPrivate::qSlicerSplinesModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerSplinesModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerSplinesModuleWidget::qSlicerSplinesModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerSplinesModuleWidgetPrivate )
{
}

//-----------------------------------------------------------------------------
qSlicerSplinesModuleWidget::~qSlicerSplinesModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerSplinesModuleWidget::setup()
{
  Q_D(qSlicerSplinesModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();
}
