import unittest
import slicer
import slicer.util

class SlicerUnitTestTest(unittest.TestCase):
    def test_getNodesNoArg(self):
        self.assertEqual(slicer.util.getNodes(), slicer.util.getNodes("*"))

    def test_getNodes_MRHead(self):
        self.assertEqual(slicer.util.getNodes("MR-head").keys(), ["MR-head"])
