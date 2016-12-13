import os
import unittest
from __main__ import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *
import logging

#
# DICOMPatcher
#

class DICOMPatcher(ScriptedLoadableModule):
  """Uses ScriptedLoadableModule base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def __init__(self, parent):
    ScriptedLoadableModule.__init__(self, parent)
    self.parent.title = "DICOM Patcher"
    self.parent.categories = ["Utilities"]
    self.parent.dependencies = ["DICOM"]
    self.parent.contributors = ["Andras Lasso (PerkLab)"]
    self.parent.helpText = """Fix common issues in DICOM files. This module may help fixing DICOM files that Slicer fails to import."""
    self.parent.acknowledgementText = """ """

#
# DICOMPatcherWidget
#

class DICOMPatcherWidget(ScriptedLoadableModuleWidget):
  """Uses ScriptedLoadableModuleWidget base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def setup(self):
    ScriptedLoadableModuleWidget.setup(self)

    # Instantiate and connect widgets ...

    #
    # Parameters Area
    #
    parametersCollapsibleButton = ctk.ctkCollapsibleButton()
    parametersCollapsibleButton.text = "Parameters"
    self.layout.addWidget(parametersCollapsibleButton)

    # Layout within the dummy collapsible button
    parametersFormLayout = qt.QFormLayout(parametersCollapsibleButton)

    self.inputDirSelector = ctk.ctkPathLineEdit()
    self.inputDirSelector.filters = ctk.ctkPathLineEdit.Dirs
    self.inputDirSelector.settingKey = 'DICOMPatcherInputDir'
    parametersFormLayout.addRow("Input DICOM directory:", self.inputDirSelector)

    self.outputDirSelector = ctk.ctkPathLineEdit()
    self.outputDirSelector.filters = ctk.ctkPathLineEdit.Dirs
    self.outputDirSelector.settingKey = 'DICOMPatcherOutputDir'
    parametersFormLayout.addRow("Output DICOM directory:", self.outputDirSelector)

    self.normalizeFileNamesCheckBox = qt.QCheckBox()
    self.normalizeFileNamesCheckBox.checked = True
    self.normalizeFileNamesCheckBox.setToolTip("Replace file and folder names with automatically generated names."
      " Fixes errors caused by file path containins special characters or being too long.")
    parametersFormLayout.addRow("Normalize file names", self.normalizeFileNamesCheckBox)

    self.generateMissingIdsCheckBox = qt.QCheckBox()
    self.generateMissingIdsCheckBox.checked = True
    self.generateMissingIdsCheckBox.setToolTip("Generate missing patient, study, series IDs. It is assumed that"
      " all files in a directory belong to the same series. Fixes error caused by too aggressive anonymization"
      " or incorrect DICOM image converters.")
    parametersFormLayout.addRow("Generate missing patient/study/series IDs", self.generateMissingIdsCheckBox)

    self.generateImagePositionFromSliceThicknessCheckBox = qt.QCheckBox()
    self.generateImagePositionFromSliceThicknessCheckBox.checked = True
    self.generateImagePositionFromSliceThicknessCheckBox.setToolTip("Generate 'image position sequence' for"
      " multi-frame files that only have 'SliceThickness' field. Fixes error in Dolphin 3D CBCT scanners.")
    parametersFormLayout.addRow("Generate slice position for multi-frame volumes", self.generateImagePositionFromSliceThicknessCheckBox)

    self.anonymizeDicomCheckBox = qt.QCheckBox()
    self.anonymizeDicomCheckBox.checked = False
    self.anonymizeDicomCheckBox.setToolTip("If checked, then some patient identifiable information will be removed"
      " from the patched DICOM files. There are many fields that can identify a patient, this function does not remove all of them.")
    parametersFormLayout.addRow("Partially anonymize", self.anonymizeDicomCheckBox)

    #
    # Patch Button
    #
    self.patchButton = qt.QPushButton("Patch")
    self.patchButton.toolTip = "Fix DICOM files in input directory and write them to output directory"
    parametersFormLayout.addRow(self.patchButton)

    #
    # Patch Button
    #
    self.importButton = qt.QPushButton("Import")
    self.importButton.toolTip = "Import DICOM files in output directory into Slicer DICOM database"
    parametersFormLayout.addRow(self.importButton)

    # connections
    self.patchButton.connect('clicked(bool)', self.onPatchButton)
    self.importButton.connect('clicked(bool)', self.onImportButton)

    self.statusLabel = qt.QPlainTextEdit()
    self.statusLabel.setTextInteractionFlags(qt.Qt.TextSelectableByMouse)
    parametersFormLayout.addRow(self.statusLabel)

    # Add vertical spacer
    self.layout.addStretch(1)

    self.logic = DICOMPatcherLogic()
    self.logic.logCallback = self.addLog

  def cleanup(self):
    pass

  def onPatchButton(self):
    slicer.app.setOverrideCursor(qt.Qt.WaitCursor)
    try:
      import tempfile
      if not self.outputDirSelector.currentPath:
        self.outputDirSelector.currentPath =  tempfile.mkdtemp(prefix="DICOMPatcher-", dir=slicer.app.temporaryPath)

      self.inputDirSelector.addCurrentPathToHistory()
      self.outputDirSelector.addCurrentPathToHistory()
      self.statusLabel.plainText = ''

      self.logic.clearRules()
      if self.generateMissingIdsCheckBox.checked:
        self.logic.addRule("GenerateMissingIDs")
      self.logic.addRule("RemoveDICOMDIR")
      if self.generateImagePositionFromSliceThicknessCheckBox.checked:
        self.logic.addRule("AddMissingSliceSpacingToMultiframe")
      if self.anonymizeDicomCheckBox.checked:
        self.logic.addRule("Anonymize")
      if self.normalizeFileNamesCheckBox.checked:
        self.logic.addRule("NormalizeFileNames")
      self.logic.patchDicomDir(self.inputDirSelector.currentPath, self.outputDirSelector.currentPath)

    except Exception as e:
      self.addLog("Unexpected error: {0}".format(e.message))
      import traceback
      traceback.print_exc()
    slicer.app.restoreOverrideCursor();

  def onImportButton(self):
    slicer.app.setOverrideCursor(qt.Qt.WaitCursor)
    try:
      self.statusLabel.plainText = ''
      self.logic.importDicomDir(self.outputDirSelector.currentPath)
    except Exception as e:
      self.addLog("Unexpected error: {0}".format(e.message))
      import traceback
      traceback.print_exc()
    slicer.app.restoreOverrideCursor();


  def addLog(self, text):
    """Append text to log window
    """
    self.statusLabel.appendPlainText(text)
    slicer.app.processEvents() # force update

#
# Patcher rules
#

class DICOMPatcherRule(object):
  def __init__(self):
    self.logCallback = None
  def addLog(self, text):
    logging.info(text)
    if self.logCallback:
      self.logCallback(text)
  def processStart(self, inputRootDir, outputRootDir):
    pass
  def processDirectory(self, currentSubDir):
    pass
  def skipFile(self, filepath):
    return False
  def processDataSet(self, ds):
    pass
  def generateOutputFilePath(self, ds, filepath):
    return filepath

#
#
#

class GenerateMissingIDs(DICOMPatcherRule):
  def __init__(self):
    self.requiredTags = ['PatientName', 'PatientID', 'StudyInstanceUID', 'SeriesInstanceUID', 'SeriesNumber']
    self.eachFileIsSeparateSeries = False
  def processStart(self, inputRootDir, outputRootDir):
    import dicom
    self.patientIDToRandomIDMap = {}
    self.studyUIDToRandomUIDMap = {}
    self.seriesUIDToRandomUIDMap = {}
    self.numberOfSeriesInStudyMap = {}
    # All files without a patient ID will be assigned to the same patient
    self.randomPatientID = dicom.UID.generate_uid(None)
  def processDirectory(self, currentSubDir):
    import dicom
    # Assume that all files in a directory belongs to the same study
    self.randomStudyUID = dicom.UID.generate_uid(None)
    # Assume that all files in a directory belongs to the same series
    self.randomSeriesInstanceUID = dicom.UID.generate_uid(None)
  def processDataSet(self, ds):
    import dicom

    for tag in self.requiredTags:
      if not hasattr(ds,tag):
        setattr(ds,tag,'')

    # Generate a new SOPInstanceUID to avoid different files having the same SOPInstanceUID
    ds.SOPInstanceUID = dicom.UID.generate_uid(None)

    if ds.PatientName == '':
      ds.PatientName = "Unspecified Patient"
    if ds.PatientID == '':
      ds.PatientID = self.randomPatientID
    if ds.StudyInstanceUID == '':
      ds.StudyInstanceUID = self.randomStudyUID
    if ds.SeriesInstanceUID == '':
      if self.eachFileIsSeparateSeries:
        ds.SeriesInstanceUID = dicom.UID.generate_uid(None)
      else:
        ds.SeriesInstanceUID = self.randomSeriesInstanceUID

    # Generate series number to make it easier to identify a sequence within a study
    if ds.SeriesNumber == '':
      if ds.StudyInstanceUID not in self.numberOfSeriesInStudyMap:
        self.numberOfSeriesInStudyMap[ds.StudyInstanceUID] = 0
      self.numberOfSeriesInStudyMap[ds.StudyInstanceUID] = self.numberOfSeriesInStudyMap[ds.StudyInstanceUID] + 1
      ds.SeriesNumber = self.numberOfSeriesInStudyMap[ds.StudyInstanceUID]

#
#
#

class RemoveDICOMDIR(DICOMPatcherRule):
  def skipFile(self, filepath):
    if os.path.basename(filepath) != 'DICOMDIR':
      return False
    self.addLog('DICOMDIR file is ignored (its contents may be inconsistent with the contents of the indexed DICOM files, therefore it is safer not to use it)')
    return True

#
#
#

class AddMissingSliceSpacingToMultiframe(DICOMPatcherRule):
  """Add missing slice spacing info to multiframe files"""
  def processDataSet(self, ds):
    import dicom

    if not hasattr(ds,'NumberOfFrames'):
      return
    numberOfFrames = ds.NumberOfFrames
    if numberOfFrames <= 1:
      return

    # Multi-frame sequence, we may need to add slice positions

    # Error in Dolphin 3D CBCT scanners, they store multiple frames but they keep using CTImageStorage as storage class
    if ds.SOPClassUID == '1.2.840.10008.5.1.4.1.1.2': # Computed Tomography Image IOD
      ds.SOPClassUID = '1.2.840.10008.5.1.4.1.1.2.1' # Enhanced CT Image IOD

    sliceStartPosition = ds.ImagePositionPatient if hasattr(ds,'ImagePositionPatient') else [0,0,0]
    sliceAxes = ds.ImageOrientationPatient if hasattr(ds,'ImagePositionPatient') else [1,0,0,0,1,0]
    x = sliceAxes[:3]
    y = sliceAxes[3:]
    z = [x[1] * y[2] - x[2] * y[1], x[2] * y[0] - x[0] * y[2], x[0] * y[1] - x[1] * y[0]] # cross(x,y)
    sliceSpacing = ds.SliceThickness if hasattr(ds,'SliceThickness') else 1.0
    pixelSpacing = ds.PixelSpacing if hasattr(ds,'PixelSpacing') else [1.0, 1.0]

    if not (dicom.tag.Tag(0x5200,0x9229) in ds):

      # (5200,9229) SQ (Sequence with undefined length #=1)     # u/l, 1 SharedFunctionalGroupsSequence
      #   (0020,9116) SQ (Sequence with undefined length #=1)     # u/l, 1 PlaneOrientationSequence
      #       (0020,0037) DS [1.00000\0.00000\0.00000\0.00000\1.00000\0.00000] #  48, 6 ImageOrientationPatient
      #   (0028,9110) SQ (Sequence with undefined length #=1)     # u/l, 1 PixelMeasuresSequence
      #       (0018,0050) DS [3.00000]                                #   8, 1 SliceThickness
      #       (0028,0030) DS [0.597656\0.597656]                      #  18, 2 PixelSpacing

      planeOrientationDataSet = dicom.dataset.Dataset()
      planeOrientationDataSet.ImageOrientationPatient = sliceAxes
      planeOrientationSequence = dicom.sequence.Sequence()
      planeOrientationSequence.insert(dicom.tag.Tag(0x0020,0x9116),planeOrientationDataSet)

      pixelMeasuresDataSet = dicom.dataset.Dataset()
      pixelMeasuresDataSet.SliceThickness = sliceSpacing
      pixelMeasuresDataSet.PixelSpacing = pixelSpacing
      pixelMeasuresSequence = dicom.sequence.Sequence()
      pixelMeasuresSequence.insert(dicom.tag.Tag(0x0028,0x9110),pixelMeasuresDataSet)

      sharedFunctionalGroupsDataSet = dicom.dataset.Dataset()
      sharedFunctionalGroupsDataSet.PlaneOrientationSequence = planeOrientationSequence
      sharedFunctionalGroupsDataSet.PixelMeasuresSequence = pixelMeasuresSequence
      sharedFunctionalGroupsSequence = dicom.sequence.Sequence()
      sharedFunctionalGroupsSequence.insert(dicom.tag.Tag(0x5200,0x9229),sharedFunctionalGroupsDataSet)
      ds.SharedFunctionalGroupsSequence = sharedFunctionalGroupsSequence

    if not (dicom.tag.Tag(0x5200,0x9230) in ds):

      #(5200,9230) SQ (Sequence with undefined length #=54)    # u/l, 1 PerFrameFunctionalGroupsSequence
      #  (0020,9113) SQ (Sequence with undefined length #=1)     # u/l, 1 PlanePositionSequence
      #    (0020,0032) DS [-94.7012\-312.701\-806.500]             #  26, 3 ImagePositionPatient
      #  (0020,9113) SQ (Sequence with undefined length #=1)     # u/l, 1 PlanePositionSequence
      #    (0020,0032) DS [-94.7012\-312.701\-809.500]             #  26, 3 ImagePositionPatient
      #  ...

      perFrameFunctionalGroupsSequence = dicom.sequence.Sequence()

      for frameIndex in range(numberOfFrames):
        planePositionDataSet = dicom.dataset.Dataset()
        slicePosition = [
          sliceStartPosition[0]+frameIndex*z[0]*sliceSpacing,
          sliceStartPosition[1]+frameIndex*z[1]*sliceSpacing,
          sliceStartPosition[2]+frameIndex*z[2]*sliceSpacing]
        planePositionDataSet.ImagePositionPatient = slicePosition
        planePositionSequence = dicom.sequence.Sequence()
        planePositionSequence.insert(dicom.tag.Tag(0x0020,0x9113),planePositionDataSet)
        perFrameFunctionalGroupsDataSet = dicom.dataset.Dataset()
        perFrameFunctionalGroupsDataSet.PlanePositionSequence = planePositionSequence
        perFrameFunctionalGroupsSequence.insert(dicom.tag.Tag(0x5200,0x9230),perFrameFunctionalGroupsDataSet)

      ds.PerFrameFunctionalGroupsSequence = perFrameFunctionalGroupsSequence

#
#
#

class Anonymize(DICOMPatcherRule):
  def __init__(self):
    self.requiredTags = ['PatientName', 'PatientID', 'StudyInstanceUID', 'SeriesInstanceUID', 'SeriesNumber']
  def processStart(self, inputRootDir, outputRootDir):
    import dicom
    self.patientIDToRandomIDMap = {}
    self.studyUIDToRandomUIDMap = {}
    self.seriesUIDToRandomUIDMap = {}
    self.numberOfSeriesInStudyMap = {}
    # All files without a patient ID will be assigned to the same patient
    self.randomPatientID = dicom.UID.generate_uid(None)
  def processDirectory(self, currentSubDir):
    import dicom
    # Assume that all files in a directory belongs to the same study
    self.randomStudyUID = dicom.UID.generate_uid(None)
    # Assume that all files in a directory belongs to the same series
    self.randomSeriesInstanceUID = dicom.UID.generate_uid(None)
  def processDataSet(self, ds):
    import dicom

    ds.StudyDate = ''
    ds.StudyTime = ''
    ds.ContentDate = ''
    ds.ContentTime = ''
    ds.AccessionNumber = ''
    ds.ReferringPhysiciansName = ''
    ds.PatientsBirthDate = ''
    ds.PatientsSex = ''
    ds.StudyID = ''
    ds.PatientName = "Unspecified Patient"

    # replace ids with random values - re-use if we have seen them before
    if ds.PatientID not in self.patientIDToRandomIDMap:
      self.patientIDToRandomIDMap[ds.PatientID] = dicom.UID.generate_uid(None)
    ds.PatientID = self.patientIDToRandomIDMap[ds.PatientID]
    if ds.StudyInstanceUID not in self.studyUIDToRandomUIDMap:
      self.studyUIDToRandomUIDMap[ds.StudyInstanceUID] = dicom.UID.generate_uid(None)
    ds.StudyInstanceUID = self.studyUIDToRandomUIDMap[ds.StudyInstanceUID]
    if ds.SeriesInstanceUID not in self.seriesUIDToRandomUIDMap:
      self.seriesUIDToRandomUIDMap[ds.SeriesInstanceUID] = dicom.UID.generate_uid(None)
    ds.SeriesInstanceUID = self.seriesUIDToRandomUIDMap[ds.SeriesInstanceUID]

#
#
#

class NormalizeFileNames(DICOMPatcherRule):
  def processStart(self, inputRootDir, outputRootDir):
    self.inputRootDir = inputRootDir
    self.outputRootDir = outputRootDir
    self.patientNameIDToFolderMap = {}
    self.studyUIDToFolderMap = {}
    self.seriesUIDToFolderMap = {}
    # Number of files or folder in the specified folder
    self.numberOfItemsInFolderMap = {}
  def getNextItemName(self, prefix, root):
    numberOfFilesInFolder = self.numberOfItemsInFolderMap[root] if root in self.numberOfItemsInFolderMap else 0
    self.numberOfItemsInFolderMap[root] = numberOfFilesInFolder+1
    return "{0}{1:03d}".format(prefix, numberOfFilesInFolder)
  def generateOutputFilePath(self, ds, filepath):
    folderName = ""
    patientNameID = ds.PatientName+"*"+ds.PatientID
    if patientNameID not in self.patientNameIDToFolderMap:
      self.patientNameIDToFolderMap[patientNameID] = self.getNextItemName("pa", folderName)
    folderName += self.patientNameIDToFolderMap[patientNameID]
    if ds.StudyInstanceUID not in self.studyUIDToFolderMap:
      self.studyUIDToFolderMap[ds.StudyInstanceUID] = self.getNextItemName("st", folderName)
    folderName += "/" + self.studyUIDToFolderMap[ds.StudyInstanceUID]
    if ds.SeriesInstanceUID not in self.seriesUIDToFolderMap:
      self.seriesUIDToFolderMap[ds.SeriesInstanceUID] = self.getNextItemName("se", folderName)
    folderName += "/" +self.seriesUIDToFolderMap[ds.SeriesInstanceUID]
    prefix = ds.Modality.lower() if hasattr(ds, 'Modality') else ""
    filePath = self.outputRootDir + "/" + folderName + "/" + self.getNextItemName(prefix, folderName)+".dcm"
    return filePath

#
# DICOMPatcherLogic
#

class DICOMPatcherLogic(ScriptedLoadableModuleLogic):
  """This class should implement all the actual
  computation done by your module.  The interface
  should be such that other python code can import
  this class and make use of the functionality without
  requiring an instance of the Widget.
  Uses ScriptedLoadableModuleLogic base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def __init__(self):
    ScriptedLoadableModuleLogic.__init__(self)
    self.logCallback = None
    self.patchingRules = []

  def clearRules(self):
    self.patchingRules = []

  def addRule(self, ruleName):
    import importlib
    ruleModule = importlib.import_module("DICOMPatcher")
    ruleClass = getattr(ruleModule, ruleName)
    ruleInstance = ruleClass()
    self.patchingRules.append(ruleInstance)

  def addLog(self, text):
    logging.info(text)
    if self.logCallback:
      self.logCallback(text)

  def patchDicomDir(self, inputDirPath, outputDirPath):
    """
    Since CTK (rightly) requires certain basic information [1] before it can import
    data files that purport to be dicom, this code patches the files in a directory
    with some needed fields.  Apparently it is possible to export files from the
    Philips PMS QLAB system with these fields missing.

    Calling this function with a directory path will make a patched copy of each file.
    Importing the old files to CTK should still fail, but the new ones should work.

    The directory is assumed to have a set of instances that are all from the
    same study of the same patient.  Also that each instance (file) is an
    independent (multiframe) series.

    [1] https://github.com/commontk/CTK/blob/16aa09540dcb59c6eafde4d9a88dfee1f0948edc/Libs/DICOM/Core/ctkDICOMDatabase.cpp#L1283-L1287
    """

    import dicom

    self.addLog('DICOM patching started...')
    logging.debug('DICOM patch input directory: '+inputDirPath)
    logging.debug('DICOM patch output directory: '+outputDirPath)

    for rule in self.patchingRules:
      rule.logCallback = self.addLog
      rule.processStart(inputDirPath, outputDirPath)

    for root, subFolders, files in os.walk(inputDirPath):

      currentSubDir = os.path.relpath(root, inputDirPath)
      rootOutput = os.path.join(outputDirPath, currentSubDir)

      # Notify rules that processing of a new subdirectory started
      for rule in self.patchingRules:
        rule.processDirectory(currentSubDir)

      for file in files:
        filePath = os.path.join(root,file)
        self.addLog('Examining %s...' % os.path.join(currentSubDir,file))

        skipFileRequestingRule = None
        for rule in self.patchingRules:
          if rule.skipFile(currentSubDir):
            skipFileRequestingRule = rule
            break
        if skipFileRequestingRule:
          self.addLog('  Rule '+rule.__class__.__name__+' requested to skip this file.')
          continue

        try:
          ds = dicom.read_file(filePath)
        except (IOError, dicom.filereader.InvalidDicomError):
          self.addLog('  Not DICOM file. Skipped.')
          continue

        self.addLog('  Patching...')

        for rule in self.patchingRules:
          rule.processDataSet(ds)

        patchedFilePath = os.path.abspath(os.path.join(rootOutput,file))
        for rule in self.patchingRules:
          patchedFilePath = rule.generateOutputFilePath(patchedFilePath, ds)

        ######################################################
        # Write

        dirName = os.path.dirname(patchedFilePath)
        if not os.path.exists(dirName):
          os.makedirs(dirName)

        self.addLog('  Writing DICOM...')
        dicom.write_file(patchedFilePath, ds)
        self.addLog('  Created DICOM file: %s' % patchedFilePath)

    self.addLog('DICOM patching completed. Patched files are written to:\n{0}'.format(outputDirPath))

  def importDicomDir(self, outputDirPath):
    """
    Utility function to import DICOM files from a directory
    """
    self.addLog('Directory: '+outputDirPath)
    self.addLog('DICOM importing started...')
    dicomWidget = slicer.modules.dicom.widgetRepresentation().self()
    indexer = ctk.ctkDICOMIndexer()
    indexer.addDirectory( slicer.dicomDatabase, outputDirPath )
    self.addLog('DICOM importing completed.')

#
# Test
#

class DICOMPatcherTest(ScriptedLoadableModuleTest):
  """
  This is the test case for your scripted module.
  Uses ScriptedLoadableModuleTest base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def setUp(self):
    """ Do whatever is needed to reset the state - typically a scene clear will be enough.
    """
    slicer.mrmlScene.Clear(0)

  def runTest(self):
    """Run as few or as many tests as needed here.
    """
    self.setUp()
    self.test_DICOMPatcher1()

  def test_DICOMPatcher1(self):
    """ Ideally you should have several levels of tests.  At the lowest level
    tests should exercise the functionality of the logic with different inputs
    (both valid and invalid).  At higher levels your tests should emulate the
    way the user would interact with your code and confirm that it still works
    the way you intended.
    One of the most important features of the tests is that it should alert other
    developers when their changes will have an impact on the behavior of your
    module.  For example, if a developer removes a feature that you depend on,
    your test should break so they know that the feature is needed.
    """

    import tempfile
    testDir = tempfile.mkdtemp(prefix="DICOMPatcherTest-", dir=slicer.app.temporaryPath)
    self.assertTrue(os.path.isdir(testDir))

    inputTestDir = testDir+"/input"
    os.makedirs(inputTestDir)
    outputTestDir = testDir+"/output"
    self.delayDisplay('Created test directory: '+testDir)

    self.delayDisplay("Generate test files")

    testFileNonDICOM = open(inputTestDir+"/NonDICOMFile.txt", "w")
    testFileNonDICOM.write("This is not a DICOM file")
    testFileNonDICOM.close()

    testFileDICOMFilename = inputTestDir+"/DICOMFile.dcm"
    self.delayDisplay('Writing test file: '+testFileDICOMFilename)
    import dicom
    file_meta = dicom.dataset.Dataset()
    file_meta.MediaStorageSOPClassUID = '1.2.840.10008.5.1.4.1.1.2'  # CT Image Storage
    file_meta.MediaStorageSOPInstanceUID = "1.2.3"  # !! Need valid UID here for real work
    file_meta.ImplementationClassUID = "1.2.3.4"  # !!! Need valid UIDs here
    ds = dicom.dataset.FileDataset(testFileDICOMFilename, {}, file_meta=file_meta, preamble="\0" * 128)
    ds.PatientName = "Test^Firstname"
    ds.PatientID = "123456"
    # Set the transfer syntax
    ds.is_little_endian = True
    ds.is_implicit_VR = True
    ds.save_as(testFileDICOMFilename)

    self.delayDisplay("Patch input files")

    logic = DICOMPatcherLogic()
    logic.addRule("GenerateMissingIDs")
    logic.addRule("RemoveDICOMDIR")
    logic.addRule("AddMissingSliceSpacingToMultiframe")
    logic.addRule("Anonymize")
    logic.addRule("NormalizeFileNames")
    logic.patchDicomDir(inputTestDir, outputTestDir)

    self.delayDisplay("Verify generated files")

    expectedWalk = []
    expectedWalk.append([[u'pa000'], [          ]])
    expectedWalk.append([[u'st000'], [          ]])
    expectedWalk.append([[u'se000'], [          ]])
    expectedWalk.append([[        ], [u'000.dcm']])
    step = 0
    for root, subFolders, files in os.walk(outputTestDir):
      self.assertEqual(subFolders, expectedWalk[step][0])
      self.assertEqual(files, expectedWalk[step][1])
      step += 1

    self.delayDisplay("Clean up")

    import shutil
    shutil.rmtree(testDir)
