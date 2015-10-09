# --------------------------------------------------------------------------
# Install support files
# --------------------------------------------------------------------------

# Should this location change, update Modules/Loadable/Colors/Logic/vtkSlicerColorLogic.cxx

file(GLOB COLORFILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "ColorFiles/*.txt")

foreach(colorfile ${COLORFILES})
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${colorfile}
    ${CMAKE_BINARY_DIR}/${Slicer_SHARE_DIR}/${colorfile}
    COPYONLY
    )
endforeach()
install(
  FILES ${COLORFILES}
  DESTINATION ${Slicer_INSTALL_SHARE_DIR}/ColorFiles
  COMPONENT Runtime
  )

# Related terminology files
file(GLOB TERMINOLOGYLUTFILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "ColorFiles/Terminology/*.csv")
file(GLOB REGIONANDCATEGORYFILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "ColorFiles/Terminology/*.xml")
set(TERMINOLOGYFILES
  ${REGIONANDCATEGORYFILES}
  ${TERMINOLOGYLUTFILES}
  )
MESSAGE(STATUS "Collected terminology files:\n${TERMINOLOGYFILES}")

foreach(terminologyfile ${TERMINOLOGYFILES})
  MESSAGE(STATUS "configuring terminology file ${terminologyfile}")
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${terminologyfile}
    ${CMAKE_BINARY_DIR}/${Slicer_SHARE_DIR}/${terminologyfile}
    COPYONLY
    )
endforeach()
install(
  FILES ${TERMINOLOGYFILES}
  DESTINATION ${Slicer_INSTALL_SHARE_DIR}/ColorFiles/Terminology
  COMPONENT Runtime
  )
