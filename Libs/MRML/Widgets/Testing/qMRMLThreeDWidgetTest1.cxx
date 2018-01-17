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

// QT includes
#include <QApplication>
#include <QTimer>

// Slicer includes
#include "vtkSlicerConfigure.h"

// qMRML includes
#include "qMRMLThreeDWidget.h"

// MRML includes
#include <vtkMRMLInteractionNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLViewNode.h>

// VTK includes
#include <vtk/Common/Core/vtkNew.h>
#ifdef Slicer_VTK_USE_QVTKOPENGLWIDGET
#include <QVTKOpenGLWidget.h>
#endif

// STD includes
#include <cstdlib>
#include <iostream>

int qMRMLThreeDWidgetTest1(int argc, char * argv [] )
{
#ifdef Slicer_VTK_USE_QVTKOPENGLWIDGET
  // Set default surface format for QVTKOpenGLWidget
  QSurfaceFormat format = QVTKOpenGLWidget::defaultFormat();
  format.setSamples(0);
  QSurfaceFormat::setDefaultFormat(format);
#endif

  QApplication app(argc, argv);
  qMRMLThreeDWidget widget;
  widget.show();

  vtkNew<vtkMRMLScene> scene;

  // vtkMRMLAbstractDisplayableManager requires selection and interaction nodes
  vtkNew<vtkMRMLSelectionNode> selectionNode;
  scene->AddNode(selectionNode.GetPointer());
  vtkNew<vtkMRMLInteractionNode> interactionNode;
  scene->AddNode(interactionNode.GetPointer());

  vtkNew<vtkMRMLViewNode> viewNode;
  scene->AddNode(viewNode.GetPointer());

  widget.setMRMLScene(scene.GetPointer());
  widget.setMRMLViewNode(viewNode.GetPointer());

  if (argc < 2 || QString(argv[1]) != "-I" )
    {
    QTimer::singleShot(200, &app, SLOT(quit()));
    }
  return app.exec();
}

