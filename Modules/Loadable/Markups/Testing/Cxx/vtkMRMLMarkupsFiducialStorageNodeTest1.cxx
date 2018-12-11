/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// MRML includes
#include "vtkMRMLCoreTestingMacros.h"
#include "vtkMRMLMarkupsDisplayNode.h"
#include "vtkMRMLMarkupsFiducialStorageNode.h"
#include "vtkMRMLMarkupsFiducialNode.h"
#include "vtkURIHandler.h"
#include "vtkMRMLScene.h"
#include "vtkPolyData.h"

// VTK includes
#include <vtkNew.h>
#include <vtkTestingOutputWindow.h>

// STD includes
#include <algorithm>

int vtkMRMLMarkupsFiducialStorageNodeTest1(int argc, char * argv[] )
{
  vtkNew<vtkMRMLMarkupsFiducialStorageNode> node1;
  EXERCISE_ALL_BASIC_MRML_METHODS(node1.GetPointer());

  vtkNew<vtkMRMLMarkupsFiducialNode> markupsNode;
  vtkNew<vtkMRMLMarkupsDisplayNode> displayNode;

  // get the file name
  std::string fileName = std::string("testMarkupsStorageNode.fcsv");
  if (argc > 1)
    {
    fileName = std::string(argv[1]);
    }
  std::cout << "Using file name " << fileName.c_str() << std::endl;

  // set up a scene
  vtkNew<vtkMRMLScene> scene;
  scene->AddNode(node1.GetPointer());
  scene->AddNode(markupsNode.GetPointer());
  scene->AddNode(displayNode.GetPointer());
  markupsNode->SetAndObserveStorageNodeID(node1->GetID());
  markupsNode->SetAndObserveDisplayNodeID(displayNode->GetID());

  // add a markup with one point with non default values
  int index =  markupsNode->AddNControlPoints(1);
  double orientation[4] = {0.2, 1.0, 0.0, 0.0};
  markupsNode->SetNthControlPointOrientationFromArray(index, orientation);
  markupsNode->ResetNthControlPointID(index);
  std::string associatedNodeID = std::string("testingAssociatedID");
  markupsNode->SetNthControlPointAssociatedNodeID(index,associatedNodeID);
  markupsNode->SetNthControlPointSelected(index, 0);
  markupsNode->SetNthControlPointVisibility(index, 0);
  markupsNode->SetNthControlPointLocked(index, 1);

  std::string label = std::string("Testing label");
  markupsNode->SetNthControlPointLabel(index, label);
  std::string desc = std::string("description with spaces");
  markupsNode->SetNthControlPointDescription(index, desc);
  double inputPoint[3] = {-9.9, 1.1, 0.87};
  markupsNode->SetNthControlPointPositionFromArray(index, inputPoint);

  // and add a markup with 1 point, default values
  markupsNode->AddNControlPoints(1);
  // and another one unsetting the label
  index = markupsNode->AddNControlPoints(1);
  markupsNode->SetNthControlPointLabel(index,"");

  // add another one with a label and description that have commas in them
  int commaIndex = markupsNode->AddNControlPoints(1);
  markupsNode->SetNthControlPointLabel(commaIndex, "Label, commas, two");
  markupsNode->SetNthControlPointDescription(commaIndex, "Description one comma \"and two quotes\", for more testing");
  // add another one with a label and description with complex combos of commas and quotes
  int quotesIndex = markupsNode->AddNControlPoints(1);
  markupsNode->SetNthControlPointLabel(quotesIndex, "Label with end quotes \"around the last phrase\"");
  markupsNode->SetNthControlPointDescription(quotesIndex, "\"Description fully quoted\"");

  //
  // test write
  //

  std::cout << "\nWriting this markup to file:" << std::endl;
  vtkIndent indent;
  markupsNode->PrintSelf(std::cout, indent);
  std::cout << std::endl;

  node1->SetFileName(fileName.c_str());
  std::cout << "Writing " << node1->GetFileName() << std::endl;
  int retval = node1->WriteData(markupsNode.GetPointer());
  if (!retval)
    {
    std::cerr << "Failed with return value " << retval
              << "\n\twhen writing to file:\n\t"
              << node1->GetFileName() << std::endl;
    return EXIT_FAILURE;
    }

  //
  // test read
  //
  vtkNew<vtkMRMLMarkupsFiducialStorageNode> snode2;
  vtkNew<vtkMRMLMarkupsFiducialNode> markupsNode2;
  vtkNew<vtkMRMLScene> scene2;
  scene2->AddNode(snode2.GetPointer());
  scene2->AddNode(markupsNode2.GetPointer());
  markupsNode2->SetAndObserveStorageNodeID(snode2->GetID());
  snode2->SetFileName(node1->GetFileName());

  std::cout << "Reading from " << snode2->GetFileName() << std::endl;

  TESTING_OUTPUT_ASSERT_WARNINGS_BEGIN();
  retval = snode2->ReadData(markupsNode2.GetPointer());
  TESTING_OUTPUT_ASSERT_WARNINGS(2); // 2 warnings are logged: display node is not available, display node is added
  TESTING_OUTPUT_ASSERT_WARNINGS_END();
  if (!retval)
    {
    std::cerr << "Failed to read from file " << snode2->GetFileName()
              << std::endl;
    return EXIT_FAILURE;
    }

  std::cout << "\nMarkup read from file = " << std::endl;
  markupsNode2->PrintSelf(std::cout, indent);
  std::cout << std::endl;

  // test values on the first markup
  index = 0;
  double newOrientation[4];
  markupsNode2->GetNthControlPointOrientation(index, newOrientation);
  for (int r = 0; r < 4; r++)
    {
    if (newOrientation[r] != orientation[r])
      {
      std::cerr << "Failed to set orientation! "
                << "Expected: "
                << orientation[0] << ", "
                << orientation[1] << ", "
                << orientation[2] << ", "
                << orientation[3]
                << " but got: "
                << newOrientation[0] << ", "
                << newOrientation[1] << ", "
                << newOrientation[2] << ", "
                << newOrientation[3] << std::endl;
      return EXIT_FAILURE;
      }
    }
  if (markupsNode2->GetNthControlPointAssociatedNodeID(index).compare(associatedNodeID) != 0)
    {
    std::cerr << "After reading in, expected markup " << index
              << " associatedNodeID of " << associatedNodeID.c_str()
              << " got "
              << markupsNode2->GetNthControlPointAssociatedNodeID(index).c_str()
              << std::endl;
    return EXIT_FAILURE;
    }
  if (markupsNode2->GetNthControlPointSelected(index) != 0)
    {
    std::cerr << "After reading in, expected markup " << index
              << " sel of 0, got "
              << markupsNode2->GetNthControlPointSelected(index) << std::endl;
    return EXIT_FAILURE;
    }
  if (markupsNode2->GetNthControlPointVisibility(index) != 0)
    {
    std::cerr << "After reading in, expected markup " << index
              << " vis of 0, got "
              << markupsNode2->GetNthControlPointVisibility(index) << std::endl;
    return EXIT_FAILURE;
    }
  if (markupsNode2->GetNthControlPointLocked(index) != 1)
    {
    std::cerr << "After reading in, expected markup " << index
              << " locked of 0, got "
              << markupsNode2->GetNthControlPointLocked(index) << std::endl;
    return EXIT_FAILURE;
    }
  if (markupsNode2->GetNthControlPointLabel(index).compare(label) != 0)
    {
    std::cerr << "After reading in, expected markup " << index
              << " label of " << label.c_str() << " got '"
              << markupsNode2->GetNthControlPointLabel(index).c_str()
              << "'" << std::endl;
    return EXIT_FAILURE;
    }
  if (markupsNode2->GetNthControlPointDescription(index).compare(desc) != 0)
    {
    std::cerr << "After reading in, expected markup " << index
              << " description of " << desc.c_str() << " got '"
              << markupsNode2->GetNthControlPointDescription(index).c_str()
              << "'" << std::endl;
    return EXIT_FAILURE;
    }
  double outputPoint[3];
  markupsNode2->GetNthControlPointPosition(index, outputPoint);
  double diff = fabs(outputPoint[0] - inputPoint[0]) + fabs(outputPoint[1] - inputPoint[1]) + fabs(outputPoint[2] - inputPoint[2]);
  if (diff > 0.1)
    {
    std::cerr << "After reading in, expected markup " << index << " point "
              << inputPoint[0] << "," << inputPoint[1] << "," << inputPoint[2]
              << " but got a diff of " << diff << " from read in point "
              << outputPoint[0] << "," << outputPoint[1] << "," << outputPoint[2]
              << std::endl;
    return EXIT_FAILURE;
    }

  // test the unset label on the third markup
  index = 2;
  if (markupsNode2->GetNthControlPointLabel(index).size() != 0)
    {
    std::cerr << "After reading in, expected markup " << index
              << " empty label, but got "
              << markupsNode2->GetNthControlPointLabel(index).c_str() << std::endl;
    return EXIT_FAILURE;
    }

  // now read it again with a display node defined
  vtkNew<vtkMRMLMarkupsDisplayNode> displayNode2;
  scene2->AddNode(displayNode2.GetPointer());
  markupsNode2->SetAndObserveDisplayNodeID(displayNode2->GetID());
  std::cout << "Added display node, re-reading from "
            << snode2->GetFileName() << std::endl;
  retval = snode2->ReadData(markupsNode2.GetPointer());
  if (!retval)
    {
    std::cerr << "Failed second read from file "
              << snode2->GetFileName() << std::endl;
    return EXIT_FAILURE;
    }

  //
  // test with use LPS flag on
  node1->UseLPSOn();
  std::cout << "Writing file with use LPS flag set:\n"
            << node1->GetFileName() << std::endl;
  retval = node1->WriteData(markupsNode.GetPointer());
  if (!retval)
    {
    std::cerr << "Failed with return value " << retval
              << "\n\twhen writing with LPS to file:\n\t"
              << node1->GetFileName() << std::endl;
    return EXIT_FAILURE;
    }
  // read it in after clearing out the test data
  snode2->UseLPSOn();
  markupsNode2->RemoveAllControlPoints();
  std::cout << "Reading with LPS from " << snode2->GetFileName() << std::endl;

  retval = snode2->ReadData(markupsNode2.GetPointer());
  if (!retval)
    {
    std::cerr << "Failed to read from file with LPS " << snode2->GetFileName()
              << std::endl;
    return EXIT_FAILURE;
    }

  std::cout << "\nMarkups with LPS read from file = " << std::endl;
  markupsNode2->PrintSelf(std::cout, indent);

  // check the point so that it's been converted back to be the same
  double outputPointAfterLPS[3];
  index = 0;
  markupsNode2->GetNthControlPointPosition(index, outputPointAfterLPS);
  diff =  fabs(outputPointAfterLPS[0] - inputPoint[0]) + fabs(outputPointAfterLPS[1] - inputPoint[1]) + fabs(outputPointAfterLPS[2] - inputPoint[2]);
  if (diff > 0.1)
    {
    std::cerr << "After reading in after using LPS, expected markup " << index
              << " point "
              << inputPoint[0] << "," << inputPoint[1] << "," << inputPoint[2]
              << " but got a diff of " << diff << " from read in point "
              << outputPointAfterLPS[0] << "," << outputPointAfterLPS[1] << ","
              << outputPointAfterLPS[2] << std::endl;
    return EXIT_FAILURE;
    }
  else
    {
    double lpsPoint[3];
    markupsNode2->GetNthControlPointPositionLPS(index, lpsPoint);
    std::cout << "Successfully wrote out point as lps "
              << lpsPoint[0] << "," << lpsPoint[1] << "," << lpsPoint[2]
              << " and read it back in as RAS "
              << outputPointAfterLPS[0] << "," << outputPointAfterLPS[1] << ","
              << outputPointAfterLPS[2] << std::endl;
    }

  // check for commas in the markup label and description
  std::string labelWithCommas = markupsNode2->GetNthControlPointLabel(commaIndex);
  int numCommas = std::count(labelWithCommas.begin(), labelWithCommas.end(), ',');
  if (numCommas != 2)
    {
    std::cerr << "Expected a label with two commas, got " << numCommas
              << " in label " << labelWithCommas.c_str() << std::endl;
    return EXIT_FAILURE;
    }
  std::string descriptionWithCommasAndQuotes = markupsNode2->GetNthControlPointDescription(commaIndex);
  numCommas = std::count(descriptionWithCommasAndQuotes.begin(),
                         descriptionWithCommasAndQuotes.end(),
                         ',');
  if (numCommas != 1)
    {
    std::cerr << "Expected a description with one comma, got " << numCommas
              << " in description '" << descriptionWithCommasAndQuotes.c_str()
              << "'" << std::endl;
    return EXIT_FAILURE;
    }
  int numQuotes = std::count(descriptionWithCommasAndQuotes.begin(),
                            descriptionWithCommasAndQuotes.end(),
                            '"');
  if (numQuotes != 2)
    {
    std::cerr << "Expected a description with two quotes, got " << numQuotes
              << " in description '" << descriptionWithCommasAndQuotes
              << "'" << std::endl;
    return EXIT_FAILURE;
    }

  // check ending quoted label
  std::string labelWithQuotes = markupsNode2->GetNthControlPointLabel(quotesIndex);
  numQuotes = std::count(labelWithQuotes.begin(), labelWithQuotes.end(), '"');
  if (numQuotes != 2)
    {
    std::cerr << "Expected 2 quotes in '" << labelWithQuotes << "' but got " << numQuotes << std::endl;
    return EXIT_FAILURE;
    }
  // check fully quoted description
  std::string descWithQuotes = markupsNode2->GetNthControlPointDescription(quotesIndex);
  numQuotes = std::count(descWithQuotes.begin(), descWithQuotes.end(), '"');
  if (numQuotes != 2)
    {
    std::cerr << "Expected 2 quotes in '" << descWithQuotes << "' but got " << numQuotes << std::endl;
    }
  return EXIT_SUCCESS;
}
