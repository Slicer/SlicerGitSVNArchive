message(AUTHOR_WARNING
  "File 'SlicerMacroBuildQtModule.cmake' is deprecated, "
  "and is scheduled for removal in version 4.8. "
  "Consider using file 'SlicerMacroBuildLoadableModule.cmake'. "
  "See http://www.na-mic.org/Bug/view.php?id=3332"
  )
include(${CMAKE_CURRENT_LIST_DIR}/SlicerMacroBuildLoadableModule.cmake)
