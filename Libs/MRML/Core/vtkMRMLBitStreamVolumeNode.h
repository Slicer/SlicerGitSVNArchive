#ifndef __vtkMRMLBitStreamVolumeNode_h
#define __vtkMRMLBitStreamVolumeNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>
#include <vtkMRMLStorageNode.h>
#include "vtkMRMLCompressionDeviceNode.h"
#include "vtkMRMLVectorVolumeDisplayNode.h"
#include "vtkMRMLVectorVolumeNode.h"
#include "vtkMRMLVolumeArchetypeStorageNode.h"

// VTK includes
#include <vtkStdString.h>
#include <vtkImageData.h>
#include <vtkObject.h>

/// \brief MRML node for representing video frame and the compressed bit stream data.
///
/// Bit Stream Volume nodes describe video frames and the bit streams that was generated
/// by compressing the video frames.
/// The node is a container for the neccesary information to interpert compressed video data:
/// 1. FrameMessage that contain the bit stream generated from current video frame.
/// 2. Key Frame Message that contain the bit stream of the key frame related to current video frame.
/// 3. The related compression device for encoding and decoding. This device is by default NULL.
///    User needs to register the compression device to the vtkMRMLBitStreamVolumeNode
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
  {return "BitStreamVolume";}

  ///
  /// Flag indicates whether key frame message was received or not.
  void SetKeyFrameReceived(bool value){this->KeyFrameReceived = value;};
  vtkGetMacro(KeyFrameReceived, bool);

  ///
  /// Flag indicates whether the current key frame message was updated or not.
  void SetKeyFrameUpdated(bool value){this->KeyFrameUpdated = value;};
  vtkGetMacro(KeyFrameUpdated, bool);

  ///
  /// Flag indicates whether the current key frame message was decoded or not.
  void SetKeyFrameDecoded(bool value){this->KeyFrameDecoded = value;};
  vtkGetMacro(KeyFrameDecoded, bool);

  ///
  /// Flag indicates whether the current frame message was updated or not.
  void SetFrameUpdated(bool value){this->FrameUpdated = value;};
  vtkGetMacro(FrameUpdated, bool);

  ///
  /// Flag indicates whether the current frame message was valid or not.
  void SetFrameMessageValid(bool value){this->FrameMessageValid = value;};
  vtkGetMacro(FrameMessageValid, bool);

  ///
  /// Get the compression codec type, a compression device could contain serveral codec.
  /// This function returns the codec that is used for encoding and decoding.
  std::string GetCodecType();

  ///
  /// Set the codec to be used in the encoding and decoding process.
  int SetCodecType(std::string type);

  ///
  /// Return the compression device that is currently observed.
  vtkMRMLCompressionDeviceNode* GetCompressionDevice();

  ///
  /// Observe an outside compression device.
  int ObserveOutsideCompressionDevice(vtkMRMLCompressionDeviceNode* devicePtr);

  ///
  /// Set the frame message
  void SetFrameMessage(std::string& buffer);

  ///
  /// Return the frame message
  std::string GetFrameMessage();

  ///
  /// Set the key frame message
  void SetKeyFrameMessage(std::string& buffer);

  ///
  /// Return the key frame message
  std::string GetKeyFrameMessage();

  ///
  /// Encode the image data using the observed compression device
  int EncodeImageData();

  ///
  /// Decode the bit stream data and store the frame in the vtk image pointer specified in the deviceContent
  /// If the image pointer is not the same as the image pointer in the vtkMRMLBitStreamVolumeNode,
  /// then force the vtkMRMLBitStreamVolumeNode to observe the image pointer in the deviceContent.
  int DecodeFrameMessage(vtkMRMLCompressionDeviceNode::ContentData* deviceContent);

  ///
  /// Helper function to copy source string to destination string
  void CopySrcStringToDestString(const std::string& srcString, std::string& destString);

protected:
  vtkMRMLBitStreamVolumeNode();
  ~vtkMRMLBitStreamVolumeNode();
  vtkMRMLBitStreamVolumeNode(const vtkMRMLBitStreamVolumeNode&);
  void operator=(const vtkMRMLBitStreamVolumeNode&);
    
  bool KeyFrameReceived;
  
  bool KeyFrameUpdated;
  
  bool KeyFrameDecoded;
  
  bool FrameUpdated;

  bool FrameMessageValid;

  std::string FrameMessage;

  std::string KeyFrameMessage;

  vtkMRMLCompressionDeviceNode* CompressionDevice;

};

#endif