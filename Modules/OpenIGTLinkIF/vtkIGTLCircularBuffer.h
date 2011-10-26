/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL$
  Date:      $Date$
  Version:   $Revision$

==========================================================================*/

#ifndef __vtkIGTLCircularBuffer_h
#define __vtkIGTLCircularBuffer_h

#include "vtkObject.h"
#include "vtkOpenIGTLinkIFWin32Header.h" 

#include <string>

#include "igtlMessageBase.h"

#define IGTLCB_CIRC_BUFFER_SIZE    3

class vtkMutexLock;

class VTK_OPENIGTLINKIF_EXPORT vtkIGTLCircularBuffer : public vtkObject
{
 public:

  static vtkIGTLCircularBuffer *New();
  vtkTypeRevisionMacro(vtkIGTLCircularBuffer,vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent);

  int GetNumberOfBuffer() { return IGTLCB_CIRC_BUFFER_SIZE; }

  int            StartPush();
  void           EndPush();
  igtl::MessageBase::Pointer GetPushBuffer();

  int            StartPull();
  void           EndPull();
  igtl::MessageBase::Pointer GetPullBuffer();

  int            IsUpdated() { return this->UpdateFlag; };

 protected:
  vtkIGTLCircularBuffer();
  virtual ~vtkIGTLCircularBuffer();

 protected:

  vtkMutexLock*      Mutex;
  int                Last;        // updated by connector thread
  int                InPush;      // updated by connector thread
  int                InUse;       // updated by main thread

  int                UpdateFlag;  // non-zero if updated since StartPull() has called

  std::string        DeviceType[IGTLCB_CIRC_BUFFER_SIZE];
  long long          Size[IGTLCB_CIRC_BUFFER_SIZE];
  unsigned char*     Data[IGTLCB_CIRC_BUFFER_SIZE];

  igtl::MessageBase::Pointer Messages[IGTLCB_CIRC_BUFFER_SIZE];
  
};


#endif //__vtkIGTLCircularBuffer_h
