
import os
import unittest
import vtk
import qt
import slicer
# import EditorLib

class SlicerMRBSaveRestoreCheckPaths(unittest.TestCase):
  """ Test for slicer data bundle with save and load and save to ensure file paths are okay.

Run manually from within slicer by pasting an version of this with the correct path into the python console:
execfile('/Users/pieper/slicer4/latest/Slicer/Applications/SlicerApp/Testing/Python/SlicerMRBSaveRestoreCheckPathsTest.py'); t = SlicerMRBSaveRestoreCheckPaths(); t.setUp(); t.runTest()

  """

  def __init__(self,methodName='runTest', uniqueDirectory=True,strict=False):
    """
    Tests the use of mrml and mrb save formats with volumes.
    Checks that after reopening an MRB and trying to save it again that the file paths are all correct.

    uniqueDirectory: boolean about save directory
                     False to reuse standard dir name
                     True timestamps dir name
    strict: boolean about how carefully to check result
                     True then check every detail
                     False then confirm basic operation, but allow non-critical issues to pass
    """
    unittest.TestCase.__init__(self,methodName)
    self.uniqueDirectory = uniqueDirectory
    self.strict = strict
    # this flag will need to be updated if the code in
    #  qSlicerSceneBundleReader::load
    # is changed from the current behaviour (delete expanded
    # files after loading the MRB into Slicer)
    self.mrbDeleteFilesAfterLoad = 1

  def setUp(self):
    slicer.mrmlScene.Clear(0)

  def runTest(self):
    self.setUp()
    self.test_SlicerMRBSaveRestoreCheckPaths()
    self.test_PercentEncode()

  #
  # find and return the storable node associated with this storage node.
  # Returns None if not found.
  #
  def getStorableNode(self, storageNode):
    numberOfStorableNodes = storageNode.GetScene().GetNumberOfNodesByClass('vtkMRMLStorableNode')
    for n in range(numberOfStorableNodes):
      storableNode = storageNode.GetScene().GetNthNodeByClass(n,'vtkMRMLStorableNode')
      if storableNode.GetStorageNodeID() == storageNode.GetID():
        return storableNode
    return None

  #
  # create and return a list of all file names found in storage nodes in
  # the scene, not including those in scene views
  #
  def checkSceneFileNames(self, scene):
    # first get the main scene storage nodes
    numberOfStorageNodes = scene.GetNumberOfNodesByClass('vtkMRMLStorageNode')
    for n in range(numberOfStorageNodes):
      storageNode = scene.GetNthNodeByClass(n,'vtkMRMLStorageNode')
      storableNode = self.getStorableNode(storageNode)
      if storageNode.GetSaveWithScene() and not storableNode.GetModifiedSinceRead():
        print 'Checking storage node: ',storageNode.GetID()
        fileName = storageNode.GetFileName()
        absFileName = storageNode.GetAbsoluteFilePath(fileName)
        if not absFileName:
          print '\tUnable to get absolute path for file name ',fileName
          self.numberOfFilesNotFound += 1
        elif not os.path.exists(absFileName):
          print '\tfile does not exist: ',absFileName
          print '\t\tnon absolute file name = ',fileName
          print '\t\tscene of the node root dir = ',storageNode.GetScene().GetRootDirectory()
          if storableNode is not None:
            print '\t\tstorable node name = ',storableNode.GetName()
            print '\t\tmodified since read = ',storableNode.GetModifiedSinceRead()
          else:
            print '\t\tNo storable node found for this storage node'
          # double check that it's not due to the unzipped files being deleted
          if (not self.mrbDeleteFilesAfterLoad) or ('BundleSaveTemp' in absFileName):
            self.numberOfFilesNotFound += 1
          else:
            print '\t\tMRB files were deleted after load, not counting this file as not found for the purposes of this test'
        else:
          print '\tfile exists:',absFileName
        # check for the file list
        numberOfFileNames = storageNode.GetNumberOfFileNames()
        for n in range(numberOfFileNames):
          fileName = storageNode.GetNthFileName(n)
          absFileName = storageNode.GetAbsoluteFilePath(fileName)
          if not os.path.exists(absFileName):
            print '\t',n,'th file list member does not exist: ',absFileName
            if (not self.mrbDeleteFilesAfterLoad) or ('BundleSaveTemp' in absFileName):
              self.numberOfFilesNotFound += 1
            else:
              print '\t\tMRB files were deleted after load, not counting this file as not foudn for the purposes of this test'

  def checkSceneViewFileNames(self, scene):
    # check for any scene views
    numberOfSceneViews = scene.GetNumberOfNodesByClass('vtkMRMLSceneViewNode')
    slicer.util.delayDisplay("Number of scene views = " + str(numberOfSceneViews))
    if numberOfSceneViews == 0:
      return;
    for n in range(numberOfSceneViews):
      sceneViewNode = slicer.mrmlScene.GetNthNodeByClass(n, 'vtkMRMLSceneViewNode')
      slicer.util.delayDisplay('\nChecking scene view ' + sceneViewNode.GetName() + ', id = ' + sceneViewNode.GetID())
      self.checkSceneFileNames(sceneViewNode.GetStoredScene())

  def checkAllFileNames(self, scene):
    slicer.util.delayDisplay("\n\nChecking all file names in scene")
    self.numberOfFilesNotFound = 0
    self.checkSceneFileNames(scene)
    self.checkSceneViewFileNames(scene)
    if self.numberOfFilesNotFound != 0:
      print 'checkAllFilesNames: there are ',self.numberOfFilesNotFound,'files that are missing from disk\n'
    self.assertTrue(self.numberOfFilesNotFound == 0)

  def test_SlicerMRBSaveRestoreCheckPaths(self):
    """
    Replicate the issue reported in bug 3956 where saving
    and restoring an MRB file does not work.
    """

    print("Running SlicerMRBSaveRestoreCheckPaths Test case with:")
    print("uniqueDirectory : %s" % self.uniqueDirectory)
    print("strict : %s" % self.strict)
    print("files deleted after load: %d" % self.mrbDeleteFilesAfterLoad)

    #
    # first, get the volume data
    #
    import SampleData
    sampleDataLogic = SampleData.SampleDataLogic()
    print("Getting MR Head Volume")
    mrHeadVolume = sampleDataLogic.downloadMRHead()

    slicer.util.delayDisplay('Finished with download of volume')

    #
    # test all current file paths
    #
    self.checkAllFileNames(slicer.mrmlScene)

    ioManager = slicer.app.ioManager()
    # grab a testing screen shot
    layoutManager = slicer.app.layoutManager()
    widget = layoutManager.threeDWidget(0)
    qpixMap = qt.QPixmap().grabWidget(widget)
    screenShot = qpixMap.toImage()

    #
    # the remote download leaves the volume in a temp directory and removes
    # the storage node, commit the scene to get to a more stable starting
    # point with the volume saved in a regular directory
    #
    tempDir = self.tempDirectory('__mrml__')
    slicer.util.delayDisplay('Temp dir = %s ' % tempDir)
    mrmlFilePath = tempDir + '/SlicerMRBSaveRestoreCheckPath.mrml'
    slicer.mrmlScene.SetURL(mrmlFilePath)
    slicer.util.delayDisplay('Saving mrml file to %s, current url of scene is %s' % (mrmlFilePath, slicer.mrmlScene.GetURL()))
    # saveScene just writes out the .mrml file
    self.assertTrue(ioManager.saveScene(mrmlFilePath, screenShot))
    slicer.util.delayDisplay('Finished saving mrml file %s, mrml url is now %s\n\n\n' % (mrmlFilePath, slicer.mrmlScene.GetURL()))
    slicer.util.delayDisplay('mrml root dir = %s' % slicer.mrmlScene.GetRootDirectory())
    # explicitly save MRHead
    mrHeadVolume.GetStorageNode().WriteData(mrHeadVolume)

    #
    # test all current file paths
    #
    self.checkAllFileNames(slicer.mrmlScene)

    #
    # save the mrb
    #
    mrbFilePath= self.tempDirectory('__mrb__') + '/SlicerMRBSaveRestoreCheckPaths-1.mrb'
    slicer.util.delayDisplay("\n\n\nSaving mrb to: %s" % mrbFilePath)
    self.assertTrue(
        ioManager.saveScene(mrbFilePath, screenShot)
    )
    slicer.util.delayDisplay("Finished saving mrb\n\n\n")

    #
    # test all current file paths
    #
    self.checkAllFileNames(slicer.mrmlScene)

    #
    # reload the mrb and restore a scene view
    #
    slicer.mrmlScene.Clear(0)
    slicer.util.delayDisplay('Now, reload the first saved MRB\n\n\n')
    mrbLoaded = ioManager.loadScene(mrbFilePath)
    # load can return false even though it succeeded - only fail if in strict mode
    self.assertTrue( not self.strict or mrbLoaded )
    slicer.app.processEvents()
    slicer.util.delayDisplay("\n\n\nFinished reloading the first saved MRB\n\n\n")
    #
    # test all current file paths
    #
    self.checkAllFileNames(slicer.mrmlScene)

    #
    # Save it again
    #
    mrbFilePath= self.tempDirectory('__mrb__') + '/SlicerMRBSaveRestoreCheckPaths-2.mrb'
    slicer.util.delayDisplay("Saving second mrb to: %s\n\n\n\n" % mrbFilePath)
    self.assertTrue(
        ioManager.saveScene(mrbFilePath, screenShot)
    )
    slicer.util.delayDisplay("\n\n\nFinished second saving mrb %s" % mrbFilePath)

    #
    # test all current file paths
    #
    self.checkAllFileNames(slicer.mrmlScene)

    slicer.util.delayDisplay("Test Finished")

  def tempDirectory(self,key='__SlicerTestTemp__',tempDir=None):
    """Come up with a unique directory name in the temp dir and make it and return it
    # TODO: switch to QTemporaryDir in Qt5.
    # For now, create a named directory if uniqueDirectory attribute is true
    Note: this directory is not automatically cleaned up
    """
    if not tempDir:
      tempDir = qt.QDir(slicer.app.temporaryPath)
    tempDirName = key
    if self.uniqueDirectory:
      key += qt.QDateTime().currentDateTime().toString("yyyy-MM-dd_hh+mm+ss.zzz")
    fileInfo = qt.QFileInfo(qt.QDir(tempDir), tempDirName)
    dirPath = fileInfo.absoluteFilePath()
    qt.QDir().mkpath(dirPath)
    return dirPath


#
# SlicerMRBSaveRestoreCheckPathsTest
#

class SlicerMRBSaveRestoreCheckPathsTest:
  """
  This class is the 'hook' for slicer to detect and recognize the test
  as a loadable scripted module (with a hidden interface)
  """
  def __init__(self, parent):
    parent.title = "SlicerMRBSaveRestoreCheckPathsTest"
    parent.categories = ["Testing"]
    parent.contributors = ["Nicole Aucoin (BWH)"]
    parent.helpText = """
    Self test for MRB multiple save file paths.
    No module interface here, only used in SelfTests module
    """
    parent.acknowledgementText = """
    This test was developed by
    Nicole Aucoin, BWH
    and was partially funded by NIH grant 3P41RR013218.
    """

    # don't show this module
    parent.hidden = True

    # Add this test to the SelfTest module's list for discovery when the module
    # is created.  Since this module may be discovered before SelfTests itself,
    # create the list if it doesn't already exist.
    try:
      slicer.selfTests
    except AttributeError:
      slicer.selfTests = {}
    slicer.selfTests['SlicerMRBSaveRestoreCheckPathsTest'] = self.runTest

  def runTest(self):
    tester = SlicerMRBSaveRestoreCheckPaths()
    tester.setUp()
    tester.runTest()
