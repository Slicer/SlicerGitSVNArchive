## TODO:  KENT:  Every one of the options listed in should be configurable as build time of wether to include them or not.
##       This one file should have if(BUILD_BRAINSFIT) around the brainsfit to deterimine if it gets built or not.
##       It is my hope that I can include this one external file in Slicer3, and ${CMAKE_PROJECT_NAME}, and the configure which tools are build with options.

set(GenerateCLP_DIR ${Slicer_BINARY_DIR}/Slicer-build/Libs/SlicerExecutionModel/GenerateCLP CACHE STRING "Where to find GenerateCLP")

set(MIDAS_REST_URL
  "http://midas.kitware.com/api/rest"
  CACHE STRING "The MIDAS server where testing data resides")

macro(BuildExtPackage PackageName PackageRepo REVISIONCODE PACKAGE_DEPENDANCIES)
  if("${PackageRepo}" STREQUAL "")
    set(SRCCMDS
      GIT_REPOSITORY ${PackageRepo}
      )
  else()
      #GIT_REPOSITORY ${PackageRepo}
      #GIT_USERNAME "slicerbot"
      #GIT_PASSWORD "slicer"
    set(SRCCMDS
      GIT_REPOSITORY ${PackageRepo}
      )
  endif()
  ExternalProject_Add(${PackageName}
    ${SRCCMDS}
    SOURCE_DIR ${Slicer_BINARY_DIR}/${PackageName}
    BINARY_DIR ${PackageName}-build
    CMAKE_GENERATOR ${gen}
    DEPENDS ${PACKAGE_DEPENDANCIES}
    CMAKE_ARGS
    ${ep_common_flags}
    -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}
    -DGenerateCLP_DIR:PATH=${GenerateCLP_DIR}
    -DMIDAS_REST_URL:STRING=${MIDAS_REST_URL}
    -DITK_DIR:PATH=${ITK_DIR}
    -DVTK_DIR:PATH=${VTK_DIR}
    -DOpenCV_DIR:PATH=${OpenCV_DIR}
    -DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE}
    -DHDF5_DIR:PATH=${HDF5_DIR}
    -Dhdf5_DIR:PATH=${hdf5_DIR}
    ### HACK:  Need to evaluate these command line arguments
    -DBRAINSCommonLib_DIR:PATH=${BRAINSCommonLib_DIR}
    -DBRAINS_BUILD:BOOL=${BRAINS_BUILD}
    -DBRAINS_CMAKE_HELPER_DIR:PATH=${BRAINS_CMAKE_HELPER_DIR}
    -DBRAINSLogo:PATH=${BRAINSLogo}
    -D${CMAKE_PROJECT_NAME}_USE_QT:BOOL=${${CMAKE_PROJECT_NAME}_USE_QT}
    -D${CMAKE_PROJECT_NAME}_USE_ITK4:BOOL=${${CMAKE_PROJECT_NAME}_USE_ITK4}
    #INSTALL_COMMAND ""
    INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}
    )

  ExternalProject_Add_Step(${PackageName} forcebuild
    COMMAND ${CMAKE_COMMAND} -E remove
    ${CMAKE_CURRENT_BUILD_DIR}/${PackageName}-prefix/src/${PackageName}-stamp/${PackageName}-build
    DEPENDEES configure
    DEPENDERS build
    ALWAYS 1
    )

  set(${PackageName}_DEPEND "${proj}")
  set(${PackageName}_DIR ${Slicer_BINARY_DIR}/lib/${PackageName})
  set(${PackageName}_SOURCE_DIR ${PROJECT_EXTERNAL_SHARED_SOURCE_TREE}/${PackageName} )
  message(STATUS "${CMAKE_PROJECT_NAME}")
  message(STATUS "${PackageName}_DIR = ${Slicer_BINARY_DIR}/lib/${PackageName})
  message(STATUS "${PackageName}_DIR = ${${PackageName}_DIR}})
endmacro(BuildExtPackage)

BuildExtPackage(BRAINSCommonLib "git://github.com/BRAINSia/BRAINSCommonLib.git" "master" "")

### HACK:  Need to change name of BuildScripts to BRAINSBuildScripts
### HACK:  Need to remove BRAINS_CMAKE_HELPER_DIR in favor of BRAINSBuildScripts
set(BRAINS_CMAKE_HELPER_DIR ${BuildScripts_SOURCE_DIR})

#-----------------------------------------------------------------------------
# BRAINSConstellationDetector
#-----------------------------------------------------------------------------
BuildExtPackage(BRAINSFit
  git://github.com/BRAINSia/BRAINSFit.git  "master" "BRAINSCommonLib" )

## Dummy package just to force the build of everything else
add_custom_target(BRAINSTools DEPENDS BRAINSCommonLib BRAINSFit)

if(0)
#-----------------------------------------------------------------------------
# BRAINSConstellationDetector
#-----------------------------------------------------------------------------
BuildExtPackage(BRAINSConstellationDetector
  https://www.nitrc.org/svn/brainscdetector/trunk  "{20110620}" "BRAINSCommonLib" )

BuildExtPackage(BRAINSMush
  https://www.nitrc.org/svn/brainsmush/trunk  "{20110620}" "BRAINSCommonLib" )
BuildExtPackage(BRAINSDemonWarp
  https://www.nitrc.org/svn/brainsdemonwarp/trunk  "{20110620}" "BRAINSCommonLib" )
BuildExtPackage(BRAINSROIAuto
  https://www.nitrc.org/svn/brainsroiauto/trunk  "{20110620}" "BRAINSCommonLib" )
BuildExtPackage(BRAINSCut
  https://www.nitrc.org/svn/brainscut/trunk  "{20110620}" "BRAINSCommonLib;${OpenCV_DEPEND}" )
BuildExtPackage(GTRACT
  https://www.nitrc.org/svn/vmagnotta/GTRACT  "{20110620}" "BRAINSCommonLib" )

BuildExtPackage(BRAINSMultiModeSegment
  https://www.nitrc.org/svn/brains/BRAINS/trunk/BRAINSTools/BRAINSMultiModeSegment  "{20110620}" "BRAINSCommonLib" )
BuildExtPackage(BRAINSResample
  https://www.nitrc.org/svn/brains/BRAINS/trunk/BRAINSTools/BRAINSResample  "{20110620}" "BRAINSCommonLib" )
BuildExtPackage(BRAINSInitializedControlPoints
  https://www.nitrc.org/svn/brains/BRAINS/trunk/BRAINSTools/BRAINSInitializedControlPoints  "{20110620}" "BRAINSCommonLib" )

if("${ITK_VERSION_MAJOR}" EQUAL "3")
  BuildExtPackage(DicomToNrrdConverter
    http://svn.slicer.org/Slicer3/trunk/Applications/CLI/DicomToNrrdConverter  "{20110620}" "BRAINSCommonLib" )
  #
  # DicomSignature won't work with ITK4
  BuildExtPackage(DicomSignature
    https://www.nitrc.org/svn/brains/BRAINS/trunk/BRAINSTools/DicomSignature  "{20110620}" "BRAINSCommonLib" )
endif("${ITK_VERSION_MAJOR}" EQUAL "3")

## HACK if(${${CMAKE_PROJECT_NAME}_USE_QT})
## HACK   BuildExtPackage(BRAINSImageEval
## HACK     http://www.nitrc.org/svn/brainsimageeval  "{20110620}" "BRAINSCommonLib" )
## HACK endif(${${CMAKE_PROJECT_NAME}_USE_QT})

#-----------------------------------------------------------------------------
# ReferenceAtlas
#-----------------------------------------------------------------------------
# Define the atlas subdirectory in one place
set(${CMAKE_PROJECT_NAME}_RUNTIME_DIR ${CMAKE_CURRENT_BINARY_DIR}/src/bin)

include(External_ReferenceAtlas)
list(APPEND ${CMAKE_PROJECT_NAME}_DEPENDENCIES ${ReferenceAtlas_DEPEND})

#-----------------------------------------------------------------------------
# BRAINSABC
#-----------------------------------------------------------------------------
BuildExtPackage(BRAINSABC
  https://www.nitrc.org/svn/brains/BRAINS/trunk/BRAINSTools/BRAINSABC  "{20110620}" "BRAINSCommonLib;${ReferenceAtlas_DEPEND}" )

endif() ## Comment out things that are not yet ready


