
#-----------------------------------------------------------------------------
set(MODULE_NAME CLIModuleTemplate)

#-----------------------------------------------------------------------------

set(PYTHON_MODULE_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/${MODULE_NAME}Python.py)

#-----------------------------------------------------------------------------
if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()
