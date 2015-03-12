
set(proj OpenBLAS)

set(${proj}_DEPENDENCIES "")
ExternalProject_Include_Dependencies(${proj} PROJECT_VAR proj DEPENDS_VAR ${proj}_DEPENDENCIES)

if(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  message(FATAL_ERROR "Enabling ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj} is not supported")
endif()

if(DEFINED ${proj}_DIR AND NOT EXISTS ${${proj}_DIR})
  message(FATAL_ERROR "${proj}_DIR variable is defined but corresponds to nonexistent directory")
endif()

if(NOT DEFINED ${proj}_DIR)

  set(${proj}_SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj})
  set(${proj}_PREFIX ${CMAKE_BINARY_DIR}/${proj}-prefix)

  if(MSVC)
    ExternalProject_Add(${proj}
      ${${proj}_EP_ARGS}
      URL http://slicer.kitware.com/midas3/download/item/160278/openblas-x86-64-2014-07.zip
      URL_MD5 92466d4e97ab7cd6b0e371a4f6a45a28
      SOURCE_DIR ${${proj}_SOURCE_DIR}
      PREFIX ${${proj}_PREFIX}
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      INSTALL_COMMAND ""
      DEPENDS
        ${${proj}_DEPENDENCIES}
      )
    set(${proj}_DIR ${${proj}_SOURCE_DIR})
  else()
    set(${proj}_BINARY_DIR ${CMAKE_BINARY_DIR}/${proj}-build)
    set(${proj}_INSTALL_DIR ${CMAKE_BINARY_DIR}/${proj}-install)
    ExternalProject_Add(${proj}
      ${${proj}_EP_ARGS}
      URL http://slicer.kitware.com/midas3/download/item/160360/OpenBLAS-0.2.11.tar.gz
      URL_MD5 c456f3c5e84c3ab69ef89b22e616627a
      SOURCE_DIR ${${proj}_SOURCE_DIR}
      BUILD_IN_SOURCE 1
      PREFIX ${${proj}_PREFIX}
      CONFIGURE_COMMAND ""
      INSTALL_COMMAND $(MAKE) PREFIX=${${proj}_INSTALL_DIR} install
      )
    set(${proj}_DIR ${${proj}_INSTALL_DIR})
  endif()

else()
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDENCIES})
endif()

ExternalProject_Message(${proj} "${proj}_DIR:${${proj}_DIR}")
