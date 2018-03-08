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
  DecodersMap.clear();
}

//---------------------------------------------------------------------------
vtkMRMLCompressionDeviceNode::~vtkMRMLCompressionDeviceNode()
{
  DecodersMap.clear();
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

//---------------------------------------------------------------------------
int vtkMRMLCompressionDeviceNode::UncompressedDataFromBitStream(std::string bitStreamData, bool checkCRC)
{
  //To do : use the buffer to update Content->image
  //Return 1 on successful, 0 for unsuccessful
  return 0;
}
void vtkMRMLCompressionDeviceNode::SetContent(ContentData* content)
{
  Content = content;
  this->Modified();
  this->InvokeEvent(DeviceModifiedEvent, this);
}

//---------------------------------------------------------------------------
std::string* vtkMRMLCompressionDeviceNode::GetCompressedBitStreamFromData()
{
  return NULL;
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


