import os
import time
import unittest
from __main__ import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *
import json

#
# MarkupsGenericStorageTest
#

class MarkupsGenericStorageTest(ScriptedLoadableModule):
  def __init__(self, parent):
    parent.title = "MarkupsGenericStorageTest"
    parent.categories = ["Testing.TestCases"]
    parent.dependencies = []
    parent.contributors = ["Johan Andruejol (Kitware Inc."]
    parent.helpText = """
    This is a test for the Generic Markups Storage NOde
    """
    parent.acknowledgementText = """
    This file was originally developed by Johan Andruejol and was partially funded by Allen Institute".
""" # replace with organization, grant and thanks.
    self.parent = parent

    # Add this test to the SelfTest module's list for discovery when the module
    # is created.  Since this module may be discovered before SelfTests itself,
    # create the list if it doesn't already exist.
    try:
      slicer.selfTests
    except AttributeError:
      slicer.selfTests = {}
    slicer.selfTests['MarkupsGenericStorageTest'] = self.runTest

  def runTest(self):
    tester = MarkupsGenericStorageTestTest()
    tester.runTest()

#
# qMarkupsGenericStorageTestWidget
#

class MarkupsGenericStorageTestWidget(ScriptedLoadableModuleWidget):

  def setup(self):
    ScriptedLoadableModuleWidget.setup(self)

#
# MarkupsGenericStorageTestLogic
#

class MarkupsGenericStorageTestLogic(ScriptedLoadableModuleLogic):

  def __init__(self):
    pass

#
# MarkupsGenericStorageTestTest
#

class MarkupsGenericStorageTestTest(ScriptedLoadableModuleTest):

  def setUp(self):
    """ Reset the state for testing.
    """
    slicer.mrmlScene.Clear(0)

  def runTest(self):
    """Run as few or as many tests as needed here.
    """
    self.setUp()
    self.test_MarkupsGenericStorageTest1()
    self.test_MarkupsGenericStorageTest2()
    self.test_MarkupsGenericStorageTest3()

  def saveAndReload(self, markup):
    self.assertIsNone(markup.GetStorageNode())

    self.assertTrue(markup.AddDefaultStorageNode())
    storage = markup.GetStorageNode()
    self.assertIsNotNone(storage)
    self.assertTrue(storage.IsA('vtkMRMLMarkupsGenericStorageNode'))

    tempFile = qt.QTemporaryFile("test-MarkupsGenericStorageTest-XXXXXX.markups.json")
    self.assertTrue(tempFile.open())

    slicer.util.saveNode(markup, tempFile.fileName())
    slicer.mrmlScene.Clear(0)

    # Make sure it can be read by python json
    file = open(tempFile.fileName())
    data = json.load(file)
    markupsKeys = [
      'Locked',
      'MarkupLabelFormat',
      'TextList',
      'TextList_Count',
      'Markups',
      'Markups_Count',
      ]
    for markupsKey in markupsKeys:
      self.assertTrue(markupsKey in data.keys(), msg="Checking for key %s" %markupsKey)

    for i in range(int(data['Markups_Count'])):
      markupSubKeys = [
        'ID',
        'Label',
        'Description',
        'AssociatedNodeID',
        'Points',
        'OrientationWXYZ',
        'Selected',
        'Locked',
        'Visibility',
        ]
      for subKey in markupSubKeys:
        self.assertTrue(subKey in data['Markups'][i].keys(), msg="Checking for sub key %s" %subKey)

    loaded = slicer.util.loadNodeFromFile(tempFile.fileName(), 'Markups', returnNode=True)
    self.assertTrue(loaded[0])
    self.assertIsNotNone(loaded[1])
    self.assertTrue(loaded[1].IsA('vtkMRMLMarkupsNode'))
    return loaded[1]


  def test_MarkupsGenericStorageTest1(self):
    self.delayDisplay("Starting testing the generic markups storage test 1")

    # Write markup to file
    markup = slicer.mrmlScene.AddNode(slicer.vtkMRMLMarkupsNode())
    newMarkup = self.saveAndReload(markup)

    self.assertEqual(newMarkup.GetLocked(), 0)
    self.assertEqual(newMarkup.GetMarkupLabelFormat(), '%N-%d')
    self.assertEqual(newMarkup.GetNumberOfTexts(), 0)
    self.assertEqual(newMarkup.GetNumberOfMarkups(), 0)

    self.delayDisplay('Test passed!')


  def test_MarkupsGenericStorageTest2(self):
    self.delayDisplay("Starting testing the generic markups storage test 2")

    # Write markup to file
    markup = slicer.mrmlScene.AddNode(slicer.vtkMRMLMarkupsNode())
    markup.SetLocked(1)
    markup.SetMarkupLabelFormat('THIS IS A TEST !')
    markup.AddText('And this is too')
    markup.AddText('finale line')

    markup.AddPointToNewMarkup(vtk.vtkVector3d(0, 0, 0))
    markup.SetNthMarkupLabel(0, "First markup")
    markup.SetNthMarkupDescription(0, "This is the description of the first markup")
    markup.SetNthMarkupAssociatedNodeID(0, 'vtkMRMLSliceNodeRed')
    markup.SetNthMarkupOrientation(0, 0, 1, 2, 3)
    markup.SetNthMarkupSelected(0, 1)
    markup.SetNthMarkupLocked(0, 1)
    markup.SetNthMarkupVisibility(0, 1)

    newMarkup = self.saveAndReload(markup)

    self.assertEqual(newMarkup.GetLocked(), 1)
    self.assertEqual(newMarkup.GetMarkupLabelFormat(), 'THIS IS A TEST !')
    self.assertEqual(newMarkup.GetNumberOfTexts(), 2)
    self.assertEqual(newMarkup.GetText(0), 'And this is too')
    self.assertEqual(newMarkup.GetText(1), 'finale line')

    self.assertEqual(newMarkup.GetNumberOfMarkups(), 1)
    self.assertEqual(newMarkup.GetNthMarkupLabel(0), "First markup")
    self.assertEqual(newMarkup.GetNthMarkupDescription(0), "This is the description of the first markup")
    self.assertEqual(newMarkup.GetNthMarkupAssociatedNodeID(0), 'vtkMRMLSliceNodeRed')
    orientation = range(4)
    newMarkup.GetNthMarkupOrientation(0, orientation)
    self.assertListEqual(orientation, [0, 1, 2, 3])
    self.assertEqual(newMarkup.GetNthMarkupSelected(0), 1)
    self.assertEqual(newMarkup.GetNthMarkupLocked(0), 1)
    self.assertEqual(newMarkup.GetNthMarkupVisibility(0), 1)

    self.delayDisplay('Test passed!')


  def test_MarkupsGenericStorageTest3(self):
    self.delayDisplay("Starting testing the generic markups storage test 3")

    # Write markup to file
    markup = slicer.mrmlScene.AddNode(slicer.vtkMRMLMarkupsNode())
    markup.AddPointToNewMarkup(vtk.vtkVector3d(0, 0, 0))

    logic = slicer.modules.markups.logic()
    defaultMarkupJSON = logic.GetModuleShareDirectory() + '/GenericStorage.markups.json'
    loaded = slicer.util.loadNodeFromFile(defaultMarkupJSON, 'Markups', returnNode=True)
    self.assertTrue(loaded[0])
    newMarkup = loaded[1]
    self.assertIsNotNone(newMarkup)

    self.assertEqual(markup.GetLocked(), newMarkup.GetLocked())
    self.assertEqual(markup.GetMarkupLabelFormat(), newMarkup.GetMarkupLabelFormat())
    self.assertEqual(markup.GetNumberOfTexts(), newMarkup.GetNumberOfTexts())

    self.assertEqual(markup.GetNumberOfMarkups(), newMarkup.GetNumberOfMarkups())
    self.assertEqual(markup.GetNthMarkupLabel(0), newMarkup.GetNthMarkupLabel(0))
    self.assertEqual(markup.GetNthMarkupDescription(0), newMarkup.GetNthMarkupDescription(0))
    self.assertEqual(markup.GetNthMarkupAssociatedNodeID(0), newMarkup.GetNthMarkupAssociatedNodeID(0))
    orientation = range(4)
    markup.GetNthMarkupOrientation(0, orientation)
    newOrientation = range(4)
    newMarkup.GetNthMarkupOrientation(0, newOrientation)
    self.assertListEqual(orientation, newOrientation)
    self.assertEqual(markup.GetNthMarkupSelected(0), newMarkup.GetNthMarkupSelected(0))
    self.assertEqual(markup.GetNthMarkupLocked(0), newMarkup.GetNthMarkupLocked(0))
    self.assertEqual(markup.GetNthMarkupVisibility(0), newMarkup.GetNthMarkupVisibility(0))

    self.delayDisplay('Test passed!')
