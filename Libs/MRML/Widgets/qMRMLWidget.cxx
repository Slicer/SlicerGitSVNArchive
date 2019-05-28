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

// MRMLWidgets includes
#include "qMRMLWidget.h"

// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkSmartPointer.h>

#ifdef Slicer_VTK_USE_QVTKOPENGLWIDGET
#include <QSurfaceFormat>
#include <QVTKOpenGLWidget.h>
#endif
#include <QApplication>

#ifdef _WIN32
#include <Windows.h> //for SetProcessDPIAware
#endif

//-----------------------------------------------------------------------------
class qMRMLWidgetPrivate
{
public:
  vtkSmartPointer<vtkMRMLScene>              MRMLScene;
};

//-----------------------------------------------------------------------------
// qMRMLWidget methods

//-----------------------------------------------------------------------------
qMRMLWidget::qMRMLWidget(QWidget * _parent, Qt::WindowFlags f):Superclass(_parent, f)
  , d_ptr(new qMRMLWidgetPrivate)
{
}

//-----------------------------------------------------------------------------
qMRMLWidget::~qMRMLWidget()
= default;

//-----------------------------------------------------------------------------
void qMRMLWidget::setMRMLScene(vtkMRMLScene* newScene)
{
  Q_D(qMRMLWidget);
  if (newScene == d->MRMLScene)
    {
    return ;
    }
  d->MRMLScene = newScene;
  emit mrmlSceneChanged(newScene);
}

//-----------------------------------------------------------------------------
vtkMRMLScene* qMRMLWidget::mrmlScene() const
{
  Q_D(const qMRMLWidget);
  return d->MRMLScene;
}

//-----------------------------------------------------------------------------
void qMRMLWidget::preInitializeApplication()
{
  #ifdef Q_OS_MACX
  if (QSysInfo::MacintoshVersion > QSysInfo::MV_10_8)
    {
    // Fix Mac OS X 10.9 (mavericks) font issue
    // https://bugreports.qt-project.org/browse/QTBUG-32789
    QFont::insertSubstitution(".Lucida Grande UI", "Lucida Grande");
    }
#endif

#ifdef _WIN32
  // Qt windows defaults to the PROCESS_PER_MONITOR_DPI_AWARE for DPI display
  // on windows. Unfortunately, this doesn't work well on multi-screens setups.
  // By calling SetProcessDPIAware(), we force the value to
  // PROCESS_SYSTEM_DPI_AWARE instead which fixes those issues.
  SetProcessDPIAware();
#endif

  // Enable automatic scaling based on the pixel density of the monitor
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

  // Enables resource sharing between the OpenGL contexts used by classes like QOpenGLWidget and QQuickWidget
  QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
}

//-----------------------------------------------------------------------------
void qMRMLWidget::postInitializeApplication(OpenGLProfileType openGLProfile/*=OpenGLProfileDefault*/)
{
  #ifdef Slicer_VTK_USE_QVTKOPENGLWIDGET
    QSurfaceFormat format = QVTKOpenGLWidget::defaultFormat();

    // Enable OpenGL compatibility profile on Windows by default.
    // Allow setting compatibility/core profile based on application
    // setting to allow testing applications with and without it.
  #ifdef Q_OS_WIN32
    bool useOpenGLCompatibilityProfile = true;
  #else
    bool useOpenGLCompatibilityProfile = false;
  #endif
    if (openGLProfile == OpenGLProfileCore)
      {
      useOpenGLCompatibilityProfile = false;
      }
    else if (openGLProfile == OpenGLProfileCompatibility)
      {
      useOpenGLCompatibilityProfile = true;
      }
    if (useOpenGLCompatibilityProfile)
      {
      format.setProfile(QSurfaceFormat::CompatibilityProfile);
      }

    // Set default surface format for QVTKOpenGLWidget. Disable multisampling to
    // support volume rendering and other VTK functionality that reads from the
    // framebuffer; see https://gitlab.kitware.com/vtk/vtk/issues/17095.
    format.setSamples(0);

    QSurfaceFormat::setDefaultFormat(format);
  #endif
  }
