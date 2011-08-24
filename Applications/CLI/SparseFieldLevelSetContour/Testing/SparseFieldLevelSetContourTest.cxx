
#ifdef WIN32
#define MODULE_IMPORT __declspec(dllimport)
#else
#define MODULE_IMPORT
#endif

#include "itkTestMain.h"

void RegisterTests()
{
  REGISTER_TEST(SparseFieldLevelSetContourTest);
}

#undef main
#define main SparseFieldLevelSetContourTest

#include "../SparseFieldLevelSetContour.cxx"
