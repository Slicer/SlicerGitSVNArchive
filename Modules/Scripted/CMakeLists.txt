
include(SlicerMacroBuildScriptedModule)

set(modules
  DataProbe
  DMRIInstall
  LabelStatistics
  PerformanceTests
  SampleData
  ScreenCapture
  SelfTests
  SurfaceToolbox
  VectorToScalarVolume
  )
if(Slicer_BUILD_EXTENSIONMANAGER_SUPPORT)
  list(APPEND modules
    ExtensionWizard
    )
endif()
if(Slicer_USE_NUMPY)
  list(APPEND modules
    Endoscopy
    )
endif()
if(Slicer_BUILD_DICOM_SUPPORT)
  list(APPEND modules
    DICOM
    DICOMLib
    DICOMPlugins
    DICOMPatcher
    )
endif()
if(Slicer_BUILD_SEGMENTATION_SUPPORT)
  list(APPEND modules
    Editor
    EditorLib
    SegmentEditor
    SegmentStatistics
    )
endif()
foreach(module ${modules})
  set(_build_module 1)
  set(${module}_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${module})
  # Is module expicitly disabled ?
  if(Slicer_QTSCRIPTEDMODULES_DISABLED)
    list(FIND Slicer_QTSCRIPTEDMODULES_DISABLED ${module} _module_disabled)
    if(${_module_disabled} GREATER -1)
      set(_build_module 0)
    endif()
  endif()
  # Is module explicitly enabled ?
  if(Slicer_QTSCRIPTEDMODULES_ENABLED)
    list(FIND Slicer_QTSCRIPTEDMODULES_ENABLED ${module} _module_enabled)
    if(NOT ${_module_enabled} GREATER -1)
      set(_build_module 0)
    endif()
  endif()
  if(_build_module)
    add_subdirectory(${module})
  endif()
endforeach()

# External modules have been moved to Modules/Remote/CMakeLists.txt
