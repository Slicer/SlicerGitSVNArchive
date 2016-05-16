#-----------------------------------------------------------------------------
set(MODULE_NAME PythonCLIModuleTemplate)

#-----------------------------------------------------------------------------
set(MODULE_PYTHON_SCRIPTS
  ${MODULE_NAME}.py
  )

set(MODULE_PYTHON_RESOURCES
  ${MODULE_NAME}.xml
  )

#-----------------------------------------------------------------------------
slicerMacroBuildScriptedModule(
  NAME ${MODULE_NAME}
  SCRIPTS ${MODULE_PYTHON_SCRIPTS}
  RESOURCES ${MODULE_PYTHON_RESOURCES}
  )

#-----------------------------------------------------------------------------
if(BUILD_TESTING)
  # Additional build-time testing
  add_subdirectory(Testing)
endif()
