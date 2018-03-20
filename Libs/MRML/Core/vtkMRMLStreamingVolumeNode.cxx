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
#include <vtkDataArray.h>

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
  this->CodecDeviceType = "";
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
std::string vtkMRMLStreamingVolumeNode::GetCodecDeviceType()
{
  return this->CodecDeviceType;
}

//-----------------------------------------------------------------------------
int vtkMRMLStreamingVolumeNode::SetCodecDeviceType(std::string name)
{
  this->CodecDeviceType = name;
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
      vtkUnsignedCharArray* bitStream = device->GetContent()->frame;
      this->UpdateFrameByDeepCopy(bitStream);
      vtkUnsignedCharArray* bitKeyStream = device->GetContent()->keyFrame;
      this->UpdateKeyFrameByDeepCopy(bitKeyStream);
      if (device->GetContent()->image != NULL)
        {
        this->SetAndObserveImageData(device->GetContent()->image);
        }
      }
    this->SetCodecName(device->GetContentDeviceName());
    this->SetCodecDeviceType(device->GetDeviceType());
    this->CompressionCodec = device; // should the interal video device point to the external video device?
    //-------
    return 1;
    }
  return 0;
}

void vtkMRMLStreamingVolumeNode::CopySrcArrayToDestArray(vtkUnsignedCharArray* srcString, vtkUnsignedCharArray* destString)
{
  destString->SetNumberOfTuples(srcString->GetNumberOfValues());
  destString->DeepCopy(srcString);
}

int vtkMRMLStreamingVolumeNode::UpdateFrameByDeepCopy(vtkUnsignedCharArray* buffer)
{
  if(this->Frame == NULL)
    {
    this->Frame = vtkUnsignedCharArray::New();
    this->Frame->SetNumberOfComponents(1);
    }
  if(buffer == NULL)
    {
    vtkWarningMacro("Souce frame not valid.")
    return 0;
    }
  this->CopySrcArrayToDestArray(buffer, this->Frame);
  this->SetFrameUpdated(true);
  return 1;
};


int vtkMRMLStreamingVolumeNode::UpdateKeyFrameByDeepCopy(vtkUnsignedCharArray* buffer)
{
  if(this->KeyFrame == NULL)
    {
    this->KeyFrame = vtkUnsignedCharArray::New();
    this->KeyFrame->SetNumberOfComponents(1);
    }
  if(buffer == NULL)
    {
    vtkWarningMacro("Souce key frame not valid.")
    return 0;
    }
  this->CopySrcArrayToDestArray(buffer, this->KeyFrame);
  this->SetKeyFrameUpdated(true);
  return 1;
};


//---------------------------------------------------------------------------
int vtkMRMLStreamingVolumeNode::DecodeFrame(vtkStreamingVolumeCodec::ContentData* deviceContent)
{
  if (this->CompressionCodec == NULL)
  {
    vtkWarningMacro("No compression device available, use the ObserveOutsideCompressionCodec() to set up the compression device.")
    return 0;
  }
  if(!this->GetKeyFrameDecoded())
    {
    this->SetKeyFrameDecoded(true);
    this->UpdateKeyFrameByDeepCopy(deviceContent->keyFrame);
    int decodingStatus = this->CompressionCodec->UncompressedDataFromStream(deviceContent->keyFrame, true);
    if (decodingStatus == 0)
      {
      vtkWarningMacro("key frame decoding failed.")
      return 0;
      }
    }
  vtkUnsignedCharArray* bitStreamData = deviceContent->frame;
  if (this->CompressionCodec->UncompressedDataFromStream(bitStreamData, true))
    {
    if (this->GetImageData() != this->CompressionCodec->GetContent()->image)
      {
      this->SetAndObserveImageData(this->CompressionCodec->GetContent()->image);
      }
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
  vtkUnsignedCharArray* compressedStream = this->CompressionCodec->GetCompressedStreamFromData();
  if (compressedStream != NULL)
    {
    if (compressedStream->GetNumberOfValues()>0)
      {
      this->UpdateFrameByDeepCopy(compressedStream);
      if(!this->GetKeyFrameReceived())
        {
        this->SetKeyFrameReceived(true);
        this->UpdateKeyFrameByDeepCopy(compressedStream);
        }
        return 1;
      }
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
  this->SetKeyFrameUpdated(false);
  this->UpdateFrameByDeepCopy(modifiedDevice->GetContent()->frame);
  if(!this->GetKeyFrameReceived() || modifiedDevice->GetContent()->keyFrameUpdated)
    {
    this->UpdateKeyFrameByDeepCopy(modifiedDevice->GetContent()->keyFrame);
    this->SetKeyFrameReceived(true);
    }
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
  vtkMRMLWriteXMLStdStringMacro(codecDeviceType, CodecDeviceType);
  vtkMRMLWriteXMLEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLStreamingVolumeNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();
  
  Superclass::ReadXMLAttributes(atts);
  vtkMRMLReadXMLBeginMacro(atts);
  vtkMRMLReadXMLStdStringMacro(codecName, CodecName);
  vtkMRMLReadXMLStdStringMacro(codecDeviceType, CodecDeviceType);
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
