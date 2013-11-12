import os
from __main__ import qt
from __main__ import slicer
import DICOMLib


#########################################################
#
#
comment = """

  DICOMPlugin is a superclass for code that plugs into the
  slicer DICOM module.

  These classes are Abstract.

# TODO :
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
# DICOMExporter
#

class DICOMExporterWorkInProgress(object):
  """Container class for ways of exporting
  slicer data into DICOM.
  Each plugin returns a list of instances of this
  from its exportOptions method
  so the DICOM module can build an appropriate
  interface to offer user the options to export
  and perform the exporting operation.
  """

  def __init__(self):
    # callable to be invoked if user selects this exporter
    self.exportCallback = None
    # name exposed to the user for the export method
    self.name = "Untitled Exporter"
    # extra information the user sees on mouse over the export option
    self.tooltip = "Creates a DICOM file from the selected data"
    # if true, only the whole scene is an option for exporting
    self.exportScene = False
    # list of node types that can be exported
    self.nodeTypes = []



#
# DICOMPlugin
#

class DICOMPlugin(object):
  """ Base class for DICOM plugins
  """

  def __init__(self):
    # displayed for the user as the pluging handling the load
    self.loadType = "Generic DICOM"
    # a dictionary that maps a list of files to a list of loadables
    # (so that subsequent requests for the same info can be
    #  serviced quickly)
    self.loadableCache = {}
    # tags is a dictionary of symbolic name keys mapping to
    # hex tag number values (as in {'pixelData': '7fe0,0010'}).
    # Each subclass should define the tags it will be using in
    # calls to the dicom database so that any needed values
    # can be effiently pre-fetched if possible.
    self.tags = {}

  def hashFiles(self,files):
    """Create a hash key for a list of files"""
    try:
      import hashlib
    except:
      return None
    m = hashlib.md5()
    for f in files:
      m.update(f)
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

  def examine(self,fileList):
    """Look at the list of lists of filenames and return
    a list of DICOMLoadables that are options for loading
    Virtual: should be overridden by the subclass
    """
    return []

  def load(self,loadable):
    """Accept a DICOMLoadable and perform the operation to convert
    the referenced data into MRML nodes
    Virtual: should be overridden by the subclass
    """
    return True
    
  def onLoadFinished(self):
    """Perform steps needed after all selected loadables have
    been loaded
    Virtual: should be overridden by the subclass
    """
    pass

  def exportOptions(self):
    """Return a list of DICOMExporter instances that describe the
    available techniques that this plugin offers to convert MRML
    data into DICOM data
    Virtual: should be overridden by the subclass
    """
    return []
