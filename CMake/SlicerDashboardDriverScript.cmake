

#
# Included from a dashboard script, this cmake file will drive the configure and build
# steps of Slicer
#

# The following variable are expected to be define in the top-level script:
set(expected_variables
  ADDITIONAL_CMAKECACHE_OPTION
  CTEST_NOTES_FILES
  CTEST_SITE
  CTEST_DASHBOARD_ROOT
  CTEST_CMAKE_GENERATOR
  WITH_MEMCHECK
  WITH_COVERAGE
  WITH_DOCUMENTATION
  CTEST_BUILD_CONFIGURATION
  CTEST_TEST_TIMEOUT
  CTEST_BUILD_FLAGS
  TEST_TO_EXCLUDE_REGEX
  CTEST_PROJECT_NAME
  CTEST_SOURCE_DIRECTORY
  CTEST_BINARY_DIRECTORY
  CTEST_BUILD_NAME
  SCRIPT_MODE
  CTEST_COVERAGE_COMMAND
  CTEST_MEMORYCHECK_COMMAND
  CTEST_SVN_COMMAND
  CTEST_GIT_COMMAND
  QT_QMAKE_EXECUTABLE
  )

if(WITH_DOCUMENTATION)
  list(APPEND expected_variables DOCUMENTATION_ARCHIVES_OUTPUT_DIRECTORY)
endif()
if(NOT DEFINED CTEST_PARALLEL_LEVEL)
  set(CTEST_PARALLEL_LEVEL 8)
endif()
if(NOT DEFINED WITH_TESTING_EXTENSIONS)
  set(WITH_TESTING_EXTENSIONS OFF)
endif()

if(WITH_PACKAGES AND NOT DEFINED MIDAS_PACKAGE_URL)
  list(APPEND expected_variables MIDAS_PACKAGE_URL)
endif()

if(EXISTS "${CTEST_LOG_FILE}")
  list(APPEND CTEST_NOTES_FILES ${CTEST_LOG_FILE})
endif()

foreach(var ${expected_variables})
  if(NOT DEFINED ${var})
    message(FATAL_ERROR "Variable ${var} should be defined in top-level script !")
  endif()
endforeach()

if(NOT DEFINED CTEST_CONFIGURATION_TYPE AND DEFINED CTEST_BUILD_CONFIGURATION)
  set(CTEST_CONFIGURATION_TYPE ${CTEST_BUILD_CONFIGURATION})
endif()

#-----------------------------------------------------------------------------
# Macro allowing to set a variable to its default value.
# The default value is set with:
#  (1) if set, the value environment variable <var>.
#  (2) if set, the value of local variable variable <var>.
#  (3) if none of the above, the value passed as a parameter.
macro(setOnlyIfNotDefined var defaultvalue)
  if(DEFINED ENV{${var}})
    message(STATUS "Setting '${var}' variable with environment variable value '$ENV{${var}}'")
    set(${var} $ENV{${var}})
  endif()
  if(NOT DEFINED ${var})
    set(${var} "${defaultvalue}")
  endif()
endmacro()

#-----------------------------------------------------------------------------
# The following variable can be used while testing the driver scripts
#-----------------------------------------------------------------------------
setOnlyIfNotDefined(run_ctest_submit TRUE)
setOnlyIfNotDefined(run_ctest_with_disable_clean FALSE)
setOnlyIfNotDefined(run_ctest_with_update TRUE)
setOnlyIfNotDefined(run_ctest_with_configure TRUE)
setOnlyIfNotDefined(run_ctest_with_build TRUE)
setOnlyIfNotDefined(run_ctest_with_test TRUE)
setOnlyIfNotDefined(run_ctest_with_coverage TRUE)
setOnlyIfNotDefined(run_ctest_with_memcheck TRUE)
setOnlyIfNotDefined(run_ctest_with_packages TRUE)
setOnlyIfNotDefined(run_ctest_with_upload TRUE)
setOnlyIfNotDefined(run_ctest_with_notes TRUE)

#-----------------------------------------------------------------------------
if(NOT DEFINED GIT_REPOSITORY)
  if(NOT DEFINED SVN_REPOSITORY)
    set(SVN_REPOSITORY "http://svn.slicer.org/${CTEST_PROJECT_NAME}")
  endif()
  if(NOT DEFINED SVN_BRANCH)
    set(SVN_BRANCH "trunk")
  endif()
  set(repository ${SVN_REPOSITORY}/${SVN_BRANCH})
  set(svn_checkout_option "")
  if(NOT "${SVN_REVISION}" STREQUAL "")
    set(repository "${repository}@${SVN_REVISION}")
    set(run_ctest_with_update FALSE)
  endif()
  message("SVN_BRANCH .............: ${SVN_BRANCH}")
  message("SVN_REVISION ...........: ${SVN_REVISION}")
  message("SVN_REPOSITORY .........: ${SVN_REPOSITORY}")
  message("SVN_URL ................: ${repository}")
else()
  set(repository ${GIT_REPOSITORY})
  set(git_branch_option "")
  if(NOT "${GIT_TAG}" STREQUAL "")
    set(git_branch_option "-b ${GIT_TAG}")
  endif()
  message("GIT_REPOSITORY ......: ${GIT_REPOSITORY}")
  message("GIT_TAG .............: ${GIT_TAG}")
endif()

# Should binary directory be cleaned?
set(empty_binary_directory FALSE)

# Attempt to build and test also if 'ctest_update' returned an error
set(force_build FALSE)

# Ensure SCRIPT_MODE is lowercase
string(TOLOWER ${SCRIPT_MODE} SCRIPT_MODE)

# Set model and track options
set(model "")
if(SCRIPT_MODE STREQUAL "experimental")
  set(empty_binary_directory FALSE)
  set(force_build TRUE)
  set(model Experimental)
elseif(SCRIPT_MODE STREQUAL "continuous")
  set(empty_binary_directory TRUE)
  set(force_build FALSE)
  set(model Continuous)
elseif(SCRIPT_MODE STREQUAL "nightly")
  set(empty_binary_directory TRUE)
  set(force_build TRUE)
  set(model Nightly)
else()
  message(FATAL_ERROR "Unknown script mode: '${SCRIPT_MODE}'. Script mode should be either 'experimental', 'continuous' or 'nightly'")
endif()
set(track ${model})
if(WITH_PACKAGES)
  set(track "${track}-Packages")
endif()
set(track ${CTEST_TRACK_PREFIX}${track}${CTEST_TRACK_SUFFIX})

# For more details, see http://www.kitware.com/blog/home/post/11
set(CTEST_USE_LAUNCHERS 1)
if(NOT "${CTEST_CMAKE_GENERATOR}" MATCHES "Make")
  set(CTEST_USE_LAUNCHERS 0)
endif()
set(ENV{CTEST_USE_LAUNCHERS_DEFAULT} ${CTEST_USE_LAUNCHERS})

if(empty_binary_directory AND NOT run_ctest_with_disable_clean)
  message("Directory ${CTEST_BINARY_DIRECTORY} cleaned !")
  ctest_empty_binary_directory(${CTEST_BINARY_DIRECTORY})
endif()

if(NOT EXISTS "${CTEST_SOURCE_DIRECTORY}")
  if(NOT DEFINED GIT_REPOSITORY)
    set(CTEST_CHECKOUT_COMMAND "${CTEST_SVN_COMMAND} checkout ${repository} ${CTEST_SOURCE_DIRECTORY}")
  else()
    set(CTEST_CHECKOUT_COMMAND "${CTEST_GIT_COMMAND} clone ${git_branch_option} ${repository} ${CTEST_SOURCE_DIRECTORY}")
  endif()
endif()

if(NOT DEFINED GIT_REPOSITORY)
  set(CTEST_UPDATE_COMMAND "${CTEST_SVN_COMMAND}")
else()
  set(CTEST_UPDATE_COMMAND "${CTEST_GIT_COMMAND}")
endif()

set(CTEST_SOURCE_DIRECTORY "${CTEST_SOURCE_DIRECTORY}")

#
# run_ctest macro
#
macro(run_ctest)
  ctest_start(${model} TRACK ${track})

  set(build_in_progress_file ${CTEST_BINARY_DIRECTORY}/Slicer-build/BUILD_IN_PROGRESS)
  file(WRITE ${build_in_progress_file} "Generated by ${CMAKE_CURRENT_LIST_FILE}\n")

  if(run_ctest_with_update)
    ctest_update(SOURCE "${CTEST_SOURCE_DIRECTORY}" RETURN_VALUE FILES_UPDATED)
  endif()

  # force a build if this is the first run and the build dir is empty
  if(NOT EXISTS "${CTEST_BINARY_DIRECTORY}/CMakeCache.txt")
    message("First time build - Initialize CMakeCache.txt")
    set(force_build TRUE)

    if(WITH_TESTING_EXTENSIONS)
      set(ADDITIONAL_CMAKECACHE_OPTION
        "${ADDITIONAL_CMAKECACHE_OPTION} CTEST_MODEL:STRING=${model}")
    endif()

    #-----------------------------------------------------------------------------
    # Write initial cache.
    #-----------------------------------------------------------------------------
    file(WRITE "${CTEST_BINARY_DIRECTORY}/CMakeCache.txt" "
QT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE}
GIT_EXECUTABLE:FILEPATH=${CTEST_GIT_COMMAND}
Subversion_SVN_EXECUTABLE:FILEPATH=${CTEST_SVN_COMMAND}
WITH_COVERAGE:BOOL=${WITH_COVERAGE}
DOCUMENTATION_TARGET_IN_ALL:BOOL=${WITH_DOCUMENTATION}
DOCUMENTATION_ARCHIVES_OUTPUT_DIRECTORY:PATH=${DOCUMENTATION_ARCHIVES_OUTPUT_DIRECTORY}
Slicer_BUILD_TESTING_EXTENSIONS:BOOL=${WITH_TESTING_EXTENSIONS}
Slicer_UPLOAD_EXTENSIONS:BOOL=${WITH_TESTING_EXTENSIONS}
${ADDITIONAL_CMAKECACHE_OPTION}
")
  endif()

  if(FILES_UPDATED GREATER 0 OR force_build)

    set(force_build FALSE)

    #-----------------------------------------------------------------------------
    # Update
    #-----------------------------------------------------------------------------
    if(run_ctest_with_update AND run_ctest_submit)
      ctest_submit(PARTS Update)
    endif()

    #-----------------------------------------------------------------------------
    # Configure
    #-----------------------------------------------------------------------------
    if(run_ctest_with_configure)
      message("----------- [ Configure ${CTEST_PROJECT_NAME} ] -----------")

      #set(label Slicer)

      #set_property(GLOBAL PROPERTY SubProject ${label})
      set_property(GLOBAL PROPERTY Label ${label})

      ctest_configure(BUILD "${CTEST_BINARY_DIRECTORY}")
      ctest_read_custom_files("${CTEST_BINARY_DIRECTORY}")
      if(run_ctest_submit)
        ctest_submit(PARTS Configure)
      endif()
    endif()

    #-----------------------------------------------------------------------------
    # Build top level
    #-----------------------------------------------------------------------------
    set(build_errors)
    if(run_ctest_with_build)
      message("----------- [ Build ${CTEST_PROJECT_NAME} ] -----------")
      ctest_build(BUILD "${CTEST_BINARY_DIRECTORY}" NUMBER_ERRORS build_errors APPEND)
      if(run_ctest_submit)
        ctest_submit(PARTS Build)
      endif()
    endif()

    file(REMOVE ${build_in_progress_file})

    #-----------------------------------------------------------------------------
    # Inner build directory
    #-----------------------------------------------------------------------------
    set(slicer_build_dir "${CTEST_BINARY_DIRECTORY}/Slicer-build")

    #-----------------------------------------------------------------------------
    # Test
    #-----------------------------------------------------------------------------
    if(run_ctest_with_test)
      message("----------- [ Test ${CTEST_PROJECT_NAME} ] -----------")
      ctest_test(
        BUILD "${slicer_build_dir}"
        #INCLUDE_LABEL ${label}
        PARALLEL_LEVEL ${CTEST_PARALLEL_LEVEL}
        EXCLUDE ${TEST_TO_EXCLUDE_REGEX})
      # runs only tests that have a LABELS property matching "${label}"
      if(run_ctest_submit)
        ctest_submit(PARTS Test)
      endif()
    endif()

    #-----------------------------------------------------------------------------
    # Global coverage ...
    #-----------------------------------------------------------------------------
    if(run_ctest_with_coverage)
      # HACK Unfortunately ctest_coverage ignores the BUILD argument, try to force it...
      file(READ ${slicer_build_dir}/CMakeFiles/TargetDirectories.txt slicer_build_coverage_dirs)
      file(APPEND "${CTEST_BINARY_DIRECTORY}/CMakeFiles/TargetDirectories.txt" "${slicer_build_coverage_dirs}")

      if(WITH_COVERAGE AND CTEST_COVERAGE_COMMAND)
        message("----------- [ Global coverage ] -----------")
        ctest_coverage(BUILD "${slicer_build_dir}")
        if(run_ctest_submit)
          ctest_submit(PARTS Coverage)
        endif()
      endif()
    endif()

    #-----------------------------------------------------------------------------
    # Global dynamic analysis ...
    #-----------------------------------------------------------------------------
    if(WITH_MEMCHECK AND CTEST_MEMORYCHECK_COMMAND AND run_ctest_with_memcheck)
        message("----------- [ Global memcheck ] -----------")
        ctest_memcheck(BUILD "${slicer_build_dir}")
        if(run_ctest_submit)
          ctest_submit(PARTS MemCheck)
        endif()
    endif()

    #-----------------------------------------------------------------------------
    # Create packages / installers ...
    #-----------------------------------------------------------------------------
    if(WITH_PACKAGES AND (run_ctest_with_packages OR run_ctest_with_upload))
      message("----------- [ WITH_PACKAGES and UPLOAD ] -----------")

      if(build_errors GREATER "0")
        message("Build Errors Detected: ${build_errors}. Aborting package generation")
      else()

        if(MY_BITNESS EQUAL 32)
          set(dashboard_Architecture "i386")
        else()
          set(dashboard_Architecture "amd64")
        endif()

        if(WIN32)
          set(dashboard_OperatingSystem "win")
        elseif(APPLE)
          set(dashboard_OperatingSystem "macosx")
        elseif(UNIX)
          set(dashboard_OperatingSystem "linux")
        endif()

        #-----------------------------------------------------------------------------
        # Build and upload Slicer packages
        #-----------------------------------------------------------------------------

        # Update CMake module path so that in addition to our custom 'FindGit' module,
        # our custom macros/functions can also be included.
        set(CMAKE_MODULE_PATH ${CTEST_SOURCE_DIRECTORY}/CMake ${CMAKE_MODULE_PATH})

        include(CTestPackage)
        include(MIDASAPIUploadPackage)
        include(MIDASCTestUploadURL)

        set(Subversion_SVN_EXECUTABLE ${CTEST_SVN_COMMAND})
        include(SlicerMacroExtractRepositoryInfo)

        SlicerMacroExtractRepositoryInfo(VAR_PREFIX Slicer SOURCE_DIR ${CTEST_SOURCE_DIRECTORY})

        set(packages)
        if(run_ctest_with_packages)
          message("Packaging ...")
          ctest_package(
            BINARY_DIR ${slicer_build_dir}
            CONFIG ${CTEST_BUILD_CONFIGURATION}
            RETURN_VAR packages)
        else()
          set(packages ${CMAKE_CURRENT_LIST_FILE})
        endif()
        if(run_ctest_with_upload)
          message("Uploading ...")
          foreach(p ${packages})
            get_filename_component(package_name "${p}" NAME)
            set(midas_upload_status "fail")
            if(DEFINED MIDAS_PACKAGE_URL
               AND DEFINED MIDAS_PACKAGE_EMAIL
               AND DEFINED MIDAS_PACKAGE_API_KEY)
              message("Uploading [${package_name}] on [${MIDAS_PACKAGE_URL}]")
              midas_api_upload_package(
                SERVER_URL ${MIDAS_PACKAGE_URL}
                SERVER_EMAIL ${MIDAS_PACKAGE_EMAIL}
                SERVER_APIKEY ${MIDAS_PACKAGE_API_KEY}
                SUBMISSION_TYPE ${SCRIPT_MODE}
                SOURCE_REVISION ${Slicer_WC_REVISION}
                SOURCE_CHECKOUTDATE ${Slicer_WC_LAST_CHANGED_DATE}
                OPERATING_SYSTEM ${dashboard_OperatingSystem}
                ARCHITECTURE ${dashboard_Architecture}
                PACKAGE_FILEPATH ${p}
                PACKAGE_TYPE "installer"
                RESULT_VARNAME midas_upload_status
                )
            endif()
            if(NOT midas_upload_status STREQUAL "ok")
              message("Uploading [${package_name}] on CDash") # On failure, upload the package to CDash instead
              ctest_upload(FILES ${p})
            else()
              message("Uploading URL on CDash")  # On success, upload a link to CDash
              midas_ctest_upload_url(
                API_URL ${MIDAS_PACKAGE_URL}
                FILEPATH ${p}
                )
            endif()
            if(run_ctest_submit)
              ctest_submit(PARTS Upload)
            endif()
          endforeach()
        endif()
      endif()
    endif()

    #-----------------------------------------------------------------------------
    # Note should be at the end
    #-----------------------------------------------------------------------------
    if(run_ctest_with_notes AND run_ctest_submit)
      ctest_submit(PARTS Notes)
    endif()

  else()
    file(REMOVE ${build_in_progress_file})
  endif()
endmacro()

if(SCRIPT_MODE STREQUAL "continuous")
  while(${CTEST_ELAPSED_TIME} LESS 46800) # Lasts 13 hours (Assuming it starts at 9am, it will end around 10pm)
    set(START_TIME ${CTEST_ELAPSED_TIME})
    run_ctest()
    set(interval 300)
    # Loop no faster than once every <interval> seconds
    message("Wait for ${interval} seconds ...")
    ctest_sleep(${START_TIME} ${interval} ${CTEST_ELAPSED_TIME})
  endwhile()
else()
  run_ctest()
endif()

