import vtk, slicer
from SegmentStatisticsPlugins import SegmentStatisticsPluginBase


class ScalarVolumeSegmentStatisticsPlugin(SegmentStatisticsPluginBase):
  """Statistical plugin for segmentations with scalar volumes"""

  def __init__(self):
    super(ScalarVolumeSegmentStatisticsPlugin,self).__init__()
    self.name = "Scalar Volume"
    self.keys = ["voxel_count", "volume_mm3", "volume_cm3", "min", "max", "mean", "stdev"]
    self.defaultKeys = self.keys # calculate all measurements by default
    #... developer may add extra options to configure other parameters

  def computeStatistics(self, segmentID):
    import vtkSegmentationCorePython as vtkSegmentationCore
    requestedKeys = self.getRequestedKeys()

    segmentationNode = slicer.mrmlScene.GetNodeByID(self.getParameterNode().GetParameter("Segmentation"))
    grayscaleNode = slicer.mrmlScene.GetNodeByID(self.getParameterNode().GetParameter("ScalarVolume"))

    if len(requestedKeys)==0:
      return {}

    containsLabelmapRepresentation = segmentationNode.GetSegmentation().ContainsRepresentation(
      vtkSegmentationCore.vtkSegmentationConverter.GetSegmentationBinaryLabelmapRepresentationName())
    if not containsLabelmapRepresentation:
      return {}

    if grayscaleNode is None or grayscaleNode.GetImageData() is None:
      return {}

    # Get geometry of grayscale volume node as oriented image data
    # reference geometry in reference node coordinate system
    referenceGeometry_Reference = vtkSegmentationCore.vtkOrientedImageData()
    referenceGeometry_Reference.SetExtent(grayscaleNode.GetImageData().GetExtent())
    ijkToRasMatrix = vtk.vtkMatrix4x4()
    grayscaleNode.GetIJKToRASMatrix(ijkToRasMatrix)
    referenceGeometry_Reference.SetGeometryFromImageToWorldMatrix(ijkToRasMatrix)

    # Get transform between grayscale volume and segmentation
    segmentationToReferenceGeometryTransform = vtk.vtkGeneralTransform()
    slicer.vtkMRMLTransformNode.GetTransformBetweenNodes(segmentationNode.GetParentTransformNode(),
      grayscaleNode.GetParentTransformNode(), segmentationToReferenceGeometryTransform)

    cubicMMPerVoxel = reduce(lambda x,y: x*y, referenceGeometry_Reference.GetSpacing())
    ccPerCubicMM = 0.001

    segment = segmentationNode.GetSegmentation().GetSegment(segmentID)
    segBinaryLabelName = vtkSegmentationCore.vtkSegmentationConverter.GetSegmentationBinaryLabelmapRepresentationName()
    segmentLabelmap = segment.GetRepresentation(segBinaryLabelName)

    segmentLabelmap_Reference = vtkSegmentationCore.vtkOrientedImageData()
    vtkSegmentationCore.vtkOrientedImageDataResample.ResampleOrientedImageToReferenceOrientedImage(
      segmentLabelmap, referenceGeometry_Reference, segmentLabelmap_Reference,
      False, # nearest neighbor interpolation
      False, # no padding
      segmentationToReferenceGeometryTransform)

    # We need to know exactly the value of the segment voxels, apply threshold to make force the selected label value
    labelValue = 1
    backgroundValue = 0
    thresh = vtk.vtkImageThreshold()
    thresh.SetInputData(segmentLabelmap_Reference)
    thresh.ThresholdByLower(0)
    thresh.SetInValue(backgroundValue)
    thresh.SetOutValue(labelValue)
    thresh.SetOutputScalarType(vtk.VTK_UNSIGNED_CHAR)
    thresh.Update()

    #  Use binary labelmap as a stencil
    stencil = vtk.vtkImageToImageStencil()
    stencil.SetInputData(thresh.GetOutput())
    stencil.ThresholdByUpper(labelValue)
    stencil.Update()

    stat = vtk.vtkImageAccumulate()
    stat.SetInputData(grayscaleNode.GetImageData())
    stat.SetStencilData(stencil.GetOutput())
    stat.Update()

    # create statistics list
    stats = {}
    if "voxel_count" in requestedKeys:
      stats["voxel_count"] = stat.GetVoxelCount()
    if "volume_mm3" in requestedKeys:
      stats["volume_mm3"] = stat.GetVoxelCount() * cubicMMPerVoxel
    if "volume_cm3" in requestedKeys:
      stats["volume_cm3"] = stat.GetVoxelCount() * cubicMMPerVoxel * ccPerCubicMM
    if stat.GetVoxelCount()>0:
      if "min" in requestedKeys:
        stats["min"] = stat.GetMin()[0]
      if "max" in requestedKeys:
        stats["max"] = stat.GetMax()[0]
      if "mean" in requestedKeys:
        stats["mean"] = stat.GetMean()[0]
      if "stdev" in requestedKeys:
        stats["stdev"] = stat.GetStandardDeviation()[0]
    return stats

  def getMeasurementInfo(self, key):
    """Get information (name, description, units, ...) about the measurement for the given key""" 
    
    scalarVolumeNode = slicer.mrmlScene.GetNodeByID(self.getParameterNode().GetParameter("ScalarVolume"))
    
    scalarVolumeQuantity = scalarVolumeNode.GetVoxelValueQuantity() if scalarVolumeNode else self.createCodedEntry("", "", "")
    scalarVolumeUnits = scalarVolumeNode.GetVoxelValueUnits() if scalarVolumeNode else self.createCodedEntry("", "", "")
    if not scalarVolumeQuantity:
      scalarVolumeQuantity = self.createCodedEntry("", "", "")
    if not scalarVolumeUnits:
      scalarVolumeUnits = self.createCodedEntry("", "", "")

    info = dict() 
    
    # @fedorov could not find any suitable DICOM quantity code for "number of voxels".
    # DCM has "Number of needles" etc., so probably "Number of voxels"
    # should be added too. Need to discuss with @dclunie. For now, a
    # QIICR private scheme placeholder.
    info["voxel_count"] = \
      self.createMeasurementInfo(name="Voxel count", description="Number of voxels", units="voxels",
                                   quantityDicomCode=self.createCodedEntry("nvoxels", "99QIICR", "Number of voxels", True),
                                   unitsDicomCode=self.createCodedEntry("voxels", "UCUM", "voxels", True))

    info["volume_mm3"] = \
      self.createMeasurementInfo(name="Volume mm3", description="Volume in mm3", units="mm3",
                                   quantityDicomCode=self.createCodedEntry("G-D705", "SRT", "Volume", True),
                                   unitsDicomCode=self.createCodedEntry("mm3", "UCUM", "cubic millimeter", True))

    info["volume_cm3"] = \
      self.createMeasurementInfo(name="Volume cm3", description="Volume in cm3", units="cm3",
                                   quantityDicomCode=self.createCodedEntry("G-D705", "SRT", "Volume", True),
                                   unitsDicomCode=self.createCodedEntry("cm3","UCUM","cubic centimeter", True),
                                   measurementMethodDicomCode=self.createCodedEntry("126030", "DCM",
                                                                             "Sum of segmented voxel volumes", True))

    info["min"] = \
      self.createMeasurementInfo(name="Minimum", description="Minimum scalar value",
                                   units=scalarVolumeUnits.GetCodeMeaning(),
                                   quantityDicomCode=scalarVolumeQuantity.GetAsString(),
                                   unitsDicomCode=scalarVolumeUnits.GetAsString(),
                                   derivationDicomCode=self.createCodedEntry("R-404FB", "SRT", "Minimum", True))

    info["max"] = \
      self.createMeasurementInfo(name="Maximum", description="Maximum scalar value",
                                   units=scalarVolumeUnits.GetCodeMeaning(),
                                   quantityDicomCode=scalarVolumeQuantity.GetAsString(),
                                   unitsDicomCode=scalarVolumeUnits.GetAsString(),
                                   derivationDicomCode=self.createCodedEntry("G-A437","SRT","Maximum", True))

    info["mean"] = \
      self.createMeasurementInfo(name="Mean", description="Mean scalar value",
                                   units=scalarVolumeUnits.GetCodeMeaning(),
                                   quantityDicomCode=scalarVolumeQuantity.GetAsString(),
                                   unitsDicomCode=scalarVolumeUnits.GetAsString(),
                                   derivationDicomCode=self.createCodedEntry("R-00317","SRT","Mean", True))

    info["stdev"] = \
      self.createMeasurementInfo(name="Standard deviation", description="Standard deviation of scalar values",
                                   units=scalarVolumeUnits.GetCodeMeaning(),
                                   quantityDicomCode=scalarVolumeQuantity.GetAsString(),
                                   unitsDicomCode=scalarVolumeUnits.GetAsString(),
                                   derivationDicomCode=self.createCodedEntry('R-10047','SRT','Standard Deviation', True))

    return info[key] if key in info else None
