
set(proj SlicerDependencies)

# Set dependency list
set(${proj}_DEPENDENCIES "")

#------------------------------------------------------------------------------
# Slicer dependency list
#------------------------------------------------------------------------------

set(ITK_EXTERNAL_NAME ITKv4)

set(VTK_EXTERNAL_NAME VTKv9)

set(${proj}_DEPENDENCIES
  curl
  CTKAppLauncherLib
  teem
  ${VTK_EXTERNAL_NAME}
  ${ITK_EXTERNAL_NAME}
  CTK
  LibArchive
  RapidJSON
  )

set(CURL_ENABLE_SSL ${Slicer_USE_PYTHONQT_WITH_OPENSSL})

if(Slicer_USE_SimpleITK)
  list(APPEND ${proj}_DEPENDENCIES SimpleITK)
endif()

if(Slicer_BUILD_CLI_SUPPORT)
  list(APPEND ${proj}_DEPENDENCIES SlicerExecutionModel)
endif()

if(Slicer_BUILD_EXTENSIONMANAGER_SUPPORT)
  list(APPEND ${proj}_DEPENDENCIES qRestAPI)
endif()

if(Slicer_BUILD_DICOM_SUPPORT)
  list(APPEND ${proj}_DEPENDENCIES DCMTK)
endif()

if(Slicer_BUILD_DICOM_SUPPORT AND Slicer_USE_PYTHONQT_WITH_OPENSSL)
  list(APPEND ${proj}_DEPENDENCIES python-pydicom)
endif()

if(Slicer_USE_PYTHONQT AND Slicer_BUILD_EXTENSIONMANAGER_SUPPORT)
  list(APPEND ${proj}_DEPENDENCIES
    python-chardet
    python-couchdb
    python-GitPython
    python-pip
    )
  if(Slicer_USE_PYTHONQT_WITH_OPENSSL OR Slicer_USE_SYSTEM_python)
    # python-PyGithub requires SSL support in Python
    list(APPEND ${proj}_DEPENDENCIES python-PyGithub)
  else()
    message(STATUS "--------------------------------------------------")
    message(STATUS "Python was built without SSL support; "
                   "github integration will not be available. "
                   "Set Slicer_USE_PYTHONQT_WITH_OPENSSL=ON to enable this feature.")
    message(STATUS "--------------------------------------------------")
  endif()
endif()

if(Slicer_USE_CTKAPPLAUNCHER)
  list(APPEND ${proj}_DEPENDENCIES CTKAPPLAUNCHER)
endif()

if(Slicer_USE_PYTHONQT)
  set(PYTHON_ENABLE_SSL ${Slicer_USE_PYTHONQT_WITH_OPENSSL})
  list(APPEND ${proj}_DEPENDENCIES python)
endif()

if(Slicer_USE_NUMPY)
  list(APPEND ${proj}_DEPENDENCIES NUMPY)
endif()

if(Slicer_USE_PYTHONQT_WITH_TCL AND UNIX)
  list(APPEND ${proj}_DEPENDENCIES incrTcl)
endif()

if(Slicer_USE_TBB)
  list(APPEND ${proj}_DEPENDENCIES tbb)
endif()

# Include dependent projects if any
ExternalProject_Include_Dependencies(${proj} PROJECT_VAR proj DEPENDS_VAR ${proj}_DEPENDENCIES)

# Sanity checks
if(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  message(FATAL_ERROR "Enabling ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj} is not supported !")
endif()
if(DEFINED ${proj}_DIR)
  message(FATAL_ERROR "Setting ${proj}_DIR variable is not supported !")
endif()

ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDENCIES})
