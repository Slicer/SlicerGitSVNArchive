#ifndef __vtkMRMLStreamingVolumeNode_h
#define __vtkMRMLStreamingVolumeNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>
#include <vtkMRMLStorageNode.h>
#include "vtkStreamingVolumeCodec.h"
#include "vtkMRMLVectorVolumeDisplayNode.h"
#include "vtkMRMLVectorVolumeNode.h"
#include "vtkMRMLVolumeArchetypeStorageNode.h"

// VTK includes
#include <vtkStdString.h>
#include <vtkImageData.h>
#include <vtkObject.h>
#include <vtkUnsignedCharArray.h>

/// \brief MRML node for representing video frame and the compressed bit stream data.
///
/// Bit Stream Volume nodes describe video frames and the bit streams that was generated
/// by compressing the video frames.
/// The node is a container for the neccesary information to interpert compressed video data:
/// 1. FrameMessage that contain the bit stream generated from current video frame.
/// 2. Key Frame Message that contain the bit stream of the key frame related to current video frame.
/// 3. The related compression device for encoding and decoding. This device is by default NULL.
///    User needs to register the compression device to the vtkMRMLStreamingVolumeNode
class  VTK_MRML_EXPORT vtkMRMLStreamingVolumeNode : public vtkMRMLVectorVolumeNode
{
public:
  
  static vtkMRMLStreamingVolumeNode *New();
  vtkTypeMacro(vtkMRMLStreamingVolumeNode,vtkMRMLVectorVolumeNode);
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
  {return "StreamingVolume";}

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
  /// Get the compression codec type, a compression device could contain serveral codec.
  /// This function returns the codec that is used for encoding and decoding.
  std::string GetCodecType();

  ///
  /// Set the codec to be used in the encoding and decoding process.
  int SetCodecType(std::string type);

  ///
  /// Return the compression device that is currently observed.
  vtkStreamingVolumeCodec* GetCompressionCodec();

  ///
  /// Observe an outside compression device.
  int ObserveOutsideCompressionCodec(vtkStreamingVolumeCodec* devicePtr);

  ///
  ///  Update the frame message from data stream
  void UpdateFrameFromDataStream(std::string& buffer);

  ///
  /// Return the frame message
  vtkSetObjectMacro(Frame, vtkUnsignedCharArray);
  vtkGetObjectMacro(Frame, vtkUnsignedCharArray);

  ///
  /// Update the key frame message from data stream
  void UpdateKeyFrameFromDataStream(std::string& buffer);

  ///
  /// Set/Get the key frame
  vtkSetObjectMacro(KeyFrame, vtkUnsignedCharArray);
  vtkGetObjectMacro(KeyFrame, vtkUnsignedCharArray);

  ///
  /// Encode the image data using the observed compression device
  int EncodeImageData();

  ///
  /// Decode the bit stream data and store the frame in the vtk image pointer specified in the deviceContent
  /// If the image pointer is not the same as the image pointer in the vtkMRMLStreamingVolumeNode,
  /// then force the vtkMRMLStreamingVolumeNode to observe the image pointer in the deviceContent.
  int DecodeFrame(vtkStreamingVolumeCodec::ContentData* deviceContent);

  ///
  /// Helper function to copy source string to destination string
  void CopySrcStringToDestArray(const std::string& srcString, vtkUnsignedCharArray* destString);

  ///
  /// Get the name of the CompressionCodec
  std::string GetCodecName();

  ///
  /// Set the name of the CompressionCodec
  int SetCodecName(std::string name);

  ///
  /// Get the class name of the CompressionCodec
  std::string GetCompressionCodecClassName();

  ///
  /// Set the class name of the CompressionCodec
  int SetCompressionCodecClassName(std::string name);

protected:
  vtkMRMLStreamingVolumeNode();
  ~vtkMRMLStreamingVolumeNode();
  vtkMRMLStreamingVolumeNode(const vtkMRMLStreamingVolumeNode&);
  void operator=(const vtkMRMLStreamingVolumeNode&);
    
  bool KeyFrameReceived;
  
  bool KeyFrameUpdated;
  
  bool KeyFrameDecoded;
  
  bool FrameUpdated;

  vtkUnsignedCharArray* Frame;

  vtkUnsignedCharArray* KeyFrame;

  vtkStreamingVolumeCodec* CompressionCodec;

  std::string CompressionCodecClassName;

  std::string CodecName;
};

#endif