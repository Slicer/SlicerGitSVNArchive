import os, glob, sys, copy
import qt
import vtk
import ctk
import slicer
from slicer.util import VTKObservationMixin

from slicer.util import settingsValue, toBool
import DICOMLib

#########################################################
#
#
comment = """

DICOMWidgets are helper classes to build an interface
to manage DICOM data in the context of slicer.

This code is slicer-specific and relies on the slicer python module
for elements like slicer.dicomDatabase and slicer.mrmlScene

"""
#
#########################################################

def setDatabasePrecacheTags(dicomBrowser=None):
  """query each plugin for tags that should be cached on import
     and set them for the dicom app widget and slicer"""
  if not slicer.dicomDatabase:
    return
  tagsToPrecache = list(slicer.dicomDatabase.tagsToPrecache)
  for pluginClass in slicer.modules.dicomPlugins:
    plugin = slicer.modules.dicomPlugins[pluginClass]()
    tagsToPrecache += plugin.tags.values()
  tagsToPrecache = list(set(tagsToPrecache))  # remove duplicates
  tagsToPrecache.sort()
  slicer.dicomDatabase.tagsToPrecache = tagsToPrecache
  if dicomBrowser:
    dicomBrowser.tagsToPrecache = tagsToPrecache

class DICOMDetailsPopup(VTKObservationMixin):
  """Implement the Qt window showing details and possible
  operations to perform on the selected dicom list item.
  This is a helper used in the DICOMWidget class.
  """

  def __init__(self,dicomBrowser=None):
    VTKObservationMixin.__init__(self)
    self.dicomBrowser = dicomBrowser
    
    # initialize the dicomDatabase
    #   - pick a default and let the user know
    if not slicer.dicomDatabase:
      self.promptForDatabaseDirectory()
    
    if self.dicomBrowser is None:
      # This creates a DICOM database in the current working directory if nothing else
      # is specified in the settings, therefore promptForDatabaseDirectory must be called before this.
      self.dicomBrowser = ctk.ctkDICOMBrowser()

    self.browserPersistent = settingsValue('DICOM/BrowserPersistent', False, converter=toBool)
    self.tableDensity = settingsValue('DICOM/tableDensity', 'Compact')
    self.popupGeometry = settingsValue('DICOM/detailsPopup.geometry', qt.QRect())
    self.advancedView = settingsValue('DICOM/advancedView', 0, converter=int)
    self.horizontalTables = settingsValue('DICOM/horizontalTables', 0, converter=int)

    self.create()
    self.popupPositioned = False
    self.pluginInstances = {}
    self.fileLists = []

    setDatabasePrecacheTags(self.dicomBrowser)

    self.dicomBrowser.connect('databaseDirectoryChanged(QString)', self.onDatabaseDirectoryChanged)

    # Update visibility
    for name in [
        'ActionImport', 'ActionExport', 'ActionQuery', 'ActionSend', 'ActionRemove', 'ActionRepair',
        'ActionViewMetadata',
        'AdvancedViewCheckBox', 'HorizontalViewCheckBox', 'BrowserPersistentCheckBox'
        ]:
      visible = settingsValue('DICOM/%s.visible' % name, True, converter=toBool)
      control = self._findChildren(name)
      control.visible = visible

    # Define set of widgets that should be hidden/shown together
    self.settingsWidgetNames = {
      'DatabaseButton' : ('DatabaseNameLabel', 'DirectoryButton'),
      'TableDensityComboBox' : ('tablesDensityLabel', 'tableDensityComboBox')
      }

    # Hide the settings button if all associated widgets should be hidden
    settingsButtonHidden = True
    for groupName in self.settingsWidgetNames.keys():
      settingsButtonHidden = settingsButtonHidden and not settingsValue('DICOM/%s.visible' % groupName, True, converter=toBool)
    self.settingsButton.visible = not settingsButtonHidden

  def _findChildren(self, name):
    """Since the ctkDICOMBrowser widgets stollen by the Slicer DICOM browser
    loses their objectName when they are re-parented, this convenience function
    will search in both the ``self.window`` and ``self.dicomBrowser``.
    """
    try:
      return slicer.util.findChildren(self.window, name=name)[0]
    except IndexError:
      return slicer.util.findChildren(self.dicomBrowser, name=name)[0]

  def create(self,widgetType='window',showHeader=False,showPreview=False):
    """
    main window is a frame with widgets from the app
    widget repacked into it along with slicer-specific
    extra widgets
    """

    # find internals of widget for reference and repacking
    self.toolBar = slicer.util.findChildren(self.dicomBrowser, 'ToolBar')[0]
    self.databaseNameLabel = slicer.util.findChildren(self.dicomBrowser, 'DatabaseNameLabel')[0]
    self.databaseDirectoryButton = slicer.util.findChildren(self.dicomBrowser, 'DirectoryButton')[0]
    self.tableDensityLabel = qt.QLabel('Density: ')
    self.tableDensityLabel.objectName = 'tablesDensityLabel'
    self.tableDensityComboBox = slicer.util.findChildren(self.dicomBrowser, 'tableDensityComboBox')[0]
    self.tableDensityComboBox.connect('currentIndexChanged(QString)', self.onTableDensityComboBox)
    index = self.tableDensityComboBox.findText(self.tableDensity)
    if  (index != -1) :
      self.tableDensityComboBox.setCurrentIndex(index)

    #
    # create and configure the Slicer browser widget - this involves
    # reaching inside and manipulating the widget hierarchy
    # - TODO: this configurability should be exposed more natively
    #   in the CTK code to avoid the findChildren calls
    #
    self.tables = slicer.util.findChildren(self.dicomBrowser, 'dicomTableManager')[0]
    patientTable = slicer.util.findChildren(self.tables, 'patientsTable')[0]
    patientTableView = slicer.util.findChildren(patientTable, 'tblDicomDatabaseView')[0]
    patientSearchBox = slicer.util.findChildren(patientTable, 'leSearchBox')[0]
    studyTable = slicer.util.findChildren(self.tables, 'studiesTable')[0]
    studyTableView = slicer.util.findChildren(studyTable, 'tblDicomDatabaseView')[0]
    studySearchBox = slicer.util.findChildren(studyTable, 'leSearchBox')[0]
    seriesTable = slicer.util.findChildren(self.tables, 'seriesTable')[0]
    seriesTableView = slicer.util.findChildren(seriesTable, 'tblDicomDatabaseView')[0]
    seriesSearchBox = slicer.util.findChildren(seriesTable, 'leSearchBox')[0]
    self.tableSplitter = qt.QSplitter()
    self.tableSplitter.addWidget(patientTableView)
    self.tableSplitter.addWidget(studyTableView)
    self.tableSplitter.addWidget(seriesTableView)

    # TODO: Move to this part to CTK
    patientTableView.resizeColumnsToContents()
    studyTableView.resizeColumnsToContents()
    seriesTableView.resizeColumnsToContents()

    self.userFrame = qt.QWidget()
    self.preview = qt.QWidget()

    self.widgetType = widgetType
    if widgetType == 'dialog':
      self.window = qt.QDialog(self.dicomBrowser)
    elif widgetType == 'window':
      self.window = qt.QWidget()
    elif widgetType == 'popup':
      self.window = ctk.ctkPopupWidget(self.dicomBrowser)
      self.window.orientation = 1
      self.window.horizontalDirection = 0
      self.window.alignment = 0x82
    elif widgetType == 'dock':
      self.dock = qt.QDockWidget(slicer.util.mainWindow())
      self.dock.setFeatures( qt.QDockWidget.DockWidgetFloatable |
                                qt.QDockWidget.DockWidgetMovable |
                                qt.QDockWidget.DockWidgetClosable )
      slicer.util.mainWindow().addDockWidget(0x15, self.dock)
      self.window = qt.QFrame()
      self.dock.setWidget(self.window)
    else:
      raise "Unknown widget type - should be dialog, window, dock or popup"

    self.window.objectName = 'SlicerDICOMBrowser'

    self.setModality(not self.browserPersistent)

    self.window.setWindowTitle('DICOM Browser')

    self.layout = qt.QVBoxLayout(self.window)

    # tool row at top, with commands and database
    self.toolFrame = qt.QWidget()
    self.toolFrame.setMaximumHeight(40)
    self.toolFrame.setContentsMargins(-5,-5,-5,-5)
    self.toolLayout = qt.QHBoxLayout(self.toolFrame)
    self.layout.addWidget(self.toolFrame)
    self.toolLayout.addWidget(self.toolBar)
    self.settingsButton = ctk.ctkExpandButton()
    self.toolLayout.addWidget(self.settingsButton)
    self.toolLayout.addWidget(self.databaseNameLabel)
    self.databaseNameLabel.visible = False
    self.toolLayout.addWidget(self.databaseDirectoryButton)
    self.databaseDirectoryButton.visible = False
    self.toolLayout.addWidget(self.tableDensityLabel)
    self.tableDensityLabel.visible = False
    self.toolLayout.addWidget(self.tableDensityComboBox)
    self.tableDensityComboBox.visible = False
    self.settingsButton.connect('toggled(bool)', self.onSettingsButton)

    # enable export button and make new connection
    self.actionExport = self.dicomBrowser.findChildren('QAction', 'ActionExport')[0]
    self.actionExport.enabled = 1
    self.actionExport.connect('triggered()', self.onExportAction)

    # search row
    self.searchFrame = qt.QWidget()
    self.searchFrame.setMaximumHeight(40)
    self.searchLayout = qt.QHBoxLayout(self.searchFrame)
    self.layout.addWidget(self.searchFrame)
    patinetsLabel = qt.QLabel('Patients: ')
    self.searchLayout.addWidget(patinetsLabel)
    self.searchLayout.addWidget(patientSearchBox)
    studiesLabel = qt.QLabel('Studies: ')
    self.searchLayout.addWidget(studiesLabel)
    self.searchLayout.addWidget(studySearchBox)
    seriesLabel = qt.QLabel('Series: ')
    self.searchLayout.addWidget(seriesLabel)
    self.searchLayout.addWidget(seriesSearchBox)

    # tables goes next, spread across 1 row, 2 columns
    if self.horizontalTables:
      self.tableSplitter.setOrientation(1)
    else:
      self.tableSplitter.setOrientation(0)
    self.layout.addWidget(self.tableSplitter)

    #
    # preview related column
    #
    self.previewLayout = qt.QVBoxLayout()
    if showPreview:
      self.previewLayout.addWidget(self.preview)
    else:
      self.preview.hide()

    #
    # action related column (interacting with slicer)
    #
    self.loadableTableFrame = qt.QWidget()
    self.loadableTableFrame.setMaximumHeight(200)
    self.loadableTableLayout = qt.QFormLayout(self.loadableTableFrame)
    self.layout.addWidget(self.loadableTableFrame)

    self.loadableTableLayout.addWidget(self.userFrame)
    self.userFrame.hide()

    tableWidth = 350 if showHeader else 600
    self.loadableTable = DICOMLoadableTable(self.userFrame,width=tableWidth)

    #
    # button row for action column
    #
    self.actionButtonsFrame = qt.QWidget()
    self.actionButtonsFrame.setMaximumHeight(40)
    self.actionButtonsFrame.objectName = 'ActionButtonsFrame'
    self.layout.addWidget(self.actionButtonsFrame)

    self.actionButtonLayout = qt.QHBoxLayout()
    self.actionButtonsFrame.setLayout(self.actionButtonLayout)

    self.loadButton = qt.QPushButton('Load')
    self.loadButton.enabled = True
    self.loadButton.toolTip = 'Load Selection to Slicer'
    self.actionButtonLayout.addWidget(self.loadButton)
    self.loadButton.connect('clicked()', self.loadCheckedLoadables)

    self.headerPopup = DICOMLib.DICOMHeaderPopup()

    self.viewMetadataButton = qt.QPushButton('Metadata')
    self.viewMetadataButton.objectName = 'ActionViewMetadata'
    self.viewMetadataButton.toolTip = 'Display Metadata of the Selected Series'
    self.viewMetadataButton.enabled = False
    self.actionButtonLayout.addWidget(self.viewMetadataButton)
    self.viewMetadataButton.connect('clicked()', self.onViewHeaderButton)
    self.viewMetadataButton.connect('clicked()', self.headerPopup.open)
    self.actionButtonLayout.addStretch(1)

    self.examineButton = qt.QPushButton('Examine')
    self.actionButtonLayout.addWidget(self.examineButton)
    self.examineButton.enabled = False
    self.examineButton.connect('clicked()', self.examineForLoading)

    self.uncheckAllButton = qt.QPushButton('Uncheck All')
    self.actionButtonLayout.addWidget(self.uncheckAllButton)
    self.uncheckAllButton.connect('clicked()', self.uncheckAllLoadables)
    self.actionButtonLayout.addStretch(1)

    self.advancedViewButton = qt.QCheckBox('Advanced')
    self.advancedViewButton.objectName = 'AdvancedViewCheckBox'
    self.actionButtonLayout.addWidget(self.advancedViewButton)
    self.advancedViewButton.enabled = True
    self.advancedViewButton.checked = self.advancedView
    self.advancedViewButton.connect('clicked()', self.onAdvanedViewButton)

    self.horizontalViewCheckBox = qt.QCheckBox('Horizontal')
    self.horizontalViewCheckBox.objectName = 'HorizontalViewCheckBox'
    self.horizontalViewCheckBox.checked = self.horizontalTables
    self.horizontalViewCheckBox.connect('clicked()', self.onHorizontalViewCheckBox)
    self.actionButtonLayout.addWidget(self.horizontalViewCheckBox)
    self.toolLayout.addStretch(1)

    self.browserPersistentButton = qt.QCheckBox('Browser Persistent')
    self.browserPersistentButton.objectName = 'BrowserPersistentCheckBox'
    self.browserPersistentButton.toolTip = 'When enabled, DICOM Browser remains open after loading data or switching to another module'
    self.browserPersistentButton.checked = self.browserPersistent
    self.actionButtonLayout.addWidget(self.browserPersistentButton)
    self.browserPersistentButton.connect('stateChanged(int)', self.setBrowserPersistence)

    if self.advancedView:
      self.loadableTableFrame.visible = True
    else:
      self.loadableTableFrame.visible = False
      self.examineButton.visible = False
      self.uncheckAllButton.visible = False
    #
    # header related column (more details about the selected file)
    #
    if showHeader:
      self.headerLayout = qt.QVBoxLayout()
      self.layout.addLayout(self.headerLayout,selectionRow,2)
      self.header = DICOMHeaderWidget(self.window)
      self.headerLayout.addWidget(self.header.widget)

    #
    # Series selection
    #
    self.tables.connect('seriesSelectionChanged(QStringList)', self.onSeriesSelected)

    #
    # Plugin selection widget
    #
    self.pluginSelector = DICOMPluginSelector(self.window)
    self.loadableTableLayout.addRow(self.pluginSelector.widget,self.loadableTable.widget)
    self.checkBoxByPlugins = []

    for pluginClass in slicer.modules.dicomPlugins:
      self.checkBox = self.pluginSelector.checkBoxByPlugin[pluginClass]
      self.checkBox.connect('stateChanged(int)', self.onPluginStateChanged)
      self.checkBoxByPlugins.append(self.checkBox)

  def onDatabaseDirectoryChanged(self,databaseDirectory):
    if not hasattr(slicer, 'dicomDatabase') or not slicer.dicomDatabase:
      slicer.dicomDatabase = ctk.ctkDICOMDatabase()
    setDatabasePrecacheTags(self.dicomBrowser)
    databaseFilepath = databaseDirectory + "/ctkDICOM.sql"
    messages = ""
    if not os.path.exists(databaseDirectory):
      try:
        os.mkdir(databaseDirectory)
      except OSError:
        messages += "Directory does not exist and cannot be created. "
    else:
      if not os.access(databaseDirectory, os.W_OK):
        messages += "Directory not writable. "
      if not os.access(databaseDirectory, os.R_OK):
        messages += "Directory not readable. "
      if os.listdir(databaseDirectory) and not os.path.isfile(databaseFilepath):
        # Prevent users from the error of trying to import a DICOM directory by selecting it as DICOM database path
        messages += "Directory is not empty and not an existing DICOM database."
        
    if messages != "":
      slicer.util.warningDisplay('The database file path "%s" cannot be used.  %s\n'
                                 'Please pick a different database directory using the '
                                 'LocalDatabase button in the DICOM Browser' % (databaseFilepath,messages),
                                 windowTitle="DICOM")
    else:
      slicer.dicomDatabase.openDatabase(databaseDirectory + "/ctkDICOM.sql", "SLICER")
      if not slicer.dicomDatabase.isOpen:
        slicer.util.warningDisplay('The database file path "%s" cannot be opened.\n'
                                   'Please pick a different database directory using the '
                                   'LocalDatabase button in the DICOM Browser.' % databaseFilepath,
                                   windowTitle="DICOM")
        self.dicomDatabase = None
      else:
        if self.dicomBrowser:
          if self.dicomBrowser.databaseDirectory != databaseDirectory:
            self.dicomBrowser.databaseDirectory = databaseDirectory
        else:
          settings = qt.QSettings()
          settings.setValue('DatabaseDirectory', databaseDirectory)
          settings.sync()
    if slicer.dicomDatabase:
      slicer.app.setDICOMDatabase(slicer.dicomDatabase)

  def promptForDatabaseDirectory(self):
    """Ask the user to pick a database directory.
    But, if the application is in testing mode, just pick
    a temp directory
    """
    commandOptions = slicer.app.commandOptions()
    if commandOptions.testingEnabled:
      databaseDirectory = slicer.app.temporaryPath + '/tempDICOMDatbase'
      qt.QDir().mkpath(databaseDirectory)
      self.onDatabaseDirectoryChanged(databaseDirectory)
    else:
      settings = qt.QSettings()
      databaseDirectory = settings.value('DatabaseDirectory')
      if databaseDirectory:
        self.onDatabaseDirectoryChanged(databaseDirectory)
      else:
        # pick the user's Documents by default
        documentsLocation = qt.QDesktopServices.DocumentsLocation
        documents = qt.QDesktopServices.storageLocation(documentsLocation)
        databaseDirectory = documents + "/SlicerDICOMDatabase"
        message = "DICOM Database will be stored in\n\n"
        message += databaseDirectory
        message += "\n\nUse the Local Database button in the DICOM Browser "
        message += "to pick a different location."
        slicer.util.infoDisplay(message, windowTitle='DICOM')
        if not os.path.exists(databaseDirectory):
          os.mkdir(databaseDirectory)
        self.onDatabaseDirectoryChanged(databaseDirectory)

  def onTableDensityComboBox(self, state):
    settings = qt.QSettings()
    settings.setValue('DICOM/tableDensity', state)

  def onPluginStateChanged(self,state):
    settings = qt.QSettings()
    settings.beginWriteArray('DICOM/disabledPlugins')

    for key in settings.allKeys():
      settings.remove(key)

    plugins = self.pluginSelector.selectedPlugins()
    arrayIndex = 0
    for  pluginClass in slicer.modules.dicomPlugins:
      if pluginClass not in plugins:
        settings.setArrayIndex(arrayIndex)
        settings.setValue(pluginClass,'disabled')
        arrayIndex += 1

    settings.endArray()

  def setBrowserPersistence(self,state):
    self.browserPersistent = state
    self.setModality(not self.browserPersistent)
    settings = qt.QSettings()
    settings.setValue('DICOM/BrowserPersistent', bool(self.browserPersistent))

  def onSettingsButton(self, status):
    for groupName in self.settingsWidgetNames.keys():
      visible = settingsValue('DICOM/%s.visible' % groupName, True, converter=toBool)
      for name in self.settingsWidgetNames[groupName]:
        control = self._findChildren(name)
        control.visible = False
        if visible:
          control.visible = self.settingsButton.checked

  def onAdvanedViewButton(self):
    self.advancedView = self.advancedViewButton.checked
    advancedWidgets = [self.loadableTableFrame, self.examineButton,
        self.uncheckAllButton]
    for widget in advancedWidgets:
      widget.visible = self.advancedView
    self.loadButton.enabled = not self.advancedView

    settings = qt.QSettings()
    settings.setValue('DICOM/advancedView',int(self.advancedView))

  def onHorizontalViewCheckBox(self):
    settings = qt.QSettings()
    self.tableSplitter.setOrientation(self.horizontalViewCheckBox.checked)
    settings.setValue('DICOM/horizontalTables', int(self.horizontalViewCheckBox.checked))

  def onViewHeaderButton(self):
    self.headerPopup.setFileLists(self.fileLists)

  def onExportAction(self):
    self.exportDialog = slicer.qSlicerDICOMExportDialog()
    self.exportDialog.setMRMLScene(slicer.mrmlScene)
    self.close()
    self.exportDialog.execDialog()

  def open(self):
    if not self.window.isVisible():
      self.window.show()
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
    self.window.raise_()

  def close(self):
    self.onPopupGeometryChanged()
    self.window.hide()

  def onPopupGeometryChanged(self):
    settings = qt.QSettings()
    self.popupGeometry = self.window.geometry
    settings.setValue('DICOM/detailsPopup.geometry', self.window.geometry)

  def setModality(self,modality):
    if self.widgetType == 'dialog':
      self.window.setModal(modality)

  def organizeLoadables(self):
    """Review the selected state and confidence of the loadables
    across plugins so that the options the user is most likely
    to want are listed at the top of the table and are selected
    by default.  Only offer one pre-selected loadable per series
    unless both plugins mark it as selected and they have equal
    confidence."""

    # first, get all loadables corresponding to a series
    seriesUIDTag = "0020,000E"
    loadablesBySeries = {}
    for plugin in self.loadablesByPlugin:
      for loadable in self.loadablesByPlugin[plugin]:
        seriesUID = slicer.dicomDatabase.fileValue(loadable.files[0],seriesUIDTag)
        if not loadablesBySeries.has_key(seriesUID):
          loadablesBySeries[seriesUID] = [loadable]
        else:
          loadablesBySeries[seriesUID].append(loadable)

    # now for each series, find the highest confidence selected loadables
    # and set all others to be unselected
    for series in loadablesBySeries:
      highestConfidenceValue = -1
      for loadable in loadablesBySeries[series]:
        if loadable.confidence > highestConfidenceValue:
          highestConfidenceValue = loadable.confidence
      for loadable in loadablesBySeries[series]:
        if loadable.confidence < highestConfidenceValue:
          loadable.selected = False

  def onSeriesSelected(self,seriesUIDList):
    self.offerLoadables(seriesUIDList, "SeriesUIDList")

  def offerLoadables(self,uidArgument,role):
    """Get all the loadable options at the currently selected level
    and present them in the loadable table"""
    self.loadableTable.setLoadables([])
    if self.advancedViewButton.checkState() == 2:
      self.loadButton.enabled = False
    self.fileLists = []
    if role == "Series":
      self.fileLists.append(slicer.dicomDatabase.filesForSeries(uidArgument))
    if role == "SeriesUIDList":
      for uid in uidArgument:
        uid = uid.replace("'","")
        self.fileLists.append(slicer.dicomDatabase.filesForSeries(uid))
    if role == "Study":
      series = slicer.dicomDatabase.seriesForStudy(uidArgument)
      for serie in series:
        self.fileLists.append(slicer.dicomDatabase.filesForSeries(serie))
    if role == "Patient":
      studies = slicer.dicomDatabase.studiesForPatient(uidArgument)
      for study in studies:
        series = slicer.dicomDatabase.seriesForStudy(study)
        for serie in series:
          fileList = slicer.dicomDatabase.filesForSeries(serie)
          self.fileLists.append(fileList)
    self.examineButton.enabled = len(self.fileLists) != 0
    self.viewMetadataButton.enabled = len(self.fileLists) != 0

  def uncheckAllLoadables(self):
    self.loadableTable.uncheckAll()

  def examineForLoading(self):
    """For selected plugins, give user the option
    of what to load"""

    loadEnabled = False
    (self.loadablesByPlugin,loadEnabled) = self.getLoadablesFromFileLists(self.fileLists)

    self.loadButton.enabled = loadEnabled
    #self.viewMetadataButton.enabled = loadEnabled
    self.organizeLoadables()
    self.loadableTable.setLoadables(self.loadablesByPlugin)

  def getLoadablesFromFileLists(self, fileLists):
    """Take list of file lists, return loadables by plugin dictionary
    """

    loadablesByPlugin = {}

    allFileCount = missingFileCount = 0
    for fileList in self.fileLists:
        for filePath in fileList:
          allFileCount += 1
          if not os.path.exists(filePath):
            missingFileCount += 1

    if missingFileCount > 0:
      slicer.util.warningDisplay("Warning: %d of %d selected files listed in the database cannot be found on disk."
                                 % (missingFileCount, allFileCount), windowTitle="DICOM")

    if missingFileCount == allFileCount:
      return

    nDicomPlugins = len(slicer.modules.dicomPlugins)
    self.progress = slicer.util.createProgressDialog(parent=self.window, value=0, maximum=nDicomPlugins)
    step = 0

    loadEnabled = False
    plugins = self.pluginSelector.selectedPlugins()
    for pluginClass in plugins:
      if not self.pluginInstances.has_key(pluginClass):
        self.pluginInstances[pluginClass] = slicer.modules.dicomPlugins[pluginClass]()
      plugin = self.pluginInstances[pluginClass]
      if self.progress.wasCanceled:
        break
      self.progress.labelText = '\nChecking %s' % pluginClass
      slicer.app.processEvents()
      self.progress.setValue(step)
      slicer.app.processEvents()
      try:
        loadablesByPlugin[plugin] = plugin.examineForImport(fileLists)
        # If regular method is not overridden (so returns empty list), try old function
        # Ensuring backwards compatibility: examineForImport used to be called examine
        if loadablesByPlugin[plugin] == []:
          loadablesByPlugin[plugin] = plugin.examine(fileLists)
        loadEnabled = loadEnabled or loadablesByPlugin[plugin] != []
      except Exception,e:
        import traceback
        traceback.print_exc()
        slicer.util.warningDisplay("Warning: Plugin failed: %s\n\nSee python console for error message." % pluginClass,
                                   windowTitle="DICOM", parent=self.window)
        print("DICOM Plugin failed: %s", str(e))
      step +=1

    self.progress.close()
    self.progress = None

    return (loadablesByPlugin,loadEnabled)

  def isFileListInCheckedLoadables(self,fileList):
    for plugin in self.loadablesByPlugin:
      for loadable in self.loadablesByPlugin[plugin]:
        if len(loadable.files) != len(fileList) or len(loadable.files) == 0:
          continue
        inputFileListCopy = copy.deepcopy(fileList)
        loadableFileListCopy = copy.deepcopy(loadable.files)
        try:
          inputFileListCopy.sort()
          loadableFileListCopy.sort()
        except Exception,e:
          pass
        for pair in zip(inputFileListCopy,loadableFileListCopy):
          if pair[0] != pair[1]:
            print(pair[0]+' != '+pair[1])
            continue
        return True
    return False

  def loadCheckedLoadables(self):
    """Invoke the load method on each plugin for the loadable
    (DICOMLoadable or qSlicerDICOMLoadable) instances that are selected"""
    if self.advancedViewButton.checkState() == 0:
      self.examineForLoading()

    self.loadableTable.updateSelectedFromCheckstate()

    # TODO: add check that disables all referenced stuff to be considered?
    # get all the references from the checked loadables
    referencedFileLists = []
    for plugin in self.loadablesByPlugin:
      for loadable in self.loadablesByPlugin[plugin]:
        if hasattr(loadable,'referencedInstanceUIDs'):
          instanceFileList = []
          for instance in loadable.referencedInstanceUIDs:
            instanceFile = slicer.dicomDatabase.fileForInstance(instance)
            if instanceFile != '':
              instanceFileList.append(instanceFile)
          if len(instanceFileList) and not self.isFileListInCheckedLoadables(instanceFileList):
            referencedFileLists.append(instanceFileList)

    # if applicable, find all loadables from the file lists
    loadEnabled = False
    if len(referencedFileLists):
      (self.referencedLoadables,loadEnabled) = self.getLoadablesFromFileLists(referencedFileLists)

    self.referencesDialog = None
    if loadEnabled:
      self.referencesDialog = qt.QDialog(self.window)
      self.referencesDialog.modal = True
      layout = qt.QFormLayout()
      layout.setSpacing(9)
      self.referencesDialog.setLayout(layout)
      windowTitle = "Referenced datasets found"
      self.referencesDialog.setWindowTitle(windowTitle)
      fm = qt.QFontMetrics(qt.QApplication.font(self.referencesDialog))
      self.referencesDialog.setMinimumWidth(fm.width(windowTitle)+50)
      label = qt.QLabel("The loaded DICOM objects contain references to other "
      "datasets you did not select for loading. Please confirm if you would "
      "like to load the following referenced datasets.")
      label.wordWrap = True
      layout.addRow(label)
      for plugin in self.referencedLoadables:
        for loadable in self.referencedLoadables[plugin]:
          if loadable.selected:
            cb = qt.QCheckBox(loadable.name, self.referencesDialog)
            cb.checked = True
            layout.addRow(cb)
      okButton = qt.QPushButton('Proceed')
      okButton.connect("clicked()",self.proceedWithReferencedLoadablesSelection)
      layout.addRow(okButton)
      self.referencesDialog.show()
      self.referencesDialog.adjustSize()
    else:
      self.proceedWithReferencedLoadablesSelection()

    return

  def proceedWithReferencedLoadablesSelection(self):
    # each check box corresponds to a referenced loadable
    # that was selected by examine; if the user confirmed
    # that reference should be loaded, add it to the self.loadablesByPlugin
    # dictionary
    if self.referencesDialog:
      children = self.referencesDialog.children()
      loadableCnt = 0
      for plugin in self.referencedLoadables:
        for loadable in self.referencedLoadables[plugin]:
          if loadable.selected:
            if children[loadableCnt+2].checked:
              self.loadablesByPlugin[plugin].append(loadable)

      self.referencesDialog.close()
      self.referencesDialog = None

    loadableCount = 0
    for plugin in self.loadablesByPlugin:
      for loadable in self.loadablesByPlugin[plugin]:
        if loadable.selected:
          loadableCount += 1
    self.progress = slicer.util.createProgressDialog(parent=self.window, value=0, maximum=loadableCount)
    step = 0
    loadingResult = ''

    loadedNodeIDs = []

    @vtk.calldata_type(vtk.VTK_OBJECT)
    def onNodeAdded(caller, event, calldata):
      node = calldata
      if isinstance(node, slicer.vtkMRMLVolumeNode):
        loadedNodeIDs.append(node.GetID())

    self.addObserver(slicer.mrmlScene, slicer.vtkMRMLScene.NodeAddedEvent, onNodeAdded);

    for plugin in self.loadablesByPlugin:
      for loadable in self.loadablesByPlugin[plugin]:
        if self.progress.wasCanceled:
          break
        slicer.app.processEvents()
        self.progress.setValue(step)
        slicer.app.processEvents()
        if loadable.selected:
          self.progress.labelText = '\nLoading %s' % loadable.name
          slicer.app.processEvents()
          if not plugin.load(loadable):
            loadingResult = '%s\nCould not load: %s as a %s' % (loadingResult,loadable.name,plugin.loadType)
          step += 1
          self.progress.setValue(step)
          slicer.app.processEvents()
        try:
          for derivedItem in loadable.derivedItems:
            indexer = ctk.ctkDICOMIndexer()
            self.progress.labelText = '\nIndexing %s' % derivedItem
            slicer.app.processEvents()
            indexer.addFile(slicer.dicomDatabase, derivedItem)
        except AttributeError:
          # no derived items or some other attribute error
          pass

    self.removeObserver(slicer.mrmlScene, slicer.vtkMRMLScene.NodeAddedEvent, onNodeAdded);

    loadedFileParameters = {}
    loadedFileParameters['nodeIDs'] = loadedNodeIDs
    slicer.app.ioManager().emitNewFileLoaded(loadedFileParameters)

    self.progress.close()
    self.progress = None
    if loadingResult:
      slicer.util.warningDisplay(loadingResult, windowTitle='DICOM loading')
    if not self.browserPersistent:
      self.close()

    return

class DICOMPluginSelector(object):
  """Implement the Qt code for a table of
  selectable DICOM Plugins that determine
  which mappings from DICOM to slicer datatypes
  will be considered.
  """

  def __init__(self,parent, width=50,height=100):
    self.widget = qt.QWidget(parent)
    self.widget.setMinimumHeight(height)
    self.widget.setMinimumWidth(width)
    self.width = width
    self.height = height
    self.layout = qt.QVBoxLayout()
    self.widget.setLayout(self.layout)
    self.checkBoxByPlugin = {}
    settings = qt.QSettings()

    slicerPlugins = slicer.modules.dicomPlugins

    for pluginClass in slicer.modules.dicomPlugins:
      self.checkBoxByPlugin[pluginClass] = qt.QCheckBox(pluginClass)
      self.layout.addWidget(self.checkBoxByPlugin[pluginClass])

    if settings.contains('DICOM/disabledPlugins/size'):
      size = settings.beginReadArray('DICOM/disabledPlugins')
      disabledPlugins = []

      for i in xrange(size):
        settings.setArrayIndex(i)
        disabledPlugins.append(str(settings.allKeys()[0]))
      settings.endArray()

      for pluginClass in slicer.modules.dicomPlugins:
        if pluginClass in disabledPlugins:
          self.checkBoxByPlugin[pluginClass].checked = False
        else:
          # Activate plugins for the ones who are not in the disabled list
          # and also plugins installed with extensions
          self.checkBoxByPlugin[pluginClass].checked = True

    else:

      # All DICOM plugins would be enabled by default
      for pluginClass in slicer.modules.dicomPlugins:
        self.checkBoxByPlugin[pluginClass].checked = True

  def selectedPlugins(self):
    """Return a list of selected plugins"""
    selectedPlugins = []
    for pluginClass in slicer.modules.dicomPlugins:
      if self.checkBoxByPlugin[pluginClass].checked:
        selectedPlugins.append(pluginClass)
    return selectedPlugins

class DICOMLoadableTable(object):
  """Implement the Qt code for a table of
  selectable slicer data to be made from
  the given dicom files
  """

  def __init__(self,parent, width=350,height=100):
    self.widget = qt.QTableWidget(parent)
    self.widget.setMinimumHeight(height)
    self.widget.setMinimumWidth(width)
    self.width = width
    self.height = height
    self.items = []
    self.loadables = {}
    self.setLoadables([])

  def addLoadableRow(self,loadable,row,reader):
    """Add a row to the loadable table
    """
    # name and check state
    qt_ItemIsEditable = 2 # not in PythonQt
    self.loadables[row] = loadable
    item = qt.QTableWidgetItem(loadable.name)
    item.setCheckState(loadable.selected * 2)
    self.items.append(item)
    self.widget.setItem(row,0,item)
    item.setToolTip(loadable.tooltip)
    # reader
    if reader:
      readerItem = qt.QTableWidgetItem(reader)
      readerItem.setFlags(readerItem.flags() ^ qt_ItemIsEditable)
      self.items.append(readerItem)
      self.widget.setItem(row,1,readerItem)
      readerItem.setToolTip(item.toolTip())
    # warning
    if loadable.warning:
      warning = loadable.warning
    else:
      warning = ''
    warnItem = qt.QTableWidgetItem(loadable.warning)
    warnItem.setFlags(warnItem.flags() ^ qt_ItemIsEditable)
    self.items.append(warnItem)
    self.widget.setItem(row,2,warnItem)
    item.setToolTip(item.toolTip() + "\n" + warning)
    warnItem.setToolTip(item.toolTip())

  def setLoadables(self,loadablesByPlugin):
    """Load the table widget with a list
    of volume options (of class DICOMVolume)
    """
    loadableCount = 0
    for plugin in loadablesByPlugin:
      for loadable in loadablesByPlugin[plugin]:
        loadableCount += 1
    self.widget.clearContents()
    self.widget.setColumnCount(3)
    self.widget.setHorizontalHeaderLabels(['DICOM Data','Reader','Warnings'])
    self.widget.setColumnWidth(0,int(self.width * 0.4))
    self.widget.setColumnWidth(1,int(self.width * 0.2))
    self.widget.setColumnWidth(2,int(self.width * 0.4))
    self.widget.setRowCount(loadableCount)
    self.loadables = {}
    row = 0

    for plugin in loadablesByPlugin:
      for thisLoadableId in xrange(len(loadablesByPlugin[plugin])):
        for prevLoadableId in xrange(0,thisLoadableId):
          thisLoadable = loadablesByPlugin[plugin][thisLoadableId]
          prevLoadable = loadablesByPlugin[plugin][prevLoadableId]
          if len(thisLoadable.files) == 1 and len(prevLoadable.files) == 1:
            # needed because of the tuple-sequence comparison does not work,
            # and sometimes tuples are created by some reason
            if thisLoadable.files[0] == prevLoadable.files[0]:
              thisLoadable.selected = False
              break
          elif thisLoadable.files == prevLoadable.files:
            thisLoadable.selected = False
            break

    for selectState in (True,False):
      for plugin in loadablesByPlugin:
        for loadable in loadablesByPlugin[plugin]:
          if loadable.selected == selectState:
            self.addLoadableRow(loadable,row,plugin.loadType)
            row += 1

    self.widget.setVerticalHeaderLabels(row * [""])

  def uncheckAll(self):
    for row in xrange(self.widget.rowCount):
      item = self.widget.item(row,0)
      item.setCheckState(False)

  def updateSelectedFromCheckstate(self):
    for row in xrange(self.widget.rowCount):
      item = self.widget.item(row,0)
      self.loadables[row].selected = (item.checkState() != 0)
      # updating the names
      self.loadables[row].name = item.text()


class DICOMHeaderWidget(object):
  """Implement the Qt code for a table of
  DICOM header values
  """
  # TODO: move this to ctk and use data dictionary for
  # tag names

  def __init__(self,parent):
    self.widget = qt.QTableWidget(parent,width=350,height=300)
    self.items = []
    self.setHeader(None)

  def setHeader(self,file):
    """Load the table widget with header values for the file
    """
    self.widget.clearContents()
    self.widget.setColumnCount(2)
    self.widget.setHorizontalHeaderLabels(['Tag','Value'])
    self.widget.setColumnWidth(0,100)
    self.widget.setColumnWidth(1,200)

    if not file:
      return

    slicer.dicomDatabase.loadFileHeader(file)
    keys = slicer.dicomDatabase.headerKeys()
    self.widget.setRowCount(len(keys))
    row = 0
    for key in keys:
      item = qt.QTableWidgetItem(key)
      self.widget.setItem(row,0,item)
      self.items.append(item)
      dump = slicer.dicomDatabase.headerValue(key)
      try:
        value = dump[dump.index('[')+1:dump.index(']')]
      except ValueError:
        value = "Unknown"
      item = qt.QTableWidgetItem(value)
      self.widget.setItem(row,1,item)
      self.items.append(item)
      row += 1

class DICOMRecentActivityWidget(object):
  """Display the recent activity of the slicer DICOM database
  """

  def __init__(self,parent,dicomDatabase=None,detailsPopup=None):
    if dicomDatabase:
      self.dicomDatabase = dicomDatabase
    else:
      self.dicomDatabase = slicer.dicomDatabase
    self.detailsPopup = detailsPopup
    self.recentSeries = []
    self.widget = qt.QWidget(parent)
    self.widget.name = 'recentActivityWidget'
    self.layout = qt.QVBoxLayout()
    self.widget.setLayout(self.layout)

    self.statusLabel = qt.QLabel(self.widget)
    self.layout.addWidget(self.statusLabel)
    self.statusLabel.text = 'No inserts in the past hour'

    self.scrollArea = qt.QScrollArea()
    self.layout.addWidget(self.scrollArea)
    self.listWidget = qt.QListWidget()
    self.listWidget.name = 'recentActivityListWidget'
    self.scrollArea.setWidget(self.listWidget)
    self.scrollArea.setWidgetResizable(True)
    self.listWidget.setProperty('SH_ItemView_ActivateItemOnSingleClick', 1)
    self.listWidget.connect('activated(QModelIndex)', self.onActivated)

    self.refreshButton = qt.QPushButton(self.widget)
    self.layout.addWidget(self.refreshButton)
    self.refreshButton.text = 'Refresh'
    self.refreshButton.connect('clicked()', self.update)

    self.tags = {}
    self.tags['seriesDescription'] = "0008,103e"
    self.tags['patientName'] = "0010,0010"

  class seriesWithTime(object):
    """helper class to track series and time..."""
    def __init__(self,series,elapsedSinceInsert,insertDateTime,text):
      self.series = series
      self.elapsedSinceInsert = elapsedSinceInsert
      self.insertDateTime = insertDateTime
      self.text = text

  def compareSeriesTimes(self,a,b):
    if a.elapsedSinceInsert > b.elapsedSinceInsert:
      return 1
    else:
      return -1

  def recentSeriesList(self):
    """Return a list of series sorted by insert time
    (counting backwards from today)
    Assume that first insert time of series is valid
    for entire series (should be close enough for this purpose)
    """
    recentSeries = []
    now = qt.QDateTime.currentDateTime()
    for patient in self.dicomDatabase.patients():
      for study in self.dicomDatabase.studiesForPatient(patient):
        for series in self.dicomDatabase.seriesForStudy(study):
          files = self.dicomDatabase.filesForSeries(series)
          if len(files) > 0:
            instance = self.dicomDatabase.instanceForFile(files[0])
            seriesTime = self.dicomDatabase.insertDateTimeForInstance(instance)
            try:
              patientName = self.dicomDatabase.instanceValue(instance,self.tags['patientName'])
            except RuntimeError:
              # this indicates that the particular instance is no longer
              # accessible to the dicom database, so we should ignore it here
              continue
            seriesDescription = self.dicomDatabase.instanceValue(instance,self.tags['seriesDescription'])
            elapsed = seriesTime.secsTo(now)
            secondsPerHour = 60 * 60
            secondsPerDay = secondsPerHour * 24
            timeNote = None
            if elapsed < secondsPerDay:
              timeNote = 'Today'
            elif elapsed < 7 * secondsPerDay:
              timeNote = 'Past Week'
            elif elapsed < 30 * 7 * secondsPerDay:
              timeNote = 'Past Month'
            if timeNote:
              text = "%s: %s for %s" % (timeNote, seriesDescription, patientName)
              recentSeries.append( self.seriesWithTime(series, elapsed, seriesTime, text) )
    recentSeries.sort(self.compareSeriesTimes)
    return recentSeries

  def update(self):
    """Load the table widget with header values for the file
    """
    self.listWidget.clear()
    secondsPerHour = 60 * 60
    insertsPastHour = 0
    self.recentSeries = self.recentSeriesList()
    for series in self.recentSeries:
      self.listWidget.addItem(series.text)
      if series.elapsedSinceInsert < secondsPerHour:
        insertsPastHour += 1
    self.statusLabel.text = '%d series added to database in the past hour' % insertsPastHour
    if len(self.recentSeries) > 0:
      statusMessage = "Most recent DICOM Database addition: %s" % self.recentSeries[0].insertDateTime.toString()
      slicer.util.showStatusMessage(statusMessage, 10000)

  def onActivated(self,modelIndex):
    print('selected row %d' % modelIndex.row())
    print(self.recentSeries[modelIndex.row()].text)
    series = self.recentSeries[modelIndex.row()]
    if self.detailsPopup:
      self.detailsPopup.open()
      self.detailsPopup.offerLoadables(series.series,"Series")

class DICOMSendDialog(object):
  """Implement the Qt dialog for doing a DICOM Send (storage SCU)
  """

  def __init__(self,files):
    self.files = files
    settings = qt.QSettings()
    self.sendAddress = settings.value('DICOM.sendAddress')
    self.sendPort = settings.value('DICOM.sendPort')
    self.open

  def open(self):
    # main dialog
    self.dialog = qt.QDialog(slicer.util.mainWindow())
    self.dialog.setWindowTitle('Send DICOM Study')
    self.dialog.setWindowModality(1)
    layout = qt.QVBoxLayout()
    self.dialog.setLayout(layout)

    self.studyLabel = qt.QLabel('Send %d items to destination' % len(self.files))
    layout.addWidget(self.studyLabel)

    # Send Parameters
    self.dicomFrame = qt.QFrame(self.dialog)
    self.dicomFormLayout = qt.QFormLayout()
    self.dicomFrame.setLayout(self.dicomFormLayout)
    self.dicomEntries = {}
    self.dicomParameters = {
      "Destination Address": self.sendAddress,
      "Destination Port": self.sendPort
    }
    for label in self.dicomParameters.keys():
      self.dicomEntries[label] = qt.QLineEdit()
      self.dicomEntries[label].text = self.dicomParameters[label]
      self.dicomFormLayout.addRow(label+": ", self.dicomEntries[label])
    layout.addWidget(self.dicomFrame)

    # button box
    bbox = qt.QDialogButtonBox(self.dialog)
    bbox.addButton(bbox.Ok)
    bbox.addButton(bbox.Cancel)
    bbox.connect('accepted()', self.onOk)
    bbox.connect('rejected()', self.onCancel)
    layout.addWidget(bbox)

    self.dialog.open()

  def onOk(self):
    address = self.dicomEntries['Destination Address'].text
    port = self.dicomEntries['Destination Port'].text
    settings = qt.QSettings()
    settings.setValue('DICOM.sendAddress', address)
    settings.setValue('DICOM.sendPort', port)
    self.progress = slicer.util.createProgressDialog(value=0, maximum=len(self.files))
    self.progressValue = 0
    try:
      DICOMLib.DICOMSender(self.files, address, port, progressCallback = self.onProgress)
    except Exception as result:
      slicer.util.warningDisplay('Could not send data: %s' % result, windowTitle='DICOM Send', parent=self.dialog)
    self.progress.close()
    self.dialog.close()

  def onProgress(self,message):
    self.progress.show()
    self.progressValue += 1
    self.progress.setValue(self.progressValue)
    self.progress.setLabelText(message)

  def onCancel(self):
    self.dialog.close()

class DICOMHeaderPopup(object):

  def __init__(self):

    self.popupGeometry = qt.QRect()
    settings = qt.QSettings()
    if settings.contains('DICOM/headerPopup.geometry'):
      self.popupGeometry = settings.value('DICOM/headerPopup.geometry')
    self.popupPositioned = False
    self.window = ctk.ctkDICOMObjectListWidget()
    self.window.setWindowTitle('DICOM File Metadata')

  def open(self):
    if not self.window.isVisible():
      self.window.show()
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
    self.window.raise_()

  def setFileLists(self,fileLists):
    filePaths = []
    for fileList in fileLists:
      for filePath in fileList:
        filePaths.append(filePath)
    self.window.setFileList(filePaths)

  def close(self):
    self.onPopupGeometryChanged()
    self.window.hide()

  def onPopupGeometryChanged(self):
    settings = qt.QSettings()
    self.popupGeometry = self.window.geometry
    settings.setValue('DICOM/headerPopup.geometry', self.window.geometry)
