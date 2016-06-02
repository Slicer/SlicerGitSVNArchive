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

        self.assertIn("Volume", slicer.util.getNodes("Volume"))
        self.assertIn("Volume", slicer.util.getNodes("Volume", useLists=True))

        self.assertEqual(slicer.util.getNodes("Volume").keys(), ["Volume"])
        self.assertIsInstance(slicer.util.getNodes("Volume")["Volume"], vtk.vtkObject)
        self.assertEqual(slicer.util.getNodes("Volume", useLists=True).keys(), ["Volume"])
        self.assertIsInstance(slicer.util.getNodes("Volume", useLists=True)["Volume"], list)

    def test_getNodes(self):
        self.assertEqual(slicer.util.getNodes(), slicer.util.getNodes("*"))

        self.assertIn("Volume1", slicer.util.getNodes("*"))
        self.assertIn("Volume2", slicer.util.getNodes("*"))
        self.assertIn("Volume1", slicer.util.getNodes())
        self.assertIn("Volume2", slicer.util.getNodes())

        self.assertEqual(slicer.util.getNodes("Volume1").keys(), ["Volume1"])
        self.assertEqual(slicer.util.getNodes("Volume2").keys(), ["Volume2"])
        self.assertEqual(slicer.util.getNodes("Volume*").keys(), ["Volume", "Volume1", "Volume2"])

    def test_getFirstNodeByName(self):
        self.assertEqual(slicer.util.getFirstNodeByName("Volume", 'vtkMRMLScalarVolumeNode').GetName(), "Volume1" )
