#ifndef __vtkMRMLBitStreamVolumeNode_h
#define __vtkMRMLBitStreamVolumeNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>
#include <vtkMRMLStorageNode.h>
#include "vtkMRMLVectorVolumeDisplayNode.h"
#include "vtkMRMLVectorVolumeNode.h"
#include "vtkMRMLVolumeArchetypeStorageNode.h"
#include "vtkMRMLCompressionDeviceNode.h"

// VTK includes
#include <vtkStdString.h>
#include <vtkImageData.h>
#include <vtkObject.h>

class  VTK_MRML_EXPORT vtkMRMLBitStreamVolumeNode : public vtkMRMLVectorVolumeNode
{
public:
  
  static vtkMRMLBitStreamVolumeNode *New();
  vtkTypeMacro(vtkMRMLBitStreamVolumeNode,vtkMRMLVectorVolumeNode);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  
  virtual vtkMRMLNode* CreateNodeInstance() VTK_OVERRIDE;
  
  virtual void ProcessMRMLEvents( vtkObject *caller, unsigned long event, void *callData ) VTK_OVERRIDE;
  
  void ProcessDeviceModifiedEvents( vtkObject *caller, unsigned long event, void *callData );
  ///
  /// Set node attributes
  virtual void ReadXMLAttributes( const char** atts) VTK_OVERRIDE;
  
  ///
  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent) VTK_OVERRIDE;
  
  ///
  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node) VTK_OVERRIDE;
  
  ///
  /// Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() VTK_OVERRIDE
  {return "BitStreamVolume";};

  void SetKeyFrameReceived(bool value){this->KeyFrameReceived = value;};
  vtkGetMacro(KeyFrameReceived, bool);

  void SetKeyFrameUpdated(bool value){this->KeyFrameReceived = value;};
  vtkGetMacro(KeyFrameUpdated, bool);

  void SetKeyFrameDecoded(bool value){this->KeyFrameDecoded = value;};
  vtkGetMacro(KeyFrameDecoded, bool);

  void SetIsCopied(bool value){this->IsCopied = value;};
  vtkGetMacro(IsCopied, bool);

  void SetMessageBufferValid(bool value){this->MessageBufferValid = value;};
  vtkGetMacro(MessageBufferValid, bool);
  
  /// Return a default file extension for writting
  //virtual const char* GetDefaultWriteFileExtension();
  
  std::string GetCodecName();
  
  int SetCodecName(std::string name);

  vtkMRMLCompressionDeviceNode* GetCompressionDevice();

  int ObserveOutsideCompressionDevice(vtkMRMLCompressionDeviceNode* devicePtr);

  void SetMessageStream(std::string buffer);

  std::string GetMessageStream();

  void SetKeyFrameStream(std::string buffer);

  std::string GetKeyFrameStream();

  int EncodeImageData();
  
  void DecodeMessageStream(vtkMRMLCompressionDeviceNode::ContentData* deviceContent);

protected:
  vtkMRMLBitStreamVolumeNode();
  ~vtkMRMLBitStreamVolumeNode();
  vtkMRMLBitStreamVolumeNode(const vtkMRMLBitStreamVolumeNode&);
  void operator=(const vtkMRMLBitStreamVolumeNode&);
    
  bool KeyFrameReceived;
  
  bool KeyFrameUpdated;
  
  bool KeyFrameDecoded;
  
  bool IsCopied;

  bool MessageBufferValid;

  std::string MessageBuffer;

  std::string KeyFrameBuffer;

  vtkMRMLCompressionDeviceNode* compressionDevice;

};

#endif