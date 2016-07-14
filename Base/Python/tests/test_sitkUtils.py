import slicer
#import SimpleITK as sitk
import sitkUtils as su

import unittest

class SitkUtilsTests(unittest.TestCase):

    def runTest(self):
        self.setUp()
        self.test_SimpleITK_SlicerPushPull()
        self.tearDown()

    def setUp(self):
        """ Download the MRHead node
        """

        from SampleData import SampleDataLogic
        SampleDataLogic().downloadMRHead()
        myNode1 = slicer.util.getNode('MRHead')

        print("Node Name is %s" % myNode1.GetName())
        self.assertIsNotNone(myNode1.GetName(),"ERROR: Node Name is None !")

        self.myNode1 = myNode1

    def test_SimpleITK_SlicerPushPull(self):
        """ Try to pull a slicer MRML image, return the SimpleITK,
        push it to Slicer, and then compare if the image are the same
        """

        sitkimage = su.PullFromSlicer(self.myNode1.GetName())
        su.PushToSlicer(sitkimage, 'MRHead', compositeView=0, overwrite=False)
        myNode2 = slicer.util.getNode('MRHead')

        self.assertEqual(self.myNode1, myNode2,
                         'Error Push Pull: Node are not the same')
        self.assertEqual(self.myNode1.GetID(), myNode2.GetID(),
                         'Error Push Pull: id are not the same')
        self.assertEqual(self.myNode1.GetMTime(), myNode2.GetMTime(),
                         'Error Push Pull: Modify Time are not the same')

        """ To perform the test:
        Try with a modified sitkimage and verify the GetMTime are diffrents
        Try with all the compositeView and overwrite (all possiblities)
        """

    def tearDown(self):
        del self.myNode1
