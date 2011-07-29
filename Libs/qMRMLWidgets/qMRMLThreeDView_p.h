/*==============================================================================

  Program: 3D Slicer

  Copyright (c) 2010 Kitware Inc.

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

#ifndef __qMRMLThreeDView_p_h
#define __qMRMLThreeDView_p_h

// Qt includes
class QToolButton;

// CTK includes
#include <ctkPimpl.h>
#include <ctkVTKObject.h>
class ctkPopupWidget;

// qMRML includes
#include "qMRMLThreeDView.h"

class vtkMRMLDisplayableManagerGroup;
class vtkMRMLViewNode;
class vtkMRMLCameraNode;
class vtkObject;

//-----------------------------------------------------------------------------
class qMRMLThreeDViewPrivate: public QObject
{
  Q_OBJECT
  QVTK_OBJECT
  Q_DECLARE_PUBLIC(qMRMLThreeDView);
protected:
  qMRMLThreeDView* const q_ptr;
public:
  qMRMLThreeDViewPrivate(qMRMLThreeDView& object);
  ~qMRMLThreeDViewPrivate();

  virtual void init();

  void setMRMLScene(vtkMRMLScene* scene);

  /// Loop over all CameraNode from the scene and return the one having
  /// its activeTag matching \a viewNode ID
//  vtkMRMLCameraNode* lookUpMRMLCameraNode(vtkMRMLViewNode* viewNode);

public slots:
  /// Handle MRML scene event
  void onSceneStartProcessing();
  void onSceneEndProcessing();

  void updateWidgetFromMRML();

protected:
  void initDisplayableManagers();

  vtkMRMLDisplayableManagerGroup*    DisplayableManagerGroup;
  vtkMRMLScene*                      MRMLScene;
  vtkMRMLViewNode*                   MRMLViewNode;
  
  QToolButton*                       PinButton;
  ctkPopupWidget*                    PopupWidget;
};

#endif
