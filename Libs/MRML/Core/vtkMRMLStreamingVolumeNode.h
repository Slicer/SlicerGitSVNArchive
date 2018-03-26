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
/// 3. The related compression codec for encoding and decoding. This codec is by default NULL.
///    User needs to register the compression codec to the vtkMRMLStreamingVolumeNode
class  VTK_MRML_EXPORT vtkMRMLStreamingVolumeNode : public vtkMRMLVectorVolumeNode
{
public:
  
  static vtkMRMLStreamingVolumeNode *New();
  vtkTypeMacro(vtkMRMLStreamingVolumeNode,vtkMRMLVectorVolumeNode);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  
  virtual vtkMRMLNode* CreateNodeInstance() VTK_OVERRIDE;
  
  virtual void ProcessMRMLEvents( vtkObject *caller, unsigned long event, void *callData ) VTK_OVERRIDE;
  
  void ProcessCodecModifiedEvents( vtkObject *caller, unsigned long event, void *callData );
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
  /// Set/Get codec type from the volume node, mainly for information storage purpose
  vtkSetStringMacro(CodecType);
  vtkGetStringMacro(CodecType);

  ///
  /// Set/Get codec name from the volume node, mainly for information storage purpose
  vtkSetStringMacro(CodecName);
  vtkGetStringMacro(CodecName);

  ///
  /// Set/Get codec class name from the volume node, mainly for information storage purpose
  vtkSetStringMacro(CodecClassName);
  vtkGetStringMacro(CodecClassName);

  ///
  /// Get the compression codec type, a compression codec could contain serveral codec.
  /// This function returns the codec that is used for encoding and decoding.
  std::string GetCodecTypeFromCompressor();

  ///
  /// Set the codec to be used in the encoding and decoding process.
  int SetCodecTypeToCompressor(std::string type);

  ///
  /// Return the compression codec that is currently observed.
  vtkStreamingVolumeCodec* GetCompressionCodec();

  ///
  /// Observe an outside compression codec. Please use a smart pointer to avoid memory leak
  int ObserveOutsideCompressionCodec(vtkStreamingVolumeCodec* codecPtr);

  ///
  /// Update the frame message from codec codec frame
  int UpdateFrameByDeepCopy(vtkSmartPointer<vtkUnsignedCharArray> buffer);

  ///
  /// Set/Get the frame message.
  void SetFrame(vtkSmartPointer<vtkUnsignedCharArray> frame){this->Frame = frame;};
  vtkUnsignedCharArray* GetFrame(){return this->Frame.GetPointer();};

  ///
  /// Update the key frame message from data stream
  int UpdateKeyFrameByDeepCopy(vtkSmartPointer<vtkUnsignedCharArray> buffer);

  ///
  /// Set/Get the key frame
  void SetKeyFrame(vtkSmartPointer<vtkUnsignedCharArray> keyFrame){this->KeyFrame = keyFrame;};
  vtkUnsignedCharArray* GetKeyFrame(){return this->KeyFrame.GetPointer();};

  ///
  /// Encode the image data using the observed compression codec
  int EncodeImageData();

  ///
  /// Decode the frame and store the frame in the vtk image pointer specified in the codecContent
  /// If the image pointer is not the same as the image pointer in the vtkMRMLStreamingVolumeNode,
  /// then force the vtkMRMLStreamingVolumeNode to observe the image pointer in the codecContent.
  int DecodeFrame(vtkUnsignedCharArray*  frame);

  ///
  /// Decode the key frame and store the key frame in the vtk image pointer specified in the codecContent
  /// If the image pointer is not the same as the image pointer in the vtkMRMLStreamingVolumeNode,
  /// then force the vtkMRMLStreamingVolumeNode to observe the image pointer in the codecContent.
  int DecodeKeyFrame(vtkUnsignedCharArray*  keyFrame);

  ///
  /// Helper function to copy source string to destination string
  void CopySrcArrayToDestArray(vtkSmartPointer<vtkUnsignedCharArray> srcString, vtkSmartPointer<vtkUnsignedCharArray> destString);

  ///
  /// Get the name of the CompressionCodec
  std::string GetCodecNameFromCompressor();

  ///
  /// Set the name of the CompressionCodec
  int SetCodecNameToCompressor(std::string name);

protected:
  vtkMRMLStreamingVolumeNode();
  ~vtkMRMLStreamingVolumeNode();
  vtkMRMLStreamingVolumeNode(const vtkMRMLStreamingVolumeNode&);
  void operator=(const vtkMRMLStreamingVolumeNode&);
  
  bool KeyFrameUpdated;
  
  bool KeyFrameDecoded;
  
  bool FrameUpdated;

  vtkSmartPointer<vtkUnsignedCharArray> Frame;

  vtkSmartPointer<vtkUnsignedCharArray> KeyFrame;

  vtkSmartPointer<vtkStreamingVolumeCodec> CompressionCodec;

  char* CodecName;

  char* CodecType;

  char* CodecClassName;

};

#endif