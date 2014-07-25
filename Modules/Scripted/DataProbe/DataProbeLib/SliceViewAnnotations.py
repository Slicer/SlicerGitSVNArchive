import os, glob, sys
from __main__ import qt
from __main__ import vtk
from __main__ import ctk
from __main__ import slicer

import DataProbeLib

class SliceAnnotations(object):
  """Implement the Qt window showing settings for Slice View Annotations
  """
  def __init__(self):

    self.sliceViewNames = []
    self.popupGeometry = qt.QRect()
    self.create()
    self.cornerTexts =[]
    # Bottom Left Corner Text
    self.cornerTexts.append({'1-Label':'','2-Foreground':'','3-Background':''})
    # Bottom Rihgt Corner Text
    # Not used - orientation figure will be draw there
    self.cornerTexts.append({'1-TR':'','2-TE':''})
    # Top Left Corner Text
    self.cornerTexts.append({'1-PatientName':'','2-PatientID':'','3-PatientInfo':'',
      '4-Bg-StudyDate':'','5-Fg-StudyDate':'','6-Bg-StudyTime':'','7-Bg-StudyTime':'',
      '8-Bg-SeriesDescription':'','9-Fg-SeriesDescription':''})
    # Top Rihgt Corner Text
    self.cornerTexts.append({'1-Institution-Name':'','2-Referring-Phisycian':'','3-Manufacturer':'',
      '4-Model':'','5-Patient-Position':'','6-TR':'','7-TE':''})

    self.topLeftAnnotationDisplay = True
    self.topRightAnnotationDisplay = True
    self.bottomLeftAnnotationDisplay = True
    self.bottomRightAnnotationDisplay = True

    self.layoutManager = slicer.app.layoutManager()
    self.sliceCornerAnnotations = {}

  def create(self):
    # Instantiate and connect widgets ...

    self.window = qt.QWidget()
    self.window.setWindowTitle('Slice View Annotations Settings')
    self.layout = qt.QVBoxLayout(self.window)

   #
    # Parameters Area
    #
    parametersCollapsibleButton = ctk.ctkCollapsibleButton()
    parametersCollapsibleButton.text = "Parameters"
    self.layout.addWidget(parametersCollapsibleButton)

    # Layout within the dummy collapsible button
    parametersFormLayout = qt.QFormLayout(parametersCollapsibleButton)

    #
    # DICOM Annotations Checkbox
    #
    self.sliceViewAnnotationsCheckBox = qt.QCheckBox('Slice View Annotations')
    parametersFormLayout.addRow(self.sliceViewAnnotationsCheckBox)

    #
    # Corner Annotations Activation Checkboxes
    #
    self.cornerActivationsGroupBox = ctk.ctkCollapsibleGroupBox()
    self.cornerActivationsGroupBox.setTitle('Active Corners')
    self.cornerActivationsGroupBox.enabled = False
    parametersFormLayout.addRow(self.cornerActivationsGroupBox)
    cornerActionHBoxLayout = qt.QHBoxLayout(self.cornerActivationsGroupBox)

    self.cornerActivationCheckbox = []

    for i in xrange(4):
      self.cornerActivationCheckbox.append(qt.QCheckBox())
      self.cornerActivationCheckbox[i].checked = True
      cornerActionHBoxLayout.addWidget(self.cornerActivationCheckbox[i])
      self.cornerActivationCheckbox[i].connect('clicked()', self.updateSliceViewFromGUI)

    self.cornerActivationCheckbox[0].setText('Top Left')
    self.cornerActivationCheckbox[1].setText('Top Right')
    self.cornerActivationCheckbox[2].setText('Bottom Left')
    self.cornerActivationCheckbox[3].setText('Bottom Right')

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
    self.timesFontRadioButton.connect('clicked()', self.updateSliceViewFromGUI)
    self.timesFontRadioButton.checked = True
    self.arialFontRadioButton = qt.QRadioButton('Arial')
    self.arialFontRadioButton.connect('clicked()', self.updateSliceViewFromGUI)
    fontPropertiesHBoxLayout.addWidget(self.arialFontRadioButton)

    fontSizeLabel = qt.QLabel('Font Size: ')
    fontPropertiesHBoxLayout.addWidget(fontSizeLabel)
    self.fontSizeSpinBox = qt.QSpinBox()
    self.fontSizeSpinBox.setMinimum(10)
    self.fontSizeSpinBox.setMaximum(20)
    self.fontSizeSpinBox.value = 14
    fontPropertiesHBoxLayout.addWidget(self.fontSizeSpinBox)
    self.fontSizeSpinBox.connect('valueChanged(int)', self.updateSliceViewFromGUI)

    #
    # Scaling Bar Area
    #
    self.scalingBarCollapsibleButton = ctk.ctkCollapsibleButton()
    self.scalingBarCollapsibleButton.enabled = False
    self.scalingBarCollapsibleButton.text = "Scaling Bar"
    self.layout.addWidget(self.scalingBarCollapsibleButton)

    # Layout within the dummy collapsible button
    scalingBarFormLayout = qt.QFormLayout(self.scalingBarCollapsibleButton)

    #
    # Scaling Bar Activation Checkbox
    #
    self.showScalingBarCheckBox = qt.QCheckBox('Show Scaling Bar')
    scalingBarFormLayout.addRow(self.showScalingBarCheckBox)

    # connections

    # Add vertical spacer
    self.layout.addStretch(1)
    self.sliceViewAnnotationsCheckBox.connect('clicked()', self.updateSliceViewFromGUI)
    self.showScalingBarCheckBox.connect('clicked()', self.updateSliceViewFromGUI)

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
    print 'updateSliceViewFromGUI'
    # Create corner annotations if have not created already
    if len(self.sliceCornerAnnotations.items()) == 0:
      self.createCornerAnnotations()

    # Updating font size and family
    if self.timesFontRadioButton.checked:
      fontFamily = 'Times'
    else:
      fontFamily = 'Arial'
    fontSize = self.fontSizeSpinBox.value

    for sliceViewName in self.sliceViewNames:
      cornerAnnotation = self.sliceCornerAnnotations[sliceViewName]
      cornerAnnotation.SetMaximumFontSize(fontSize)
      cornerAnnotation.SetMinimumFontSize(fontSize)
      textProperty = cornerAnnotation.GetTextProperty()
      if fontFamily == 'Times':
        textProperty.SetFontFamilyToTimes()
      else:
        textProperty.SetFontFamilyToArial()

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

    if self.cornerActivationCheckbox[3].checked:
      self.bottomRightAnnotationDisplay = True
    else:
      self.bottomRightAnnotationDisplay = False

    if self.sliceViewAnnotationsCheckBox.checked:
      self.cornerActivationsGroupBox.enabled = True
      self.fontPropertiesGroupBox.enabled = True
      self.scalingBarCollapsibleButton.enabled = True

      for sliceViewName in self.sliceViewNames:
        sliceWidget = self.layoutManager.sliceWidget(sliceViewName)
        sl = sliceWidget.sliceLogic()
        #bl =sl.GetBackgroundLayer()
        self.makeAnnotationText(sl)
    else:
      self.cornerActivationsGroupBox.enabled = False
      self.fontPropertiesGroupBox.enabled = False
      self.scalingBarCollapsibleButton.enabled = False
      # Remove Observers
      for sliceViewName in self.sliceViewNames:
        sliceWidget = self.layoutManager.sliceWidget(sliceViewName)
        sl = sliceWidget.sliceLogic()
        sl.RemoveAllObservers()

      # Clear Annotations
      for sliceViewName in self.sliceViewNames:
        self.sliceCornerAnnotations[sliceViewName].SetText(0, "")
        self.sliceCornerAnnotations[sliceViewName].SetText(1, "")
        self.sliceCornerAnnotations[sliceViewName].SetText(2, "")
        self.sliceCornerAnnotations[sliceViewName].SetText(3, "")
        self.sliceViews[sliceViewName].scheduleRender()

  def createGlobalVariables(self):
    self.sliceViewNames = []
    self.sliceWidgets = {}
    self.sliceViews = {}
    self.cameras = {}
    self.blNodeObserverTag = {}
    self.sliceLogicObserverTag = {}
    self.sliceCornerAnnotations = {}
    self.renderers = {}
    self.scalingBarActors = {}
    self.points = {}
    self.scalingBarTextActors = {}

  def createCornerAnnotations(self):
    print 'createCornerAnnotations'
    self.createGlobalVariables()
    sliceViewNames = self.layoutManager.sliceViewNames()

    for sliceViewName in sliceViewNames:
      self.sliceViewNames.append(sliceViewName)
    for sliceViewName in self.sliceViewNames:
      self.addObserver(sliceViewName)
      self.createActors(sliceViewName)

  def addObserver(self, sliceViewName):
    print('addObserver(%s)'%sliceViewName)
    sliceWidget = self.layoutManager.sliceWidget(sliceViewName)
    self.sliceWidgets[sliceViewName] = sliceWidget
    sliceView = sliceWidget.sliceView()

    self.sliceViews[sliceViewName] = sliceView
    self.sliceCornerAnnotations[sliceViewName] = sliceView.cornerAnnotation()
    sliceLogic = sliceWidget.sliceLogic()
    self.sliceLogicObserverTag[sliceViewName] = sliceLogic.AddObserver(vtk.vtkCommand.ModifiedEvent,
                                             self.updateCornerAnnotations)

  def createActors(self, sliceViewName):
    print('createActors(%s)'%sliceViewName)
    sliceWidget = self.layoutManager.sliceWidget(sliceViewName)
    self.sliceWidgets[sliceViewName] = sliceWidget
    sliceView = sliceWidget.sliceView()

    renderWindow = sliceView.renderWindow()
    renderer = renderWindow.GetRenderers().GetItemAsObject(0)
    self.renderers[sliceViewName] = renderer
    self.scalingBarTextActors[sliceViewName] = vtk.vtkTextActor()
    self.scalingBarActors[sliceViewName] = vtk.vtkActor2D()
    # Create Scale Bar
    self.createScaleBar(sliceViewName)

  def createScaleBar(self, sliceViewName):
    print('createScaleBar(%s)'%sliceViewName)
    renderer = self.renderers[sliceViewName]
    #actor = self.scalingBarTextActors[sliceViewName]
    #textActor = self.scalingBarTextActors[sliceViewName]
    #self.createActors(sliceViewName)
    #
    # Create the Scaling Bar
    #
    self.points[sliceViewName] = vtk.vtkPoints()
    self.points[sliceViewName].SetNumberOfPoints(10)

    # Create line#0
    line0 = vtk.vtkLine()
    line0.GetPointIds().SetId(0,0)
    line0.GetPointIds().SetId(1,1)

    # Create line#1
    line1 = vtk.vtkLine()
    line1.GetPointIds().SetId(0,0)
    line1.GetPointIds().SetId(1,2)

    # Create line#2
    line2 = vtk.vtkLine()
    line2.GetPointIds().SetId(0,2)
    line2.GetPointIds().SetId(1,3)

    # Create line#3
    line3 = vtk.vtkLine()
    line3.GetPointIds().SetId(0,2)
    line3.GetPointIds().SetId(1,4)

    # Create line#4
    line4 = vtk.vtkLine()
    line4.GetPointIds().SetId(0,4)
    line4.GetPointIds().SetId(1,5)

    # Create line#5
    line5 = vtk.vtkLine()
    line5.GetPointIds().SetId(0,4)
    line5.GetPointIds().SetId(1,6)

    # Create line#6
    line6 = vtk.vtkLine()
    line6.GetPointIds().SetId(0,6)
    line6.GetPointIds().SetId(1,7)

    # Create line#7
    line7 = vtk.vtkLine()
    line7.GetPointIds().SetId(0,6)
    line7.GetPointIds().SetId(1,8)

    # Create line#8
    line8 = vtk.vtkLine()
    line8.GetPointIds().SetId(0,8)
    line8.GetPointIds().SetId(1,9)

    # Create a cell array to store the lines in and add the lines to it
    lines = vtk.vtkCellArray()
    lines.InsertNextCell(line0)
    lines.InsertNextCell(line1)
    lines.InsertNextCell(line2)
    lines.InsertNextCell(line3)
    lines.InsertNextCell(line4)
    lines.InsertNextCell(line5)
    lines.InsertNextCell(line6)
    lines.InsertNextCell(line7)
    lines.InsertNextCell(line8)

    # Create a polydata to store everything in
    linesPolyData = vtk.vtkPolyData()

    # Add the points to the dataset
    linesPolyData.SetPoints(self.points[sliceViewName])

    # Add the lines to the dataset
    linesPolyData.SetLines(lines)

    #self.scalingBarTextActors[sliceViewName] = vtk.vtkTextActor()

    # mapper
    mapper = vtk.vtkPolyDataMapper2D()
    if vtk.VTK_MAJOR_VERSION <= 5:
      mapper.SetInput(linesPolyData)
    else:
      mapper.SetInputData(linesPolyData)
    # actor
    #self.scalingBarActors[sliceViewName] = vtk.vtkActor2D()
    actor = self.scalingBarActors[sliceViewName]
    actor.SetMapper(mapper)
    # color actor
    actor.GetProperty().SetColor(1,1,1)
    actor.GetProperty().SetLineWidth(1)
    textActor = self.scalingBarTextActors[sliceViewName]
    textProperty = textActor.GetTextProperty()
    # Turn off shadow
    textProperty.ShadowOff()
    if self.showScalingBarCheckBox.checked:
      renderer.AddActor(actor)
      renderer.AddActor(textActor)

  def updateCornerAnnotations(self,caller,event):
    print 'updateCornerAnnotations'
    sliceViewNames = self.layoutManager.sliceViewNames()
    for sliceViewName in sliceViewNames:
      if sliceViewName not in self.sliceViewNames:
        self.sliceViewNames.append(sliceViewName)
        self.addObserver(sliceViewName)
        self.createActors(sliceViewName)
        #self.updateSliceViewFromGUI()
    self.makeAnnotationText(caller)

  def sliceLogicModifiedEvent(self, caller,event):
    self.updateLayersAnnotation(caller)

  def makeAnnotationText(self, sliceLogic):
    #print('makeAnnotationText(%s)'%sliceLogic)
    print 'makeAnnotationText'
    self.resetTexts()
    sliceCompositeNode = sliceLogic.GetSliceCompositeNode()

    backgroundLayer = sliceLogic.GetBackgroundLayer()
    foregroundLayer = sliceLogic.GetForegroundLayer()
    labelLayer = sliceLogic.GetLabelLayer()

    backgroundVolume = backgroundLayer.GetVolumeNode()
    foregroundVolume = foregroundLayer.GetVolumeNode()
    labelVolume = labelLayer.GetVolumeNode()

    sliceNode = backgroundLayer.GetSliceNode()
    sliceViewName = sliceNode.GetLayoutName()
    self.currentSliceViewName = sliceNode.GetLayoutName()

    if self.sliceViews[self.currentSliceViewName]:

      self.minimumWidthForScalingRuler = 300

      viewWidth = self.sliceViews[self.currentSliceViewName].width
      viewHeight = self.sliceViews[self.currentSliceViewName].height

      #self.createScaleBar(sliceViewName)
      rasToXY = vtk.vtkMatrix4x4()
      m = sliceNode.GetXYToRAS()
      rasToXY.DeepCopy(m)
      rasToXY.Invert()
      #print rasToXY
      scalingFactorString = " mm"

      #print rasToXY
      # TODO: Fix the bug
      scalingFactor = 1
      import math
      scalingFactor = math.sqrt( rasToXY.GetElement(0,0)**2 +
          rasToXY.GetElement(0,1)**2 +rasToXY.GetElement(0,2) **2 )

      rulerArea = viewWidth/scalingFactor/7

      if self.showScalingBarCheckBox.checked and \
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
        pts.SetPoint(0,[(viewWidth-RASRulerSize*scalingFactor)/2,10, 0])
        pts.SetPoint(1,[(viewWidth-RASRulerSize*scalingFactor)/2,20, 0])
        pts.SetPoint(2,[(viewWidth-RASRulerSize*scalingFactor/2)/2,10, 0])
        pts.SetPoint(3,[(viewWidth-RASRulerSize*scalingFactor/2)/2,17, 0])
        pts.SetPoint(4,[viewWidth/2,10, 0])
        pts.SetPoint(5,[viewWidth/2,20, 0])
        pts.SetPoint(6,[(viewWidth+RASRulerSize*scalingFactor/2)/2,10, 0])
        pts.SetPoint(7,[(viewWidth+RASRulerSize*scalingFactor/2)/2,17, 0])
        pts.SetPoint(8,[(viewWidth+RASRulerSize*scalingFactor)/2,10, 0])
        pts.SetPoint(9,[(viewWidth+RASRulerSize*scalingFactor)/2,20, 0])
        textActor = self.scalingBarTextActors[self.currentSliceViewName]

        textActor.SetInput(scalingFactorString)

        textActor.SetDisplayPosition(int((viewWidth+RASRulerSize*scalingFactor)/2)+10,7)

        self.renderers[self.currentSliceViewName].AddActor(
            self.scalingBarActors[self.currentSliceViewName])
        self.renderers[self.currentSliceViewName].AddActor(textActor)
      else:
        self.renderers[self.currentSliceViewName].RemoveActor(
            self.scalingBarActors[self.currentSliceViewName])
        self.renderers[self.currentSliceViewName].RemoveActor(
            self.scalingBarTextActors[self.currentSliceViewName])

      rw = self.sliceViews[sliceViewName].renderWindow()

      # Both background and foregraound
      if ( backgroundVolume != None and foregroundVolume != None):
        foregroundOpacity = sliceCompositeNode.GetForegroundOpacity()
        backgroundVolumeName = backgroundVolume.GetName()
        foregroundVolumeName = foregroundVolume.GetName()
        self.cornerTexts[0]['3-Background'] = 'B: ' + backgroundVolumeName
        self.cornerTexts[0]['2-Foreground'] = 'F: ' + foregroundVolumeName +  ' (' + str(
                      "%.1f"%foregroundOpacity) + ')'

        bgUids = backgroundVolume.GetAttribute('DICOM.instanceUIDs')
        fgUids = foregroundVolume.GetAttribute('DICOM.instanceUIDs')
        if (bgUids and fgUids):
          bgUid = bgUids.partition(' ')[0]
          fgUid = fgUids.partition(' ')[0]
          self.makeDicomAnnotation(bgUid,fgUid)
        else:
          for key in self.cornerTexts[2]:
            self.cornerTexts[2][key] = ''

      # Only background
      elif (backgroundVolume != None):
        backgroundVolumeName = backgroundVolume.GetName()
        if self.bottomLeftAnnotationDisplay:
          self.cornerTexts[0]['3-Background'] = 'B: ' + backgroundVolumeName

        uids = backgroundVolume.GetAttribute('DICOM.instanceUIDs')
        if uids:
          uid = uids.partition(' ')[0]
          self.makeDicomAnnotation(uid,None)

      # Only foreground
      elif (foregroundVolume != None):
        if self.bottomLeftAnnotationDisplay:
          foregroundVolumeName = foregroundVolume.GetName()
          self.cornerTexts[0]['2-Foreground'] = 'F: ' + foregroundVolumeName

        uids = foregroundVolume.GetAttribute('DICOM.instanceUIDs')
        if uids:
          uid = uids.partition(' ')[0]
          # passed UID as bg
          self.makeDicomAnnotation(uid,None)

      if (labelVolume != None):
        labelOpacity = sliceCompositeNode.GetLabelOpacity()
        labelVolumeName = labelVolume.GetName()
        self.cornerTexts[0]['1-Label'] = 'L: ' + labelVolumeName + ' (' + str(
                      "%.1f"%labelOpacity) + ')'

      self.drawCornerAnnotations()
      #labelOpacity = sliceCompositeNode.GetLabelOpacity()

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
                self.cornerTexts[2][key] = ''
        else:
          self.cornerTexts[2]['1-PatientName'] = backgroundDicomDic['Patient Name'].replace('^',', ')
          self.cornerTexts[2]['2-PatientID'] = 'ID: ' + backgroundDicomDic['Patient ID']
          backgroundDicomDic['Patient Birth Date'] = self.formatDICOMDate(backgroundDicomDic['Patient Birth Date'])
          self.cornerTexts[2]['3-PatientInfo'] = self.makePatientInfo(backgroundDicomDic)

          if (backgroundDicomDic['Study Date'] != foregroundDicomDic['Study Date']):
            self.cornerTexts[2]['4-Bg-StudyDate'] = 'B: ' + self.formatDICOMDate(backgroundDicomDic['Study Date'])
            self.cornerTexts[2]['5-Fg-StudyDate'] = 'F: ' + self.formatDICOMDate(foregroundDicomDic['Study Date'])
          else:
            self.cornerTexts[2]['4-Bg-StudyDate'] =  self.formatDICOMDate(backgroundDicomDic['Study Date'])

          if (backgroundDicomDic['Study Time'] != foregroundDicomDic['Study Time']):
            self.cornerTexts[2]['6-Bg-StudyTime'] = 'B: ' + self.formatDICOMTime(backgroundDicomDic['Study Time'])
            self.cornerTexts[2]['7-Fg-StudyTime'] = 'F: ' + self.formatDICOMTime(foregroundDicomDic['Study Time'])
          else:
            self.cornerTexts[2]['6-Bg-StudyTime'] = self.formatDICOMTime(backgroundDicomDic['Study Time'])

          if (backgroundDicomDic['Series Description'] != foregroundDicomDic['Series Description']):
            self.cornerTexts[2]['8-Bg-SeriesDescription'] = 'B: ' + backgroundDicomDic['Series Description']
            self.cornerTexts[2]['9-Fg-SeriesDescription'] = 'F: ' + foregroundDicomDic['Series Description']
          else:
            self.cornerTexts[2]['8-Bg-SeriesDescription'] = backgroundDicomDic['Series Description']

    # Only Background or Only Foreground
    else:
      uid = bgUid
      dicomDic = self.extractDICOMValues(uid)
      if self.topLeftAnnotationDisplay:
        self.cornerTexts[2]['1-PatientName'] = dicomDic['Patient Name'].replace('^',', ')
        self.cornerTexts[2]['2-PatientID'] = 'ID: ' + dicomDic ['Patient ID']
        dicomDic['Patient Birth Date'] = self.formatDICOMDate(dicomDic['Patient Birth Date'])
        self.cornerTexts[2]['3-PatientInfo'] = self.makePatientInfo(dicomDic)
        self.cornerTexts[2]['4-Bg-StudyDate']  = self.formatDICOMDate(dicomDic['Study Date'])
        self.cornerTexts[2]['6-Bg-StudyTime'] = self.formatDICOMTime(dicomDic['Study Time'])
        self.cornerTexts[2]['8-Bg-SeriesDescription'] = dicomDic['Series Description']

      if (self.sliceWidgets[self.currentSliceViewName].width > 600 and self.topRightAnnotationDisplay):
        self.cornerTexts[3]['1-Institution-Name'] = dicomDic['Institution Name']
        self.cornerTexts[3]['2-Referring-Phisycian'] = dicomDic['Referring Physician Name'].replace('^',', ')
        self.cornerTexts[3]['3-Manufacturer'] = dicomDic['Manufacturer']
        self.cornerTexts[3]['4-Model'] = dicomDic['Model']
        self.cornerTexts[3]['5-Patient-Position'] = dicomDic['Patient Position']
        modality = dicomDic['Modality']
        if modality == 'MR':
         self.cornerTexts[3]['6-TR']  = 'TR ' + dicomDic['Repetition Time']
         self.cornerTexts[3]['7-TE'] = 'TE ' + dicomDic['Echo Time']

      # Bottom Right Corner Annotations:
      # Modality Specific and Image Comments
      #
      #if self.bottomRightAnnotationDisplay:

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

  def drawCornerAnnotations(self):
    cornerAnnotation = ''
    for i, cornerText in enumerate(self.cornerTexts):
      keys = sorted(cornerText.keys())
      cornerAnnotation = ''
      for key in keys:
        if ( cornerText[key] != ''):
          cornerAnnotation = cornerAnnotation+ cornerText[key] + '\n'
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
      self.cornerTexts[i] =  dict((k,'') for k,v in cornerText.iteritems())

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
    if self.bottomRightAnnotationDisplay:
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
