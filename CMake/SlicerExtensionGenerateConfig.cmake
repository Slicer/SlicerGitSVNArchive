# Generate the <Extension>Config.cmake file in the build tree.
# This file makes it possible an extension to depend on targets defined
# in other extensions.
# This file is based on SlicerGenerateSlicerConfig.cmake.
#
# Example: add these lines to the CMakeLists.txt to use vtkMRMLSequenceNode
# implemented in Sequences extension:
#
#   find_package(Sequences REQUIRED)
#   ...
#   set(MODULE_INCLUDE_DIRECTORIES
#     ...
#     ${vtkSlicerSequencesModuleMRML_INCLUDE_DIRS}
#     )
#   ...
#   set(MODULE_TARGET_LIBRARIES
#     ...
#     vtkSlicerSequencesModuleMRML
#     )
#

get_property(_module_targets GLOBAL PROPERTY SLICER_MODULE_TARGETS)
if(_module_targets)
  foreach(target ${_module_targets})
    set(EXTENSION_INCLUDE_MODULE_DIRS_CONFIG
"${EXTENSION_INCLUDE_MODULE_DIRS_CONFIG}
set(${target}_INCLUDE_DIRS
  \"${${target}_INCLUDE_DIRS}\")"
)
  endforeach()
endif()

get_property(_module_logic_targets GLOBAL PROPERTY SLICER_MODULE_LOGIC_TARGETS)
if(_module_logic_targets)
  foreach(target ${_module_logic_targets})
    set(EXTENSION_INCLUDE_MODULE_LOGIC_DIRS_CONFIG
"${EXTENSION_INCLUDE_MODULE_LOGIC_DIRS_CONFIG}
set(${target}_INCLUDE_DIRS
  \"${${target}_INCLUDE_DIRS}\")"
)
    list(APPEND EXTENSION_ModuleLogic_INCLUDE_DIRS_CONFIG "\${${target}_INCLUDE_DIRS}")
  endforeach()
endif()

get_property(_module_mrml_targets GLOBAL PROPERTY SLICER_MODULE_MRML_TARGETS)
if(_module_mrml_targets)
  foreach(target ${_module_mrml_targets})
    set(EXTENSION_INCLUDE_MODULE_MRML_DIRS_CONFIG
"${EXTENSION_INCLUDE_MODULE_MRML_DIRS_CONFIG}
set(${target}_INCLUDE_DIRS
  \"${${target}_INCLUDE_DIRS}\")"
)
    list(APPEND EXTENSION_ModuleMRML_INCLUDE_DIRS_CONFIG "\${${target}_INCLUDE_DIRS}")
  endforeach()
endif()

get_property(_module_widget_targets GLOBAL PROPERTY SLICER_MODULE_WIDGET_TARGETS)
if(_module_widget_targets)
  foreach(target ${_module_widget_targets})
    set(EXTENSION_INCLUDE_MODULE_WIDGET_DIRS_CONFIG
"${EXTENSION_INCLUDE_MODULE_WIDGET_DIRS_CONFIG}
set(${target}_INCLUDE_DIRS
  \"${${target}_INCLUDE_DIRS}\")"
)
    list(APPEND EXTENSION_ModuleWidgets_INCLUDE_DIRS_CONFIG "\${${target}_INCLUDE_DIRS}")
  endforeach()
endif()

# Export Targets file.
set(EXTENSION_TARGETS_FILE "${EXTENSION_SUPERBUILD_BINARY_DIR}/${EXTENSION_NAME}Targets.cmake")

set(EXTENSION_SOURCE_DIR_CONFIG "set(${EXTENSION_NAME}_SOURCE_DIR \"${${EXTENSION_NAME}_SOURCE_DIR}\")")

# Configure <Extension>Config.cmake for the build tree.
configure_file(
  ${Slicer_CMAKE_DIR}/SlicerExtensionConfig.cmake.in
  ${EXTENSION_SUPERBUILD_BINARY_DIR}/${EXTENSION_NAME}Config.cmake @ONLY)
