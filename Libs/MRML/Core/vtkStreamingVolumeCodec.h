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


/// \brief vtk object for representing volume compression codec (normally a video compression codec).
///
/// Any nodes that encapsulates video codec should derive from the vtkStreamingVolumeCodec.
/// This compression device node is observed by the vtkMRMLStreamingVolumeNode. This device node
/// generates keyframeMessage and frameMessage from the image data in the vtkMRMLStreamingVolumeNode
/// See this derived node for more detail:
/// https://github.com/openigtlink/SlicerOpenIGTLink/blob/BitStreamNodeRemoval/OpenIGTLinkIF/MRML/vtkIGTLStreamingVolumeCodec.h
/// Two functions in this class needs to be derived from child class:
/// 1. virtual int UncompressedDataFromStream(std::string bitStreamData, bool checkCRC);
/// 2. virtual std::string GetCompressedStreamFromData();
class VTK_MRML_EXPORT vtkStreamingVolumeCodec : public vtkObject
{
public:
  static vtkStreamingVolumeCodec *New();
  vtkTypeMacro(vtkStreamingVolumeCodec, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  
  struct ContentData
  {
    vtkSmartPointer<vtkImageData> image;
    int frameType;
    bool keyFrameUpdated;
    std::string deviceName;
    std::string codecType;
    vtkSmartPointer<vtkUnsignedCharArray> frame; // for saving the compressed data.
    vtkSmartPointer<vtkUnsignedCharArray> keyFrame; // for saving the compressed data.
  };
  
  ///
  /// The device modified event. This event could be invoke when bit stream is generated or decoded.
  virtual unsigned int GetDeviceContentModifiedEvent() const;

  ///
  /// Get the compression device type, a compression device could contain serveral codec.
  /// This function only returns the compresion device type.
  virtual std::string GetDeviceType() const;

  ///
  /// Decode bit stream and update the image pointer in content.
  /// The image pointer normally from the vtkMRMLStreamingVolumeNode
  virtual int UncompressedDataFromStream(vtkSmartPointer<vtkUnsignedCharArray> bitStreamData, bool checkCRC);

  ///
  /// Return the compressed bit stream from the image.
  virtual vtkSmartPointer<vtkUnsignedCharArray> GetCompressedStreamFromData();

  ///
  /// Get the compression codec type, a compression device could contain serveral codec.
  /// This function returns the codec that is used for encoding and decoding.
  virtual std::string GetCurrentCodecType();

  ///
  /// Return the Content data
  virtual ContentData GetContent();

  ///
  /// Return the image in the Content data
  virtual vtkSmartPointer<vtkImageData> GetContentImage();

  ///
  /// Return the device name in the Content data
  virtual std::string GetContentDeviceName();

  ///
  /// Set the codec typt in the Content data, will be useful to indicate which codec to use.
  virtual int SetCurrentCodecType(std::string codecType);

  ///
  /// Set the Content data
  virtual void SetContent(ContentData content);

  ///
  /// Set the image pointer in the Content data
  virtual void SetContentImage(vtkSmartPointer<vtkImageData> image);

  ///
  /// Set the device name in the Content data
  virtual void SetContentDeviceName(std::string name);

protected:
  vtkStreamingVolumeCodec();
  ~vtkStreamingVolumeCodec();
  vtkStreamingVolumeCodec(const vtkStreamingVolumeCodec&);
  void operator=(const vtkStreamingVolumeCodec&);

  enum {
    DeviceModifiedEvent         = 118961, //Todo, should have a different id, as it is conflict with openigtlinkIO VideoDeviceModified event
  };

  enum {
    UndefinedFrameType         = -1 // undefined frame type
  };

  ContentData Content;
};

#endif