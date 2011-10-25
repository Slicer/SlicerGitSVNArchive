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

#ifndef __qSlicerViewersToolBar_p_h
#define __qSlicerViewersToolBar_p_h

// Qt includes
#include <QToolBar>
#include <QMenu>
#include <QCheckBox>

// CTK includes
#include <ctkPimpl.h>
#include <ctkSignalMapper.h>
#include <ctkVTKObject.h>
#include "qSlicerBaseQTGUIExport.h"

// SlicerQt includes
#include "qSlicerViewersToolBar.h"

// MRMLLogic includes
#include <vtkMRMLApplicationLogic.h>

// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkSmartPointer.h>

class qSlicerViewersToolBarPrivate;
class QAction;
class QActionGroup;
class QToolButton;

class qSlicerViewersToolBarPrivate: public QObject
{
  Q_OBJECT
  QVTK_OBJECT
  Q_DECLARE_PUBLIC(qSlicerViewersToolBar);

protected:
  qSlicerViewersToolBar* const q_ptr;

public:
  qSlicerViewersToolBarPrivate(qSlicerViewersToolBar& object);

  void init();
  void setMRMLScene(vtkMRMLScene* newScene);
  void updateWidgetFromMRML();

public slots:

  void onMRMLSceneAboutToBeClosedEvent();
  void onMRMLSceneImportedEvent();
  void onMRMLSceneClosedEvent();
  void onCrosshairNodeModeChangedEvent();
  
  void setCrosshairMode(int);
  void setCrosshairMode(bool); // used to toggle between last style and off
  void setNavigation(bool);

public:
  vtkSmartPointer<vtkMRMLScene>            MRMLScene;
  vtkSmartPointer<vtkMRMLApplicationLogic> MRMLAppLogic;

  /// Crosshair
  QToolButton *CrosshairToolButton;
  QMenu*        CrosshairMenu;
  
  ctkSignalMapper* CrosshairMapper;
  QAction*      CrosshairNavigationAction;
  QAction*      CrosshairNoAction;
  QAction*      CrosshairBasicAction;
  QAction*      CrosshairBasicIntersectionAction;
  QAction*      CrosshairSmallBasicAction;
  QAction*      CrosshairSmallBasicIntersectionAction;

  QAction*      CrosshairToggleAction;

  int           CrosshairLastMode;
};

#endif
