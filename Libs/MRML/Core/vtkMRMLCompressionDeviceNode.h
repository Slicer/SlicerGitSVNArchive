#ifndef __vtkMRMLCompressionDeviceNode_h
#define __vtkMRMLCompressionDeviceNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>
#include <vtkMRMLStorageNode.h>

// VTK includes
#include <vtkStdString.h>
#include <vtkImageData.h>
#include <vtkObject.h>
#include <vtkSmartPointer.h>


/// \brief MRML node for representing video codec.
///
/// Any nodes that encapsulates video codec should derive from the vtkMRMLCompressionDeviceNode.
/// This compression device node is observed by the vtkMRMLBitStreamVolumeNode. This device node
/// generates keyframeMessage and frameMessage from the image data in the vtkMRMLBitStreamVolumeNode
/// See this derived node for more detail:
/// https://github.com/openigtlink/SlicerOpenIGTLink/blob/BitStreamNodeRemoval/OpenIGTLinkIF/MRML/vtkMRMLIGTLIOCompressionDeviceNode.h
/// Two functions in this class needs to be derived from child class:
/// 1. virtual int UncompressedDataFromBitStream(std::string bitStreamData, bool checkCRC);
/// 2. virtual std::string GetCompressedBitStreamFromData();
class VTK_MRML_EXPORT vtkMRMLCompressionDeviceNode : public vtkMRMLNode
{
public:
  static vtkMRMLCompressionDeviceNode *New();
  vtkTypeMacro(vtkMRMLCompressionDeviceNode, vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  struct ContentData
  {
    vtkSmartPointer<vtkImageData> image;
    int frameType;
    bool keyFrameUpdated;
    std::string deviceName;
    std::string codecType;
    std::string frameMessage; // for saving the compressed data.
    std::string keyFrameMessage; // for saving the compressed data.
  };
  
  virtual vtkMRMLNode* CreateNodeInstance() VTK_OVERRIDE;
  
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
  {return "CompressionDevice";}

  ///
  /// The device modified event. This event could be invoke when bit stream is generated or decoded.
  virtual unsigned int GetDeviceContentModifiedEvent() const;

  ///
  /// Get the compression device type, a compression device could contain serveral codec.
  /// This function only returns the compresion device type.
  virtual std::string GetDeviceType() const;

  ///
  /// Decode bit stream and update the image pointer in content.
  /// The image pointer normally from the vtkMRMLBitStreamVolumeNode
  virtual int UncompressedDataFromBitStream(std::string bitStreamData, bool checkCRC);

  ///
  /// Return the compressed bit stream from the image.
  virtual std::string GetCompressedBitStreamFromData();

  ///
  /// Get the compression codec type, a compression device could contain serveral codec.
  /// This function returns the codec that is used for encoding and decoding.
  virtual std::string GetCurrentCodecType();

  ///
  /// Return the Content data
  virtual ContentData* GetContent();

  ///
  /// Return the image in the Content data
  virtual vtkImageData* GetContentImage();

  ///
  /// Return the device name in the Content data
  virtual std::string GetContentDeviceName();

  ///
  /// Set the codec typt in the Content data, will be useful to indicate which codec to use.
  virtual int SetCurrentCodecType(std::string codecType);

  ///
  /// Set the Content data
  virtual void SetContent(ContentData* content);

  ///
  /// Set the image pointer in the Content data
  virtual void SetContentImage(vtkImageData* image);

  ///
  /// Set the device name in the Content data
  virtual void SetContentDeviceName(std::string name);

protected:
  vtkMRMLCompressionDeviceNode();
  ~vtkMRMLCompressionDeviceNode();

  enum {
    DeviceModifiedEvent         = 118961, //Todo, should have a different id, as it is conflict with openigtlinkIO VideoDeviceModified event
  };

  enum {
    UndefinedFrameType         = -1 // undefined frame type
  };

  ContentData* Content;
};

#endif