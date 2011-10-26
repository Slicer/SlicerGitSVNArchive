/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL: http://svn.slicer.org/Slicer4/trunk/Modules/OpenIGTLinkIF/vtkIGTLToMRMLTrackingData.h $
  Date:      $Date: 2009-08-12 21:30:38 -0400 (Wed, 12 Aug 2009) $
  Version:   $Revision: 10236 $

==========================================================================*/

#ifndef __vtkIGTLToMRMLTrackingData_h
#define __vtkIGTLToMRMLTrackingData_h

#include "vtkObject.h"
#include "vtkOpenIGTLinkIFWin32Header.h" 
#include "vtkMRMLNode.h"
#include "vtkIGTLToMRMLBase.h"

#include "igtlTrackingDataMessage.h"

class vtkMRMLVolumeNode;

class VTK_OPENIGTLINKIF_EXPORT vtkIGTLToMRMLTrackingData : public vtkIGTLToMRMLBase
{
 public:

  static vtkIGTLToMRMLTrackingData *New();
  vtkTypeRevisionMacro(vtkIGTLToMRMLTrackingData,vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent);

  virtual const char*  GetIGTLName() { return "TDATA"; };
  virtual const char*  GetMRMLName() { return "IGTLTrackingDataSplitter"; };

  virtual vtkIntArray* GetNodeEvents();
  virtual vtkMRMLNode* CreateNewNode(vtkMRMLScene* scene, const char* name);

  virtual int          IGTLToMRML(igtl::MessageBase::Pointer buffer, vtkMRMLNode* node);
  virtual int          MRMLToIGTL(unsigned long event, vtkMRMLNode* mrmlNode, int* size, void** igtlMsg);


 protected:
  vtkIGTLToMRMLTrackingData();
  ~vtkIGTLToMRMLTrackingData();

  void CenterImage(vtkMRMLVolumeNode *volumeNode);

 protected:
  //igtl::TransformMessage::Pointer OutTransformMsg;
  igtl::TrackingDataMessage::Pointer      OutTrackingMetaMsg;
  igtl::StartTrackingDataMessage::Pointer StartTrackingDataMessage;
  igtl::StopTrackingDataMessage::Pointer  StopTrackingDataMessage;
  
};


#endif //__vtkIGTLToMRMLTrackingData_h
