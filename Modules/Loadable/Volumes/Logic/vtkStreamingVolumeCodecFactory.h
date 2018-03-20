/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkStreamingVolumeCodecFactory.h,v $
  Date:      $Date: 2006/03/19 17:12:29 $
  Version:   $Revision: 1.13 $

=========================================================================auto=*/


#ifndef __vtkStreamingVolumeCodecFactory_h
#define __vtkStreamingVolumeCodecFactory_h

// VTK includes
#include <vtkObject.h>
#include <vtkSmartPointer.h>

// STD MAP includes
#include <map>

#include "vtkSlicerVolumesModuleLogicExport.h"
#include "vtkStreamingVolumeCodec.h"

/// \ingroup Volumes
/// \brief Class that can create compresion device for streaming volume instances.
///
/// This singleton class is a repository of all compression codecs for compressing volume .
/// Singleton pattern adopted from vtkEventBroker class.
class VTK_SLICER_VOLUMES_MODULE_LOGIC_EXPORT vtkStreamingVolumeCodecFactory : public vtkObject
{
public:
  
  typedef vtkStreamingVolumeCodec*  (*PointerToCodecBaseNew)();
  
  typedef std::map<std::string, PointerToCodecBaseNew> CodecListType;

  vtkTypeMacro(vtkStreamingVolumeCodecFactory, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /// Add a compression codec and pointer to new function
  /// Usage: RegisterConverterRule( vtkStreamingVolumeCodec::GetDeviceType(), (PointerToMessageBaseNew)&vtkStreamingVolumeCodec::New );
  /// \param codecTypeName The name of the message type
  /// \param newCodecPointer Function pointer to the codec type new function (e.g. (PointerToCodecBaseNew)&vtkStreamingVolumeCodec::New )
  int RegisterStreamingVolumeCodec(const std::string& codecTypeName, vtkStreamingVolumeCodecFactory::PointerToCodecBaseNew codecNewPointer);

  /// Remove a codec from the factory.
  /// This does not affect codecs that have already been created.
  int UnregisterStreamingVolumeCodec(const std::string& codecTypeName);
  
  /// Get pointer to codec new function, or NULL if the codec type not registered
  /// Usage: vtkSmartPointer<vtkStreamingVolumeCodec> codec = GetVolumeCodecNewPointerByType("igtlioVideoDevice")();
  /// Returns NULL if codec type is not found
  vtkStreamingVolumeCodecFactory::PointerToCodecBaseNew GetVolumeCodecNewPointerByType(const std::string& codecTypeName) const;

  /// Get all registered Codecs
  const CodecListType& GetStreamingVolumeCodecs();

public:
  /// Return the singleton instance with no reference counting.
  static vtkStreamingVolumeCodecFactory* GetInstance();

  /// This is a singleton pattern New.  There will only be ONE
  /// reference to a vtkStreamingVolumeCodecFactory object per process.  Clients that
  /// call this must call Delete on the object so that the reference
  /// counting will work. The single instance will be unreferenced when
  /// the program exits.
  static vtkStreamingVolumeCodecFactory* New();

protected:
  vtkStreamingVolumeCodecFactory();
  ~vtkStreamingVolumeCodecFactory();
  vtkStreamingVolumeCodecFactory(const vtkStreamingVolumeCodecFactory&);
  void operator=(const vtkStreamingVolumeCodecFactory&);
  
  friend class vtkStreamingVolumeCodecFactoryInitialize;
  typedef vtkStreamingVolumeCodecFactory Self;
  
  // Singleton management functions.
  static void classInitialize();
  static void classFinalize();

  /// Registered converter rules
  /*! Map codec types and the New() static methods of vtkStreamingVolumeCodec classes */
  CodecListType CodecList;
};


/// Utility class to make sure qSlicerModuleManager is initialized before it is used.
class VTK_SLICER_VOLUMES_MODULE_LOGIC_EXPORT vtkStreamingVolumeCodecFactoryInitialize
{
public:
  typedef vtkStreamingVolumeCodecFactoryInitialize Self;

  vtkStreamingVolumeCodecFactoryInitialize();
  ~vtkStreamingVolumeCodecFactoryInitialize();
private:
  static unsigned int Count;
};

/// This instance will show up in any translation unit that uses
/// vtkStreamingVolumeCodecFactory.  It will make sure vtkStreamingVolumeCodecFactory is initialized
/// before it is used.
static vtkStreamingVolumeCodecFactoryInitialize vtkStreamingVolumeCodecFactoryInitializer;

#endif
