import slicer
import sitkUtils as su

import unittest

class SitkUtilsTests(unittest.TestCase):

    def setUp(self):
        """ Download the MRHead node
        """

        from SampleData import SampleDataLogic
        SampleDataLogic().downloadMRHead()
        volumeNode1 = slicer.util.getNode('MRHead')
        self.assertEqual(volumeNode1.GetName(), "MRHead")
        self.volumeNode1 = volumeNode1

    def test_SimpleITK_SlicerPushPull(self):
        """ Try to pull a slicer MRML image, return the SimpleITK,
        push it to Slicer, and then compare if the image are the same
        """

        sitkimage = su.PullFromSlicer(self.volumeNode1.GetName())
        su.PushToSlicer(sitkimage, 'MRHead', compositeView=0, overwrite=False)
        volumeNode2 = slicer.util.getNode('MRHead')

        self.assertEqual(self.volumeNode1, volumeNode2,
                         'Error Push Pull: volumeNode are not the same')
        self.assertEqual(self.volumeNode1.GetID(), volumeNode2.GetID(),
                         'Error Push Pull: ID are not the same')
        self.assertEqual(self.volumeNode1.GetMTime(), volumeNode2.GetMTime(),
                         'Error Push Pull: Modify Time are not the same')

        """ To perform the test:
        Try to modified the image with sitk tools (just few sanity checks)
        Verify the GetMTime are different after a modification of sitkimage
        Try with all the compositeView and overwrite (any possibility)
        """

    def tearDown(self):
        del self.volumeNode1
        slicer.mrmlScene.Clear(0)
