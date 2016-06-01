import unittest
import slicer
import slicer.util
import vtk

class SlicerUtilTest(unittest.TestCase):
    def setUp(self):
        slicer.mrmlScene.AddNode(slicer.vtkMRMLScalarVolumeNode()).SetName("Volume1")
        slicer.mrmlScene.AddNode(slicer.vtkMRMLScalarVolumeNode()).SetName("Volume2")
        slicer.mrmlScene.AddNode(slicer.vtkMRMLScalarVolumeNode()).SetName("Volume")
        slicer.mrmlScene.AddNode(slicer.vtkMRMLScalarVolumeNode()).SetName("Volume")

    def test_getNodesMultipleNodesSharingName(self):

        self.assertTrue("Volume" in slicer.util.getNodes("Volume"))
        self.assertTrue("Volume" in slicer.util.getNodes("Volume",useLists=True))

        self.assertEqual(slicer.util.getNodes("Volume").keys(), ["Volume"])
        self.assertEqual(isinstance(slicer.util.getNodes("Volume")["Volume"], vtk.vtkObject), True)
        self.assertEqual(slicer.util.getNodes("Volume",useLists=True).keys(), ["Volume"])
        self.assertEqual(isinstance(slicer.util.getNodes("Volume",useLists=True)["Volume"], list), True)

    def test_getNodes(self):
        self.assertEqual(slicer.util.getNodes(), slicer.util.getNodes("*"))

        self.assertTrue("Volume1" in slicer.util.getNodes("*"))
        self.assertTrue("Volume2" in slicer.util.getNodes("*"))
        self.assertTrue("Volume1" in slicer.util.getNodes())
        self.assertTrue("Volume2" in slicer.util.getNodes())

        self.assertEqual(slicer.util.getNodes("Volume1").keys(), ["Volume1"])
        self.assertEqual(slicer.util.getNodes("Volume2").keys(), ["Volume2"])
        self.assertEqual(slicer.util.getNodes("Volume*").keys(), ["Volume", "Volume1", "Volume2"])
