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
  this->CompressionCodec = vtkSmartPointer<vtkStreamingVolumeCodec>::New();
  this->FrameUpdated = false;
  this->KeyFrameReceived = false;
  this->KeyFrameDecoded = false;
  this->KeyFrameUpdated = false;
  this->KeyFrame = vtkSmartPointer<vtkUnsignedCharArray>::New();
  this->Frame = vtkSmartPointer<vtkUnsignedCharArray>::New();
  this->CodecDeviceType = "";
  this->CodecName = "";
}

//-----------------------------------------------------------------------------
vtkMRMLStreamingVolumeNode::~vtkMRMLStreamingVolumeNode()
{
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
int vtkMRMLStreamingVolumeNode::ObserveOutsideCompressionCodec(vtkSmartPointer<vtkStreamingVolumeCodec> device)
{
  if (device)
    {
    device->AddObserver(device->GetDeviceContentModifiedEvent(), this, &vtkMRMLStreamingVolumeNode::ProcessDeviceModifiedEvents);
    //------
    if (device->GetContent().frame != NULL && device->GetContent().keyFrame != NULL)
      {
      vtkSmartPointer<vtkUnsignedCharArray> bitStream = device->GetContent().frame;
      this->UpdateFrameByDeepCopy(bitStream);
      vtkSmartPointer<vtkUnsignedCharArray> bitKeyStream = device->GetContent().keyFrame;
      this->UpdateKeyFrameByDeepCopy(bitKeyStream);
      if (device->GetContent().image != NULL)
        {
        this->SetAndObserveImageData(device->GetContent().image);
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

void vtkMRMLStreamingVolumeNode::CopySrcArrayToDestArray(vtkSmartPointer<vtkUnsignedCharArray> srcString, vtkSmartPointer<vtkUnsignedCharArray> destString)
{
  destString->SetNumberOfTuples(srcString->GetNumberOfValues());
  destString->DeepCopy(srcString);
}

int vtkMRMLStreamingVolumeNode::UpdateFrameByDeepCopy(vtkSmartPointer<vtkUnsignedCharArray> buffer)
{
  if(this->Frame == NULL)
    {
    this->Frame = vtkSmartPointer<vtkUnsignedCharArray>::New();
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


int vtkMRMLStreamingVolumeNode::UpdateKeyFrameByDeepCopy(vtkSmartPointer<vtkUnsignedCharArray> buffer)
{
  if(this->KeyFrame == NULL)
    {
    this->KeyFrame = vtkSmartPointer<vtkUnsignedCharArray>::New();
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
int vtkMRMLStreamingVolumeNode::DecodeFrame(vtkStreamingVolumeCodec::ContentData deviceContent)
{
  if (this->CompressionCodec == NULL)
  {
    vtkWarningMacro("No compression device available, use the ObserveOutsideCompressionCodec() to set up the compression device.")
    return 0;
  }
  if(!this->GetKeyFrameDecoded())
    {
    this->SetKeyFrameDecoded(true);
    this->UpdateKeyFrameByDeepCopy(deviceContent.keyFrame);
    int decodingStatus = this->CompressionCodec->UncompressedDataFromStream(deviceContent.keyFrame, true);
    if (decodingStatus == 0)
      {
      vtkWarningMacro("key frame decoding failed.")
      return 0;
      }
    }
  vtkSmartPointer<vtkUnsignedCharArray> bitStreamData = deviceContent.frame;
  if (this->CompressionCodec->UncompressedDataFromStream(bitStreamData, true))
    {
    if (this->GetImageData() != this->CompressionCodec->GetContent().image)
      {
      this->SetAndObserveImageData(this->CompressionCodec->GetContent().image);
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
  if (this->GetImageData() != this->CompressionCodec->GetContent().image)
    {
    this->CompressionCodec->SetContentImage(this->GetImageData());
    }
  vtkSmartPointer<vtkUnsignedCharArray> compressedStream = this->CompressionCodec->GetCompressedStreamFromData();
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
  vtkSmartPointer<vtkStreamingVolumeCodec> device = this->GetCompressionCodec();
  if (device)
    {
    return device->GetCurrentCodecType();
    }
  return "";
}


int vtkMRMLStreamingVolumeNode::SetCodecType(std::string name)
{
  vtkSmartPointer<vtkStreamingVolumeCodec> device = this->GetCompressionCodec();
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
  vtkSmartPointer<vtkStreamingVolumeCodec> modifiedDevice = reinterpret_cast<vtkStreamingVolumeCodec*>(caller);
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
  this->UpdateFrameByDeepCopy(modifiedDevice->GetContent().frame);
  if(!this->GetKeyFrameReceived() || modifiedDevice->GetContent().keyFrameUpdated)
    {
    this->UpdateKeyFrameByDeepCopy(modifiedDevice->GetContent().keyFrame);
    this->SetKeyFrameReceived(true);
    }
  this->Modified();
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkStreamingVolumeCodec> vtkMRMLStreamingVolumeNode::GetCompressionCodec()
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
