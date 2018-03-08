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

//---------------------------------------------------------------------------
class vtkMRMLBitStreamVolumeNode::vtkInternal:public vtkObject
{
public:
  //---------------------------------------------------------------------------
  vtkInternal(vtkMRMLBitStreamVolumeNode* external);
  ~vtkInternal();
  
  void SetVideoMessageDevice(vtkMRMLCompressionDeviceNode* inDevice)
  {
    this->compressionDevice = inDevice;
  };

  vtkMRMLCompressionDeviceNode* GetCompressionDevice()
  {
    return this->compressionDevice;
  }

  int ObserveOutsideCompressionDevice(vtkMRMLCompressionDeviceNode* device);

  vtkMRMLBitStreamVolumeNode* External;

  std::string MessageBuffer;
  std::string KeyFrameBuffer;
  vtkMRMLCompressionDeviceNode* compressionDevice;
};

//----------------------------------------------------------------------------
// vtkInternal methods

//---------------------------------------------------------------------------
vtkMRMLBitStreamVolumeNode::vtkInternal::vtkInternal(vtkMRMLBitStreamVolumeNode* external)
  : External(external)
{
  compressionDevice = NULL;
}

//---------------------------------------------------------------------------
vtkMRMLBitStreamVolumeNode::vtkInternal::~vtkInternal()
{
}

//---------------------------------------------------------------------------
int vtkMRMLBitStreamVolumeNode::vtkInternal::ObserveOutsideCompressionDevice(vtkMRMLCompressionDeviceNode* device)
{
  if (device)
  {
    device->AddObserver(device->GetDeviceContentModifiedEvent(), this->External, &vtkMRMLBitStreamVolumeNode::ProcessDeviceModifiedEvents);
    //------
    std::string bitStream = std::string(device->GetContent()->frameMessage);
    this->External->SetMessageStream(bitStream);
    this->External->SetAndObserveImageData(device->GetContent()->image);
    this->compressionDevice = device; // should the interal video device point to the external video device?
    //-------
    return 1;
  }
  return 0;
}

void vtkMRMLBitStreamVolumeNode::SetMessageStream(std::string buffer)
{
  this->Internal->MessageBuffer.resize(buffer.length());
  char * array = new char[buffer.length()];
  memcpy(array, buffer.c_str(), buffer.length());
  this->Internal->MessageBuffer.assign(array, buffer.length());
  this->MessageBufferValid = true;
};

std::string* vtkMRMLBitStreamVolumeNode::GetMessageStreamBuffer()
{
  std::string* returnMSG = new std::string(this->Internal->MessageBuffer);
  return returnMSG;
};

void vtkMRMLBitStreamVolumeNode::SetKeyFrameStream(std::string buffer)
{
  this->Internal->KeyFrameBuffer.resize(buffer.length());
  char * array = new char[buffer.length()];
  memcpy(array, buffer.c_str(), buffer.length());
  this->Internal->KeyFrameBuffer.assign(array, buffer.length());
};

std::string* vtkMRMLBitStreamVolumeNode::GetKeyFrameStream()
{
  std::string* returnMSG = new std::string(this->Internal->KeyFrameBuffer);
  return returnMSG;
};

//---------------------------------------------------------------------------
void vtkMRMLBitStreamVolumeNode::DecodeMessageStream(vtkMRMLCompressionDeviceNode::ContentData* deviceContent)
{
  if (this->Internal->compressionDevice == NULL)
  {
    vtkWarningMacro("No compression device available, use the ObserveOutsideCompressionDevice() to set up the compression device.")
    return;
  }
  if (this->GetImageData() != this->Internal->compressionDevice->GetContent()->image)
  {
    this->SetAndObserveImageData(this->Internal->compressionDevice->GetContent()->image);
  }
  if(!this->GetKeyFrameDecoded())
    {
    this->SetKeyFrameDecoded(true);
    this->SetKeyFrameStream(deviceContent->keyFrameMessage);
    this->Internal->compressionDevice->UncompressedDataFromBitStream(deviceContent->keyFrameMessage, true);
    }
  std::string bitStreamData = std::string(deviceContent->frameMessage);
  if (this->Internal->compressionDevice->UncompressedDataFromBitStream(bitStreamData, true))
    {
    this->Modified();
    }
}


std::string vtkMRMLBitStreamVolumeNode::GetCodecName()
{
  vtkMRMLCompressionDeviceNode*  device = this->GetCompressionDevice();
  if (device && device->GetContent())
  {
  return device->GetContent()->codecName;
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

//----------------------------------------------------------------------------
// vtkMRMLBitStreamVolumeNode methods

//-----------------------------------------------------------------------------
vtkMRMLBitStreamVolumeNode::vtkMRMLBitStreamVolumeNode()
{
  this->Internal = new vtkInternal(this);
  IsCopied = false;
  KeyFrameReceived = false;
  KeyFrameDecoded = false;
  KeyFrameUpdated = false;
  MessageBufferValid = false;
}

//-----------------------------------------------------------------------------
vtkMRMLBitStreamVolumeNode::~vtkMRMLBitStreamVolumeNode()
{
  delete this->Internal;
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

//----------------------------------------------------------------------------
vtkMRMLStorageNode* vtkMRMLBitStreamVolumeNode::CreateDefaultStorageNode()
{
  return vtkMRMLNRRDStorageNode::New();
}

//----------------------------------------------------------------------------
vtkMRMLCompressionDeviceNode* vtkMRMLBitStreamVolumeNode::GetCompressionDevice()
{
  return this->Internal->GetCompressionDevice();
}

//----------------------------------------------------------------------------
int vtkMRMLBitStreamVolumeNode::ObserveOutsideCompressionDevice(vtkMRMLCompressionDeviceNode* devicePtr)
{
  // add a default compression device here.
  return this->Internal->ObserveOutsideCompressionDevice(devicePtr);
}