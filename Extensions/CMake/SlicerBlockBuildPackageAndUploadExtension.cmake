#-----------------------------------------------------------------------------
# Extract dashboard option passed from command line
#-----------------------------------------------------------------------------
# Note: The syntax to pass option from the command line while invoking ctest is
#       the following: ctest -S /path/to/script.cmake,OPTNAME1##OPTVALUE1^^OPTNAME2##OPTVALUE2
#
# Example:
#       ctest -S /path/to/script.cmake,SCRIPT_MODE##continuous^^GIT_TAG##next
#
if(NOT CTEST_SCRIPT_ARG STREQUAL "")
  cmake_policy(PUSH)
  cmake_policy(SET CMP0007 OLD)
  string(REPLACE "^^" "\\;" CTEST_SCRIPT_ARG_AS_LIST "${CTEST_SCRIPT_ARG}")
  set(CTEST_SCRIPT_ARG_AS_LIST ${CTEST_SCRIPT_ARG_AS_LIST})
  foreach(argn_argv ${CTEST_SCRIPT_ARG_AS_LIST})
    string(REPLACE "##" "\\;" argn_argv_list ${argn_argv})
    set(argn_argv_list ${argn_argv_list})
    list(LENGTH argn_argv_list argn_argv_list_length)
    list(GET argn_argv_list 0 argn)
    if(argn_argv_list_length GREATER 1)
      list(REMOVE_AT argn_argv_list 0) # Take first item
      set(argv) # Convert from list to string separated by '='
      foreach(str_item ${argn_argv_list})
        set(argv "${argv}=${str_item}")
      endforeach()
      string(SUBSTRING ${argv} 1 -1 argv) # Remove first unwanted '='
      string(REPLACE "/-/" "//" argv ${argv}) # See http://www.cmake.org/Bug/view.php?id=12953
      string(REPLACE "-AMP-" "&" argv ${argv})
      string(REPLACE "-WHT-" "?" argv ${argv})
      string(REPLACE "-LPAR-" "(" argv ${argv})
      string(REPLACE "-RPAR-" ")" argv ${argv})
      string(REPLACE "-EQUAL-" "=" argv ${argv})
      string(REPLACE "-DOTDOT-" ".." argv ${argv})
      set(${argn} ${argv})
    endif()
  endforeach()
  cmake_policy(POP)
endif()

#-----------------------------------------------------------------------------
# Macro allowing to set a variable to its default value only if not already defined
macro(setIfNotDefined var defaultvalue)
  if(NOT DEFINED ${var})
    set(${var} "${defaultvalue}")
  endif()
endmacro()

#-----------------------------------------------------------------------------
# Sanity checks
set(expected_defined_vars GIT_EXECUTABLE Subversion_SVN_EXECUTABLE EXTENSION_NAME EXTENSION_SOURCE_DIR EXTENSION_SUPERBUILD_BINARY_DIR EXTENSION_BUILD_SUBDIRECTORY EXTENSION_ENABLED CTEST_CMAKE_GENERATOR CTEST_BUILD_CONFIGURATION Slicer_CMAKE_DIR Slicer_DIR Slicer_WC_REVISION EXTENSION_BUILD_OPTIONS_STRING RUN_CTEST_CONFIGURE RUN_CTEST_BUILD RUN_CTEST_TEST RUN_CTEST_PACKAGES RUN_CTEST_SUBMIT RUN_CTEST_UPLOAD BUILD_TESTING Slicer_EXTENSIONS_TRACK_QUALIFIER)
if(RUN_CTEST_UPLOAD)
  list(APPEND expected_defined_vars
    MIDAS_PACKAGE_URL MIDAS_PACKAGE_EMAIL MIDAS_PACKAGE_API_KEY
    EXTENSION_ARCHITECTURE EXTENSION_BITNESS EXTENSION_OPERATING_SYSTEM
    )
endif()
foreach(var ${expected_defined_vars})
  if(NOT DEFINED ${var})
    message(FATAL_ERROR "Variable ${var} is not defined !")
  endif()
endforeach()

#-----------------------------------------------------------------------------
set(CMAKE_MODULE_PATH
  ${Slicer_CMAKE_DIR}
  ${Slicer_CMAKE_DIR}/../Extensions/CMake
  ${CMAKE_MODULE_PATH}
  )

include(CMakeParseArguments)
include(CTestPackage)
include(MIDASAPIUploadExtension)
include(MIDASCTestUploadURL)
include(UseSlicerMacros) # for slicer_setting_variable_message

#-----------------------------------------------------------------------------
set(optional_vars EXTENSION_CATEGORY EXTENSION_HOMEPAGE EXTENSION_ICONURL EXTENSION_CONTRIBUTORS EXTENSION_DESCRIPTION EXTENSION_SCREENSHOTURLS EXTENSION_STATUS)
foreach(var ${optional_vars})
  slicer_setting_variable_message(${var})
endforeach()

#-----------------------------------------------------------------------------
# Set site name and force to lower case
site_name(CTEST_SITE)
string(TOLOWER "${CTEST_SITE}" ctest_site_lowercase)
set(CTEST_SITE ${ctest_site_lowercase} CACHE STRING "Name of the computer/site where compile is being run" FORCE)

# Get working copy information
include(SlicerMacroExtractRepositoryInfo)
SlicerMacroExtractRepositoryInfo(VAR_PREFIX EXTENSION SOURCE_DIR ${EXTENSION_SOURCE_DIR})

# Set build name
set(CTEST_BUILD_NAME "${Slicer_WC_REVISION}-${EXTENSION_NAME}-${EXTENSION_WC_TYPE}${EXTENSION_WC_REVISION}-${EXTENSION_COMPILER}-${EXTENSION_BUILD_OPTIONS_STRING}-${CTEST_BUILD_CONFIGURATION}")

setIfNotDefined(CTEST_PARALLEL_LEVEL 8)
setIfNotDefined(CTEST_MODEL "Experimental")

set(label ${EXTENSION_NAME})
set_property(GLOBAL PROPERTY SubProject ${label})
set_property(GLOBAL PROPERTY Label ${label})

# If no CTestConfig.cmake file is found in ${ctestconfig_dest_dir},
# one will be generated.
set(ctestconfig_dest_dir ${EXTENSION_SUPERBUILD_BINARY_DIR}/${EXTENSION_BUILD_SUBDIRECTORY})
if(${CMAKE_VERSION} VERSION_LESS "2.8.7")
  set(ctestconfig_dest_dir ${EXTENSION_SOURCE_DIR})
endif()
if(NOT EXISTS ${ctestconfig_dest_dir}/CTestConfig.cmake)
  message(STATUS "CTestConfig.cmake has been written to: ${ctestconfig_dest_dir}")
  file(WRITE ${ctestconfig_dest_dir}/CTestConfig.cmake
"set(CTEST_PROJECT_NAME \"${EXTENSION_NAME}\")
set(CTEST_NIGHTLY_START_TIME \"3:00:00 UTC\")

set(CTEST_DROP_METHOD \"http\")
set(CTEST_DROP_SITE \"slicer.cdash.org\")
set(CTEST_DROP_LOCATION \"/submit.php?project=Slicer4\")
set(CTEST_DROP_SITE_CDASH TRUE)")
endif()

set(track_qualifier_cleaned "${Slicer_EXTENSIONS_TRACK_QUALIFIER}-")
# Track associated with 'master' should default to either 'Continuous', 'Nightly' or 'Experimental'
if(track_qualifier_cleaned STREQUAL "master-")
  set(track_qualifier_cleaned "")
endif()
set(track "Extensions-${track_qualifier_cleaned}${CTEST_MODEL}")
ctest_start(${CTEST_MODEL} TRACK ${track} ${EXTENSION_SOURCE_DIR} ${EXTENSION_SUPERBUILD_BINARY_DIR})
ctest_read_custom_files(${EXTENSION_SUPERBUILD_BINARY_DIR} ${EXTENSION_SUPERBUILD_BINARY_DIR}/${EXTENSION_BUILD_SUBDIRECTORY})

set(cmakecache_content
"#Generated by SlicerBlockBuildPackageAndUploadExtension.cmake
${EXTENSION_NAME}_BUILD_SLICER_EXTENSION:BOOL=ON
CMAKE_BUILD_TYPE:STRING=${CTEST_BUILD_CONFIGURATION}
CMAKE_GENERATOR:STRING=${CTEST_CMAKE_GENERATOR}
CMAKE_PROJECT_NAME:STRING=${EXTENSION_NAME}
CMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM}
CMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
CMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
GIT_EXECUTABLE:FILEPATH=${GIT_EXECUTABLE}
Subversion_SVN_EXECUTABLE:FILEPATH=${Subversion_SVN_EXECUTABLE}
Slicer_DIR:PATH=${Slicer_DIR}
Slicer_EXTENSIONS_TRACK_QUALIFIER:STRING=${Slicer_EXTENSIONS_TRACK_QUALIFIER}
MIDAS_PACKAGE_URL:STRING=${MIDAS_PACKAGE_URL}
MIDAS_PACKAGE_EMAIL:STRING=${MIDAS_PACKAGE_EMAIL}
MIDAS_PACKAGE_API_KEY:STRING=${MIDAS_PACKAGE_API_KEY}
EXTENSION_DEPENDS:STRING=${EXTENSION_DEPENDS}
")

foreach(dep ${EXTENSION_DEPENDS})
  set(cmakecache_content "${cmakecache_content}\n${dep}_DIR:PATH=${CMAKE_CURRENT_BINARY_DIR}/../${dep}-build")
endforeach()

#-----------------------------------------------------------------------------
# Write CMakeCache.txt only if required
set(cmakecache_current "")
if(EXISTS ${EXTENSION_SUPERBUILD_BINARY_DIR}/CMakeCache.txt)
  file(READ ${EXTENSION_SUPERBUILD_BINARY_DIR}/CMakeCache.txt cmakecache_current)
endif()
if(NOT ${cmakecache_content} STREQUAL "${cmakecache_current}")
  file(WRITE ${EXTENSION_SUPERBUILD_BINARY_DIR}/CMakeCache.txt ${cmakecache_content})
endif()

# Explicitly set CTEST_BINARY_DIRECTORY so that ctest_submit find
# the xml part files in <EXTENSION_SUPERBUILD_BINARY_DIR>/Testing
set(CTEST_BINARY_DIRECTORY ${EXTENSION_SUPERBUILD_BINARY_DIR})

#-----------------------------------------------------------------------------
# Configure extension
if(RUN_CTEST_CONFIGURE)
  #message("----------- [ Configuring extension ${EXTENSION_NAME} ] -----------")
  ctest_configure(
    BUILD ${EXTENSION_SUPERBUILD_BINARY_DIR}
    SOURCE ${EXTENSION_SOURCE_DIR}
    )
  if(RUN_CTEST_SUBMIT)
    ctest_submit(PARTS Configure)
  endif()
endif()

#-----------------------------------------------------------------------------
# Build extension
set(build_errors)
if(RUN_CTEST_BUILD)
  #message("----------- [ Building extension ${EXTENSION_NAME} ] -----------")
  ctest_build(BUILD ${EXTENSION_SUPERBUILD_BINARY_DIR} NUMBER_ERRORS build_errors APPEND)
  if(RUN_CTEST_SUBMIT)
    ctest_submit(PARTS Build)
  endif()
endif()

#-----------------------------------------------------------------------------
# Test extension
if(BUILD_TESTING AND RUN_CTEST_TEST)
  #message("----------- [ Testing extension ${EXTENSION_NAME} ] -----------")
  # Check if there are tests to run
  execute_process(COMMAND ${CMAKE_CTEST_COMMAND} -C ${CTEST_BUILD_CONFIGURATION} -N
    WORKING_DIRECTORY ${EXTENSION_SUPERBUILD_BINARY_DIR}/${EXTENSION_BUILD_SUBDIRECTORY}
    OUTPUT_VARIABLE output
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  string(REGEX REPLACE ".*Total Tests: ([0-9]+)" "\\1" test_count "${output}")
  if("${test_count}" GREATER 0)
    ctest_test(
        BUILD ${EXTENSION_SUPERBUILD_BINARY_DIR}/${EXTENSION_BUILD_SUBDIRECTORY}
        PARALLEL_LEVEL ${CTEST_PARALLEL_LEVEL})
    if(RUN_CTEST_SUBMIT)
      ctest_submit(PARTS Test)
    endif()
  endif()
endif()

#-----------------------------------------------------------------------------
# Package extension
if(build_errors GREATER "0")
  message(WARNING "Skip extension packaging: ${build_errors} build error(s) occured !")
else()
  #message("----------- [ Packaging extension ${EXTENSION_NAME} ] -----------")
  message("Packaging extension ${EXTENSION_NAME} ...")
  set(extension_packages)
  if(RUN_CTEST_PACKAGES)
    ctest_package(
      BINARY_DIR ${EXTENSION_SUPERBUILD_BINARY_DIR}/${EXTENSION_BUILD_SUBDIRECTORY}
      CONFIG ${CTEST_BUILD_CONFIGURATION}
      RETURN_VAR extension_packages)
  else()
    set(extension_packages "${CPACK_PACKAGE_FILE_NAME}.tar.gz")
  endif()

  if(RUN_CTEST_UPLOAD AND COMMAND ctest_upload)
    message("Uploading extension ${EXTENSION_NAME} ...")

    foreach(p ${extension_packages})
      get_filename_component(package_name "${p}" NAME)
      message("Uploading [${package_name}] on [${MIDAS_PACKAGE_URL}]")
      midas_api_upload_extension(
        SERVER_URL ${MIDAS_PACKAGE_URL}
        SERVER_EMAIL ${MIDAS_PACKAGE_EMAIL}
        SERVER_APIKEY ${MIDAS_PACKAGE_API_KEY}
        TMP_DIR ${EXTENSION_SUPERBUILD_BINARY_DIR}/${EXTENSION_BUILD_SUBDIRECTORY}
        SUBMISSION_TYPE ${CTEST_MODEL}
        SLICER_REVISION ${Slicer_WC_REVISION}
        EXTENSION_NAME ${EXTENSION_NAME}
        EXTENSION_CATEGORY ${EXTENSION_CATEGORY}
        EXTENSION_ICONURL ${EXTENSION_ICONURL}
        EXTENSION_CONTRIBUTORS ${EXTENSION_CONTRIBUTORS}
        EXTENSION_DESCRIPTION ${EXTENSION_DESCRIPTION}
        EXTENSION_HOMEPAGE ${EXTENSION_HOMEPAGE}
        EXTENSION_SCREENSHOTURLS ${EXTENSION_SCREENSHOTURLS}
        EXTENSION_REPOSITORY_TYPE ${EXTENSION_WC_TYPE}
        EXTENSION_REPOSITORY_URL ${EXTENSION_WC_URL}
        EXTENSION_SOURCE_REVISION ${EXTENSION_WC_REVISION}
        EXTENSION_ENABLED ${EXTENSION_ENABLED}
        OPERATING_SYSTEM ${EXTENSION_OPERATING_SYSTEM}
        ARCHITECTURE ${EXTENSION_ARCHITECTURE}
        PACKAGE_FILEPATH ${p}
        PACKAGE_TYPE "archive"
        RELEASE ${release}
        RESULT_VARNAME slicer_midas_upload_status
        )
      if(NOT slicer_midas_upload_status STREQUAL "ok")
        message("Uploading [${package_name}] on CDash") # on failure, upload the package to CDash instead
        ctest_upload(FILES ${p})
      else()
        message("Uploading URL on CDash")  # On success, upload a link to CDash
        midas_ctest_upload_url(
          API_URL ${MIDAS_PACKAGE_URL}
          FILEPATH ${p}
          )
      endif()
      if(RUN_CTEST_SUBMIT)
        ctest_submit(PARTS Upload)
      endif()
    endforeach()
  endif()
endif()
