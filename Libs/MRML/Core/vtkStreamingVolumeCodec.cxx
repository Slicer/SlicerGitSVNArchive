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
  this->Content = new ContentData();
  this->Content->image = NULL;
  this->Content->frameType = UndefinedFrameType;
  this->Content->keyFrameUpdated = false;
  this->Content->codecType = "";
  this->Content->deviceName = "";
}

//---------------------------------------------------------------------------
vtkStreamingVolumeCodec::~vtkStreamingVolumeCodec()
{
  if (this->Content != NULL)
    {
    delete this->Content;
    }
  
}

//---------------------------------------------------------------------------
unsigned int vtkStreamingVolumeCodec::GetDeviceContentModifiedEvent() const
{
  return DeviceModifiedEvent;
}

//---------------------------------------------------------------------------
std::string vtkStreamingVolumeCodec::GetDeviceType() const
{
  return "";
}

std::string vtkStreamingVolumeCodec::GetCurrentCodecType()
{
  return this->Content->codecType;
};

vtkStreamingVolumeCodec::ContentData* vtkStreamingVolumeCodec::GetContent()
{
  return this->Content;
}

vtkImageData* vtkStreamingVolumeCodec::GetContentImage()
{
  return this->Content->image;
}

std::string vtkStreamingVolumeCodec::GetContentDeviceName()
{
  return this->Content->deviceName;
}

int vtkStreamingVolumeCodec::SetCurrentCodecType(std::string codecType)
{
  this->Content->codecType = std::string(codecType);
  return 0;
}

void vtkStreamingVolumeCodec::SetContent(ContentData* content)
{
  Content = content;
  this->Modified();
  this->InvokeEvent(DeviceModifiedEvent, this);
}


void vtkStreamingVolumeCodec::SetContentImage(vtkImageData* image)
{
  this->Content->image = image;
}

void vtkStreamingVolumeCodec::SetContentDeviceName(std::string name)
{
  this->Content->deviceName = std::string(name);
}


//---------------------------------------------------------------------------
int vtkStreamingVolumeCodec::UncompressedDataFromStream(std::string bitStreamData, bool checkCRC)
{
  //To do : decode the bitStreamData to update Content->image
  //Return 1 on successful, 0 for unsuccessful
  return 0;
}

//---------------------------------------------------------------------------
std::string vtkStreamingVolumeCodec::GetCompressedStreamFromData()
{
  return "";
}

//---------------------------------------------------------------------------
void vtkStreamingVolumeCodec::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << indent << "Video:\t" <<"\n";
  Content->image->PrintSelf(os, indent.GetNextIndent());
}

