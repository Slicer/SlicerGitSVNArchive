#include "vtkMRMLBitStreamVolumeNode.h"

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
vtkMRMLNodeNewMacro(vtkMRMLBitStreamVolumeNode);

//----------------------------------------------------------------------------
// vtkMRMLBitStreamVolumeNode methods

//-----------------------------------------------------------------------------
vtkMRMLBitStreamVolumeNode::vtkMRMLBitStreamVolumeNode()
{
  this->CompressionDevice = NULL;
  this->FrameUpdated = false;
  this->KeyFrameReceived = false;
  this->KeyFrameDecoded = false;
  this->KeyFrameUpdated = false;
  this->FrameMessageValid = false;
  this->KeyFrameMessage = "";
  this->FrameMessage = "";
}

//-----------------------------------------------------------------------------
vtkMRMLBitStreamVolumeNode::~vtkMRMLBitStreamVolumeNode()
{
  if (this->CompressionDevice != NULL)
    {
    this->CompressionDevice->Delete();
    }
}

//---------------------------------------------------------------------------
int vtkMRMLBitStreamVolumeNode::ObserveOutsideCompressionDevice(vtkMRMLCompressionDeviceNode* device)
{
  if (device)
    {
    device->AddObserver(device->GetDeviceContentModifiedEvent(), this, &vtkMRMLBitStreamVolumeNode::ProcessDeviceModifiedEvents);
    //------
    if (device->GetContent() != NULL)
      {
      std::string bitStream = std::string(device->GetContent()->frameMessage);
      this->SetFrameMessage(bitStream);
      if (device->GetContent()->image != NULL)
        {
        this->SetAndObserveImageData(device->GetContent()->image);
        }
      }
    this->CompressionDevice = device; // should the interal video device point to the external video device?
    //-------
    return 1;
    }
  return 0;
}

void vtkMRMLBitStreamVolumeNode::CopySrcStringToDestString(const std::string& srcString, std::string& destString)
{
  destString.resize(srcString.length());
  char * array = new char[srcString.length()];
  memcpy(array, srcString.c_str(), srcString.length());
  destString.assign(array, srcString.length());
}

void vtkMRMLBitStreamVolumeNode::SetFrameMessage(std::string& buffer)
{
  this->CopySrcStringToDestString(buffer, this->FrameMessage);
  this->FrameMessageValid = true;
};

std::string vtkMRMLBitStreamVolumeNode::GetFrameMessage()
{
  std::string returnMSG(this->FrameMessage);
  return returnMSG;
};

void vtkMRMLBitStreamVolumeNode::SetKeyFrameMessage(std::string& buffer)
{
  this->CopySrcStringToDestString(buffer, this->KeyFrameMessage);
};

std::string vtkMRMLBitStreamVolumeNode::GetKeyFrameMessage()
{
  std::string returnMSG(this->KeyFrameMessage);
  return returnMSG;
};

//---------------------------------------------------------------------------
int vtkMRMLBitStreamVolumeNode::DecodeFrameMessage(vtkMRMLCompressionDeviceNode::ContentData* deviceContent)
{
  if (this->CompressionDevice == NULL)
  {
    vtkWarningMacro("No compression device available, use the ObserveOutsideCompressionDevice() to set up the compression device.")
    return 0;
  }
  if (this->GetImageData() != this->CompressionDevice->GetContent()->image)
  {
    this->SetAndObserveImageData(this->CompressionDevice->GetContent()->image);
  }
  if(!this->GetKeyFrameDecoded())
    {
    this->SetKeyFrameDecoded(true);
    this->SetKeyFrameMessage(deviceContent->keyFrameMessage);
    int decodingStatus = this->CompressionDevice->UncompressedDataFromBitStream(deviceContent->keyFrameMessage, true);
    if (decodingStatus == 0)
      {
      vtkWarningMacro("key frame decoding failed.")
      return 0;
      }
    }
  std::string bitStreamData = std::string(deviceContent->frameMessage);
  if (this->CompressionDevice->UncompressedDataFromBitStream(bitStreamData, true))
    {
    this->Modified();
    return 1;
    }
  return 0;
}

//---------------------------------------------------------------------------
int vtkMRMLBitStreamVolumeNode::EncodeImageData()
{
  if (this->CompressionDevice == NULL)
    {
    vtkWarningMacro("No compression device available, use the ObserveOutsideCompressionDevice() to set up the compression device.")
    return 0;
    }
  if (this->GetImageData() != this->CompressionDevice->GetContent()->image)
    {
    this->CompressionDevice->SetContentImage(this->GetImageData());
    }
  std::string compressedStream = this->CompressionDevice->GetCompressedBitStreamFromData();
  if (compressedStream.size()>0)
    {
    this->SetFrameMessage(compressedStream);
    if(!this->GetKeyFrameReceived())
      {
      this->SetKeyFrameReceived(true);
      this->SetKeyFrameMessage(compressedStream);
      }
      return 1;
    }
  return 0;
}

std::string vtkMRMLBitStreamVolumeNode::GetCodecType()
{
  vtkMRMLCompressionDeviceNode*  device = this->GetCompressionDevice();
  if (device)
    {
    return device->GetCurrentCodecType();
    }
  return "";
}


int vtkMRMLBitStreamVolumeNode::SetCodecType(std::string name)
{
  vtkMRMLCompressionDeviceNode*  device = this->GetCompressionDevice();
  if(device)
  {
    device->SetCurrentCodecType(name);
    return 1;
  }
  return 0;
}

void vtkMRMLBitStreamVolumeNode::ProcessMRMLEvents(vtkObject *caller, unsigned long event, void *callData )
{
  this->vtkMRMLNode::ProcessMRMLEvents(caller, event, callData);
  this->InvokeEvent(ImageDataModifiedEvent);
}

void vtkMRMLBitStreamVolumeNode::ProcessDeviceModifiedEvents( vtkObject *caller, unsigned long event, void *callData )
{
  vtkMRMLCompressionDeviceNode* modifiedDevice = reinterpret_cast<vtkMRMLCompressionDeviceNode*>(caller);
  if (modifiedDevice == NULL)
    {
    // we are only interested in proxy node modified events
    return;
    }
  if (event != modifiedDevice->GetDeviceContentModifiedEvent())
    {
    return;
    }
  long streamLength = modifiedDevice->GetContent()->frameMessage.length();
  std::string* bitStream = new std::string(modifiedDevice->GetContent()->frameMessage);
  this->SetFrameUpdated(true);
  this->SetKeyFrameUpdated(false);
  this->SetFrameMessage(*bitStream);
  if(!this->GetKeyFrameReceived() || modifiedDevice->GetContent()->keyFrameUpdated)
    {
    streamLength = modifiedDevice->GetContent()->keyFrameMessage.length();
    std::string*  keyFrameStream = new std::string(modifiedDevice->GetContent()->keyFrameMessage);
    this->SetKeyFrameMessage(*keyFrameStream);
    this->SetKeyFrameReceived(true);
    this->SetKeyFrameUpdated(true);
    delete keyFrameStream;
    }
  delete bitStream;
  this->Modified();
}

//----------------------------------------------------------------------------
vtkMRMLCompressionDeviceNode* vtkMRMLBitStreamVolumeNode::GetCompressionDevice()
{
  return this->CompressionDevice;
}

//----------------------------------------------------------------------------
void vtkMRMLBitStreamVolumeNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
}

//----------------------------------------------------------------------------
void vtkMRMLBitStreamVolumeNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();
  
  Superclass::ReadXMLAttributes(atts);
  
  this->EndModify(disabledModify);
}


//----------------------------------------------------------------------------
void vtkMRMLBitStreamVolumeNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();
  Superclass::Copy(anode);
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLBitStreamVolumeNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}
