
set(proj qRestAPI)

# Set dependency list
set(${proj}_DEPENDENCIES "")

# Include dependent projects if any
ExternalProject_Include_Dependencies(${proj} PROJECT_VAR proj DEPENDS_VAR ${proj}_DEPENDENCIES)

if(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  message(FATAL_ERROR "Enabling ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj} is not supported !")
endif()

# Sanity checks
if(DEFINED qRestAPI_DIR AND NOT EXISTS ${qRestAPI_DIR})
  message(FATAL_ERROR "qRestAPI_DIR variable is defined but corresponds to nonexistent directory")
endif()

if(NOT DEFINED qRestAPI_DIR)

  if(NOT DEFINED git_protocol)
    set(git_protocol "git")
  endif()

  set(ep_cache_args)
  if(Slicer_REQUIRED_QT_VERSION VERSION_LESS "5")
    list(APPEND ep_cache_args
      -DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE}
      -DqRestAPI_QT_VERSION:STRING=4
      )
  else()
    list(APPEND ep_cache_args
      -DQt5_DIR:FILEPATH=${Qt5_DIR}
      -DqRestAPI_QT_VERSION:STRING=5
      )
  endif()

  ExternalProject_Add(${proj}
    ${${proj}_EP_ARGS}
    GIT_REPOSITORY "${git_protocol}://github.com/commontk/qRestAPI.git"
    GIT_TAG "c5e4c2a7d5af7d0bfcec62b4f3c4c85bc67e2b5c"
    SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj}
    BINARY_DIR ${proj}-build
    CMAKE_CACHE_ARGS
      -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
      -DCMAKE_CXX_FLAGS:STRING=${ep_common_cxx_flags}
      -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
      #-DCMAKE_C_FLAGS:STRING=${ep_common_c_flags} # Unused
      -DBUILD_TESTING:BOOL=OFF
      -DBUILD_SHARED_LIBS:BOOL=OFF
      ${ep_cache_args}
    INSTALL_COMMAND ""
    DEPENDS
      ${${proj}_DEPENDENCIES}
    )
  set(qRestAPI_DIR ${CMAKE_BINARY_DIR}/${proj}-build)

else()
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDENCIES})
endif()

mark_as_superbuild(
  VARS qRestAPI_DIR:PATH
  LABELS "FIND_PACKAGE"
  )
