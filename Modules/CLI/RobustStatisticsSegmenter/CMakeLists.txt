#-----------------------------------------------------------------------------
set(MODULE_NAME RobustStatisticsSegmenter)

#-----------------------------------------------------------------------------
SEMMacroBuildCLI(
  NAME ${MODULE_NAME}
  LOGO_HEADER ${Slicer_SOURCE_DIR}/Resources/NAMICLogo.h
  TARGET_LIBRARIES ${ITK_LIBRARIES}
  ADDITIONAL_SRCS
    SFLS.h
    SFLSSegmentor3D.h
    SFLSSegmentor3D.hxx
    SFLSRobustStatSegmentor3DLabelMap.h
    SFLSRobustStatSegmentor3DLabelMap.hxx
  INCLUDE_DIRECTORIES
    ${vtkTeem_INCLUDE_DIRS}
    ${SlicerBaseCLI_SOURCE_DIR} ${SlicerBaseCLI_BINARY_DIR}
  )

#-----------------------------------------------------------------------------
if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()
