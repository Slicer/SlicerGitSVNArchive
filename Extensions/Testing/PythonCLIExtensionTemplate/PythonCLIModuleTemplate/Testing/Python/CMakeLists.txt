#-----------------------------------------------------------------------------
set(BASELINE ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/Baseline)
set(INPUT ${CMAKE_CURRENT_SOURCE_DIR}/../../Data/Input)
set(TEMP "${CMAKE_BINARY_DIR}/Testing/Temporary")

set(CLP ${MODULE_NAME})

#-----------------------------------------------------------------------------
set(testname ${CLP}Test)
ExternalData_add_test(${CLP}Data NAME ${testname} COMMAND ${Slicer_LAUNCHER_EXECUTABLE} ${MODULE_NAME}.py
  --sigma 2.5 DATA{${INPUT}/CTHeadAxial.nhdr,CTHeadAxial.raw.gz} ${TEMP}/${CLP}Test.nhdr
  DATA{${BASELINE}/${CLP}Test.nhdr,${CLP}Test.raw}
  )
set_property(TEST ${testname} PROPERTY LABELS ${CLP})

#-----------------------------------------------------------------------------
ExternalData_add_target(${CLP}Data)
