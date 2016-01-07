import vtk, qt, ctk, slicer

from slicer.ScriptedLoadableModule import *

#
# VectorToScalarVolume
#

class VectorToScalarVolume(ScriptedLoadableModule):
  def __init__(self, parent):
    ScriptedLoadableModule.__init__(self, parent)
    self.parent.title = "Vector to Scalar Volume"
    self.parent.categories = ["Converters"]
    self.parent.dependencies = []
    self.parent.contributors = ["Steve Pieper (Isomics)",]
    self.parent.helpText = """
    Make a scalar (1 component) volume from a vector volume
    """
    self.parent.acknowledgementText = """
Developed by Steve Pieper, Isomics, Inc.,
partially funded by NIH grant 3P41RR013218-12S1 (NAC) and is part of the National Alliance
for Medical Image Computing (NA-MIC), funded by the National Institutes of Health through the
NIH Roadmap for Medical Research, Grant U54 EB005149."""

#
# VectorToScalarVolumeWidget
#

class VectorToScalarVolumeWidget(ScriptedLoadableModuleWidget):

  def setup(self):
    ScriptedLoadableModuleWidget.setup(self)

    # Collapsible button
    self.selectionCollapsibleButton = ctk.ctkCollapsibleButton()
    self.selectionCollapsibleButton.text = "Selection"
    self.layout.addWidget(self.selectionCollapsibleButton)

    # Layout within the collapsible button
    self.formLayout = qt.QFormLayout(self.selectionCollapsibleButton)

    #
    # the volume selectors
    #
    self.inputFrame = qt.QFrame(self.selectionCollapsibleButton)
    self.inputFrame.setLayout(qt.QHBoxLayout())
    self.formLayout.addWidget(self.inputFrame)
    self.inputSelector = qt.QLabel("Input Vector Volume: ", self.inputFrame)
    self.inputFrame.layout().addWidget(self.inputSelector)
    self.inputSelector = slicer.qMRMLNodeComboBox(self.inputFrame)
    self.inputSelector.nodeTypes = ["vtkMRMLVectorVolumeNode"]
    self.inputSelector.addEnabled = False
    self.inputSelector.removeEnabled = False
    self.inputSelector.setMRMLScene( slicer.mrmlScene )
    self.inputFrame.layout().addWidget(self.inputSelector)

    self.outputFrame = qt.QFrame(self.selectionCollapsibleButton)
    self.outputFrame.setLayout(qt.QHBoxLayout())
    self.formLayout.addWidget(self.outputFrame)
    self.outputSelector = qt.QLabel("Output Scalar Volume: ", self.outputFrame)
    self.outputFrame.layout().addWidget(self.outputSelector)
    self.outputSelector = slicer.qMRMLNodeComboBox(self.outputFrame)
    self.outputSelector.nodeTypes = ["vtkMRMLScalarVolumeNode"]
    self.outputSelector.setMRMLScene( slicer.mrmlScene )
    self.outputSelector.addEnabled = True
    self.outputSelector.renameEnabled = True
    self.outputSelector.baseName = "Scalar Volume"
    self.outputFrame.layout().addWidget(self.outputSelector)

    # Apply button
    self.applyButton = qt.QPushButton("Apply")
    self.applyButton.toolTip = "Run Convert the vector to scalar."
    self.formLayout.addWidget(self.applyButton)
    self.applyButton.connect('clicked(bool)', self.onApply)

    # Add vertical spacer
    self.layout.addStretch(1)

  def onApply(self):
    inputVolume = self.inputSelector.currentNode()
    outputVolume = self.outputSelector.currentNode()
    # check for input data
    if not (inputVolume and outputVolume):
      slicer.util.errorDisplay('Input and output volumes are required for conversion', windowTitle='Luminance')
      return
    # check that data has enough components
    inputImage = inputVolume.GetImageData()
    if not inputImage or inputImage.GetNumberOfScalarComponents() < 3:
      slicer.util.errorDisplay('Input does not have enough components for conversion',
                               windowTitle='Vector to Scalar Volume')
      return
    # run the filter
    # - extract the RGB portions
    extract = vtk.vtkImageExtractComponents()
    extract.SetComponents(0,1,2)
    luminance = vtk.vtkImageLuminance()
    extract.SetInputConnection(inputVolume.GetImageDataConnection())
    luminance.SetInputConnection(extract.GetOutputPort())
    luminance.Update()
    ijkToRAS = vtk.vtkMatrix4x4()
    inputVolume.GetIJKToRASMatrix(ijkToRAS)
    outputVolume.SetIJKToRASMatrix(ijkToRAS)
    outputVolume.SetImageDataConnection(luminance.GetOutputPort())
    # make the output volume appear in all the slice views
    selectionNode = slicer.app.applicationLogic().GetSelectionNode()
    selectionNode.SetReferenceActiveVolumeID(outputVolume.GetID())
    slicer.app.applicationLogic().PropagateVolumeSelection(0)
