#include "vtkMRMLStreamingVolumeNode.h"

// MRML includes
#include "vtkMRMLScene.h"
#include "vtkMRMLNRRDStorageNode.h"

// VTK includes
#include <vtkCollection.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkXMLUtilities.h>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLStreamingVolumeNode);

//----------------------------------------------------------------------------
// vtkMRMLStreamingVolumeNode methods

//-----------------------------------------------------------------------------
vtkMRMLStreamingVolumeNode::vtkMRMLStreamingVolumeNode()
{
  this->CompressionCodec = NULL;
  this->FrameUpdated = false;
  this->KeyFrameReceived = false;
  this->KeyFrameDecoded = false;
  this->KeyFrameUpdated = false;
  this->KeyFrame = NULL;
  this->Frame = NULL;
  this->CompressionCodecClassName = "";
  this->CodecName = "";
}

//-----------------------------------------------------------------------------
vtkMRMLStreamingVolumeNode::~vtkMRMLStreamingVolumeNode()
{
  if (this->CompressionCodec != NULL)
    {
    this->CompressionCodec->Delete();
    }
  if (this->KeyFrame != NULL)
    {
    this->KeyFrame->Delete();
    }
  if (this->Frame != NULL)
    {
    this->Frame->Delete();
    }
}

//-----------------------------------------------------------------------------
std::string vtkMRMLStreamingVolumeNode::GetCodecName()
{
  return this->CodecName;
}

//-----------------------------------------------------------------------------
int vtkMRMLStreamingVolumeNode::SetCodecName(std::string name)
{
  this->CodecName = name;
  return 1;
}

//-----------------------------------------------------------------------------
std::string vtkMRMLStreamingVolumeNode::GetCompressionCodecClassName()
{
  return this->CompressionCodecClassName;
}

//-----------------------------------------------------------------------------
int vtkMRMLStreamingVolumeNode::SetCompressionCodecClassName(std::string name)
{
  this->CompressionCodecClassName = name;
  return 1;
}

//---------------------------------------------------------------------------
int vtkMRMLStreamingVolumeNode::ObserveOutsideCompressionCodec(vtkStreamingVolumeCodec* device)
{
  if (device)
    {
    device->AddObserver(device->GetDeviceContentModifiedEvent(), this, &vtkMRMLStreamingVolumeNode::ProcessDeviceModifiedEvents);
    //------
    if (device->GetContent() != NULL)
      {
      std::string bitStream = std::string(device->GetContent()->frame);
      this->UpdateFrameFromDataStream(bitStream);
      std::string bitKeyStream = std::string(device->GetContent()->keyFrame);
      this->UpdateKeyFrameFromDataStream(bitKeyStream);
      if (device->GetContent()->image != NULL)
        {
        this->SetAndObserveImageData(device->GetContent()->image);
        }
      }
    this->SetCodecName(device->GetContentDeviceName());
    this->SetCompressionCodecClassName(device->GetClassName());
    this->CompressionCodec = device; // should the interal video device point to the external video device?
    //-------
    return 1;
    }
  return 0;
}

void vtkMRMLStreamingVolumeNode::CopySrcStringToDestArray(const std::string& srcString, vtkUnsignedCharArray* destString)
{
  destString->SetNumberOfTuples(static_cast<vtkIdType>(srcString.length()));
  char* destStringPoiter = reinterpret_cast<char*> (destString->GetPointer(0));
  memcpy(destStringPoiter, srcString.c_str(), srcString.length());
}

void vtkMRMLStreamingVolumeNode::UpdateFrameFromDataStream(std::string& buffer)
{
  if(this->Frame == NULL)
    {
    this->Frame = vtkUnsignedCharArray::New();
    this->Frame->SetNumberOfComponents(1);
    }
  this->CopySrcStringToDestArray(buffer, this->Frame);
};


void vtkMRMLStreamingVolumeNode::UpdateKeyFrameFromDataStream(std::string& buffer)
{
  if(this->KeyFrame == NULL)
    {
    this->KeyFrame = vtkUnsignedCharArray::New();
    this->KeyFrame->SetNumberOfComponents(1);
    }
  this->CopySrcStringToDestArray(buffer, this->KeyFrame);
};


//---------------------------------------------------------------------------
int vtkMRMLStreamingVolumeNode::DecodeFrame(vtkStreamingVolumeCodec::ContentData* deviceContent)
{
  if (this->CompressionCodec == NULL)
  {
    vtkWarningMacro("No compression device available, use the ObserveOutsideCompressionCodec() to set up the compression device.")
    return 0;
  }
  if (this->GetImageData() != this->CompressionCodec->GetContent()->image)
  {
    this->SetAndObserveImageData(this->CompressionCodec->GetContent()->image);
  }
  if(!this->GetKeyFrameDecoded())
    {
    this->SetKeyFrameDecoded(true);
    this->UpdateKeyFrameFromDataStream(deviceContent->keyFrame);
    int decodingStatus = this->CompressionCodec->UncompressedDataFromStream(deviceContent->keyFrame, true);
    if (decodingStatus == 0)
      {
      vtkWarningMacro("key frame decoding failed.")
      return 0;
      }
    }
  std::string bitStreamData = std::string(deviceContent->frame);
  if (this->CompressionCodec->UncompressedDataFromStream(bitStreamData, true))
    {
    this->Modified();
    return 1;
    }
  return 0;
}

//---------------------------------------------------------------------------
int vtkMRMLStreamingVolumeNode::EncodeImageData()
{
  if (this->CompressionCodec == NULL)
    {
    vtkWarningMacro("No compression device available, use the ObserveOutsideCompressionCodec() to set up the compression device.")
    return 0;
    }
  if (this->GetImageData() != this->CompressionCodec->GetContent()->image)
    {
    this->CompressionCodec->SetContentImage(this->GetImageData());
    }
  std::string compressedStream = this->CompressionCodec->GetCompressedStreamFromData();
  if (compressedStream.size()>0)
    {
    this->UpdateFrameFromDataStream(compressedStream);
    if(!this->GetKeyFrameReceived())
      {
      this->SetKeyFrameReceived(true);
      this->UpdateKeyFrameFromDataStream(compressedStream);
      }
      return 1;
    }
  return 0;
}

std::string vtkMRMLStreamingVolumeNode::GetCodecType()
{
  vtkStreamingVolumeCodec*  device = this->GetCompressionCodec();
  if (device)
    {
    return device->GetCurrentCodecType();
    }
  return "";
}


int vtkMRMLStreamingVolumeNode::SetCodecType(std::string name)
{
  vtkStreamingVolumeCodec*  device = this->GetCompressionCodec();
  if(device)
  {
    device->SetCurrentCodecType(name);
    return 1;
  }
  return 0;
}

void vtkMRMLStreamingVolumeNode::ProcessMRMLEvents(vtkObject *caller, unsigned long event, void *callData )
{
  this->vtkMRMLNode::ProcessMRMLEvents(caller, event, callData);
  this->InvokeEvent(ImageDataModifiedEvent);
}

void vtkMRMLStreamingVolumeNode::ProcessDeviceModifiedEvents( vtkObject *caller, unsigned long event, void *callData )
{
  vtkStreamingVolumeCodec* modifiedDevice = reinterpret_cast<vtkStreamingVolumeCodec*>(caller);
  if (modifiedDevice == NULL)
    {
    // we are only interested in proxy node modified events
    return;
    }
  if (event != modifiedDevice->GetDeviceContentModifiedEvent())
    {
    return;
    }
  long streamLength = modifiedDevice->GetContent()->frame.length();
  std::string* bitStream = new std::string(modifiedDevice->GetContent()->frame);
  this->SetFrameUpdated(true);
  this->SetKeyFrameUpdated(false);
  this->UpdateFrameFromDataStream(*bitStream);
  if(!this->GetKeyFrameReceived() || modifiedDevice->GetContent()->keyFrameUpdated)
    {
    streamLength = modifiedDevice->GetContent()->keyFrame.length();
    std::string*  keyFrameStream = new std::string(modifiedDevice->GetContent()->keyFrame);
    this->UpdateKeyFrameFromDataStream(*keyFrameStream);
    this->SetKeyFrameReceived(true);
    this->SetKeyFrameUpdated(true);
    delete keyFrameStream;
    }
  delete bitStream;
  this->Modified();
}

//----------------------------------------------------------------------------
vtkStreamingVolumeCodec* vtkMRMLStreamingVolumeNode::GetCompressionCodec()
{
  return this->CompressionCodec;
}

//----------------------------------------------------------------------------
void vtkMRMLStreamingVolumeNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
  vtkMRMLWriteXMLBeginMacro(of);
  vtkMRMLWriteXMLStdStringMacro(codecName, CodecName);
  vtkMRMLWriteXMLStdStringMacro(compressionCodecClassName, CompressionCodecClassName);
  vtkMRMLWriteXMLEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLStreamingVolumeNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();
  
  Superclass::ReadXMLAttributes(atts);
  vtkMRMLReadXMLBeginMacro(atts);
  vtkMRMLReadXMLStdStringMacro(codecName, CodecName);
  vtkMRMLReadXMLStdStringMacro(compressionCodecClassName, CompressionCodecClassName);
  vtkMRMLReadXMLEndMacro();
  this->EndModify(disabledModify);
}


//----------------------------------------------------------------------------
void vtkMRMLStreamingVolumeNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();
  Superclass::Copy(anode);
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLStreamingVolumeNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}
