
set(proj OpenBLAS)

set(${proj}_DEPENDENCIES "")
ExternalProject_Include_Dependencies(${proj} PROJECT_VAR proj DEPENDS_VAR ${proj}_DEPENDENCIES)

if(${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj})
  message(FATAL_ERROR "Enabling ${CMAKE_PROJECT_NAME}_USE_SYSTEM_${proj} is not supported")
endif()

if(DEFINED ${proj}_DIR AND NOT EXISTS ${${proj}_DIR})
  message(FATAL_ERROR "${proj}_DIR variable is defined but corresponds to nonexistent directory")
endif()

set(${proj}_REPOSITORY ${git_protocol}://github.com/xianyi/OpenBLAS.git)
set(${proj}_GIT_TAG 12ab1804b6ebcd38b26960d65d254314d8bc33d6)

if(NOT DEFINED ${proj}_DIR)

  set(${proj}_SOURCE_DIR ${CMAKE_BINARY_DIR}/${proj})
  set(${proj}_PREFIX ${CMAKE_BINARY_DIR}/${proj}-prefix)

  set(${proj}_BINARY_DIR ${CMAKE_BINARY_DIR}/${proj}-build)
  set(${proj}_INSTALL_DIR ${CMAKE_BINARY_DIR}/${proj}-install)
  ExternalProject_Add(${proj}
    ${${proj}_EP_ARGS}
    GIT_REPOSITORY ${${proj}_REPOSITORY}
    GIT_TAG ${${proj}_GIT_TAG}
    SOURCE_DIR ${${proj}_SOURCE_DIR}
    BUILD_IN_SOURCE 1
    PREFIX ${${proj}_PREFIX}
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND $(MAKE) PREFIX=${${proj}_INSTALL_DIR} install
    )
  set(${proj}_DIR ${${proj}_INSTALL_DIR})

else()
  ExternalProject_Add_Empty(${proj} DEPENDS ${${proj}_DEPENDENCIES})
endif()

ExternalProject_Message(${proj} "${proj}_DIR:${${proj}_DIR}")
