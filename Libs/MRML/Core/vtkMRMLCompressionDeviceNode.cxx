#include "vtkMRMLCompressionDeviceNode.h"

// MRML includes
#include "vtkMRMLScene.h"
#include "vtkMRMLNRRDStorageNode.h"

// VTK includes
#include <vtkCollection.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkXMLUtilities.h>

//---------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLCompressionDeviceNode);
//---------------------------------------------------------------------------
vtkMRMLCompressionDeviceNode::vtkMRMLCompressionDeviceNode()
{
  Content = new ContentData();
  Content->image = NULL;
  Content->frameType = -1; // -1 is an invalid value
  Content->keyFrameUpdated = false;
  Content->codecName = "";
  Content->deviceName = "";
}

//---------------------------------------------------------------------------
vtkMRMLCompressionDeviceNode::~vtkMRMLCompressionDeviceNode()
{
  delete Content;
}

//---------------------------------------------------------------------------
unsigned int vtkMRMLCompressionDeviceNode::GetDeviceContentModifiedEvent() const
{
  return DeviceModifiedEvent;
}


//---------------------------------------------------------------------------
std::string vtkMRMLCompressionDeviceNode::GetDeviceType() const
{
  return "";
}

std::string vtkMRMLCompressionDeviceNode::GetCurrentCodecType()
{
  return this->Content->codecName;
};

vtkMRMLCompressionDeviceNode::ContentData* vtkMRMLCompressionDeviceNode::GetContent()
{
  return Content;
}

vtkImageData* vtkMRMLCompressionDeviceNode::GetContentImage()
{
  return this->Content->image;
}

std::string vtkMRMLCompressionDeviceNode::GetContentDeviceName()
{
  return this->Content->deviceName;
}

int vtkMRMLCompressionDeviceNode::SetCurrentCodecType(std::string codecType)
{
  this->Content->codecName = std::string(codecType);
  return 0;
}

void vtkMRMLCompressionDeviceNode::SetContent(ContentData* content)
{
  Content = content;
  this->Modified();
  this->InvokeEvent(DeviceModifiedEvent, this);
}


void vtkMRMLCompressionDeviceNode::SetContentImage(vtkImageData* image)
{
  this->Content->image = image;
}

void vtkMRMLCompressionDeviceNode::SetContentDeviceName(std::string name)
{
  this->Content->deviceName = std::string(name);
}


//---------------------------------------------------------------------------
int vtkMRMLCompressionDeviceNode::UncompressedDataFromBitStream(std::string bitStreamData, bool checkCRC)
{
  //To do : use the buffer to update Content->image
  //Return 1 on successful, 0 for unsuccessful
  return 0;
}

//---------------------------------------------------------------------------
std::string vtkMRMLCompressionDeviceNode::GetCompressedBitStreamFromData()
{
  return "";
}

//---------------------------------------------------------------------------
void vtkMRMLCompressionDeviceNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << indent << "Video:\t" <<"\n";
  Content->image->PrintSelf(os, indent.GetNextIndent());
}


//----------------------------------------------------------------------------
void vtkMRMLCompressionDeviceNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
}

//----------------------------------------------------------------------------
void vtkMRMLCompressionDeviceNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();
  
  Superclass::ReadXMLAttributes(atts);
  
  this->EndModify(disabledModify);
}


//----------------------------------------------------------------------------
void vtkMRMLCompressionDeviceNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();
  Superclass::Copy(anode);
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
vtkMRMLStorageNode* vtkMRMLCompressionDeviceNode::CreateDefaultStorageNode()
{
  return vtkMRMLNRRDStorageNode::New();
}


