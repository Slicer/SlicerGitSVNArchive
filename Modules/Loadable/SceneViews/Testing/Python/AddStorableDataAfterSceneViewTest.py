import os
import unittest
from __main__ import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *
import logging

#
# AddStorableDataAfterSceneViewTest
#

class AddStorableDataAfterSceneViewTest(ScriptedLoadableModule):
  """Uses ScriptedLoadableModule base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def __init__(self, parent):
    ScriptedLoadableModule.__init__(self, parent)
    self.parent.title = "Add Storable Data After Scene View Test"
    self.parent.categories = ["Testing.TestCases"]
    self.parent.dependencies = []
    self.parent.contributors = ["Nicole Aucoin (BWH)"]
    self.parent.helpText = """
    This self test adds some data, creates a scene view, then adds more storable data.
    It tests Slicer's functionality after the scene view is restored, is the new storable
    node still present? With the current implementation it only passes if the new storable
    node is NOT present.
    """
    self.parent.acknowledgementText = """
    This file was originally developed by Nicole Aucoin, BWH, and was partially funded by NIH grant 3P41RR013218-12S1.
"""

#
# qAddStorableDataAfterSceneViewTestWidget
#

class AddStorableDataAfterSceneViewTestWidget(ScriptedLoadableModuleWidget):
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

    #
    # check box to trigger taking screen shots for later use in tutorials
    #
    self.enableScreenshotsFlagCheckBox = qt.QCheckBox()
    self.enableScreenshotsFlagCheckBox.checked = 0
    self.enableScreenshotsFlagCheckBox.setToolTip("If checked, take screen shots for tutorials. Use Save Data to write them to disk.")
    parametersFormLayout.addRow("Enable Screenshots", self.enableScreenshotsFlagCheckBox)

    #
    # Apply Button
    #
    self.applyButton = qt.QPushButton("Apply")
    self.applyButton.toolTip = "Run the test."
    self.applyButton.enabled = True
    parametersFormLayout.addRow(self.applyButton)

    # connections
    self.applyButton.connect('clicked(bool)', self.onApplyButton)

    # Add vertical spacer
    self.layout.addStretch(1)

  def cleanup(self):
    pass

  def onApplyButton(self):
    logic = AddStorableDataAfterSceneViewTestLogic()
    enableScreenshotsFlag = self.enableScreenshotsFlagCheckBox.checked
    logic.run(enableScreenshotsFlag)

#
# AddStorableDataAfterSceneViewTestLogic
#

class AddStorableDataAfterSceneViewTestLogic(ScriptedLoadableModuleLogic):
  """
  Uses ScriptedLoadableModuleLogic base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def takeScreenshot(self,name,description,type=-1):
    # show the message even if not taking a screen shot
    slicer.util.delayDisplay('Take screenshot: '+description+'.\nResult is available in the Annotations module.', 3000)

    lm = slicer.app.layoutManager()
    # switch on the type to get the requested window
    widget = 0
    if type == slicer.qMRMLScreenShotDialog.FullLayout:
      # full layout
      widget = lm.viewport()
    elif type == slicer.qMRMLScreenShotDialog.ThreeD:
      # just the 3D window
      widget = lm.threeDWidget(0).threeDView()
    elif type == slicer.qMRMLScreenShotDialog.Red:
      # red slice window
      widget = lm.sliceWidget("Red")
    elif type == slicer.qMRMLScreenShotDialog.Yellow:
      # yellow slice window
      widget = lm.sliceWidget("Yellow")
    elif type == slicer.qMRMLScreenShotDialog.Green:
      # green slice window
      widget = lm.sliceWidget("Green")
    else:
      # default to using the full window
      widget = slicer.util.mainWindow()
      # reset the type so that the node is set correctly
      type = slicer.qMRMLScreenShotDialog.FullLayout

    # grab and convert to vtk image data
    qpixMap = qt.QPixmap().grabWidget(widget)
    qimage = qpixMap.toImage()
    imageData = vtk.vtkImageData()
    slicer.qMRMLUtils().qImageToVtkImageData(qimage,imageData)

    annotationLogic = slicer.modules.annotations.logic()
    annotationLogic.CreateSnapShot(name, description, type, 1, imageData)

  def run(self, enableScreenshots=0):
    """
    Run the test via GUI
    """

    logging.info('Test started from GUI')

    try:
      evalString = 'AddStorableDataAfterSceneViewTestTest()'
      tester = eval(evalString)
      tester.runTest()
    except Exception, e:
      import traceback
      traceback.print_exc()
      errorMessage = "Add storable data after scene view test: Exception!\n\n" + str(e) + "\n\nSee Python Console for Stack Trace"
      slicer.util.errorDisplay(errorMessage)

    # Capture screenshot
    if enableScreenshots:
      self.takeScreenshot('AddStorableDataAfterSceneViewTest-Start','MyScreenshot',-1)

    logging.info('Processing completed')

    return True


class AddStorableDataAfterSceneViewTestTest(ScriptedLoadableModuleTest):
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
    self.test_AddStorableDataAfterSceneViewTest1()

  def test_AddStorableDataAfterSceneViewTest1(self):

    self.delayDisplay("Starting the test")

    #
    # add a fiducial
    #
    slicer.modules.markups.logic().AddFiducial()

    #
    # save a scene view
    #
    sv = slicer.mrmlScene.AddNode(slicer.vtkMRMLSceneViewNode())
    sv.StoreScene()

    #
    # add another storable node, a volume
    #
    self.delayDisplay("Adding a new storable node, after creating a scene view")
    import SampleData
    sampleDataLogic = SampleData.SampleDataLogic()
    mrHeadVolume = sampleDataLogic.downloadMRHead()
    mrHeadID = mrHeadVolume.GetID()

    #
    # restore the scene view
    #
    self.delayDisplay("Restoring the scene view")
    sv.RestoreScene()

    #
    # Is the new storable data still present?
    #
    restoredData = slicer.mrmlScene.GetNodeByID(mrHeadID)

    # for now, the non scene view storable data is removed
    self.assertTrue( restoredData == None )
    self.delayDisplay('Success: extra storable node removed with scene view restore')

    self.delayDisplay('Test passed!')
