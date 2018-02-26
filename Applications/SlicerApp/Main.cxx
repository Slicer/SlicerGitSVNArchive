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

// Slicer includes
#include "qSlicerApplication.h"
#include "qSlicerApplicationHelper.h"

// SlicerApp includes
#include "qSlicerAppMainWindow.h"
#include "qSlicerStyle.h"

namespace
{

//----------------------------------------------------------------------------
int SlicerAppMain(int argc, char* argv[])
{
  qSlicerApplicationHelper::preInitializeApplication(
        argv[0], new qSlicerStyle);

  qSlicerApplication app(argc, argv);
  if (app.returnCode() != -1)
    {
    return app.returnCode();
    }

  QScopedPointer<QSplashScreen> splashScreen;

  typedef qSlicerAppMainWindow SlicerMainWindowType;
  QScopedPointer<SlicerMainWindowType> window;

  qSlicerApplicationHelper::postInitializeApplication<SlicerMainWindowType>(
        app, splashScreen, window);

  if (!window.isNull())
    {
    QString windowTitle = "%1 %2";
    window->setWindowTitle(
      windowTitle.arg(window->windowTitle()).arg(Slicer_VERSION_FULL));
    }

  return app.exec();
}

} // end of anonymous namespace

#include "qSlicerApplicationMainWrapper.cxx"
