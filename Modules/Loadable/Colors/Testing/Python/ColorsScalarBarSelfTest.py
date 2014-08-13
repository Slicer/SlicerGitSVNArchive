import os
import time
import unittest
from __main__ import vtk, qt, ctk, slicer

#
# ColorsScalarBarSelfTest
#

class ColorsScalarBarSelfTest:
  def __init__(self, parent):
    parent.title = "ColorsScalarBarSelfTest"
    parent.categories = ["Testing.TestCases"]
    parent.dependencies = []
    parent.contributors = ["Kevin Wang (PMH)"]
    parent.helpText = """
    This is a test case that test the new vtkSlicerScalarBarActor class.
    """
    parent.acknowledgementText = """
    This file was originally developed by Kevin Wang, PMH and was funded by CCO and OCAIRO.
""" # replace with organization, grant and thanks.
    self.parent = parent

    # Add this test to the SelfTest module's list for discovery when the module
    # is created.  Since this module may be discovered before SelfTests itself,
    # create the list if it doesn't already exist.
    try:
      slicer.selfTests
    except AttributeError:
      slicer.selfTests = {}
    slicer.selfTests['ColorsScalarBarSelfTestTest'] = self.runTest

  def runTest(self):
    tester =ColorsScalarBarSelfTestTest()
    tester.runTest()

#
# ColorsScalarBarSelfTestWidget
#

class ColorsScalarBarSelfTestWidget:
  def __init__(self, parent = None):
    if not parent:
      self.parent = slicer.qMRMLWidget()
      self.parent.setLayout(qt.QVBoxLayout())
      self.parent.setMRMLScene(slicer.mrmlScene)
    else:
      self.parent = parent
    self.layout = self.parent.layout()
    if not parent:
      self.setup()
      self.parent.show()

  def setup(self):
    # Instantiate and connect widgets ...

    #
    # Reload and Test area
    #
    reloadCollapsibleButton = ctk.ctkCollapsibleButton()
    reloadCollapsibleButton.text = "Reload && Test"
    self.layout.addWidget(reloadCollapsibleButton)
    reloadFormLayout = qt.QFormLayout(reloadCollapsibleButton)

    # reload button
    # (use this during development, but remove it when delivering
    #  your module to users)
    self.reloadButton = qt.QPushButton("Reload")
    self.reloadButton.toolTip = "Reload this module."
    reloadFormLayout.addWidget(self.reloadButton)
    self.reloadButton.connect('clicked()', self.onReload)

    # reload and test button
    # (use this during development, but remove it when delivering
    #  your module to users)
    self.reloadAndTestButton = qt.QPushButton("Reload and Test")
    self.reloadAndTestButton.toolTip = "Reload this module and then run the self tests."
    reloadFormLayout.addWidget(self.reloadAndTestButton)
    self.reloadAndTestButton.connect('clicked()', self.onReloadAndTest)

    #
    # Parameters Area
    #
    parametersCollapsibleButton = ctk.ctkCollapsibleButton()
    parametersCollapsibleButton.text = "Parameters"
    self.layout.addWidget(parametersCollapsibleButton)

    # Layout within the dummy collapsible button
    parametersFormLayout = qt.QFormLayout(parametersCollapsibleButton)

    # Apply Button
    #
    self.applyButton = qt.QPushButton("Apply")
    self.applyButton.toolTip = "Run the algorithm."
    self.applyButton.enabled = True
    parametersFormLayout.addRow(self.applyButton)

    # connections
    self.applyButton.connect('clicked(bool)', self.onApplyButton)

    # Add vertical spacer
    self.layout.addStretch(1)

  def cleanup(self):
    pass

  def onApplyButton(self):
    logic =ColorsScalarBarSelfTestLogic()
    print("Run the test algorithm")
    logic.run()

  def onReload(self,moduleName="ColorsScalarBarSelfTest"):
    """Generic reload method for any scripted module.
    ModuleWizard will subsitute correct default moduleName.
    """
    globals()[moduleName] = slicer.util.reloadScriptedModule(moduleName)

  def onReloadAndTest(self,moduleName="ColorsScalarBarSelfTest"):
    try:
      self.onReload()
      evalString = 'globals()["%s"].%sTest()' % (moduleName, moduleName)
      tester = eval(evalString)
      tester.runTest()
    except Exception, e:
      import traceback
      traceback.print_exc()
      qt.QMessageBox.warning(slicer.util.mainWindow(),
          "Reload and Test", 'Exception!\n\n' + str(e) + "\n\nSee Python Console for Stack Trace")


#
#ColorsScalarBarSelfTestLogic
#

class ColorsScalarBarSelfTestLogic:

  def __init__(self):
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

  def run(self):
    """
    Run the actual algorithm
    """
    # start in the colors module
    m = slicer.util.mainWindow()
    m.moduleSelector().selectModule('Colors')
    self.delayDisplay('In Colors module')

    colorWidget = slicer.modules.colors.widgetRepresentation()
    ctkScalarBarWidget = slicer.util.findChildren(colorWidget, name='VTKScalarBar')[0]
    # show the scalar bar widget
    ctkScalarBarWidget.setDisplay(1)
    activeColorNodeSelector = slicer.util.findChildren(colorWidget, 'ColorTableComboBox')[0]
    useColorNameAsLabelCheckbox = slicer.util.findChildren(colorWidget, 'UseColorNameAsLabelCheckBox')[0]
    checked = useColorNameAsLabelCheckbox.isChecked()
    # iterate over the color nodes and set each one active
    numColorNodes = slicer.mrmlScene.GetNumberOfNodesByClass('vtkMRMLColorNode')
    for n in range(numColorNodes):
      colorNode = slicer.mrmlScene.GetNthNodeByClass(n, 'vtkMRMLColorNode')
      useColorNameAsLabelCheckbox.setChecked(checked)
      print("%d/%d" % (n, numColorNodes-1))
      self.delayDisplay('Setting Color Node To %s' % colorNode.GetName(), 100)
      activeColorNodeSelector.setCurrentNodeID(colorNode.GetID())
      # use the delay display here to ensure a render
      self.delayDisplay('Set Color Node To %s' % colorNode.GetName(), 500)
      useColorNameAsLabelCheckbox.setChecked(not checked)
      self.delayDisplay('Toggled using names as labels', 500)

    return True


class ColorsScalarBarSelfTestTest(unittest.TestCase):
  """
  This is the test case for your scripted module.
  """

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

  def setUp(self):
    """ Do whatever is needed to reset the state - typically a scene clear will be enough.
    """
    slicer.mrmlScene.Clear(0)

  def runTest(self):
    """Run as few or as many tests as needed here.
    """
    self.setUp()
    self.test_ColorsScalarBarSelfTest1()

  def test_ColorsScalarBarSelfTest1(self):

    self.delayDisplay("Starting the scalarbar test")

    logic =ColorsScalarBarSelfTestLogic()
    logic.run()

    self.delayDisplay('Test passed!')
