
#-----------------------------------------------------------------------------
set(BASELINE ${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline)
set(CLP ${MODULE_NAME})

#-----------------------------------------------------------------------------
add_executable(${CLP}Test ${CLP}Test.cxx)
target_link_libraries(${CLP}Test ${CLP}Lib)
set_target_properties(${CLP}Test PROPERTIES LABELS ${CLP})

set(testname ${CLP}Test000)
add_test(NAME ${testname} COMMAND ${SEM_LAUNCH_COMMAND} $<TARGET_FILE:${CLP}Test>
  ModuleEntryPoint  --help
  )

set(testname ${CLP}Test00)
add_test(NAME ${testname} COMMAND ${SEM_LAUNCH_COMMAND} $<TARGET_FILE:${CLP}Test>
  --compare ${BASELINE}/${CLP}Test.mha
            ${TEMP}/${CLP}Test.mha  
  ModuleEntryPoint
  --numPoints 100
  --dontPrune
   ${BASELINE}/${CLP}.mha
   ${TEMP}/${CLP}Test.mha
  )
set_property(TEST ${testname} PROPERTY LABELS ${CLP})