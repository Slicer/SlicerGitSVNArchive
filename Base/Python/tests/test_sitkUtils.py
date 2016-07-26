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
        DONE : Try to modified the image with sitk tools (just few sanity checks)
        DONE : Verify the GetMTime are different after a modification of sitkimage
        Try with all the compositeView and overwrite (any possibility)
        """

        """ Few modification of the image : Direction, Origin """
        sitkimage.SetDirection((-1.0, 1.0, 0.0, 0.0, -1.0, 1.0, 1.0, 0.0, 1.0))
        sitkimage.SetOrigin((100.0, 100.0, 100.0))

        """ Few pixel changed """
        size = sitkimage.GetSize()
        for x in xrange(0,size[0],(int)(size[0]/10)):
            for y in xrange(0,size[1],(int)(size[1]/10)):
                for z in xrange(0,size[2],(int)(size[2]/10)):
                    sitkimage.SetPixel(x,y,z,0L)

        su.PushToSlicer(sitkimage, 'ImageChanged', compositeView=0, overwrite=False)
        volumeNode2 = slicer.util.getNode('ImageChanged')
        self.assertEqual(volumeNode2.GetName(), "ImageChanged",
                         'Error Push Pull: Node Name are not the same')
        self.assertNotEqual(self.volumeNode1.GetMTime(), volumeNode2.GetMTime(),
                            'Error Push Pull: Modify Time are the same')

        """ Test the consistency between sitkimage and volumeNode2
        """
        tmp = volumeNode2.GetOrigin()
        valToCompare = (-tmp[0], -tmp[1], tmp[2])
        self.assertEqual(valToCompare,sitkimage.GetOrigin(),
                         'Error Push Pull: Different Origin')

        """ ===========
        Viewing options
        ---------------
        bit 0: Set as background image
        bit 1: Set as foreground image
        bit 2: Set as label image
        """
        """ TODO : one by one those Push are working but there is an error during the loop
        caused by the hack in EnsureRegistration() in sitkUtils.py : _DUMMY_DOES_NOT_EXISTS__

        for compositeView in xrange(3):
            for overwrite in [False, True]:
                su.PushToSlicer(sitkimage, 'volumeNode'+str(compositeView)+str(overwrite),
                                compositeView, overwrite)
                volumeNode = slicer.util.getNode('volumeNode'+str(compositeView)+str(overwrite))

                print("compositeView : %s" %compositeView )
                print("overwrite : %s " %overwrite )

                #Check if it's a label
                if compositeView == 2:
                    self.assertEqual(volumeNode.GetClassName(),'vtkMRMLLabelMapVolumeNode',
                                     'Error Push Pull: Not a label Class Name')
                else:
                    self.assertEqual(volumeNode.GetClassName(), 'vtkMRMLScalarVolumeNode',
                                     'Error Push Pull: Not a back/foreground Class Name')
        """

    def tearDown(self):
        del self.volumeNode1
        slicer.mrmlScene.Clear(0)
