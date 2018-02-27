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

#ifndef __qSlicerApplicationHelper_h
#define __qSlicerApplicationHelper_h

// Qt includes
#include <QScopedPointer>
#include <QObject>
#include <QSplashScreen>

// Slicer includes
#include <qSlicerApplication.h>

#include "qSlicerBaseQTAppExport.h"

class ctkProxyStyle;
class qSlicerModuleFactoryManager;

class Q_SLICER_BASE_QTAPP_EXPORT qSlicerApplicationHelper : public QObject
{
  Q_OBJECT
public:
  typedef QObject Superclass;
  typedef qSlicerApplicationHelper Self;

  qSlicerApplicationHelper(QObject * parent = 0);
  virtual ~qSlicerApplicationHelper();

  static void preInitializeApplication(const char* argv0, ctkProxyStyle* style);

  template<typename SlicerMainWindowType>
  static void postInitializeApplication(
      qSlicerApplication& app,
      QScopedPointer<QSplashScreen>& splashScreen,
      QScopedPointer<SlicerMainWindowType>& window);

  static void setupModuleFactoryManager(qSlicerModuleFactoryManager * moduleFactoryManager);

  static void showMRMLEventLoggerWidget();

private:
  Q_DISABLE_COPY(qSlicerApplicationHelper);
};

#include "qSlicerApplicationHelper.txx"

#endif
