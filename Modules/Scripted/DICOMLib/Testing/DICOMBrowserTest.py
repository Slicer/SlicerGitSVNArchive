
import os, logging
import unittest
import qt
import slicer, ctk

class DICOMBrowserVolumeNaming(unittest.TestCase):
  def setUp(self):
    pass

  def delayDisplay(self,message,msec=1000):
    """This utility method displays a small dialog and waits.
    This does two things: 1) it lets the event loop catch up
    to the state of the test so that rendering and widget updates
    have all taken place before the test continues and 2) it
    shows the user/developer/tester the state of the test
    so that we'll know when it breaks.
    """
    print(message)
    self.info = qt.QDialog()
    self.infoLayout = qt.QVBoxLayout()
    self.info.setLayout(self.infoLayout)
    self.label = qt.QLabel(message,self.info)
    self.infoLayout.addWidget(self.label)
    qt.QTimer.singleShot(msec, self.info.close)
    self.info.exec_()

  def runTest(self):
    self.test_LoadDICOMScalarVolume()

  def switchToDICOMModule(self):
    self.delayDisplay('Importing DICOM')
    mainWindow = slicer.util.mainWindow()
    mainWindow.moduleSelector().selectModule('DICOM')
    dicomFilesDirectory = slicer.app.temporaryPath + '/DICOMFilesDirectory'
    indexer = ctk.ctkDICOMIndexer()
    indexer.addDirectory(slicer.dicomDatabase, dicomFilesDirectory, None)
    indexer.waitForImportFinished()


  def test_LoadDICOMScalarVolume(self):
    """
    Load a DICOM file with default and customized volume name template, and check that in both cases the name of
    the volume loaded is ok.
    The DICOM plugin used is DICOMScalarVolumePlugin
    """
    self.switchToDICOMModule()
    # Get the data
    selfTestDir = slicer.app.temporaryPath + '/DICOMBrowserVolumeNaming'
    if not os.access(selfTestDir, os.F_OK):
      os.mkdir(selfTestDir)

    urlDownload = 'http://slicer.kitware.com/midas3/download/item/119940'
    fileName = 'DicomTestImage00001.dcm'
    filePath = os.path.join(selfTestDir, fileName)
    if not os.path.isfile(filePath):
      import urllib
      self.delayDisplay("Downloading DICOM data")
      urllib.urlretrieve(urlDownload, filePath)

    # Load the DICOM file via the DICOM Scalar Volume
    plugin = slicer.modules.dicomPlugins["DICOMScalarVolumePlugin"]()
    loadables = plugin.examineForImport([[filePath]])
    plugin.load(loadables[0])
    # Check the volume is using default template (seriesNumber: seriesDescription)
    self.assertIsNotNone(slicer.util.getNode("6: Unknown"))

    # Open a new copy of the volume using a different template (Patient name, series number)
    plugin.loadableCache.clear()  # Clear cache
    plugin.volumeNameTagsTemplate = "@PatientName@: @0020,0011@"
    loadables = plugin.examineForImport([[filePath]])
    plugin.load(loadables[0])
    slicer.util.getNode("RANDO^PROSTATE: 6")

    self.delayDisplay("Test passed!")

#
# DICOM Browser tests
#
class DICOMBrowserTest:
  """
  This class is the 'hook' for slicer to detect and recognize the test
  as a loadable scripted module (with a hidden interface)
  """
  def __init__(self, parent):
    parent.title = "DICOMBrowserTest"
    parent.categories = ["Testing.TestCases"]
    parent.dependencies = ['DICOM']
    parent.contributors = ["Jorge Onieva (BWH)"]
    parent.helpText = """Self tests for the DICOM Browser. So far, no GUI is tested"""

    # don't show this module
    parent.hidden = True

    # Add this test to the SelfTest module's list for discovery when the module
    # is created.  Since this module may be discovered before SelfTests itself,
    # create the list if it doesn't already exist.
    try:
      slicer.selfTests
    except AttributeError:
      slicer.selfTests = {}
    slicer.selfTests['DICOMBrowserTest'] = self.runTest

  def runTest(self):
    tester = DICOMBrowserVolumeNaming()
    tester.setUp()
    tester.runTest()


#
# DICOMBrowserSelfTestWidget
#

class DICOMBrowserSelfTestWidget:
  def __init__(self, parent = None):
    self.parent = parent

  def setup(self):
    # don't display anything for this widget - it will be hidden anyway
    pass

  def enter(self):
    pass

  def exit(self):
    pass


