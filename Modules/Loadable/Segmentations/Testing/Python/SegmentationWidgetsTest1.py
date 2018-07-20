import os
import unittest
import vtk, qt, ctk, slicer
import logging

import vtkSegmentationCorePython as vtkSegmentationCore

class SegmentationWidgetsTest1(unittest.TestCase):
  def setUp(self):
    """ Do whatever is needed to reset the state - typically a scene clear will be enough.
    """
    slicer.mrmlScene.Clear(0)

  def runTest(self):
    """Run as few or as many tests as needed here.
    """
    self.setUp()
    self.test_SegmentationWidgetsTest1()

  #------------------------------------------------------------------------------
  def test_SegmentationWidgetsTest1(self):
    # Check for modules
    self.assertIsNotNone( slicer.modules.segmentations )

    self.TestSection_00_SetupPathsAndNames()
    self.TestSection_01_GenerateInputData()
    self.TestSection_1_qMRMLSegmentsTableView()
    self.TestSection_2_qMRMLSegmentationGeometryWidget()

    logging.info('Test finished')

  #------------------------------------------------------------------------------
  def TestSection_00_SetupPathsAndNames(self):
    self.inputSegmentationNode = None

  #------------------------------------------------------------------------------
  def TestSection_01_GenerateInputData(self):

    self.inputSegmentationNode = slicer.vtkMRMLSegmentationNode()
    slicer.mrmlScene.AddNode(self.inputSegmentationNode)

    # Create new segments
    import random
    for segmentName in ['first', 'second', 'third']:
      sphereSegment = vtkSegmentationCore.vtkSegment()
      sphereSegment.SetName(segmentName)
      sphereSegment.SetColor(random.uniform(0.0,1.0), random.uniform(0.0,1.0), random.uniform(0.0,1.0))

      sphere = vtk.vtkSphereSource()
      sphere.SetCenter(random.uniform(0,100),random.uniform(0,100),random.uniform(0,100))
      sphere.SetRadius(random.uniform(20,30))
      sphere.Update()
      spherePolyData = sphere.GetOutput()
      sphereSegment.AddRepresentation(
        vtkSegmentationCore.vtkSegmentationConverter.GetSegmentationClosedSurfaceRepresentationName(),
        spherePolyData)

      self.inputSegmentationNode.GetSegmentation().AddSegment(sphereSegment)

    self.assertEqual(self.inputSegmentationNode.GetSegmentation().GetNumberOfSegments(), 3)

  #------------------------------------------------------------------------------
  def TestSection_1_qMRMLSegmentsTableView(self):
    logging.info('Test section 1: qMRMLSegmentsTableView')

    segmentsTableView = slicer.qMRMLSegmentsTableView()
    segmentsTableView.setMRMLScene(slicer.mrmlScene)
    segmentsTableView.setSegmentationNode(self.inputSegmentationNode)
    self.assertEqual(len(segmentsTableView.displayedSegmentIDs()), 3)
    segmentsTableView.show()

    slicer.app.processEvents()
    slicer.util.delayDisplay("All shown")

    segmentsTableView.setHideSegments(['second'])
    self.assertEqual(len(segmentsTableView.displayedSegmentIDs()), 2)
    slicer.app.processEvents()
    slicer.util.delayDisplay("Hidden 'second'")

  #------------------------------------------------------------------------------
  def compareOutputGeometry(self, orientedImageData, spacing, origin, directions):
    if orientedImageData is None:
      logging.error('Invalid input oriented image data')
      return False
    if (not isinstance(spacing, list) and not isinstance(spacing, tuple)) \
        or (not isinstance(origin, list) and not isinstance(origin, tuple)) \
        or not isinstance(directions, list):
      logging.error('Invalid baseline object types - need lists')
      return False
    if len(spacing) != 3 or len(origin) != 3 or len(directions) != 3 \
        or len(directions[0]) != 3 or len(directions[1]) != 3 or len(directions[2]) != 3:
      logging.error('Baseline lists need to contain 3 elements each, the directions 3 lists of 3')
      return False
    import numpy
    tolerance = 0.0001
    actualSpacing = orientedImageData.GetSpacing()
    actualOrigin = orientedImageData.GetOrigin()
    actualDirections = [[0]*3]*3
    orientedImageData.GetDirections(actualDirections)
    for i in [0,1,2]:
      if not numpy.isclose(spacing[i], actualSpacing[i], tolerance):
        logging.warning('Spacing discrepancy: ' + str(spacing) + ' != ' + str(actualSpacing))
        return False
      if not numpy.isclose(origin[i], actualOrigin[i], tolerance):
        logging.warning('Origin discrepancy: ' + str(origin) + ' != ' + str(actualOrigin))
        return False
      for j in [0,1,2]:
        if not numpy.isclose(directions[i][j], actualDirections[i][j], tolerance):
          logging.warning('Directions discrepancy: ' + str(directions) + ' != ' + str(actualDirections))
          return False
    return True

  #------------------------------------------------------------------------------
  def imageDataContainsData(self, imageData):
    if imageData is None:
      logging.error('Invalid input image data')
      return False
    acc = vtk.vtkImageAccumulate()
    acc.SetInputData(imageData)
    acc.SetIgnoreZero(1)
    acc.Update()
    if acc.GetVoxelCount() > 0:
      return True
    return False

  #------------------------------------------------------------------------------
  def TestSection_2_qMRMLSegmentationGeometryWidget(self):
    logging.info('Test section 2: qMRMLSegmentationGeometryWidget')

    import vtkSegmentationCore
    binaryLabelmapReprName = vtkSegmentationCore.vtkSegmentationConverter.GetBinaryLabelmapRepresentationName()
    closedSurfaceReprName = vtkSegmentationCore.vtkSegmentationConverter.GetClosedSurfaceRepresentationName()

    # Use MRHead and Tinypatient for testing
    import SampleData
    sampleDataLogic = SampleData.SampleDataLogic()
    mrVolumeNode = sampleDataLogic.downloadMRHead()
    [tinyVolumeNode, tinySegmentationNode] = sampleDataLogic.downloadSample('TinyPatient')

    # Convert MRHead to oriented image data
    import vtkSlicerSegmentationsModuleLogicPython as vtkSlicerSegmentationsModuleLogic
    mrOrientedImageData = vtkSlicerSegmentationsModuleLogic.vtkSlicerSegmentationsModuleLogic.CreateOrientedImageDataFromVolumeNode(mrVolumeNode)
    mrOrientedImageData.UnRegister(None)

    # Create segmentation node with binary labelmap master and one segment with MRHead geometry
    segmentationNode = slicer.vtkMRMLSegmentationNode()
    segmentationNode.GetSegmentation().SetMasterRepresentationName(binaryLabelmapReprName)
    geometryStr = vtkSegmentationCore.vtkSegmentationConverter.SerializeImageGeometry(mrOrientedImageData)
    segmentationNode.GetSegmentation().SetConversionParameter(
      vtkSegmentationCore.vtkSegmentationConverter.GetReferenceImageGeometryParameterName(), geometryStr)
    slicer.mrmlScene.AddNode(segmentationNode)
    threshold = vtk.vtkImageThreshold()
    threshold.SetInputData(mrOrientedImageData)
    threshold.ThresholdByUpper(16.0)
    threshold.SetInValue(1)
    threshold.SetOutValue(0)
    threshold.SetOutputScalarType(vtk.VTK_UNSIGNED_CHAR)
    threshold.Update()
    segmentOrientedImageData = vtkSegmentationCore.vtkOrientedImageData()
    segmentOrientedImageData.DeepCopy(threshold.GetOutput())
    mrImageToWorldMatrix = vtk.vtkMatrix4x4()
    mrOrientedImageData.GetImageToWorldMatrix(mrImageToWorldMatrix)
    segmentOrientedImageData.SetImageToWorldMatrix(mrImageToWorldMatrix)
    segment = vtkSegmentationCore.vtkSegment()
    segment.SetName('Brain')
    segment.SetColor(0.0,0.0,1.0)
    segment.AddRepresentation(binaryLabelmapReprName, segmentOrientedImageData)
    segmentationNode.GetSegmentation().AddSegment(segment)

    # Create geometry widget
    geometryWidget = slicer.qMRMLSegmentationGeometryWidget()
    geometryWidget.setSegmentationNode(segmentationNode)
    geometryWidget.editEnabled = True
    geometryImageData = vtkSegmentationCore.vtkOrientedImageData() # To contain the output later

    # Volume source with no transforms
    geometryWidget.setSourceNode(tinyVolumeNode)
    geometryWidget.geometryImageData(geometryImageData)
    self.assertTrue(self.compareOutputGeometry(geometryImageData,
        (49,49,23), (248.8439, 248.2890, -123.75),
        [[0.0, 0.0, 1.0], [0.0, 0.0, 1.0], [0.0, 0.0, 1.0]]))
    slicer.util.delayDisplay('Volume source with no transforms - OK')

    # Transformed volume source
    translationTransformMatrix = vtk.vtkMatrix4x4()
    translationTransformMatrix.SetElement(0,3,24.5)
    translationTransformMatrix.SetElement(1,3,24.5)
    translationTransformMatrix.SetElement(2,3,11.5)
    translationTransformNode = slicer.vtkMRMLLinearTransformNode()
    translationTransformNode.SetName('TestTranslation')
    slicer.mrmlScene.AddNode(translationTransformNode)
    translationTransformNode.SetMatrixTransformToParent(translationTransformMatrix)

    tinyVolumeNode.SetAndObserveTransformNodeID(translationTransformNode.GetID())
    geometryWidget.geometryImageData(geometryImageData)
    self.assertTrue(self.compareOutputGeometry(geometryImageData,
        (49,49,23), (224.3439, 223.7890, -135.25),
        [[0.0, 0.0, 1.0], [0.0, 0.0, 1.0], [0.0, 0.0, 1.0]]))
    slicer.util.delayDisplay('Transformed volume source - OK')

    # Volume source with isotropic spacing
    tinyVolumeNode.SetAndObserveTransformNodeID(None)
    geometryWidget.setIsotropicSpacing(True)
    geometryWidget.geometryImageData(geometryImageData)
    self.assertTrue(self.compareOutputGeometry(geometryImageData,
        (23,23,23), (248.8439, 248.2890, -123.75),
        [[0.0, 0.0, 1.0], [0.0, 0.0, 1.0], [0.0, 0.0, 1.0]]))
    slicer.util.delayDisplay('Volume source with isotropic spacing - OK')

    # Volume source with oversampling
    geometryWidget.setIsotropicSpacing(False)
    geometryWidget.setOversamplingFactor(2.0)
    geometryWidget.geometryImageData(geometryImageData)
    self.assertTrue(self.compareOutputGeometry(geometryImageData,
        (24.5, 24.5, 11.5), (261.0939, 260.5390, -129.5),
        [[0.0, 0.0, 1.0], [0.0, 0.0, 1.0], [0.0, 0.0, 1.0]]))
    slicer.util.delayDisplay('Volume source with oversampling - OK')

    # Segmentation source with binary labelmap master
    geometryWidget.setOversamplingFactor(1.0)
    geometryWidget.setSourceNode(tinySegmentationNode)
    geometryWidget.geometryImageData(geometryImageData)
    self.assertTrue(self.compareOutputGeometry(geometryImageData,
        (49,49,23), (248.8439, 248.2890, -123.75),
        [[0.0, 0.0, 1.0], [0.0, 0.0, 1.0], [0.0, 0.0, 1.0]]))
    slicer.util.delayDisplay('Segmentation source with binary labelmap master - OK')

    # Segmentation source with closed surface master
    tinySegmentationNode.GetSegmentation().SetConversionParameter('Smoothing factor', '0.0')
    self.assertTrue(tinySegmentationNode.GetSegmentation().CreateRepresentation(closedSurfaceReprName))
    tinySegmentationNode.GetSegmentation().SetMasterRepresentationName(closedSurfaceReprName)
    tinySegmentationNode.Modified() # Trigger re-calculation of geometry (only generic Modified event is observed)
    geometryWidget.geometryImageData(geometryImageData)
    # Manually resample and check if the output is non-empty
    # Note: This is done for all poly data based geometry calculations below
    #TODO: Come up with proper geometry baselines for these too
    vtkSegmentationCore.vtkOrientedImageDataResample.ResampleOrientedImageToReferenceOrientedImage(
      segmentOrientedImageData, geometryImageData, geometryImageData, False, True)
    self.assertTrue(self.imageDataContainsData(geometryImageData))
    slicer.util.delayDisplay('Segmentation source with closed surface master - OK')

    # Model source with no transform
    outputModelHierarchy = slicer.vtkMRMLModelHierarchyNode()
    slicer.mrmlScene.AddNode(outputModelHierarchy)
    success = vtkSlicerSegmentationsModuleLogic.vtkSlicerSegmentationsModuleLogic.ExportVisibleSegmentsToModelHierarchy(
      tinySegmentationNode, outputModelHierarchy )
    self.assertTrue(success)
    modelNode = slicer.util.getNode('Body_Contour')
    geometryWidget.setSourceNode(modelNode)
    geometryWidget.geometryImageData(geometryImageData)
    vtkSegmentationCore.vtkOrientedImageDataResample.ResampleOrientedImageToReferenceOrientedImage(
      segmentOrientedImageData, geometryImageData, geometryImageData, False, True)
    self.assertTrue(self.imageDataContainsData(geometryImageData))
    slicer.util.delayDisplay('Model source with no transform - OK')

    # Transformed model source
    rotationTransform = vtk.vtkTransform()
    rotationTransform.RotateX(45)
    rotationTransformMatrix = vtk.vtkMatrix4x4()
    rotationTransform.GetMatrix(rotationTransformMatrix)
    rotationTransformNode = slicer.vtkMRMLLinearTransformNode()
    rotationTransformNode.SetName('TestRotation')
    slicer.mrmlScene.AddNode(rotationTransformNode)
    rotationTransformNode.SetMatrixTransformToParent(rotationTransformMatrix)

    modelNode.SetAndObserveTransformNodeID(rotationTransformNode.GetID())
    modelNode.Modified()
    geometryWidget.geometryImageData(geometryImageData)
    vtkSegmentationCore.vtkOrientedImageDataResample.ResampleOrientedImageToReferenceOrientedImage(
      segmentOrientedImageData, geometryImageData, geometryImageData, False, True)
    self.assertTrue(self.imageDataContainsData(geometryImageData))
    slicer.util.delayDisplay('Transformed model source - OK')

    # ROI source
    roiNode = slicer.vtkMRMLAnnotationROINode()
    roiNode.SetName('SourceROI')
    slicer.mrmlScene.AddNode(roiNode)
    roiNode.UnRegister(None)
    xyz = [0]*3
    center = [0]*3
    slicer.vtkMRMLSliceLogic.GetVolumeRASBox(tinyVolumeNode, xyz, center)
    radius = map(lambda x: x/2.0, xyz)
    roiNode.SetXYZ(center)
    roiNode.SetRadiusXYZ(radius)
    geometryWidget.setSourceNode(roiNode)
    geometryWidget.geometryImageData(geometryImageData)
    vtkSegmentationCore.vtkOrientedImageDataResample.ResampleOrientedImageToReferenceOrientedImage(
      segmentOrientedImageData, geometryImageData, geometryImageData, False, True)
    self.assertTrue(self.imageDataContainsData(geometryImageData))
    slicer.util.delayDisplay('ROI source - OK')

    slicer.util.delayDisplay('Segmentation geometry widget test passed')
