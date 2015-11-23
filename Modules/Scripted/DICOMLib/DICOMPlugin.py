import os
import qt
import slicer
import DICOMLib


#########################################################
#
#
comment = """

  DICOMPlugin is a superclass for code that plugs into the
  slicer DICOM module.

  These classes are Abstract.

"""
#
#########################################################

#
# DICOMLoadable
#

class DICOMLoadable(object):
  """Container class for things that can be
  loaded from dicom files into slicer.
  Each plugin returns a list of instances from its
  evaluate method and accepts a list of these
  in its load method corresponding to the things
  the user has selected for loading
  NOTE: This class is deprecated, use qSlicerDICOMLoadable
  instead.
  """

  def __init__(self):
    # the file list of the data to be loaded
    self.files = []
    # name exposed to the user for the node
    self.name = "Unknown"
    # extra information the user sees on mouse over of the thing
    self.tooltip = "No further information available"
    # things the user should know before loading this data
    self.warning = ""
    # is the object checked for loading by default
    self.selected = False
    # confidence - from 0 to 1 where 0 means low chance
    # that the user actually wants to load their data this
    # way up to 1, which means that the plugin is very confident
    # that this is the best way to load the data.
    # When more than one plugin marks the same series as
    # selected, the one with the highest confidence is
    # actually selected by default.  In the case of a tie,
    # both series are selected for loading.
    self.confidence = 0.5


#
# DICOMPlugin
#

class DICOMPlugin(object):
  """ Base class for DICOM plugins
  """

  def __init__(self):
    # displayed for the user as the plugin handling the load
    self.loadType = "Generic DICOM"
    # a dictionary that maps a list of files to a list of loadables
    # (so that subsequent requests for the same info can be
    #  serviced quickly)
    self.loadableCache = {}
    # tags is a dictionary of symbolic name keys mapping to
    # hex tag number values (as in {'pixelData': '7fe0,0010'}).
    # Each subclass should define the tags it will be using in
    # calls to the DICOM database so that any needed values
    # can be efficiently pre-fetched if possible.
    self.tags = {}
    self.tags['seriesDescription'] = "0008,103E"
    self.tags['seriesNumber'] = "0020,0011"

  def hashFiles(self,files):
    """Create a hash key for a list of files"""
    try:
      import hashlib
    except:
      return None
    m = hashlib.md5()
    for f in files:
      # Unicode-objects must be encoded before hashing
      m.update(f.encode('UTF-8', 'ignore'))
    return(m.digest())

  def getCachedLoadables(self,files):
    """ Helper method to access the results of a previous
    examination of a list of files"""
    key = self.hashFiles(files)
    if self.loadableCache.has_key(key):
      return self.loadableCache[key]
    return None

  def cacheLoadables(self,files,loadables):
    """ Helper method to store the results of examining a list
    of files for later quick access"""
    key = self.hashFiles(files)
    self.loadableCache[key] = loadables

  def examineForImport(self,fileList):
    """Look at the list of lists of filenames and return
    a list of DICOMLoadables that are options for loading
    Virtual: should be overridden by the subclass
    """
    return []

  def examine(self,fileList):
    """Backwards compatibility function for examineForImport
    (renamed on introducing examineForExport to avoid confusion)
    """
    return self.examineForImport(fileList)

  def load(self,loadable):
    """Accept a DICOMLoadable and perform the operation to convert
    the referenced data into MRML nodes
    Virtual: should be overridden by the subclass
    """
    return True

  def examineForExport(self,node):
    """Return a list of DICOMExportable instances that describe the
    available techniques that this plugin offers to convert MRML
    data into DICOM data
    Virtual: should be overridden by the subclass
    """
    return []

  def export(self,exportable):
    """Export an exportable (one series) to file(s)
    Return error message, empty if success
    Virtual: should be overridden by the subclass
    """
    return ""

  def defaultSeriesNodeName(self,seriesUID):
    """Generate a name suitable for use as a mrml node name based
    on the series level data in the database"""
    instanceFilePaths = slicer.dicomDatabase.filesForSeries(seriesUID)
    if len(instanceFilePaths) == 0:
      return "Unnamed Series"
    seriesDescription = slicer.dicomDatabase.fileValue(instanceFilePaths[0],self.tags['seriesDescription'])
    seriesNumber = slicer.dicomDatabase.fileValue(instanceFilePaths[0],self.tags['seriesNumber'])
    name = seriesDescription
    if seriesDescription == "":
      name = "Unnamed Series"
    if seriesNumber != "":
      name = seriesNumber + ": " + name
    return name

  def addSeriesInSubjectHierarchy(self,loadable,dataNode):
    """Add loaded DICOM series into subject hierarchy.
    The DICOM tags are read from the first file referenced by the
    given loadable. The dataNode argument is associated to the created
    series node and provides fallback name in case of empty series
    description.
    This function should be called from the load() function of
    each subclass of the DICOMPlugin class.
    """
    tags = {}
    tags['seriesInstanceUID'] = "0020,000E"
    tags['seriesModality'] = "0008,0060"
    tags['seriesNumber'] = "0020,0011"
    tags['studyInstanceUID'] = "0020,000D"
    tags['studyID'] = "0020,0010"
    tags['studyDescription'] = "0008,1030"
    tags['studyDate'] = "0008,0020"
    tags['studyTime'] = "0008,0030"
    tags['patientID'] = "0010,0020"
    tags['patientName'] = "0010,0010"
    tags['patientSex'] = "0010,0040"
    tags['patientBirthDate'] = "0010,0030"
    tags['patientComments'] = "0010,4000"
    tags['instanceUID'] = "0008,0018"

    # Import and check dependencies
    from vtkSlicerSubjectHierarchyModuleMRML import vtkMRMLSubjectHierarchyNode
    from vtkSlicerSubjectHierarchyModuleLogic import vtkSlicerSubjectHierarchyModuleLogic
    try:
      vtkMRMLSubjectHierarchyNode
      vtkSlicerSubjectHierarchyModuleLogic
    except AttributeError:
      import sys
      sys.stderr.write('Unable to create SubjectHierarchy nodes: SubjectHierarchy module not found!')
      return

    # Validate dataNode argument
    if dataNode is None or not dataNode.IsA('vtkMRMLNode'):
      import sys
      sys.stderr.write('Unable to create SubjectHierarchy nodes: invalid data node provided!')
      return

    # Get first file to access DICOM tags from it
    firstFile = loadable.files[0]

    # Set up subject hierarchy node
    seriesNode = vtkMRMLSubjectHierarchyNode.CreateSubjectHierarchyNode(slicer.mrmlScene, None, slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMLevelSeries(), slicer.util.toVTKString(loadable.name), dataNode)

    # Specify details of series node
    seriesInstanceUid = slicer.dicomDatabase.fileValue(firstFile,tags['seriesInstanceUID'])
    seriesNode.AddUID(slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMUIDName(), seriesInstanceUid)
    seriesNode.SetAttribute(slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMSeriesModalityAttributeName(), slicer.dicomDatabase.fileValue(firstFile, tags['seriesModality']))
    seriesNode.SetAttribute(slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMSeriesNumberAttributeName(), slicer.dicomDatabase.fileValue(firstFile, tags['seriesNumber']))
    # Set instance UIDs
    instanceUIDs = ""
    for file in loadable.files:
      uid = slicer.dicomDatabase.fileValue(file,tags['instanceUID'])
      if uid == "":
        uid = "Unknown"
      instanceUIDs += uid + " "
    instanceUIDs = instanceUIDs[:-1]  # strip last space
    seriesNode.AddUID(slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMInstanceUIDName(), instanceUIDs)
    
    # Set referenced instance UIDs from loadable to series
    referencedInstanceUIDs = ""
    if hasattr(loadable,'referencedInstanceUIDs'):
      for instanceUID in loadable.referencedInstanceUIDs:
        referencedInstanceUIDs += instanceUID + " "
    referencedInstanceUIDs = referencedInstanceUIDs[:-1]  # strip last space
    seriesNode.SetAttribute(slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMReferencedInstanceUIDsAttributeName(), referencedInstanceUIDs)

    # Add series node to hierarchy under the right study and patient nodes. If they are present then used, if not, then created
    patientId = slicer.dicomDatabase.fileValue(firstFile,tags['patientID'])
    patientNode = vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNodeByUID(slicer.mrmlScene, slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMUIDName(), patientId)
    studyInstanceUid = slicer.dicomDatabase.fileValue(firstFile,tags['studyInstanceUID'])
    studyId = slicer.dicomDatabase.fileValue(firstFile,tags['studyID'])
    studyNode = vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNodeByUID(slicer.mrmlScene, slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMUIDName(), studyInstanceUid)
    vtkSlicerSubjectHierarchyModuleLogic.InsertDicomSeriesInHierarchy(slicer.mrmlScene, patientId, studyInstanceUid, seriesInstanceUid)

    if patientNode is None:
      patientNode = vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNodeByUID(slicer.mrmlScene, slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMUIDName(), patientId)
      if patientNode is not None:
        # Add attributes for DICOM tags
        patientName = slicer.util.toVTKString(slicer.dicomDatabase.fileValue(firstFile,tags['patientName']))
        if patientName == '':
          patientName = 'No name'
        patientNode.SetAttribute(slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMPatientNameAttributeName(),patientName)
        patientNode.SetAttribute(slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMPatientIDAttributeName(),slicer.dicomDatabase.fileValue(firstFile, tags['patientID']))
        patientNode.SetAttribute(slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMPatientSexAttributeName(),slicer.dicomDatabase.fileValue(firstFile, tags['patientSex']))
        patientNode.SetAttribute(slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMPatientBirthDateAttributeName(),slicer.dicomDatabase.fileValue(firstFile, tags['patientBirthDate']))
        patientNode.SetAttribute(slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMPatientCommentsAttributeName(),slicer.dicomDatabase.fileValue(firstFile, tags['patientComments']))
        # Set node name
        patientNode.SetName(patientName + slicer.vtkMRMLSubjectHierarchyConstants.GetSubjectHierarchyNodeNamePostfix())

    if studyNode is None:
      studyNode = vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNodeByUID(slicer.mrmlScene, slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMUIDName(), studyInstanceUid)
      if studyNode is not None:
        # Add attributes for DICOM tags
        studyDescription = slicer.util.toVTKString(slicer.dicomDatabase.fileValue(firstFile,tags['studyDescription']))
        if studyDescription == '':
          studyDescription = 'No study description'
        studyNode.SetAttribute(slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMStudyDescriptionAttributeName(),studyDescription)
        studyDate = slicer.util.toVTKString(slicer.dicomDatabase.fileValue(firstFile,tags['studyDate']))
        studyNode.SetAttribute(slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMStudyInstanceUIDTagName(),studyInstanceUid)
        studyNode.SetAttribute(slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMStudyIDTagName(),studyId)
        studyNode.SetAttribute(slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMStudyDateAttributeName(),studyDate)
        studyNode.SetAttribute(slicer.vtkMRMLSubjectHierarchyConstants.GetDICOMStudyTimeAttributeName(),slicer.dicomDatabase.fileValue(firstFile, tags['studyTime']))
        # Set node name
        studyNode.SetName(studyDescription + ' (' + studyDate + ')' + slicer.vtkMRMLSubjectHierarchyConstants.GetSubjectHierarchyNodeNamePostfix())
