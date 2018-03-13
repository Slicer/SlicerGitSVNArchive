#include "vtkMRMLBitStreamVolumeNode.h"

// OpenIGTLink includes
//#include "igtlioVideoDevice.h"

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
  this->compressionDevice = NULL;
  IsCopied = false;
  KeyFrameReceived = false;
  KeyFrameDecoded = false;
  KeyFrameUpdated = false;
  MessageBufferValid = false;
  KeyFrameBuffer = "";
  MessageBuffer = "";
}

//-----------------------------------------------------------------------------
vtkMRMLBitStreamVolumeNode::~vtkMRMLBitStreamVolumeNode()
{
  if (compressionDevice != NULL)
     compressionDevice->Delete();
}

//---------------------------------------------------------------------------
int vtkMRMLBitStreamVolumeNode::ObserveOutsideCompressionDevice(vtkMRMLCompressionDeviceNode* device)
{
  if (device)
  {
    device->AddObserver(device->GetDeviceContentModifiedEvent(), this, &vtkMRMLBitStreamVolumeNode::ProcessDeviceModifiedEvents);
    //------
    std::string bitStream = std::string(device->GetContent()->frameMessage);
    this->SetMessageStream(bitStream);
    this->SetAndObserveImageData(device->GetContent()->image);
    this->compressionDevice = device; // should the interal video device point to the external video device?
    //-------
    return 1;
  }
  return 0;
}

void vtkMRMLBitStreamVolumeNode::SetMessageStream(std::string buffer)
{
  this->MessageBuffer.resize(buffer.length());
  char * array = new char[buffer.length()];
  memcpy(array, buffer.c_str(), buffer.length());
  this->MessageBuffer.assign(array, buffer.length());
  this->MessageBufferValid = true;
};

std::string vtkMRMLBitStreamVolumeNode::GetMessageStream()
{
  std::string returnMSG(this->MessageBuffer);
  return returnMSG;
};

void vtkMRMLBitStreamVolumeNode::SetKeyFrameStream(std::string buffer)
{
  this->KeyFrameBuffer.resize(buffer.length());
  char * array = new char[buffer.length()];
  memcpy(array, buffer.c_str(), buffer.length());
  this->KeyFrameBuffer.assign(array, buffer.length());
};

std::string vtkMRMLBitStreamVolumeNode::GetKeyFrameStream()
{
  std::string returnMSG(this->KeyFrameBuffer);
  return returnMSG;
};

//---------------------------------------------------------------------------
void vtkMRMLBitStreamVolumeNode::DecodeMessageStream(vtkMRMLCompressionDeviceNode::ContentData* deviceContent)
{
  if (this->compressionDevice == NULL)
  {
    vtkWarningMacro("No compression device available, use the ObserveOutsideCompressionDevice() to set up the compression device.")
    return;
  }
  if (this->GetImageData() != this->compressionDevice->GetContent()->image)
  {
    this->SetAndObserveImageData(this->compressionDevice->GetContent()->image);
  }
  if(!this->GetKeyFrameDecoded())
    {
    this->SetKeyFrameDecoded(true);
    this->SetKeyFrameStream(deviceContent->keyFrameMessage);
    this->compressionDevice->UncompressedDataFromBitStream(deviceContent->keyFrameMessage, true);
    }
  std::string bitStreamData = std::string(deviceContent->frameMessage);
  if (this->compressionDevice->UncompressedDataFromBitStream(bitStreamData, true))
    {
    this->Modified();
    }
}

//---------------------------------------------------------------------------
int vtkMRMLBitStreamVolumeNode::EncodeImageData()
{
  if (this->compressionDevice == NULL)
    {
    vtkWarningMacro("No compression device available, use the ObserveOutsideCompressionDevice() to set up the compression device.")
    return 0;
    }
  if (this->GetImageData() != this->compressionDevice->GetContent()->image)
    {
    this->compressionDevice->SetContentImage(this->GetImageData());
    }
  std::string compressedStream = this->compressionDevice->GetCompressedBitStreamFromData();
  if (compressedStream.size()>0)
    {
    this->SetMessageStream(compressedStream);
    if(!this->GetKeyFrameReceived())
      {
      this->SetKeyFrameReceived(true);
      this->SetKeyFrameStream(compressedStream);
      }
      return 1;
    }
  return 0;
}

std::string vtkMRMLBitStreamVolumeNode::GetCodecName()
{
  vtkMRMLCompressionDeviceNode*  device = this->GetCompressionDevice();
  if (device)
    {
    if (device->GetContent())
      return device->GetContent()->codecName;
    return "";
    }
  return "";
}


int vtkMRMLBitStreamVolumeNode::SetCodecName(std::string name)
{
  vtkMRMLCompressionDeviceNode*  device = this->GetCompressionDevice();
  if(device)
  {
    device->GetContent()->codecName = name;
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
  if (event != vtkMRMLCompressionDeviceNode::DeviceModifiedEvent)
    {
    return;
    }
  long streamLength = modifiedDevice->GetContent()->frameMessage.length();
  std::string* bitStream = new std::string(modifiedDevice->GetContent()->frameMessage);
  this->SetIsCopied(false);
  this->SetKeyFrameUpdated(false);
  this->SetMessageStream(*bitStream);
  if(!this->GetKeyFrameReceived() || modifiedDevice->GetContent()->keyFrameUpdated)
    {
    streamLength = modifiedDevice->GetContent()->keyFrameMessage.length();
    std::string*  keyFrameStream = new std::string(modifiedDevice->GetContent()->keyFrameMessage);
    this->SetKeyFrameStream(*keyFrameStream);
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
  return this->compressionDevice;
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
