project(DicomToNrrdConverter)

cmake_minimum_required(VERSION 2.8.0)

set(RUN_EXTENDED_LOCAL_TESTING OFF CACHE BOOL "Run extended tests?")

if(RUN_EXTENDED_LOCAL_TESTING)
  add_subdirectory(ExtendedTesting)
endif()

# find_library(DCMTKDATALIB dcmdata PATHS ${Slicer_BINARY_DIR}/CTK-build/CMakeExternals/Install/lib)
# if("XX${DCMTKDATALIB}" STREQUAL "XX")
#   message(FATAL_ERROR "Can't find dicom libraries")
# endif()
# get_filename_component(DCMTK_LIB_DIR ${DCMTKDATALIB} PATH)
# link_directories(${DCMTK_LIB_DIR})
if(Slicer_SOURCE_DIR)
  set(DCMTK_DIR ${Slicer_BINARY_DIR}/../CTK-build/CMakeExternals/Install)
endif()

find_package(DCMTK REQUIRED)
if(NOT DCMTK_FOUND)
message(FATAL_ERROR "Can't build without DCMTK, set DCMTK_DIR and rerun CMake")
endif(NOT DCMTK_FOUND)

set(CLP DicomToNrrdConverter)
if(Slicer_SOURCE_DIR)
  slicerMacroBuildCLI(
    NAME ${CLP}
    LOGO_HEADER ${Slicer_SOURCE_DIR}/Resources/NAMICLogo.h
    TARGET_LIBRARIES ${ITK_LIBRARIES} ${DCMTK_LIBRARIES}
    )

  if(BUILD_TESTING)
    ## There's limited testing from within Slicer.  There's extensive testing when run
    ## at UIowa but the testing data isn't publicly accessible yet.  Other packages that
    ## use this tool will not run either set of tests.
    add_subdirectory(Testing)
  endif()

else()
  enable_testing()

  find_package(SlicerExecutionModel REQUIRED GenerateCLP)
  include(${GenerateCLP_USE_FILE})

  find_package(ITK REQUIRED)

  if(NOT ITK_USE_REVIEW)
    message(FATAL_ERROR "ITK version was not built with ITK_USE_REVIEW true")
  endif()

  if(NOT ITK_USE_REVIEW_STATISTICS)
    message(FATAL_ERROR "ITK version was not built with ITK_USE_REVIEW_STATISTICS true")
  endif()

  if(NOT ITK_USE_OPTIMIZED_REGISTRATION_METHODS)
    message(FATAL_ERROR "ITK version was not built with ITK_USE_OPTIMIZED_REGISTRATION_METHODS true")
  endif()

  generateclp(${CLP}_SOURCE ${CLP}.xml)

  add_executable(${CLP} ${CLP}.cxx)
  target_link_libraries(${CLP} ${ITK_LIBRARIES} ${DCMTK_LIBRARIES})
  set_target_properties(${CLP} PROPERTIES LABELS ${CLP})
endif()
