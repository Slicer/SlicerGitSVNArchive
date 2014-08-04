import os, glob, sys
from __main__ import qt
from __main__ import vtk
from __main__ import ctk
from __main__ import slicer

import DataProbeLib
import DataProbeUtil

class SliceAnnotations(object):
  """Implement the Qt window showing settings for Slice View Annotations
  """
  def __init__(self):

    self.dataProbeUtil = DataProbeUtil.DataProbeUtil()

    self.sliceViewNames = []
    self.popupGeometry = qt.QRect()
    self.cornerTexts =[]
    # Bottom Left Corner Text
    self.cornerTexts.append({
      '1-Label':{'text':'','category':'A'},
      '2-Foreground':{'text':'','category':'A'},
      '3-Background':{'text':'','category':'A'}
      })
    # Bottom Rihgt Corner Text
    # Not used - orientation figure will be draw there
    self.cornerTexts.append({
      '1-TR':{'text':'','category':'A'},
      '2-TE':{'text':'','category':'A'}
      })
    # Top Left Corner Text
    self.cornerTexts.append({
      '1-PatientName':{'text':'','category':'B'},
      '2-PatientID':{'text':'','category':'A'},
      '3-PatientInfo':{'text':'','category':'B'},
      '4-Bg-StudyDate':{'text':'','category':'B'},
      '5-Fg-StudyDate':{'text':'','category':'B'},
      '6-Bg-StudyTime':{'text':'','category':'C'},
      '7-Bg-StudyTime':{'text':'','category':'C'},
      '8-Bg-SeriesDescription':{'text':'','category':'C'},
      '9-Fg-SeriesDescription':{'text':'','category':'C'}
      })
    # Top Rihgt Corner Text
    self.cornerTexts.append({
      '1-Institution-Name':{'text':'','category':'B'},
      '2-Referring-Phisycian':{'text':'','category':'B'},
      '3-Manufacturer':{'text':'','category':'C'},
      '4-Model':{'text':'','category':'C'},
      '5-Patient-Position':{'text':'','category':'A'},
      '6-TR':{'text':'','category':'A'},
      '7-TE':{'text':'','category':'A'}
      })

    self.layoutManager = slicer.app.layoutManager()
    self.sliceCornerAnnotations = {}

    # Check for user settings
    # Load user settings

    # If there is no user settings load defualts

    self.annotationsDisplayAmount = 0

    self.topLeftAnnotationDisplay = 1
    self.topRightAnnotationDisplay = 1
    self.bottomLeftAnnotationDisplay = 1

    settings = qt.QSettings()
    if settings.contains('DataProbe/sliceViewAnnotations.show'):
      self.showSliceViewAnnotations= int(settings.value(
          'DataProbe/sliceViewAnnotations.show'))
    else:
      self.showSliceViewAnnotations = 1
    self.showCornerTextAnnotations = 1

    if settings.contains('DataProbe/sliceViewAnnotations.showScalingRuler'):
      self.showScalingRuler= int(settings.value(
          'DataProbe/sliceViewAnnotations.showScalingRuler'))
    else:
      self.showScalingRuler = 1

    if settings.contains('DataProbe/sliceViewAnnotations.showColorScalarBar'):
      self.showColorScalarBar= int(settings.value(
          'DataProbe/sliceViewAnnotations.showColorScalarBar'))
    else:
      self.showColorScalarBar = 1

    if settings.contains('DataProbe/sliceViewAnnotations.showColorScalarBar'):
      self.fontFamily = settings.value('DataProbe/sliceViewAnnotations.fontFamily')
    else:
      self.fontFamily = 'Times'

    if settings.contains('DataProbe/sliceViewAnnotations.showColorScalarBar'):
      self.fontSize = int(settings.value('DataProbe/sliceViewAnnotations.fontSize'))
    else:
      self.fontSize = 14
    self.maximumTextLength= 35

    self.parameter = 'showSliceViewAnnotations'
    self.parameterNode = self.dataProbeUtil.getParameterNode()
    self.parameterNodeTag = self.parameterNode.AddObserver(
        vtk.vtkCommand.ModifiedEvent, self.updateGUIFromMRML)
    #outputColor = int(parameterNode.GetParameter("ChangeLabelEffect,outputColor"))

    self.colorbarSelectedLayer = 'background'
    self.create()
    self.updateSliceViewFromGUI()

  def create(self):
    # Instantiate and connect widgets ...

    self.window = qt.QWidget()
    self.window.setWindowTitle('Slice View Annotations Settings')
    self.layout = qt.QVBoxLayout(self.window)

    #
    # Show Annotations Checkbox
    #
    self.sliceViewAnnotationsCheckBox = qt.QCheckBox('Slice View Annotations')
    self.layout.addWidget(self.sliceViewAnnotationsCheckBox)
    self.sliceViewAnnotationsCheckBox.checked = self.showSliceViewAnnotations

    #
    # Corner Text Parameters Area
    #
    #
    self.cornerTextParametersCollapsibleButton = ctk.ctkCollapsibleButton()
    self.cornerTextParametersCollapsibleButton.enabled = False
    self.cornerTextParametersCollapsibleButton.text = "Corner Text Annotation"
    self.layout.addWidget(self.cornerTextParametersCollapsibleButton)

    # Layout within the dummy collapsible button
    parametersFormLayout = qt.QFormLayout(self.cornerTextParametersCollapsibleButton)

    # Show Corner Text Annotations Checkbox
    #
    self.cornerTextAnnotationsCheckBox = qt.QCheckBox('Enable')
    #parametersFormLayout.addRow(self.cornerTextAnnotationsCheckBox)
    self.cornerTextAnnotationsCheckBox.checked = self.showCornerTextAnnotations

    #
    # Corner Annotations Activation Checkboxes
    #
    self.cornerActivationsGroupBox = ctk.ctkCollapsibleGroupBox()
    self.cornerActivationsGroupBox.setTitle('Active Corners')
    self.cornerActivationsGroupBox.enabled = False
    parametersFormLayout.addRow(self.cornerActivationsGroupBox)
    cornerActionHBoxLayout = qt.QHBoxLayout(self.cornerActivationsGroupBox)

    self.cornerActivationCheckbox = []

    for i in xrange(3):
      self.cornerActivationCheckbox.append(qt.QCheckBox())
      self.cornerActivationCheckbox[i].checked = True
      cornerActionHBoxLayout.addWidget(self.cornerActivationCheckbox[i])
      self.cornerActivationCheckbox[i].connect('clicked()', self.updateSliceViewFromGUI)

    self.cornerActivationCheckbox[0].setText('Top Left')
    self.cornerActivationCheckbox[1].setText('Top Right')
    self.cornerActivationCheckbox[2].setText('Bottom Left')

    #
    # Corner Annotations Font Properties
    #
    self.annotationsAmountGroupBox = ctk.ctkCollapsibleGroupBox()
    self.annotationsAmountGroupBox.setTitle('Amount')
    self.annotationsAmountGroupBox.enabled = False
    parametersFormLayout.addRow(self.annotationsAmountGroupBox)
    annotationsAmountHBoxLayout = qt.QHBoxLayout(self.annotationsAmountGroupBox)

    amountLabel = qt.QLabel('Annotation Display Amount: ')
    annotationsAmountHBoxLayout.addWidget(amountLabel)
    self.level1RadioButton = qt.QRadioButton('level 1')
    annotationsAmountHBoxLayout.addWidget(self.level1RadioButton)
    self.level1RadioButton.connect('clicked()', self.updateSliceViewFromGUI)
    self.level1RadioButton.checked = True
    self.level2RadioButton = qt.QRadioButton('level 2')
    annotationsAmountHBoxLayout.addWidget(self.level2RadioButton)
    self.level2RadioButton.connect('clicked()', self.updateSliceViewFromGUI)
    self.level3RadioButton = qt.QRadioButton('level 3')
    annotationsAmountHBoxLayout.addWidget(self.level3RadioButton)
    self.level3RadioButton.connect('clicked()', self.updateSliceViewFromGUI)

    #
    # Corner Annotations Font Properties
    #
    self.fontPropertiesGroupBox = ctk.ctkCollapsibleGroupBox()
    self.fontPropertiesGroupBox.setTitle('Font Properties')
    self.fontPropertiesGroupBox.enabled = False
    parametersFormLayout.addRow(self.fontPropertiesGroupBox)
    fontPropertiesHBoxLayout = qt.QHBoxLayout(self.fontPropertiesGroupBox)

    fontFamilyLabel = qt.QLabel('Font Family: ')
    fontPropertiesHBoxLayout.addWidget(fontFamilyLabel)
    self.timesFontRadioButton = qt.QRadioButton('Times')
    fontPropertiesHBoxLayout.addWidget(self.timesFontRadioButton)
    self.timesFontRadioButton.connect('clicked()', self.onFontFamilyRadioButton)

    self.arialFontRadioButton = qt.QRadioButton('Arial')
    self.arialFontRadioButton.connect('clicked()', self.onFontFamilyRadioButton)
    fontPropertiesHBoxLayout.addWidget(self.arialFontRadioButton)

    if self.fontFamily == 'Times':
      self.timesFontRadioButton.checked = True
    else:
      self.arialFontRadioButton.checked = True

    fontSizeLabel = qt.QLabel('Font Size: ')
    fontPropertiesHBoxLayout.addWidget(fontSizeLabel)
    self.fontSizeSpinBox = qt.QSpinBox()
    self.fontSizeSpinBox.setMinimum(10)
    self.fontSizeSpinBox.setMaximum(20)
    self.fontSizeSpinBox.value = self.fontSize
    fontPropertiesHBoxLayout.addWidget(self.fontSizeSpinBox)
    self.fontSizeSpinBox.connect('valueChanged(int)', self.onFontSizeSpinBox)

    maximumTextLengthLabel = qt.QLabel('Maximum Text Length: ')
    self.textLengthSpinBox = qt.QSpinBox()
    self.textLengthSpinBox.setMinimum(1)
    self.textLengthSpinBox.setMaximum(100)
    self.textLengthSpinBox.value = self.maximumTextLength
    parametersFormLayout.addRow('Maximum Text Length: ', self.textLengthSpinBox)
    self.textLengthSpinBox.connect('valueChanged(int)', self.onTextLengthSpinBox)

    #
    # Scaling Ruler Area
    #
    self.scalingRulerCollapsibleButton = ctk.ctkCollapsibleButton()
    self.scalingRulerCollapsibleButton.enabled = False
    self.scalingRulerCollapsibleButton.text = "Scaling Ruler"
    self.layout.addWidget(self.scalingRulerCollapsibleButton)

    # Layout within the dummy collapsible button
    scalingRulerFormLayout = qt.QFormLayout(self.scalingRulerCollapsibleButton)

    #
    # Scaling Ruler Activation Checkbox
    #
    self.showScalingRulerCheckBox = qt.QCheckBox('Enable')
    scalingRulerFormLayout.addRow(self.showScalingRulerCheckBox)
    self.showScalingRulerCheckBox.checked = self.showScalingRuler

    #
    # Color Scalar Bar Area
    #
    self.colorScalarBarCollapsibleButton = ctk.ctkCollapsibleButton()
    self.colorScalarBarCollapsibleButton.enabled = False
    self.colorScalarBarCollapsibleButton.text = "Color Bar"
    self.layout.addWidget(self.colorScalarBarCollapsibleButton)

    # Layout within the dummy collapsible button
    colorScalarBarFormLayout = qt.QFormLayout(self.colorScalarBarCollapsibleButton)

    #
    # Color Scalar Bar Activation Checkbox
    #
    self.showColorScalarBarCheckBox = qt.QCheckBox('Enable')
    colorScalarBarFormLayout.addRow(self.showColorScalarBarCheckBox)
    self.showColorScalarBarCheckBox.checked = self.showColorScalarBar

    #
    # ColorBar Layer selection group box (background/foreground)
    #
    self.colorBarLayerSelectionGroupBox = ctk.ctkCollapsibleGroupBox()
    self.colorBarLayerSelectionGroupBox.setTitle('Colorbar Layer Selection')
    self.colorBarLayerSelectionGroupBox.enabled = False
    '''
    This is not used here as there is only no method in vtkMRMLSliceLogic
    to get Foreground window level and range.
    '''
    #colorScalarBarFormLayout.addRow(self.colorBarLayerSelectionGroupBox )
    layerSelectionHBoxLayout = qt.QHBoxLayout(self.colorBarLayerSelectionGroupBox)

    layerLabel = qt.QLabel('Layer: ')
    layerSelectionHBoxLayout.addWidget(fontFamilyLabel)
    self.backgroundRadioButton = qt.QRadioButton('Background')
    layerSelectionHBoxLayout.addWidget(self.backgroundRadioButton)
    self.backgroundRadioButton.connect('clicked()',
        self.onLayerSelectionRadioButton)
    self.backgroundRadioButton.checked = True

    self.foregroundRadioButton = qt.QRadioButton('Foreground')
    layerSelectionHBoxLayout.addWidget(self.foregroundRadioButton)
    self.foregroundRadioButton.connect('clicked()',
        self.onLayerSelectionRadioButton)

    colorScalarBarSettingsGroupBox = ctk.ctkCollapsibleGroupBox()
    colorScalarBarSettingsGroupBox.setTitle('Color Scalar Bar Settings')
    colorScalarBarFormLayout.addRow(colorScalarBarSettingsGroupBox)
    colorScalarBarSettingsLayout = qt.QFormLayout(colorScalarBarSettingsGroupBox)

    # X Position Slider
    self.colorScalarBarXPostionSlider = ctk.ctkSliderWidget()
    #self.colorScalarBarWidthSlider.value = 100
    self.colorScalarBarXPostionSlider.minimum = 0
    self.colorScalarBarXPostionSlider.pageStep= 0.01
    self.colorScalarBarXPostionSlider.value = 0.8
    self.colorScalarBarXPostionSlider.singleStep= 0.01
    self.colorScalarBarXPostionSlider.maximum = 1
    colorScalarBarSettingsLayout.addRow('x position:',self.colorScalarBarXPostionSlider )

    # Y Position Slider
    self.colorScalarBarYPostionSlider = ctk.ctkSliderWidget()
    self.colorScalarBarYPostionSlider.minimum = 0
    self.colorScalarBarYPostionSlider.value = 0.2
    self.colorScalarBarYPostionSlider.pageStep= 0.01
    self.colorScalarBarYPostionSlider.singleStep = 0.01
    self.colorScalarBarYPostionSlider.maximum = 1
    colorScalarBarSettingsLayout.addRow('y position:',self.colorScalarBarYPostionSlider)

    # Width Slider
    self.colorScalarBarWidthSlider = ctk.ctkSliderWidget()
    #self.colorScalarBarWidthSlider.value = 100
    self.colorScalarBarWidthSlider.minimum = 0
    self.colorScalarBarWidthSlider.pageStep= 0.01
    self.colorScalarBarWidthSlider.value = 0.2
    self.colorScalarBarWidthSlider.singleStep= 0.01
    self.colorScalarBarWidthSlider.maximum = 1
    colorScalarBarSettingsLayout.addRow('width:',self.colorScalarBarWidthSlider)

    # Height Slider
    self.colorScalarBarHeightSlider = ctk.ctkSliderWidget()
    #self.colorScalarBarHeightSlider.value = 100
    self.colorScalarBarHeightSlider.minimum = 0
    self.colorScalarBarHeightSlider.value = 0.5
    self.colorScalarBarHeightSlider.pageStep= 0.01
    self.colorScalarBarHeightSlider.singleStep = 0.01
    self.colorScalarBarHeightSlider.maximum = 1
    colorScalarBarSettingsLayout.addRow('height:',self.colorScalarBarHeightSlider)

    # Maximum Width Slider
    self.colorScalarBarMaxWidthSlider = ctk.ctkSliderWidget()
    #self.colorScalarBarWidthSlider.value = 100
    self.colorScalarBarMaxWidthSlider.minimum = 10
    self.colorScalarBarMaxWidthSlider.pageStep= 10
    self.colorScalarBarMaxWidthSlider.value = 50
    self.colorScalarBarMaxWidthSlider.singleStep= 1
    self.colorScalarBarMaxWidthSlider.maximum = 500
    colorScalarBarSettingsLayout.addRow('Max width in pixels:',self.colorScalarBarMaxWidthSlider)

    # Height Slider
    self.colorScalarBarMaxHeightSlider = ctk.ctkSliderWidget()
    self.colorScalarBarMaxHeightSlider.minimum = 10
    self.colorScalarBarMaxHeightSlider.value = 200
    self.colorScalarBarMaxHeightSlider.pageStep= 10
    self.colorScalarBarMaxHeightSlider.singleStep = 1
    self.colorScalarBarMaxHeightSlider.maximum = 500
    colorScalarBarSettingsLayout.addRow('Max height in pixels:',self.colorScalarBarMaxHeightSlider)

    # Defualts Button
    restorDefaultsButton = qt.QPushButton('Defualt Values')
    colorScalarBarSettingsLayout.addRow('Restore: ',restorDefaultsButton)

    # connections

    # Add vertical spacer
    self.layout.addStretch(1)
    self.sliceViewAnnotationsCheckBox.connect('clicked()', self.onSliceViewAnnotationsCheckbox)
    self.cornerTextAnnotationsCheckBox.connect('clicked()', self.onCornerTextAnnotationsCheckbox)
    self.showScalingRulerCheckBox.connect('clicked()', self.onShowScalingRulerCheckbox)
    self.showColorScalarBarCheckBox.connect('clicked()', self.onShowColorScalarBarCheckbox)
    self.colorScalarBarXPostionSlider.connect('valueChanged(double)', self.updateSliceViewFromGUI)
    self.colorScalarBarYPostionSlider.connect('valueChanged(double)', self.updateSliceViewFromGUI)
    self.colorScalarBarWidthSlider.connect('valueChanged(double)', self.updateSliceViewFromGUI)
    self.colorScalarBarHeightSlider.connect('valueChanged(double)', self.updateSliceViewFromGUI)
    self.colorScalarBarMaxWidthSlider.connect('valueChanged(double)', self.updateSliceViewFromGUI)
    self.colorScalarBarMaxHeightSlider.connect('valueChanged(double)', self.updateSliceViewFromGUI)
    restorDefaultsButton.connect('clicked()', self.restoreDefaultValues)

  def onSliceViewAnnotationsCheckbox(self):
    if self.sliceViewAnnotationsCheckBox.checked:
      self.showSliceViewAnnotations = 1
      self.showScalingRuler = 1
      self.showColorScalarBar = 1
    else:
      self.showSliceViewAnnotations = 0
      self.showScalingRuler = 0
      self.showColorScalarBar = 0
    settings = qt.QSettings()
    settings.setValue('DataProbe/sliceViewAnnotations.show',self.showSliceViewAnnotations)
    self.updateSliceViewFromGUI()

  def onCornerTextAnnotationsCheckbox(self):
    #print 'onCornerTextAnnotationsCheckbox'
    if self.cornerTextAnnotationsCheckBox.checked:
      self.showCornerTextAnnotations = 1
      self.topLeftAnnotationDisplay = 1
      self.topRightAnnotationDisplay = 1
      self.bottomLeftAnnotationDisplay = 1
    else:
      self.showCornerTextAnnotations = 0
      self.topLeftAnnotationDisplay = 0
      self.topRightAnnotationDisplay = 0
      self.bottomLeftAnnotationDisplay = 0
    self.updateSliceViewFromGUI()

  def onShowScalingRulerCheckbox(self):
    if self.showScalingRulerCheckBox.checked:
      self.showScalingRuler = 1
    else:
      self.showScalingRuler = 0
    settings = qt.QSettings()
    settings.setValue('DataProbe/sliceViewAnnotations.showScalingRuler',
        self.showScalingRuler)
    self.updateSliceViewFromGUI()

  def onLayerSelectionRadioButton(self):
    if self.backgroundRadioButton.checked:
      self.colorbarSelectedLayer = 'background'
    else:
      self.colorbarSelectedLayer = 'foreground'
    self.updateSliceViewFromGUI()

  def onShowColorScalarBarCheckbox(self):
    if self.showColorScalarBarCheckBox.checked:
      self.showColorScalarBar = 1
    else:
      self.showColorScalarBar = 0
    settings = qt.QSettings()
    settings.setValue('DataProbe/sliceViewAnnotations.showColorScalarBar',
        self.showColorScalarBar)
    self.updateSliceViewFromGUI()

  def onFontFamilyRadioButton(self):
    # Updating font size and family
    if self.timesFontRadioButton.checked:
      self.fontFamily = 'Times'
    else:
      self.fontFamily = 'Arial'
    self.updateSliceViewFromGUI()
    settings = qt.QSettings()
    settings.setValue('DataProbe/sliceViewAnnotations.fontFamily',
        self.fontFamily)
    self.updateSliceViewFromGUI()

  def onFontSizeSpinBox(self):
    self.fontSize = self.fontSizeSpinBox.value
    settings = qt.QSettings()
    settings.setValue('DataProbe/sliceViewAnnotations.fontSize',
        self.fontSize)
    self.updateSliceViewFromGUI()

  def onTextLengthSpinBox(self):
    self.maximumTextLength = self.textLengthSpinBox.value
    self.updateSliceViewFromGUI()

  def restoreDefaultValues(self):
    self.colorScalarBarXPostionSlider.value = 0.8
    self.colorScalarBarYPostionSlider.value = 0.2
    self.colorScalarBarWidthSlider.value = 0.2
    self.colorScalarBarHeightSlider.value = 0.5
    self.colorScalarBarMaxWidthSlider.value = 50
    self.colorScalarBarMaxHeightSlider.value = 200

  def updateMRMLFromGUI(self):
    self.parameterNode.SetParameter(self.parameter, str(showSliceViewAnnotations))

  def updateGUIFromMRML(self,caller,event):
    if self.parameterNode.GetParameter(self.parameter) == '':
      # parameter does not exist - probably intializing
      return
    showStatus = int(self.parameterNode.GetParameter(self.parameter))
    self.showSliceViewAnnotations = showStatus
    self.showScalingRuler = showStatus
    self.showColorScalarBar = showStatus
    self.updateSliceViewFromGUI()

  def openSettingsPopup(self):

    if not self.window.isVisible():
      self.window.show()
    """
      if self.popupGeometry.isValid():
        self.window.setGeometry(self.popupGeometry)
        self.popupPositioned = True

    if not self.popupPositioned:
      mainWindow = slicer.util.mainWindow()
      screenMainPos = mainWindow.pos
      x = screenMainPos.x() + 100
      y = screenMainPos.y() + 100
      self.window.move(qt.QPoint(x,y))
      self.popupPositioned = True
    """
    self.window.raise_()


  def close(self):
    self.window.hide()

  """
  def cleanup(self):
    pass
  """
  def updateSliceViewFromGUI(self):
    #print 'updateSliceViewFromGUI'
    # Create corner annotations if have not created already
    if len(self.sliceCornerAnnotations.items()) == 0:
      self.createCornerAnnotations()

    for sliceViewName in self.sliceViewNames:
      cornerAnnotation = self.sliceCornerAnnotations[sliceViewName]
      cornerAnnotation.SetMaximumFontSize(self.fontSize)
      cornerAnnotation.SetMinimumFontSize(self.fontSize)
      textProperty = cornerAnnotation.GetTextProperty()
      scalarBar = self.colorScalarBars[sliceViewName]
      scalarBarTextProperty = scalarBar.GetLabelTextProperty()
      scalarBarTextProperty.ItalicOff()
      scalarBarTextProperty.BoldOff()
      if self.fontFamily == 'Times':
        textProperty.SetFontFamilyToTimes()
        scalarBarTextProperty.SetFontFamilyToTimes()
      else:
        textProperty.SetFontFamilyToArial()
        scalarBarTextProperty.SetFontFamilyToArial()

    # Updating Annotations Amount
    if self.level1RadioButton.checked:
      self.annotationsDisplayAmount = 0
    elif self.level2RadioButton.checked:
      self.annotationsDisplayAmount = 1
    elif self.level3RadioButton.checked:
      self.annotationsDisplayAmount = 2

    if self.cornerActivationCheckbox[0].checked:
      self.topLeftAnnotationDisplay = True
    else:
      self.topLeftAnnotationDisplay = False

    if self.cornerActivationCheckbox[1].checked:
      self.topRightAnnotationDisplay = True
    else:
      self.topRightAnnotationDisplay = False

    if self.cornerActivationCheckbox[2].checked:
      self.bottomLeftAnnotationDisplay = True
    else:
      self.bottomLeftAnnotationDisplay = False

    if self.showSliceViewAnnotations:
      self.cornerTextParametersCollapsibleButton.enabled = True
      self.cornerActivationsGroupBox.enabled = True
      self.fontPropertiesGroupBox.enabled = True
      self.colorBarLayerSelectionGroupBox.enabled = True
      self.annotationsAmountGroupBox.enabled = True
      self.scalingRulerCollapsibleButton.enabled = True
      self.colorScalarBarCollapsibleButton.enabled = True

      for sliceViewName in self.sliceViewNames:
        sliceWidget = self.layoutManager.sliceWidget(sliceViewName)
        sl = sliceWidget.sliceLogic()
        #bl =sl.GetBackgroundLayer()
        self.makeAnnotationText(sl)
        self.makeScalingRuler(sl)
        self.makeColorScalarBar(sl)
    else:
      self.cornerTextParametersCollapsibleButton.enabled = False
      self.cornerActivationsGroupBox.enabled = False
      self.colorBarLayerSelectionGroupBox.enabled = False
      self.fontPropertiesGroupBox.enabled = False
      self.annotationsAmountGroupBox.enabled = False
      self.scalingRulerCollapsibleButton.enabled = False
      self.colorScalarBarCollapsibleButton.enabled = False
      # Remove Observers
      for sliceViewName in self.sliceViewNames:
        sliceWidget = self.layoutManager.sliceWidget(sliceViewName)
        sl = sliceWidget.sliceLogic()
        #sl.RemoveAllObservers()
        self.makeScalingRuler(sl)
        self.makeColorScalarBar(sl)

      # Clear Annotations
      for sliceViewName in self.sliceViewNames:
        self.sliceCornerAnnotations[sliceViewName].SetText(0, "")
        self.sliceCornerAnnotations[sliceViewName].SetText(1, "")
        self.sliceCornerAnnotations[sliceViewName].SetText(2, "")
        self.sliceCornerAnnotations[sliceViewName].SetText(3, "")
        self.sliceViews[sliceViewName].scheduleRender()

      # reset golobal variables
      self.sliceCornerAnnotations = {}

  def createGlobalVariables(self):
    self.sliceViewNames = []
    self.sliceWidgets = {}
    self.sliceViews = {}
    self.cameras = {}
    self.blNodeObserverTag = {}
    self.sliceLogicObserverTag = {}
    self.sliceCornerAnnotations = {}
    self.renderers = {}
    self.scalingRulerActors = {}
    self.points = {}
    self.scalingRulerTextActors = {}
    self.colorScalarBars = {}
    #self.fgColorScalarBars = {}

  def createCornerAnnotations(self):
    #print 'createCornerAnnotations'
    self.createGlobalVariables()
    sliceViewNames = self.layoutManager.sliceViewNames()

    for sliceViewName in sliceViewNames:
      self.sliceViewNames.append(sliceViewName)
    for sliceViewName in self.sliceViewNames:
      self.addObserver(sliceViewName)
      self.createActors(sliceViewName)

  def addObserver(self, sliceViewName):
    #print('addObserver(%s)'%sliceViewName)
    sliceWidget = self.layoutManager.sliceWidget(sliceViewName)
    self.sliceWidgets[sliceViewName] = sliceWidget
    sliceView = sliceWidget.sliceView()

    renderWindow = sliceView.renderWindow()
    renderWindow.GetRenderers()
    renderer = renderWindow.GetRenderers().GetItemAsObject(0)
    self.renderers[sliceViewName] = renderer

    self.sliceViews[sliceViewName] = sliceView
    self.sliceCornerAnnotations[sliceViewName] = sliceView.cornerAnnotation()
    sliceLogic = sliceWidget.sliceLogic()
    self.sliceLogicObserverTag[sliceViewName] = sliceLogic.AddObserver(vtk.vtkCommand.ModifiedEvent,
                                             self.updateCornerAnnotations)

  def createActors(self, sliceViewName):
    #print('createActors(%s)'%sliceViewName)
    sliceWidget = self.layoutManager.sliceWidget(sliceViewName)
    self.sliceWidgets[sliceViewName] = sliceWidget
    sliceView = sliceWidget.sliceView()

    self.scalingRulerTextActors[sliceViewName] = vtk.vtkTextActor()
    textActor = self.scalingRulerTextActors[sliceViewName]
    textProperty = textActor.GetTextProperty()
    textProperty.ShadowOn()
    self.scalingRulerActors[sliceViewName] = vtk.vtkActor2D()
    # Create Scaling Ruler
    self.createScalingRuler(sliceViewName)
    # Create Color Scalar Bar
    self.colorScalarBars[sliceViewName] = self.createColorScalarBar(sliceViewName)
    #self.fgColorScalarBars[sliceViewName] = self.createColorScalarBar(sliceViewName)

  def createScalingRuler(self, sliceViewName):
    #print('createScalingRuler(%s)'%sliceViewName)
    #renderer = self.renderers[sliceViewName]
    #
    # Create the Scaling Ruler
    #
    self.points[sliceViewName] = vtk.vtkPoints()
    self.points[sliceViewName].SetNumberOfPoints(22)

    lines = []
    for i in xrange(0,21):
      line = vtk.vtkLine()
      lines.append(line)

    # setting the points to lines
    lines[0].GetPointIds().SetId(0,0)
    lines[0].GetPointIds().SetId(1,1)
    lines[1].GetPointIds().SetId(0,0)
    lines[1].GetPointIds().SetId(1,2)
    lines[2].GetPointIds().SetId(0,2)
    lines[2].GetPointIds().SetId(1,3)
    lines[3].GetPointIds().SetId(0,2)
    lines[3].GetPointIds().SetId(1,4)
    lines[4].GetPointIds().SetId(0,4)
    lines[4].GetPointIds().SetId(1,5)
    lines[5].GetPointIds().SetId(0,4)
    lines[5].GetPointIds().SetId(1,6)
    lines[6].GetPointIds().SetId(0,6)
    lines[6].GetPointIds().SetId(1,7)
    lines[7].GetPointIds().SetId(0,6)
    lines[7].GetPointIds().SetId(1,8)
    lines[8].GetPointIds().SetId(0,8)
    lines[8].GetPointIds().SetId(1,9)
    lines[9].GetPointIds().SetId(0,8)
    lines[9].GetPointIds().SetId(1,10)
    lines[10].GetPointIds().SetId(0,10)
    lines[10].GetPointIds().SetId(1,11)
    lines[11].GetPointIds().SetId(0,10)
    lines[11].GetPointIds().SetId(1,12)
    lines[12].GetPointIds().SetId(0,12)
    lines[12].GetPointIds().SetId(1,13)
    lines[13].GetPointIds().SetId(0,12)
    lines[13].GetPointIds().SetId(1,14)
    lines[14].GetPointIds().SetId(0,14)
    lines[14].GetPointIds().SetId(1,15)
    lines[15].GetPointIds().SetId(0,14)
    lines[15].GetPointIds().SetId(1,16)
    lines[16].GetPointIds().SetId(0,16)
    lines[16].GetPointIds().SetId(1,17)
    lines[17].GetPointIds().SetId(0,16)
    lines[17].GetPointIds().SetId(1,18)
    lines[18].GetPointIds().SetId(0,18)
    lines[18].GetPointIds().SetId(1,19)
    lines[19].GetPointIds().SetId(0,18)
    lines[19].GetPointIds().SetId(1,20)
    lines[20].GetPointIds().SetId(0,20)
    lines[20].GetPointIds().SetId(1,21)

    # Create a cell array to store the lines in and add the lines to it
    linesArray = vtk.vtkCellArray()
    for i in xrange(0,21):
      linesArray.InsertNextCell(lines[i])

    # Create a polydata to store everything in
    linesPolyData = vtk.vtkPolyData()

    # Add the points to the dataset
    linesPolyData.SetPoints(self.points[sliceViewName])

    # Add the lines to the dataset
    linesPolyData.SetLines(linesArray)

    # mapper
    mapper = vtk.vtkPolyDataMapper2D()
    if vtk.VTK_MAJOR_VERSION <= 5:
      mapper.SetInput(linesPolyData)
    else:
      mapper.SetInputData(linesPolyData)
    # actor
    #self.scalingRulerActors[sliceViewName] = vtk.vtkActor2D()
    actor = self.scalingRulerActors[sliceViewName]
    actor.SetMapper(mapper)
    # color actor
    actor.GetProperty().SetColor(1,1,1)
    actor.GetProperty().SetLineWidth(1)
    textActor = self.scalingRulerTextActors[sliceViewName]
    textProperty = textActor.GetTextProperty()
    # Turn off shadow
    textProperty.ShadowOff()

  def createColorScalarBar(self, sliceViewName):
    scalarBar = vtk.vtkScalarBarActor()
    # adjust text property
    lookupTable = vtk.vtkLookupTable()
    scalarBar.SetLookupTable(lookupTable)
    #renderer.AddActor2D(scalarBar)
    return scalarBar

  def updateCornerAnnotations(self,caller,event):
    #print 'updateCornerAnnotations'
    sliceViewNames = self.layoutManager.sliceViewNames()
    for sliceViewName in sliceViewNames:
      if sliceViewName not in self.sliceViewNames:
        self.sliceViewNames.append(sliceViewName)
        self.addObserver(sliceViewName)
        self.createActors(sliceViewName)
        #self.updateSliceViewFromGUI()
    self.makeAnnotationText(caller)
    self.makeScalingRuler(caller)
    self.makeColorScalarBar(caller)

  def sliceLogicModifiedEvent(self, caller,event):
    self.updateLayersAnnotation(caller)

  def makeScalingRuler(self, sliceLogic):
    sliceCompositeNode = sliceLogic.GetSliceCompositeNode()

    # Get the layers
    backgroundLayer = sliceLogic.GetBackgroundLayer()
    foregroundLayer = sliceLogic.GetForegroundLayer()
    labelLayer = sliceLogic.GetLabelLayer()

    # Get the volumes
    backgroundVolume = backgroundLayer.GetVolumeNode()
    foregroundVolume = foregroundLayer.GetVolumeNode()
    labelVolume = labelLayer.GetVolumeNode()

    # Get slice view name
    sliceNode = backgroundLayer.GetSliceNode()
    sliceViewName = sliceNode.GetLayoutName()
    self.currentSliceViewName = sliceViewName

    renderer = self.renderers[sliceViewName]

    if self.sliceViews[sliceViewName]:

      #
      # update scaling ruler
      #
      self.minimumWidthForScalingRuler = 300
      viewWidth = self.sliceViews[sliceViewName].width
      viewHeight = self.sliceViews[sliceViewName].height

      rasToXY = vtk.vtkMatrix4x4()
      m = sliceNode.GetXYToRAS()
      rasToXY.DeepCopy(m)
      rasToXY.Invert()
      #print rasToXY
      scalingFactorString = " mm"

      #print rasToXY
      # TODO: Fix the bug
      import math
      scalingFactor = math.sqrt( rasToXY.GetElement(0,0)**2 +
          rasToXY.GetElement(0,1)**2 +rasToXY.GetElement(0,2) **2 )

      rulerArea = viewWidth/scalingFactor/2

      if self.showScalingRuler and \
          viewWidth > self.minimumWidthForScalingRuler and\
         rulerArea>0.5 and rulerArea<500 :
        import numpy as np
        rulerSizesArray = np.array([1,5,10,50,100])
        index = np.argmin(np.abs(rulerSizesArray- rulerArea))

        if rulerSizesArray[index]/10 > 1:
          scalingFactorString = str(int(rulerSizesArray[index]/10))+" cm"
        else:
          scalingFactorString = str(rulerSizesArray[index])+" mm"

        RASRulerSize = rulerSizesArray[index]

        pts = self.points[sliceViewName]
        pts.SetPoint(0,[(viewWidth/2-RASRulerSize*scalingFactor/10*5),5, 0])
        pts.SetPoint(1,[(viewWidth/2-RASRulerSize*scalingFactor/10*5),15, 0])
        pts.SetPoint(2,[(viewWidth/2-RASRulerSize*scalingFactor/10*4),5, 0])
        pts.SetPoint(3,[(viewWidth/2-RASRulerSize*scalingFactor/10*4),10, 0])
        pts.SetPoint(4,[(viewWidth/2-RASRulerSize*scalingFactor/10*3),5, 0])
        pts.SetPoint(5,[(viewWidth/2-RASRulerSize*scalingFactor/10*3),15, 0])
        pts.SetPoint(6,[(viewWidth/2-RASRulerSize*scalingFactor/10*2),5, 0])
        pts.SetPoint(7,[(viewWidth/2-RASRulerSize*scalingFactor/10*2),10, 0])
        pts.SetPoint(8,[(viewWidth/2-RASRulerSize*scalingFactor/10),5, 0])
        pts.SetPoint(9,[(viewWidth/2-RASRulerSize*scalingFactor/10),15, 0])
        pts.SetPoint(10,[viewWidth/2,5, 0])
        pts.SetPoint(11,[viewWidth/2,10, 0])
        pts.SetPoint(12,[(viewWidth/2+RASRulerSize*scalingFactor/10),5, 0])
        pts.SetPoint(13,[(viewWidth/2+RASRulerSize*scalingFactor/10),15, 0])
        pts.SetPoint(14,[(viewWidth/2+RASRulerSize*scalingFactor/10*2),5, 0])
        pts.SetPoint(15,[(viewWidth/2+RASRulerSize*scalingFactor/10*2),10, 0])
        pts.SetPoint(16,[(viewWidth/2+RASRulerSize*scalingFactor/10*3),5, 0])
        pts.SetPoint(17,[(viewWidth/2+RASRulerSize*scalingFactor/10*3),15, 0])
        pts.SetPoint(18,[(viewWidth/2+RASRulerSize*scalingFactor/10*4),5, 0])
        pts.SetPoint(19,[(viewWidth/2+RASRulerSize*scalingFactor/10*4),10, 0])
        pts.SetPoint(20,[(viewWidth/2+RASRulerSize*scalingFactor/10*5),5, 0])
        pts.SetPoint(21,[(viewWidth/2+RASRulerSize*scalingFactor/10*5),15, 0])

        textActor = self.scalingRulerTextActors[sliceViewName]
        textActor.SetInput(scalingFactorString)
        textActor.SetDisplayPosition(int((viewWidth+RASRulerSize*scalingFactor)/2)+10,5)

        renderer.AddActor2D(self.scalingRulerActors[sliceViewName])
        renderer.AddActor2D(textActor)

      else:
        renderer.RemoveActor2D(self.scalingRulerActors[sliceViewName])
        renderer.RemoveActor2D(self.scalingRulerTextActors[sliceViewName])

  def makeColorScalarBar(self, sliceLogic):
    sliceCompositeNode = sliceLogic.GetSliceCompositeNode()

    # Get the layers
    backgroundLayer = sliceLogic.GetBackgroundLayer()
    foregroundLayer = sliceLogic.GetForegroundLayer()
    labelLayer = sliceLogic.GetLabelLayer()

    # Get the volumes
    backgroundVolume = backgroundLayer.GetVolumeNode()
    foregroundVolume = foregroundLayer.GetVolumeNode()
    labelVolume = labelLayer.GetVolumeNode()

    # Get slice view name
    sliceNode = backgroundLayer.GetSliceNode()
    sliceViewName = sliceNode.GetLayoutName()
    self.currentSliceViewName = sliceViewName

    renderer = self.renderers[sliceViewName]

    if self.sliceViews[sliceViewName]:

      scalarBar = self.colorScalarBars[sliceViewName]
      scalarBar.SetTextPositionToPrecedeScalarBar()
      scalarBar.SetPosition(self.colorScalarBarXPostionSlider.value,
          self.colorScalarBarYPostionSlider.value)
      scalarBar.SetWidth(self.colorScalarBarWidthSlider.value)
      scalarBar.SetHeight(self.colorScalarBarHeightSlider.value)
      scalarBar.SetLabelFormat("%4.0f")
      scalarBar.SetMaximumWidthInPixels(int(self.colorScalarBarMaxWidthSlider.value))
      scalarBar.SetMaximumHeightInPixels(int(self.colorScalarBarMaxHeightSlider.value))
      #numberOfLables = int((viewHeight*0.5)/30)
      #scalarBar.SetNumberOfLabels(numberOfLables)
      #fgScalarBar.SetPosition(0.8,0.4)
      #fgScalarBar.SetPosition2(0.1,0.2)

      if self.showColorScalarBar:
        if (backgroundVolume != None and foregroundVolume == None):
        #if (backgroundVolume != None and self.colorbarSelectedLayer == 'background'):
          self.updateScalarBarRange(sliceLogic, backgroundVolume, scalarBar)
          renderer.AddActor2D(scalarBar)
          '''
        elif (foregroundVolume != None and self.colorbarSelectedLayer == 'foreground'):
          self.updateScalarBarRange(sliceLogic, foregroundVolume, scalarBar)
          renderer.AddActor2D(scalarBar)
          '''
        else:
          renderer.RemoveActor2D(scalarBar)
          #renderer.RemoveActor2D(fgScalarBar)
      else:
          renderer.RemoveActor2D(scalarBar)
          #renderer.RemoveActor2D(fgScalarBar)

  def makeAnnotationText(self, sliceLogic):
    #print 'makeAnnotationText'
    self.resetTexts()
    sliceCompositeNode = sliceLogic.GetSliceCompositeNode()

    # Get the layers
    backgroundLayer = sliceLogic.GetBackgroundLayer()
    foregroundLayer = sliceLogic.GetForegroundLayer()
    labelLayer = sliceLogic.GetLabelLayer()

    # Get the volumes
    backgroundVolume = backgroundLayer.GetVolumeNode()
    foregroundVolume = foregroundLayer.GetVolumeNode()
    labelVolume = labelLayer.GetVolumeNode()

    # Get slice view name
    sliceNode = backgroundLayer.GetSliceNode()
    sliceViewName = sliceNode.GetLayoutName()
    self.currentSliceViewName = sliceViewName

    renderer = self.renderers[sliceViewName]

    if self.sliceViews[sliceViewName]:
      #
      # Update slice corner annotations
      #
      # Case I: Both background and foregraound
      if ( backgroundVolume != None and foregroundVolume != None):
        foregroundOpacity = sliceCompositeNode.GetForegroundOpacity()
        backgroundVolumeName = backgroundVolume.GetName()
        foregroundVolumeName = foregroundVolume.GetName()
        self.cornerTexts[0]['3-Background']['text'] = 'B: ' + backgroundVolumeName
        self.cornerTexts[0]['2-Foreground']['text'] = 'F: ' + foregroundVolumeName +  ' (' + str(
                      "%.1f"%foregroundOpacity) + ')'

        bgUids = backgroundVolume.GetAttribute('DICOM.instanceUIDs')
        fgUids = foregroundVolume.GetAttribute('DICOM.instanceUIDs')
        if (bgUids and fgUids):
          bgUid = bgUids.partition(' ')[0]
          fgUid = fgUids.partition(' ')[0]
          self.makeDicomAnnotation(bgUid,fgUid)
        else:
          for key in self.cornerTexts[2]:
            self.cornerTexts[2][key]['text'] = ''

      # Case II: Only background
      elif (backgroundVolume != None):
        backgroundVolumeName = backgroundVolume.GetName()
        if self.bottomLeftAnnotationDisplay:
          self.cornerTexts[0]['3-Background']['text'] = 'B: ' + backgroundVolumeName

        uids = backgroundVolume.GetAttribute('DICOM.instanceUIDs')
        if uids:
          uid = uids.partition(' ')[0]
          self.makeDicomAnnotation(uid,None)

      # Case III: Only foreground
      elif (foregroundVolume != None):
        if self.bottomLeftAnnotationDisplay:
          foregroundVolumeName = foregroundVolume.GetName()
          self.cornerTexts[0]['2-Foreground']['text'] = 'F: ' + foregroundVolumeName

        uids = foregroundVolume.GetAttribute('DICOM.instanceUIDs')
        if uids:
          uid = uids.partition(' ')[0]
          # passed UID as bg
          self.makeDicomAnnotation(uid,None)

      if (labelVolume != None):
        labelOpacity = sliceCompositeNode.GetLabelOpacity()
        labelVolumeName = labelVolume.GetName()
        self.cornerTexts[0]['1-Label']['text'] = 'L: ' + labelVolumeName + ' (' + str(
                      "%.1f"%labelOpacity) + ')'

      self.drawCornerAnnotations()

  def updateScalarBarRange(self, sliceLogic, volumeNode, scalarBar):
    vdn = volumeNode.GetDisplayNode()
    vcn = vdn.GetColorNode()
    lut = vcn.GetLookupTable()
    lut2 = vtk.vtkLookupTable()
    lut2.DeepCopy(lut)
    width = vtk.mutable(0)
    level = vtk.mutable(0)
    rangeLow = vtk.mutable(0)
    rangeHigh = vtk.mutable(0)
    sliceLogic.GetBackgroundWindowLevelAndRange(width,level,rangeLow,rangeHigh)
    lut2.SetRange(int(level-width/2),int(level+width/2))
    scalarBar.SetLookupTable(lut2)

  def makeDicomAnnotation(self,bgUid,fgUid):
    if fgUid != None and bgUid != None:
      backgroundDicomDic = self.extractDICOMValues(bgUid)
      foregroundDicomDic = self.extractDICOMValues(fgUid)
      # check if background and foreground are from different patients
      # and remove the annotations

      if self.topLeftAnnotationDisplay:
        if backgroundDicomDic['Patient Name'] != foregroundDicomDic['Patient Name'
            ] or backgroundDicomDic['Patient ID'] != foregroundDicomDic['Patient ID'
              ] or backgroundDicomDic['Patient Birth Date'] != foregroundDicomDic['Patient Birth Date']:
              for key in self.cornerTexts[2]:
                self.cornerTexts[2][key]['text'] = ''
        else:
          self.cornerTexts[2]['1-PatientName']['text'] = backgroundDicomDic['Patient Name'].replace('^',', ')
          self.cornerTexts[2]['2-PatientID']['text'] = 'ID: ' + backgroundDicomDic['Patient ID']
          backgroundDicomDic['Patient Birth Date'] = self.formatDICOMDate(backgroundDicomDic['Patient Birth Date'])
          self.cornerTexts[2]['3-PatientInfo']['text'] = self.makePatientInfo(backgroundDicomDic)

          if (backgroundDicomDic['Study Date'] != foregroundDicomDic['Study Date']):
            self.cornerTexts[2]['4-Bg-StudyDate']['text'] = 'B: ' + self.formatDICOMDate(backgroundDicomDic['Study Date'])
            self.cornerTexts[2]['5-Fg-StudyDate']['text'] = 'F: ' + self.formatDICOMDate(foregroundDicomDic['Study Date'])
          else:
            self.cornerTexts[2]['4-Bg-StudyDate']['text'] =  self.formatDICOMDate(backgroundDicomDic['Study Date'])

          if (backgroundDicomDic['Study Time'] != foregroundDicomDic['Study Time']):
            self.cornerTexts[2]['6-Bg-StudyTime']['text'] = 'B: ' + self.formatDICOMTime(backgroundDicomDic['Study Time'])
            self.cornerTexts[2]['7-Fg-StudyTime']['text'] = 'F: ' + self.formatDICOMTime(foregroundDicomDic['Study Time'])
          else:
            self.cornerTexts[2]['6-Bg-StudyTime']['text'] = self.formatDICOMTime(backgroundDicomDic['Study Time'])

          if (backgroundDicomDic['Series Description'] != foregroundDicomDic['Series Description']):
            self.cornerTexts[2]['8-Bg-SeriesDescription']['text'] = 'B: ' + backgroundDicomDic['Series Description']
            self.cornerTexts[2]['9-Fg-SeriesDescription']['text'] = 'F: ' + foregroundDicomDic['Series Description']
          else:
            self.cornerTexts[2]['8-Bg-SeriesDescription']['text'] = backgroundDicomDic['Series Description']

    # Only Background or Only Foreground
    else:
      uid = bgUid
      dicomDic = self.extractDICOMValues(uid)
      if self.topLeftAnnotationDisplay:
        self.cornerTexts[2]['1-PatientName']['text'] = dicomDic['Patient Name'].replace('^',', ')
        self.cornerTexts[2]['2-PatientID']['text'] = 'ID: ' + dicomDic ['Patient ID']
        dicomDic['Patient Birth Date'] = self.formatDICOMDate(dicomDic['Patient Birth Date'])
        self.cornerTexts[2]['3-PatientInfo']['text'] = self.makePatientInfo(dicomDic)
        self.cornerTexts[2]['4-Bg-StudyDate']['text'] = self.formatDICOMDate(dicomDic['Study Date'])
        self.cornerTexts[2]['6-Bg-StudyTime']['text'] = self.formatDICOMTime(dicomDic['Study Time'])
        self.cornerTexts[2]['8-Bg-SeriesDescription']['text'] = dicomDic['Series Description']

      if (self.sliceWidgets[self.currentSliceViewName].width > 600 and self.topRightAnnotationDisplay):
        self.cornerTexts[3]['1-Institution-Name']['text'] = dicomDic['Institution Name']
        self.cornerTexts[3]['2-Referring-Phisycian']['text'] = dicomDic['Referring Physician Name'].replace('^',', ')
        self.cornerTexts[3]['3-Manufacturer']['text'] = dicomDic['Manufacturer']
        self.cornerTexts[3]['4-Model']['text'] = dicomDic['Model']
        self.cornerTexts[3]['5-Patient-Position']['text'] = dicomDic['Patient Position']
        modality = dicomDic['Modality']
        if modality == 'MR':
         self.cornerTexts[3]['6-TR']['text']  = 'TR ' + dicomDic['Repetition Time']
         self.cornerTexts[3]['7-TE']['text'] = 'TE ' + dicomDic['Echo Time']

  def makePatientInfo(self,dicomDic):
    # This will give an string of patient's birth date,
    # patient's age and sex
    patientInfo = dicomDic['Patient Birth Date'
          ] + ', ' + dicomDic['Patient Age'
              ] + ', ' + dicomDic['Patient Sex']
    return patientInfo

  def formatDICOMDate(self, date):
      return date[4:6] + '/' + date[6:]+ '/' + date[:4]

  def formatDICOMTime(self, time):
    studyH = time[:2]
    if int(studyH) > 12 :
      studyH = str (int(studyH) - 12)
      clockTime = ' PM'
    else:
      studyH = studyH
      clockTime = ' AM'
    studyM = time[2:4]
    studyS = time[4:6]
    return studyH + ':' + studyM  + ':' + studyS +clockTime

  def fitText(self,text,textSize):
    if len(text) > textSize:
      preSize = textSize/ 2
      postSize = preSize - 3
      text = text[:preSize] + "..." + text[-postSize:]
    return text

  def drawCornerAnnotations(self):
    cornerAnnotation = ''
    for i, cornerText in enumerate(self.cornerTexts):
      keys = sorted(cornerText.keys())
      cornerAnnotation = ''
      for key in keys:
        text = cornerText[key]['text']
        if ( text != ''):
          text = self.fitText(text, self.maximumTextLength)
          # level 1: All categories will be displayed
          if self.annotationsDisplayAmount == 0:
            cornerAnnotation = cornerAnnotation+ text + '\n'
          # level 2: Category A and B will be displayed
          elif self.annotationsDisplayAmount == 1:
            if (cornerText[key]['category'] != 'C'):
              cornerAnnotation = cornerAnnotation+ text + '\n'
          # level 3 only Category A will be displayed
          elif self.annotationsDisplayAmount == 2:
            if (cornerText[key]['category'] == 'A'):
              cornerAnnotation = cornerAnnotation+ text + '\n'
      sliceCornerAnnotation = self.sliceCornerAnnotations[self.currentSliceViewName]
      sliceCornerAnnotation.SetText(i, cornerAnnotation)
      # get shadow
      #textProperty = sliceCornerAnnotation.GetTextProperty()
      #print 'shadow', textProperty.GetShadow()
      #textProperty.ShadowOn()
      #print 'shadow', textProperty.GetShadow()
    self.sliceViews[self.currentSliceViewName].scheduleRender()
  def resetTexts(self):
    for i, cornerText in enumerate(self.cornerTexts):
      #self.cornerTexts[i] =  dict((k,'') for k,v in cornerText.iteritems())
      for key in cornerText.keys():
        self.cornerTexts[i][key]['text'] = ''

  def extractDICOMValues(self,uid):
    p ={}
    slicer.dicomDatabase.loadInstanceHeader(uid)
    tags = {
    "0008,0020": "Study Date",
    "0008,0030": "Study Time",
    "0008,0060": "Modality",
    "0008,0070": "Manufacturer",
    "0008,0080": "Institution Name",
    "0008,0090": "Referring Physician Name",
    # "0008,1030": "Study Description",
    "0008,103e": "Series Description",
    "0008,1090": "Model",
    "0010,0010": "Patient Name",
    "0010,0020": "Patient ID",
    "0010,0030": "Patient Birth Date",
    "0010,0040": "Patient Sex",
    "0010,1010": "Patient Age",
    # "0010,4000": "Patient Comments",
    # "0018,1030": "Protocol Name",
    "0018,5100": "Patient Position",
    # "0020,0010": "Study ID",
    # "0020,0011": "Series Number",
    #"0020,4000": "Image Comments"
    }

    p = self.extractTagValue(p, tags)
    if p['Modality'] == 'MR':
      mrTags = {
      "0018,0080": "Repetition Time",
      "0018,0081": "Echo Time"
      }
      p = self.extractTagValue(p, mrTags)
    '''
      These tags are not available in dicom tag cache now
      if p['Modality'] == 'CT':
        ctTags = {
        "0018,0060": "KVP",
        "0018,1152": "Exposure"
        }
        p = self.extractTagValue(p, ctTags)
      '''
    return p

  def extractTagValue(self,p,tags):
    for tag in tags.keys():
        dump = slicer.dicomDatabase.headerValue(tag)
        try:
          value = dump[dump.index('[')+1:dump.index(']')]
        except ValueError:
          value = "Unknown"
        p[tags[tag]] = value
    return p
