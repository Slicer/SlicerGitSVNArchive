/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkStreamingVolumeCodec.cxx,v $
  Date:      $Date: 2006/01/06 17:56:48 $
  Version:   $Revision: 1.58 $

=========================================================================auto=*/

#include "vtkStreamingVolumeCodec.h"

// VTK includes
#include <vtkCollection.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkXMLUtilities.h>

//---------------------------------------------------------------------------
vtkStreamingVolumeCodec::vtkStreamingVolumeCodec()
{
  this->Content.image = NULL;
  this->Content.frame = NULL;
  this->Content.keyFrame = NULL;
  this->Content.frameType = UndefinedFrameType;
  this->Content.keyFrameUpdated = false;
  this->Content.codecType = "";
  this->Content.codecName = "";
}

//---------------------------------------------------------------------------
vtkStreamingVolumeCodec::~vtkStreamingVolumeCodec()
{
}

//---------------------------------------------------------------------------
unsigned int vtkStreamingVolumeCodec::GetCodecContentModifiedEvent() const
{
  return CodecModifiedEvent;
}

//---------------------------------------------------------------------------
std::string vtkStreamingVolumeCodec::GetCodecType() const
{
  return "";
}

std::string vtkStreamingVolumeCodec::GetContentCodecType()
{
  return this->Content.codecType;
};

vtkStreamingVolumeCodec::ContentData vtkStreamingVolumeCodec::GetContent()
{
  return this->Content;
}

vtkSmartPointer<vtkImageData> vtkStreamingVolumeCodec::GetContentImage()
{
  return this->Content.image;
}

std::string vtkStreamingVolumeCodec::GetContentCodecName()
{
  return this->Content.codecName;
}

int vtkStreamingVolumeCodec::SetContentCodecType(std::string codecType)
{
  this->Content.codecType = std::string(codecType);
  return 0;
}

void vtkStreamingVolumeCodec::SetContent(ContentData content)
{
  Content.codecType = content.codecType;
  Content.image = content.image;
  this->Content.image = content.image;
  this->Content.frameType = content.frameType;
  this->Content.keyFrameUpdated = content.keyFrameUpdated;
  this->Content.codecType = content.codecType;
  this->Content.codecName = content.codecName;
  this->Modified();
  this->InvokeEvent(CodecModifiedEvent, this);
}


void vtkStreamingVolumeCodec::SetContentImage(vtkSmartPointer<vtkImageData> image)
{
  this->Content.image = image;
}

void vtkStreamingVolumeCodec::SetContentCodecName(std::string name)
{
  this->Content.codecName = std::string(name);
}


//---------------------------------------------------------------------------
int vtkStreamingVolumeCodec::UncompressedDataFromStream(vtkSmartPointer<vtkUnsignedCharArray> bitStreamData, bool checkCRC)
{
  //To do : decode the bitStreamData to update Content.image
  //Return 1 on successful, 0 for unsuccessful
  return 0;
}

//---------------------------------------------------------------------------
vtkSmartPointer<vtkUnsignedCharArray> vtkStreamingVolumeCodec::GetCompressedStreamFromData()
{
  return NULL;
}

//---------------------------------------------------------------------------
void vtkStreamingVolumeCodec::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << indent << "Video:\t" <<"\n";
  Content.image->PrintSelf(os, indent.GetNextIndent());
}

