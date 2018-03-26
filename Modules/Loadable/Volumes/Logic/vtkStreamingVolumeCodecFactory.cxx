/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkStreamingVolumeCodecFactory.cxx,v $
  Date:      $Date: 2006/01/06 17:56:48 $
  Version:   $Revision: 1.58 $

=========================================================================auto=*/

#include "vtkStreamingVolumeCodecFactory.h"
#include <algorithm>
// VTK includes
#include <vtkObjectFactory.h>
#include <vtkDataObject.h>

//----------------------------------------------------------------------------
// The compression codec manager singleton.
// This MUST be default initialized to zero by the compiler and is
// therefore not initialized here.  The ClassInitialize and ClassFinalize methods handle this instance.
static vtkStreamingVolumeCodecFactory* vtkStreamingVolumeCodecFactoryInstance;


//----------------------------------------------------------------------------
// Must NOT be initialized.  Default initialization to zero is necessary.
unsigned int vtkStreamingVolumeCodecFactoryInitialize::Count;

//----------------------------------------------------------------------------
// Implementation of vtkStreamingVolumeCodecFactoryInitialize class.
//----------------------------------------------------------------------------
vtkStreamingVolumeCodecFactoryInitialize::vtkStreamingVolumeCodecFactoryInitialize()
{
  if(++Self::Count == 1)
    {
    vtkStreamingVolumeCodecFactory::classInitialize();
    }
}

//----------------------------------------------------------------------------
vtkStreamingVolumeCodecFactoryInitialize::~vtkStreamingVolumeCodecFactoryInitialize()
{
  if(--Self::Count == 0)
    {
    vtkStreamingVolumeCodecFactory::classFinalize();
    }
}

//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// Up the reference count so it behaves like New
vtkStreamingVolumeCodecFactory* vtkStreamingVolumeCodecFactory::New()
{
  vtkStreamingVolumeCodecFactory* ret = vtkStreamingVolumeCodecFactory::GetInstance();
  ret->Register(NULL);
  return ret;
}

//----------------------------------------------------------------------------
// Return the single instance of the vtkStreamingVolumeCodecFactory
vtkStreamingVolumeCodecFactory* vtkStreamingVolumeCodecFactory::GetInstance()
{
  if(!vtkStreamingVolumeCodecFactoryInstance)
    {
    // Try the factory first
    vtkStreamingVolumeCodecFactoryInstance = (vtkStreamingVolumeCodecFactory*)vtkObjectFactory::CreateInstance("vtkStreamingVolumeCodecFactory");
    // if the factory did not provide one, then create it here
    if(!vtkStreamingVolumeCodecFactoryInstance)
      {
      vtkStreamingVolumeCodecFactoryInstance = new vtkStreamingVolumeCodecFactory;
#ifdef VTK_HAS_INITIALIZE_OBJECT_BASE
      vtkStreamingVolumeCodecFactoryInstance->InitializeObjectBase();
#endif
      }
    }
  // return the instance
  return vtkStreamingVolumeCodecFactoryInstance;
}

//----------------------------------------------------------------------------
vtkStreamingVolumeCodecFactory::vtkStreamingVolumeCodecFactory()
{
}

//----------------------------------------------------------------------------
vtkStreamingVolumeCodecFactory::~vtkStreamingVolumeCodecFactory()
{
}

//----------------------------------------------------------------------------
void vtkStreamingVolumeCodecFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkStreamingVolumeCodecFactory::classInitialize()
{
  // Allocate the singleton
  vtkStreamingVolumeCodecFactoryInstance = vtkStreamingVolumeCodecFactory::GetInstance();
}

//----------------------------------------------------------------------------
void vtkStreamingVolumeCodecFactory::classFinalize()
{
  vtkStreamingVolumeCodecFactoryInstance->Delete();
  vtkStreamingVolumeCodecFactoryInstance = 0;
}

//----------------------------------------------------------------------------
int vtkStreamingVolumeCodecFactory::RegisterStreamingCodec(vtkSmartPointer<vtkStreamingVolumeCodec> codec)
{
  for (unsigned int i = 0; i < this->RegisteredCodecs.size(); ++i)
    {
    if (strcmp(this->RegisteredCodecs[i]->GetClassName(), codec->GetClassName())==0)
      {
      return 0;
      }
    }
  this->RegisteredCodecs.push_back(codec);
  return 1;
}

//----------------------------------------------------------------------------
int vtkStreamingVolumeCodecFactory::UnregisterStreamingCodecByClassName(const std::string&  codecClassName)
{
  for (unsigned int i = 0; i < this->RegisteredCodecs.size(); ++i)
    {
    if (strcmp(this->RegisteredCodecs[i]->GetClassName(), codecClassName.c_str())==0)
      {
      this->RegisteredCodecs.erase(this->RegisteredCodecs.begin() + i);
      return 1;
      }
    }
  vtkWarningMacro("UnregisterStreamingVolumeCodec failed: codec not found");
  return 0;
}

//----------------------------------------------------------------------------
 vtkStreamingVolumeCodec* vtkStreamingVolumeCodecFactory::CreateCodecByClassName(const std::string& codecClassName)
{
  for (unsigned int i = 0; i < this->RegisteredCodecs.size(); ++i)
    {
    if (strcmp(this->RegisteredCodecs[i]->GetClassName(), codecClassName.c_str())==0)
      {
      return vtkStreamingVolumeCodec::SafeDownCast(this->RegisteredCodecs[i]->CreateCodecInstance());
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
const std::vector<vtkSmartPointer<vtkStreamingVolumeCodec> >& vtkStreamingVolumeCodecFactory::GetStreamingCodecClassNames()
{
  return this->RegisteredCodecs;
}

