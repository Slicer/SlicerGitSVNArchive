/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL: http://svn.slicer.org/Slicer4/trunk/Modules/OpenIGTLinkIF/vtkOpenIGTLinkIFGUI.h $
  Date:      $Date: 2010-04-01 11:42:15 -0400 (Thu, 01 Apr 2010) $
  Version:   $Revision: 12582 $

==========================================================================*/

#ifndef __vtkIGTLTrackingDataControllerWindow_h
#define __vtkIGTLTrackingDataControllerWindow_h

// for getting display device information
#ifdef WIN32
#include "Windows.h"
#endif

#include "vtkOpenIGTLinkIFWin32Header.h"

#include "vtkSmartPointer.h"
//#include "vtkSlicerFiducialListWidget.h"
//#include "vtkSlicerROIViewerWidget.h"
//#include "vtkSlicerSecondaryViewerWidget.h"
#include "vtkSlicerViewerWidget.h"
#include "vtkKWMultiColumnList.h"
#include "vtkKWMultiColumnListWithScrollbars.h"
#include "vtkKWPushButton.h"
#include "vtkKWTopLevel.h"
#include "vtkSmartPointer.h"
#include "vtkMRMLIGTLConnectorNode.h"
#include "vtkMRMLIGTLQueryNode.h"

class vtkOpenIGTLinkIFGUI;

class VTK_OPENIGTLINKIF_EXPORT vtkIGTLTrackingDataControllerWindow : public vtkKWTopLevel
{
public:

  vtkGetObjectMacro(MRMLScene, vtkMRMLScene);
  vtkSetObjectMacro(MRMLScene, vtkMRMLScene);

  vtkGetObjectMacro(Connector, vtkMRMLIGTLConnectorNode);
  vtkSetObjectMacro(Connector, vtkMRMLIGTLConnectorNode);

  static vtkIGTLTrackingDataControllerWindow *New();  
  vtkTypeRevisionMacro(vtkIGTLTrackingDataControllerWindow,vtkKWTopLevel);
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetInGUICallbackFlag (int flag) {
    this->InGUICallbackFlag = flag;
    }
  vtkGetMacro(InGUICallbackFlag, int);
  void SetInMRMLCallbackFlag (int flag) {
    this->InMRMLCallbackFlag = flag;
  }
  vtkGetMacro(InMRMLCallbackFlag, int);

  void SetAndObserveMRMLScene ( vtkMRMLScene *mrml )
    {
    vtkMRMLScene *oldValue = this->MRMLScene;
    this->MRMLObserverManager->SetAndObserveObject ( vtkObjectPointer( &this->MRMLScene), (vtkObject*)mrml );
    if ( oldValue != this->MRMLScene )
      {
      this->InvokeEvent (vtkCommand::ModifiedEvent);
      }
    }

  void SetAndObserveMRMLSceneEvents ( vtkMRMLScene *mrml, vtkIntArray *events )
    {
    vtkObject *oldValue = this->MRMLScene;
    this->MRMLObserverManager->SetAndObserveObjectEvents ( vtkObjectPointer( &this->MRMLScene), mrml, events );
    if ( oldValue != this->MRMLScene )
      {
      this->InvokeEvent (vtkCommand::ModifiedEvent);
      }
    }

  void DisplayOnWindow();
  void SetOpenIGTLinkIFGUI(vtkOpenIGTLinkIFGUI* moduleGUI)
  {
    this->ModuleGUI = moduleGUI;
  }

protected:

  vtkIGTLTrackingDataControllerWindow();
  ~vtkIGTLTrackingDataControllerWindow();  

  static void MRMLCallback(vtkObject *caller,
                           unsigned long eid, void *clientData, void *callData );

  static void GUICallback(vtkObject *caller,
                          unsigned long eid, void *clientData, void *callData );
  
  
  virtual void CreateWidget();
  virtual void ProcessGUIEvents(vtkObject *caller, unsigned long event, void *callData);
  virtual void ProcessMRMLEvents(vtkObject *caller, unsigned long event, void *callData);
  virtual void AddGUIObservers();
  virtual void RemoveGUIObservers();
  
 protected:
  
  //----------------------------------------------------------------
  // GUI widgets and Callbacks
  //----------------------------------------------------------------
  vtkKWFrame* MainFrame;
  vtkSlicerViewerWidget* ViewerWidget;
  
  vtkKWMultiColumnListWithScrollbars* RemoteDataList;

  vtkKWPushButton* StartTrackingButton;
  vtkKWPushButton* StopTrackingButton;
  vtkKWPushButton* CloseButton;

  vtkCallbackCommand *MRMLCallbackCommand;  
  vtkCallbackCommand *GUICallbackCommand;
  vtkObserverManager *MRMLObserverManager;
  int InGUICallbackFlag;
  int InMRMLCallbackFlag;
  int IsObserverAddedFlag;
  
  //----------------------------------------------------------------
  // Logic Values
  //----------------------------------------------------------------
  bool MultipleMonitorsAvailable; 
  int  WindowPosition[2]; // virtual screen position in pixels
  int  WindowSize[2]; // virtual screen size in pixels
  
  vtkOpenIGTLinkIFGUI* ModuleGUI;
  vtkMRMLScene* MRMLScene;
  
  vtkMRMLIGTLConnectorNode* Connector;
  
  vtkMRMLIGTLQueryNode* TrackingDataQueryNode;
  std::list<vtkMRMLIGTLQueryNode*> ImageQueryNodeList;

 private:
  vtkIGTLTrackingDataControllerWindow(const vtkIGTLTrackingDataControllerWindow&);
  void operator=(const vtkIGTLTrackingDataControllerWindow&);


};

#endif
