import os
import unittest
import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *
import logging

#
# SampleData methods
#

def downloadFromURL(uris, fileNames, nodeNames=None,
  customDownloader=None, loadFileTypes=None, loadFileProperties={}):
  """Download and optionally load data into the application.

  :param uris: Download URL(s).
  :param fileNames: File name(s) that will be loaded.
  :param nodeNames: Node name(s) in the scene.
  :param customDownloader: Custom function for downloading.
  :param loadFileTypes: file format name(s) ('VolumeFile' by default).
  :param loadFileProperties: custom properties passed to the IO plugin.

  If the given ``fileNames`` are not found in the application cache directory, they
  are downloaded using the associated URIs.
  See ``slicer.mrmlScene.GetCacheManager().GetRemoteCacheDirectory()``

  If not explicitly provided or if set to ``None``, the ``loadFileTypes`` are
  guessed based on the corresponding filename extensions.

  If not explicitly provided or if set to ``None``, the ``nodeNames`` are
  set based on the corresponding filename basename.

  The ``loadFileProperties`` are common for all files. If different properties
  need to be associated with files of different types, downloadFromURL must
  be called for each.
  """
  return SampleDataLogic().downloadFromURL(
    uris, fileNames, nodeNames, customDownloader, loadFileTypes, loadFileProperties)


#
# SampleData
#

class SampleData(ScriptedLoadableModule):
  """Uses ScriptedLoadableModule base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def __init__(self, parent):
    ScriptedLoadableModule.__init__(self, parent)
    self.parent.title = "Sample Data"
    self.parent.categories = ["Informatics"]
    self.parent.dependencies = []
    self.parent.contributors = ["Steve Pieper (Isomics), Benjamin Long (Kitware), Jean-Christophe Fillion-Robin (Kitware)"]
    self.parent.helpText = """
The SampleData module can be used to download data for working with in slicer.  Use of this module requires an active network connection.
"""
    self.parent.helpText += self.getDefaultModuleDocumentationLink()
    self.parent.acknowledgementText = """
<p>This work was was funded by Cancer Care Ontario
and the Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO)</p>

<p>CTA abdomen (Panoramix) dataset comes from <a href="http://www.osirix-viewer.com/resources/dicom-image-library/">Osirix DICOM image library</a>
and is exclusively available for research and teaching. You are not authorized to redistribute or sell it, or
use it for commercial purposes.</p>
"""

    if slicer.mrmlScene.GetTagByClassName( "vtkMRMLScriptedModuleNode" ) != 'ScriptedModule':
      slicer.mrmlScene.RegisterNodeClass(vtkMRMLScriptedModuleNode())

    # Trigger the menu to be added when application has started up
    if not slicer.app.commandOptions().noMainWindow :
      slicer.app.connect("startupCompleted()", self.addMenu)

    # allow other modules to register sample data sources by appending
    # instances or subclasses SampleDataSource objects on this list
    try:
      slicer.modules.sampleDataSources
    except AttributeError:
      slicer.modules.sampleDataSources = {}


  def addMenu(self):
    actionIcon = self.parent.icon
    a = qt.QAction(actionIcon, 'Download Sample Data', slicer.util.mainWindow())
    a.setToolTip('Go to the SampleData module to download data from the network')
    a.connect('triggered()', self.select)

    fileMenu = slicer.util.lookupTopLevelWidget('FileMenu')
    if fileMenu:
      for action in fileMenu.actions():
        if action.text == 'Save':
          fileMenu.insertAction(action,a)


  def select(self):
    m = slicer.util.mainWindow()
    m.moduleSelector().selectModule('SampleData')

#
# SampleDataSource
#
class SampleDataSource:
  """Describe a set of sample data associated with one or multiple URIs and filenames.

  Example::

    import SampleData
    dataSource = SampleData.SampleDataSource(
      fileNames='fixed.nrrd',
      uris='http://slicer.kitware.com/midas3/download/item/157188/small-mr-eye-fixed.nrrd')
    loadedNode = SampleData.SampleDataLogic().downloadFromSource(dataSource)[0]
  """

  def __init__(self, sampleName=None, uris=None, fileNames=None, nodeNames=None,
    customDownloader=None, thumbnailFileName=None,
    loadFileType=None, loadFileProperties={}):
    """
    :param sampleName: Displayed name of data set in SampleData module GUI.
    :param thumbnailFileName: Displayed thumbnail of data set in SampleData module GUI,
    :param uris: Download URL(s).
    :param fileNames: File name(s) that will be loaded.
    :param nodeNames: Node name(s) in the scene.
    :param customDownloader: Custom function for downloading.
    :param loadFileType: file format name(s) ('VolumeFile' by default).
    :param loadFileProperties: custom properties passed to the IO plugin.
    """
    self.sampleName = sampleName
    if (isinstance(uris, list) or isinstance(uris, tuple)):
      if isinstance(loadFileType, basestring) or loadFileType is None:
        loadFileType = [loadFileType] * len(uris)
    elif isinstance(uris, basestring):
      uris = [uris,]
      fileNames = [fileNames,]
      nodeNames = [nodeNames,]
      loadFileType = [loadFileType,]

    updatedFileType = []
    updatedNodeNames = []
    for fileName, nodeName, fileType in zip(fileNames, nodeNames, loadFileType):
      # If not explicitly specified, attempt to guess fileType
      if fileType is None:
        # TODO: Use method from Slicer IO logic ?
        ext = os.path.splitext(fileName.lower())[1]
        if ext in [".mrml", ".mrb"]:
          fileType = "SceneFile"
        elif ext in [".zip"]:
          fileType = "ZipFile"
        else:
          fileType = "VolumeFile"
      updatedFileType.append(fileType)
      # If not explicitly specified, attempt to guess nodeName
      if nodeName is None and fileType not in ["SceneFile", "ZipFile"]:
        nodeName = fileName.split(".")[0]
      updatedNodeNames.append(nodeName)

    self.uris = uris
    self.fileNames = fileNames
    self.nodeNames = updatedNodeNames
    self.customDownloader = customDownloader
    self.thumbnailFileName = thumbnailFileName
    self.loadFileType = updatedFileType
    self.loadFileProperties = loadFileProperties
    if len(uris) != len(fileNames) or len(uris) != len(updatedNodeNames) or len(uris) != len(updatedFileType):
      raise Exception("All fields of sample data source must have the same length")


#
# SampleData widget
#

class SampleDataWidget(ScriptedLoadableModuleWidget):
  """Uses ScriptedLoadableModuleWidget base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def setup(self):
    ScriptedLoadableModuleWidget.setup(self)

    # This module is often used in developer mode, therefore
    # collapse reload & test section by default.
    if hasattr(self, "reloadCollapsibleButton"):
      self.reloadCollapsibleButton.collapsed = True

    self.observerTags = []
    self.logic = SampleDataLogic(self.logMessage)

    numberOfColumns = 3
    iconPath = os.path.join(os.path.dirname(__file__).replace('\\','/'), 'Resources','Icons')
    desktop = qt.QDesktopWidget()
    mainScreenSize = desktop.availableGeometry(desktop.primaryScreen)
    iconSize = qt.QSize(mainScreenSize.width()/15,mainScreenSize.height()/10)

    categories = slicer.modules.sampleDataSources.keys()
    categories.sort()
    if self.logic.builtInCategoryName in categories:
      categories.remove(self.logic.builtInCategoryName)
    categories.insert(0,self.logic.builtInCategoryName)
    for category in categories:
      if category == self.logic.developmentCategoryName and self.developerMode is False:
        continue
      frame = ctk.ctkCollapsibleGroupBox(self.parent)
      self.layout.addWidget(frame)
      frame.title = category
      frame.name = '%sCollapsibleGroupBox' % category
      layout = qt.QGridLayout(frame)
      columnIndex = 0
      rowIndex = 0
      for source in slicer.modules.sampleDataSources[category]:
        name = source.sampleName
        if not name:
          name = source.nodeNames[0]

        b = qt.QToolButton()
        b.setText(name)

        # Set thumbnail
        if source.thumbnailFileName:
          # Thumbnail provided
          thumbnailImage = source.thumbnailFileName
        else:
          # Look for thumbnail image with the name of any node name with .png extension
          thumbnailImage = None
          for nodeName in source.nodeNames:
            if not nodeName:
              continue
            thumbnailImageAttempt = os.path.join(iconPath, nodeName+'.png')
            if os.path.exists(thumbnailImageAttempt):
              thumbnailImage = thumbnailImageAttempt
              break
        if thumbnailImage and os.path.exists(thumbnailImage):
          b.setIcon(qt.QIcon(thumbnailImage))

        b.setIconSize(iconSize)
        b.setToolButtonStyle(qt.Qt.ToolButtonTextUnderIcon)
        qSize = qt.QSizePolicy()
        qSize.setHorizontalPolicy(qt.QSizePolicy.Expanding)
        b.setSizePolicy(qSize)

        b.name = '%sPushButton' % name
        layout.addWidget(b, rowIndex, columnIndex)
        columnIndex += 1
        if columnIndex==numberOfColumns:
          rowIndex += 1
          columnIndex = 0
        if source.customDownloader:
          b.connect('clicked()', source.customDownloader)
        else:
          b.connect('clicked()', lambda s=source: self.logic.downloadFromSource(s))

    self.log = qt.QTextEdit()
    self.log.readOnly = True
    self.layout.addWidget(self.log)
    self.logMessage('<p>Status: <i>Idle</i>')

    # Add spacer to layout
    self.layout.addStretch(1)

  def logMessage(self, message, logLevel=logging.INFO):
    # Set text color based on log level
    if logLevel >= logging.ERROR:
      message = '<font color="red">' + message + '</font>'
    elif logLevel >= logging.WARNING:
      message = '<font color="orange">' + message + '</font>'
    # Show message in status bar
    doc = qt.QTextDocument()
    doc.setHtml(message)
    slicer.util.showStatusMessage(doc.toPlainText(),3000)
    # Show message in log window at the bottom of the module widget
    self.log.insertHtml(message)
    self.log.insertPlainText('\n')
    self.log.ensureCursorVisible()
    self.log.repaint()
    logging.log(logLevel, message)
    slicer.app.processEvents(qt.QEventLoop.ExcludeUserInputEvents)

#
# SampleData logic
#

class SampleDataLogic:
  """Manage the slicer.modules.sampleDataSources dictionary.
  The dictionary keys are categories of sample data sources.
  The BuiltIn category is managed here.  Modules or extensions can
  register their own sample data by creating instances of the
  SampleDataSource class.  These instances should be stored in a
  list that is assigned to a category following the model
  used in registerBuiltInSampleDataSources below.
  """

  @staticmethod
  def registerCustomSampleDataSource(category='Custom',
    sampleName=None, uris=None, fileNames=None, nodeNames=None,
    customDownloader=None, thumbnailFileName=None,
    loadFileType='VolumeFile', loadFileProperties={}):
    """Adds custom data sets to SampleData.
    :param category: Section title of data set in SampleData module GUI.
    :param sampleName: Displayed name of data set in SampleData module GUI.
    :param thumbnailFileName: Displayed thumbnail of data set in SampleData module GUI,
    :param uris: Download URL(s).
    :param fileNames: File name(s) that will be loaded.
    :param nodeNames: Node name(s) in the scene.
    :param customDownloader: Custom function for downloading.
    :param loadFileType: file format name(s) ('VolumeFile' by default).
    :param loadFileProperties: custom properties passed to the IO plugin.
    """

    try:
      slicer.modules.sampleDataSources
    except AttributeError:
      slicer.modules.sampleDataSources = {}

    if not slicer.modules.sampleDataSources.has_key(category):
      slicer.modules.sampleDataSources[category] = []

    slicer.modules.sampleDataSources[category].append(SampleDataSource(
      sampleName=sampleName,
      uris=uris,
      fileNames=fileNames,
      nodeNames=nodeNames,
      thumbnailFileName=thumbnailFileName,
      loadFileType=loadFileType,
      loadFileProperties=loadFileProperties
      ))

  def __init__(self, logMessage=None):
    if logMessage:
      self.logMessage = logMessage
    self.builtInCategoryName = 'BuiltIn'
    self.developmentCategoryName = 'Development'
    self.registerBuiltInSampleDataSources()
    self.registerDevelopmentSampleDataSources()

  def registerBuiltInSampleDataSources(self):
    """Fills in the pre-define sample data sources"""
    sourceArguments = (
        ('MRHead', 'http://slicer.kitware.com/midas3/download/item/292308/MR-head.nrrd', 'MR-head.nrrd', 'MRHead'),
        ('CTChest', 'http://slicer.kitware.com/midas3/download/item/292307/CT-chest.nrrd', 'CT-chest.nrrd', 'CTChest'),
        ('CTACardio', 'http://slicer.kitware.com/midas3/download/item/292309/CTA-cardio.nrrd', 'CTA-cardio.nrrd', 'CTACardio'),
        ('DTIBrain', 'http://slicer.kitware.com/midas3/download/item/292310/DTI-brain.nrrd', 'DTI-Brain.nrrd', 'DTIBrain'),
        ('MRBrainTumor1', 'http://slicer.kitware.com/midas3/download/item/292312/RegLib_C01_1.nrrd', 'RegLib_C01_1.nrrd', 'MRBrainTumor1'),
        ('MRBrainTumor2', 'http://slicer.kitware.com/midas3/download/item/292313/RegLib_C01_2.nrrd', 'RegLib_C01_2.nrrd', 'MRBrainTumor2'),
        ('BaselineVolume', 'http://slicer.kitware.com/midas3/download/?items=2009,1', 'BaselineVolume.nrrd', 'BaselineVolume'),
        ('DTIVolume',
          ('http://slicer.kitware.com/midas3/download/?items=2011,1',
            'http://slicer.kitware.com/midas3/download/?items=2010,1', ),
          ('DTIVolume.raw.gz', 'DTIVolume.nhdr'), (None, 'DTIVolume')),
        ('DWIVolume', ('http://slicer.kitware.com/midas3/download/?items=2142,1', 'http://slicer.kitware.com/midas3/download/?items=2141,1'),
          ('dwi.raw.gz', 'dwi.nhdr'), (None, 'dwi')),
        ('CTA abdomen\n(Panoramix)', 'http://slicer.kitware.com/midas3/download/?items=9073,1', 'Panoramix-cropped.nrrd', 'Panoramix-cropped'),
        ('CBCTDentalSurgery',
          ('http://slicer.kitware.com/midas3/download/item/94510/Greyscale_presurg.gipl.gz',
            'http://slicer.kitware.com/midas3/download/item/94509/Greyscale_postsurg.gipl.gz',),
          ('PreDentalSurgery.gipl.gz', 'PostDentalSurgery.gipl.gz'), ('PreDentalSurgery', 'PostDentalSurgery')),
        ('MR-US Prostate',
          ('http://slicer.kitware.com/midas3/download/item/142475/Case10-MR.nrrd',
            'http://slicer.kitware.com/midas3/download/item/142476/case10_US_resampled.nrrd',),
          ('Case10-MR.nrrd', 'case10_US_resampled.nrrd'), ('MRProstate', 'USProstate')),
        ('CT-MR Brain',
          ('http://slicer.kitware.com/midas3/download/item/284192/CTBrain.nrrd',
           'http://slicer.kitware.com/midas3/download/item/330508/MRBrainT1.nrrd',
           'http://slicer.kitware.com/midas3/download/item/330509/MRBrainT2.nrrd',),
          ('CT-brain.nrrd', 'MR-brain-T1.nrrd', 'MR-brain-T2.nrrd'),
          ('CTBrain', 'MRBrainT1', 'MRBrainT2')),
        )

    if not slicer.modules.sampleDataSources.has_key(self.builtInCategoryName):
      slicer.modules.sampleDataSources[self.builtInCategoryName] = []
    for sourceArgument in sourceArguments:
      slicer.modules.sampleDataSources[self.builtInCategoryName].append(SampleDataSource(*sourceArgument))

  def registerDevelopmentSampleDataSources(self):
    """Fills in the sample data sources displayed only if developer mode is enabled."""
    iconPath = os.path.join(os.path.dirname(__file__).replace('\\','/'), 'Resources','Icons')
    self.registerCustomSampleDataSource(
      category=self.developmentCategoryName, sampleName='TinyPatient',
      uris=['http://slicer.kitware.com/midas3/download/item/245205/TinyPatient_CT.nrrd',
            'http://slicer.kitware.com/midas3/download/item/367020/TinyPatient_Structures.seg.nrrd'],
      fileNames=['TinyPatient_CT.nrrd', 'TinyPatient_Structures.seg.nrrd'],
      nodeNames=['TinyPatient_CT', 'TinyPatient_Segments'],
      thumbnailFileName=os.path.join(iconPath, 'TinyPatient.png'),
      loadFileType=['VolumeFile', 'SegmentationFile']
      )

  def downloadFileIntoCache(self, uri, name):
    """Given a uri and and a filename, download the data into
    a file of the given name in the scene's cache"""
    destFolderPath = slicer.mrmlScene.GetCacheManager().GetRemoteCacheDirectory()
    if not os.access(destFolderPath, os.W_OK):
      errorMessage = '<b>Cache folder %s is not writable!</b>' % destFolderPath
      self.logMessage(errorMessage, logging.ERROR)
    return self.downloadFile(uri, destFolderPath, name)

  def downloadSourceIntoCache(self, source):
    """Download all files for the given source and return a
    list of file paths for the results"""
    filePaths = []
    for uri,fileName in zip(source.uris,source.fileNames):
      filePaths.append(self.downloadFileIntoCache(uri, fileName))
    return filePaths

  def downloadFromSource(self,source,attemptCount=0):
    """Given an instance of SampleDataSource, downloads the associated data and
    load them into Slicer if it applies.

    The function always returns a list.

    Based on the file type associated with the source, different values may
    be returned.

      - for ``SceneFile``, returns path of downloaded file
      - for ``VolumeFile`` or any other type supported by Slicer, returns associated node
      - for ``ZipFile``, returns directory of extracted archive
    """
    nodes = []

    for uri,fileName,nodeName,loadFileType in zip(source.uris,source.fileNames,source.nodeNames,source.loadFileType):

      current_source = SampleDataSource(uris=uri, fileNames=fileName, nodeNames=nodeName, loadFileType=loadFileType, loadFileProperties=source.loadFileProperties)
      filePath = self.downloadFileIntoCache(uri, fileName)

      if loadFileType == 'ZipFile':
        outputDir = slicer.mrmlScene.GetCacheManager().GetRemoteCacheDirectory() + "/" + os.path.splitext(os.path.basename(filePath))[0]
        qt.QDir().mkpath(outputDir)
        success = slicer.util.extractArchive(filePath, outputDir)
        if not success and attemptCount < 5:
          attemptCount += 1
          self.logMessage('<b>Load failed! Trying to download again (%d of 5 attempts)...</b>' % (attemptCount), logging.ERROR)
          file = qt.QFile(filePath)
          if not file.remove():
            self.logMessage('<b>Load failed! Unable to delete and try again loading %s!</b>' % filePath, logging.ERROR)
            nodes.append(None)
            break
          outputDir = self.downloadFromSource(current_source,attemptCount)[0]
        nodes.append(outputDir)

      elif loadFileType == 'SceneFile':
        success = self.loadScene(filePath, source.loadFileProperties)
        if not success and attemptCount < 5:
          attemptCount += 1
          self.logMessage('<b>Load failed! Trying to download again (%d of 5 attempts)...</b>' % (attemptCount), logging.ERROR)
          file = qt.QFile(filePath)
          if not file.remove():
            self.logMessage('<b>Load failed! Unable to delete and try again loading %s!</b>' % filePath, logging.ERROR)
            nodes.append(None)
            break
          filePath = self.downloadFromSource(current_source,attemptCount)[0]
        nodes.append(filePath)

      elif nodeName:
        loadedNode = self.loadNode(filePath, nodeName, loadFileType, source.loadFileProperties)
        if loadedNode is None and attemptCount < 5:
          attemptCount += 1
          self.logMessage('<b>Load failed! Trying to download again (%d of 5 attempts)...</b>' % (attemptCount), logging.ERROR)
          file = qt.QFile(filePath)
          if not file.remove():
            self.logMessage('<b>Load failed! Unable to delete and try again loading %s!</b>' % filePath, logging.ERROR)
            loadedNode.append(None)
            break
          loadedNode = self.downloadFromSource(current_source,attemptCount)[0]
        nodes.append(loadedNode)

    return nodes

  def sourceForSampleName(self,sampleName):
    """For a given sample name this will search the available sources.
    Returns SampleDataSource instance."""
    for category in slicer.modules.sampleDataSources.keys():
      for source in slicer.modules.sampleDataSources[category]:
        if sampleName == source.sampleName:
          return source
    return None

  def downloadFromURL(self, uris, fileNames, nodeNames=None,
    customDownloader=None, loadFileTypes=None, loadFileProperties={}):
    """Download and optionally load data into the application.

    :param uris: Download URL(s).
    :param fileNames: File name(s) that will be loaded.
    :param nodeNames: Node name(s) in the scene.
    :param customDownloader: Custom function for downloading.
    :param loadFileTypes: file format name(s) ('VolumeFile' by default).
    :param loadFileProperties: custom properties passed to the IO plugin.

    If the given ``fileNames`` are not found in the application cache directory, they
    are downloaded using the associated URIs.
    See ``slicer.mrmlScene.GetCacheManager().GetRemoteCacheDirectory()``

    If not explicitly provided or if set to ``None``, the ``loadFileTypes`` are
    guessed based on the corresponding filename extensions.

    If not explicitly provided or if set to ``None``, the ``nodeNames`` are
    set based on the corresponding filename basename.

    The ``loadFileProperties`` are common for all files. If different properties
    need to be associated with files of different types, downloadFromURL must
    be called for each.
    """
    return self.downloadFromSource(SampleDataSource(
      uris=uris, fileNames=fileNames, nodeNames=nodeNames,
      loadFileType=loadFileTypes, loadFileProperties=loadFileProperties
    ))

  def downloadSample(self,sampleName):
    """For a given sample name this will search the available sources
    and load it if it is available.  Returns the loaded nodes."""
    source = self.sourceForSampleName(sampleName)
    nodes = []
    if source:
      nodes = self.downloadFromSource(source)
    return nodes

  def logMessage(self,message,logLevel=logging.INFO):
    print(message)

  """Utility methods for backwards compatibility"""
  def downloadMRHead(self):
    return self.downloadSample('MRHead')[0]

  def downloadCTChest(self):
    return self.downloadSample('CTChest')[0]

  def downloadCTACardio(self):
    return self.downloadSample('CTACardio')[0]

  def downloadDTIBrain(self):
    return self.downloadSample('DTIBrain')[0]

  def downloadMRBrainTumor1(self):
    return self.downloadSample('MRBrainTumor1')[0]

  def downloadMRBrainTumor2(self):
    return self.downloadSample('MRBrainTumor2')[0]

  def downloadWhiteMatterExplorationBaselineVolume(self):
    return self.downloadSample('BaselineVolume')[0]

  def downloadWhiteMatterExplorationDTIVolume(self):
    return self.downloadSample('DTIVolume')[0]

  def downloadDiffusionMRIDWIVolume(self):
    return self.downloadSample('dwi')[0]

  def downloadAbdominalCTVolume(self):
    return self.downloadSample('CTA abdomen\n(Panoramix)')[0]

  def downloadDentalSurgery(self):
    # returns list since that's what earlier method did
    return self.downloadSample('CBCTDentalSurgery')

  def downloadMRUSPostate(self):
    # returns list since that's what earlier method did
    return self.downloadSample('MR-US Prostate')

  def humanFormatSize(self,size):
    """ from http://stackoverflow.com/questions/1094841/reusable-library-to-get-human-readable-version-of-file-size"""
    for x in ['bytes','KB','MB','GB']:
      if size < 1024.0 and size > -1024.0:
        return "%3.1f %s" % (size, x)
      size /= 1024.0
    return "%3.1f %s" % (size, 'TB')

  def reportHook(self,blocksSoFar,blockSize,totalSize):
    # we clamp to 100% because the blockSize might be larger than the file itself
    percent = min(int((100. * blocksSoFar * blockSize) / totalSize), 100)
    if percent == 100 or (percent - self.downloadPercent >= 10):
      # we clamp to totalSize when blockSize is larger than totalSize
      humanSizeSoFar = self.humanFormatSize(min(blocksSoFar * blockSize, totalSize))
      humanSizeTotal = self.humanFormatSize(totalSize)
      self.logMessage('<i>Downloaded %s (%d%% of %s)...</i>' % (humanSizeSoFar, percent, humanSizeTotal))
      self.downloadPercent = percent

  def downloadFile(self, uri, destFolderPath, name):
    filePath = destFolderPath + '/' + name
    if not os.path.exists(filePath) or os.stat(filePath).st_size == 0:
      import urllib
      self.logMessage('<b>Requesting download</b> <i>%s</i> from %s...' % (name, uri))
      # add a progress bar
      self.downloadPercent = 0
      try:
        urllib.urlretrieve(uri, filePath, self.reportHook)
        self.logMessage('<b>Download finished</b>')
      except IOError as e:
        self.logMessage('<b>\tDownload failed: %s</b>' % e, logging.ERROR)
    else:
      self.logMessage('<b>File already exists in cache - reusing it.</b>')
    return filePath

  def loadScene(self, uri,  fileProperties = {}):
    self.logMessage('<b>Requesting load</b> %s...' % uri)
    fileProperties['fileName'] = uri
    success = slicer.app.coreIOManager().loadNodes('SceneFile', fileProperties)
    if not success:
      self.logMessage('<b>\tLoad failed!</b>', logging.ERROR)
      return False
    self.logMessage('<b>Load finished</b>')
    return True

  def loadNode(self, uri, name, fileType = 'VolumeFile', fileProperties = {}):
    self.logMessage('<b>Requesting load</b> <i>%s</i> from %s...' % (name, uri))

    fileProperties['fileName'] = uri
    fileProperties['name'] = name
    firstLoadedNode = None
    loadedNodes = vtk.vtkCollection()
    success = slicer.app.coreIOManager().loadNodes(fileType, fileProperties, loadedNodes)

    if not success or loadedNodes.GetNumberOfItems()<1:
      self.logMessage('<b>\tLoad failed!</b>', logging.ERROR)
      return None

    self.logMessage('<b>Load finished</b>')

    # since nodes were read from a temp directory remove the storage nodes
    for i in range(loadedNodes.GetNumberOfItems()):
      loadedNode = loadedNodes.GetItemAsObject(i)
      if not loadedNode.IsA("vtkMRMLStorableNode"):
        continue
      storageNode = loadedNode.GetStorageNode()
      if not storageNode:
        continue
      slicer.mrmlScene.RemoveNode(storageNode)
      loadedNode.SetAndObserveStorageNodeID(None)

    return loadedNodes.GetItemAsObject(0)
