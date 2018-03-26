/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkStreamingVolumeCodec.h,v $
  Date:      $Date: 2006/03/19 17:12:29 $
  Version:   $Revision: 1.13 $

=========================================================================auto=*/

#ifndef __vtkStreamingVolumeCodec_h
#define __vtkStreamingVolumeCodec_h

// MRML includes
#include "vtkMRML.h"

// VTK includes
#include <vtkObject.h>
#include <vtkStdString.h>
#include <vtkImageData.h>
#include <vtkObject.h>
#include <vtkSmartPointer.h>
#include <vtkUnsignedCharArray.h>


#ifndef vtkMRMLCodecNewMacro
#define vtkMRMLCodecNewMacro(newClass) \
vtkStandardNewMacro(newClass); \
vtkStreamingVolumeCodec* newClass::CreateCodecInstance() \
{ \
return newClass::New(); \
}
#endif

/// \brief vtk object for representing volume compression codec (normally a video compression codec).
///
/// Any nodes that encapsulates video codec should derive from the vtkStreamingVolumeCodec.
/// This compression codec node is observed by the vtkMRMLStreamingVolumeNode. This codec node
/// generates keyframeMessage and frameMessage from the image data in the vtkMRMLStreamingVolumeNode
/// See this derived node for more detail:
/// https://github.com/openigtlink/SlicerOpenIGTLink/blob/BitStreamNodeRemoval/OpenIGTLinkIF/MRML/vtkIGTLStreamingVolumeCodec.h
/// Two functions in this class needs to be derived from child class:
/// 1. virtual int UncompressedDataFromStream(std::string bitStreamData, bool checkCRC);
/// 2. virtual std::string GetCompressedStreamFromData();
class VTK_MRML_EXPORT vtkStreamingVolumeCodec : public vtkObject
{
public:
  //static vtkStreamingVolumeCodec *New();
  vtkTypeMacro(vtkStreamingVolumeCodec, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  
  struct ContentData
  {
    vtkSmartPointer<vtkImageData> image;
    int frameType;
    bool keyFrameUpdated;
    std::string codecName;
    std::string codecType;
    vtkSmartPointer<vtkUnsignedCharArray> frame; // for saving the compressed data.
    vtkSmartPointer<vtkUnsignedCharArray> keyFrame; // for saving the compressed data.
  };
  
  ///
  /// The codec modified event. This event could be invoke when bit stream is generated or decoded.
  virtual unsigned int GetCodecContentModifiedEvent() const;
  
  ///
  /// Create Codec Instance
  virtual vtkStreamingVolumeCodec* CreateCodecInstance() = 0;

  ///
  /// Get the compression codec type, a compression codec could contain serveral codec.
  /// This function only returns the compresion codec type.
  virtual std::string GetCodecType() const;

  ///
  /// Decode bit stream and update the image pointer in content.
  /// The image pointer normally from the vtkMRMLStreamingVolumeNode
  virtual int UncompressedDataFromStream(vtkSmartPointer<vtkUnsignedCharArray> bitStreamData, bool checkCRC);

  ///
  /// Return the compressed bit stream from the image.
  virtual vtkSmartPointer<vtkUnsignedCharArray> GetCompressedStreamFromData();

  ///
  /// Get the compression codec type, a compression codec could contain serveral codec.
  /// This function returns the codec that is used for encoding and decoding.
  virtual std::string GetContentCodecType();

  ///
  /// Return the Content data
  virtual ContentData GetContent();

  ///
  /// Return the image in the Content data
  virtual vtkSmartPointer<vtkImageData> GetContentImage();

  ///
  /// Return the codec name in the Content data
  virtual std::string GetContentCodecName();

  ///
  /// Set the codec typt in the Content data, will be useful to indicate which codec to use.
  virtual int SetContentCodecType(std::string codecType);

  ///
  /// Set the Content data
  virtual void SetContent(ContentData content);

  ///
  /// Set the image pointer in the Content data
  virtual void SetContentImage(vtkSmartPointer<vtkImageData> image);

  ///
  /// Set the codec name in the Content data
  virtual void SetContentCodecName(std::string name);

protected:
  vtkStreamingVolumeCodec();
  ~vtkStreamingVolumeCodec();
  vtkStreamingVolumeCodec(const vtkStreamingVolumeCodec&);
  void operator=(const vtkStreamingVolumeCodec&);

  enum {
    CodecModifiedEvent         = 118961, //Todo, should have a different id, as it is conflict with openigtlinkIO VideoDeviceModified event
  };

  enum {
    UndefinedFrameType         = -1 // undefined frame type
  };

  ContentData Content;
};

#endif