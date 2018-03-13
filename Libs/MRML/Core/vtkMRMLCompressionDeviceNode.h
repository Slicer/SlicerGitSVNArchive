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

class VTK_MRML_EXPORT vtkMRMLCompressionDeviceNode : public vtkMRMLNode
{
public:
  
  enum {
    DeviceModifiedEvent         = 118961, //Todo, should have a different id, as it is conflict with openigtlinkIO VideoDeviceModified event
  };
  
  struct ContentData
  {
    vtkSmartPointer<vtkImageData> image;
    int frameType;
    bool keyFrameUpdated;
    std::string deviceName;
    std::string codecName;
    std::string frameMessage; // for saving the compressed data.
    std::string keyFrameMessage; // for saving the compressed data.
  };
  
  virtual vtkMRMLNode* CreateNodeInstance() VTK_OVERRIDE;
  
  /// Set node attributes
  virtual void ReadXMLAttributes( const char** atts) VTK_OVERRIDE;
  
  /// Create default storage node or NULL if does not have one
  virtual vtkMRMLStorageNode* CreateDefaultStorageNode() VTK_OVERRIDE;
  
  ///
  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent) VTK_OVERRIDE;
  
  ///
  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node) VTK_OVERRIDE;
  
  ///
  /// Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() VTK_OVERRIDE
  {return "CompressionDevice";};

  virtual unsigned int GetDeviceContentModifiedEvent() const;

  virtual std::string GetDeviceType() const;

  ///
  /// Decode bit stream
  virtual int UncompressedDataFromBitStream(std::string bitStreamData, bool checkCRC);

  ///
  ///
  virtual std::string GetCompressedBitStreamFromData();
  
  virtual std::string GetCurrentCodecType();

  virtual ContentData* GetContent();

  virtual vtkImageData* GetContentImage();

  virtual std::string GetContentDeviceName();

  virtual int SetCurrentCodecType(std::string codecType);

  virtual void SetContent(ContentData* content);

  virtual void SetContentImage(vtkImageData* image);

  virtual void SetContentDeviceName(std::string name);




public:
  static vtkMRMLCompressionDeviceNode *New();
  vtkTypeMacro(vtkMRMLCompressionDeviceNode, vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  
protected:
  vtkMRMLCompressionDeviceNode();
  ~vtkMRMLCompressionDeviceNode();
  
protected:
  
  ContentData* Content;
};

#endif