/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Johan Andruejol, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// Volumes logic
#include "vtkSlicerUnitsLogic.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLUnitNode.h>

// VTK includes
#include <vtkNew.h>

namespace
{
template <class T>void printNodes(const std::vector<T>& nodes);
template <class T>bool areValidNodes(const std::vector<T>& nodes, const char* testName = 0);
bool testScene();
bool testSelectionNode();
bool testCloseScene();
bool testSaveAndReloadScene();
bool testImportScene(const char* sceneFilePath);
}

//-----------------------------------------------------------------------------
int vtkSlicerUnitsLogicTest1( int argc , char * argv[] )
{
  bool res = true;
  res = res && testScene();
  res = res && testSelectionNode();
  res = res && testCloseScene();
  res = res && testSaveAndReloadScene();
  if (argc > 1)
    {
    res = res && testImportScene(argv[1]);
    }
  return res ? EXIT_SUCCESS : EXIT_FAILURE;
}

namespace
{

//-----------------------------------------------------------------------------
template <class T> void printNodes(const std::vector<T>& nodes)
{
  for (size_t i = 0; i < nodes.size(); ++i)
    {
    vtkMRMLUnitNode* node = vtkMRMLUnitNode::SafeDownCast(nodes[i]);
    if (!node)
      {
      std::cout << i << ": NULL" << std::endl;
      }
    else
      {
      std::cout << i << ": " << node->GetID() << " "  << node << std::endl;
      }
    }
}

//-----------------------------------------------------------------------------
template <class T> bool areValidNodes(const std::vector<T>& nodes,
                                      const char* testName)
{
  const size_t numberOfUnits = 2;
  const char* unitNodeIDs[numberOfUnits] = {"vtkMRMLUnitNodeApplicationLength",
                                            "vtkMRMLUnitNodeApplicationTime",
                                            };

  if (nodes.size() != numberOfUnits)
    {
    std::cerr << (testName ? testName : "") << ":" << std::endl
              << "Not the right number of unit node. "
              << "Expected " << numberOfUnits << " nodes, "
              << " got instead " << nodes.size() << " nodes."
              << std::endl;
    printNodes(nodes);
    return false;
    }

  for (size_t i = 0; i < numberOfUnits; ++i)
    {
    vtkMRMLUnitNode* node = vtkMRMLUnitNode::SafeDownCast(nodes[i]);
    if (!node || strcmp(node->GetID(), unitNodeIDs[i]) != 0)
      {
      std::cerr << (testName ? testName : "") << ":" << std::endl
                << "Expecting node " << unitNodeIDs[i]<<" Got: "
                << (node ? node->GetID() : "NONE") << std::endl;
      return false;
      }

    if (node->GetSaveWithScene())
      {
      std::cerr << (testName ? testName : "") << ":" << std::endl
                << "Node " << node->GetID()
                <<" should not be saved with the scene !" << std::endl;
      return false;
      }
    }
  return true;
}

//-----------------------------------------------------------------------------
bool testScene()
{
  std::cout << "test scene..." << std::endl;
  vtkNew<vtkMRMLScene> scene;

  vtkNew<vtkSlicerUnitsLogic> logic;
  logic->SetMRMLScene(scene.GetPointer());

  // Test scene
  std::vector<vtkMRMLNode*> nodes;
  scene->GetNodesByClass("vtkMRMLUnitNode", nodes);

  return areValidNodes(nodes, "testScene");
}

//-----------------------------------------------------------------------------
bool testSelectionNode()
{
  std::cout << "test selection node..." << std::endl;

  vtkNew<vtkMRMLScene> scene;
  vtkNew<vtkMRMLSelectionNode> selectionNode;
  scene->AddNode(selectionNode.GetPointer());

  vtkNew<vtkSlicerUnitsLogic> logic;
  logic->SetMRMLScene(scene.GetPointer());

  // Test scene
  std::vector<vtkMRMLUnitNode*> nodes;
  selectionNode->GetUnitNodes(nodes);

  return areValidNodes(nodes, "testSelectionNode");
}

//-----------------------------------------------------------------------------
bool testCloseScene()
{
  std::cout << "test close scene..." << std::endl;
  vtkNew<vtkMRMLScene> scene;
  vtkNew<vtkMRMLSelectionNode> selectionNode;
  scene->AddNode(selectionNode.GetPointer());

  vtkNew<vtkSlicerUnitsLogic> logic;
  logic->SetMRMLScene(scene.GetPointer());

  // logic should ensure units stay in the selection node even after a Clear().
  scene->Clear(0);

  std::vector<vtkMRMLUnitNode*> nodes;
  selectionNode->GetUnitNodes(nodes);

  return areValidNodes(nodes, "testCloseScene");
}

//-----------------------------------------------------------------------------
bool testSaveAndReloadScene()
{
  std::cout << "test save&reload scene..." << std::endl;
  vtkNew<vtkMRMLScene> scene;
  vtkNew<vtkMRMLSelectionNode> selectionNode;
  scene->AddNode(selectionNode.GetPointer());

  vtkNew<vtkSlicerUnitsLogic> logic;
  logic->SetMRMLScene(scene.GetPointer());

  std::cout << "  ...commit" << std::endl;
  scene->SetSaveToXMLString(1);
  scene->Commit();
  std::string xmlScene = scene->GetSceneXMLString();

  scene->SetLoadFromXMLString(1);
  scene->SetSceneXMLString(xmlScene);
  scene->Import();

  // Test scene
  std::vector<vtkMRMLNode*> nodes;
  scene->GetNodesByClass("vtkMRMLUnitNode", nodes);
  if (!areValidNodes(nodes, "testSaveAndReloadScene-sceneAfterImport"))
    {
    return false;
    }
  nodes.clear();

  std::vector<vtkMRMLUnitNode*> unitNodes;
  selectionNode->GetUnitNodes(unitNodes);

  if (!areValidNodes(unitNodes, "testSaveAndReloadScene-selectionNodeAfterImport"))
    {
    return false;
    }
  unitNodes.clear();

  // logic should ensure units stay in the selection node even after a Clear().
  std::cout << "  ...clear" << std::endl;
  scene->Clear(0);

  scene->GetNodesByClass("vtkMRMLUnitNode", nodes);
  if (!areValidNodes(nodes, "testSaveAndReloadScene-sceneAfterClear"))
    {
    return false;
    }
  nodes.clear();

  selectionNode->GetUnitNodes(unitNodes);
  return areValidNodes(unitNodes, "testSaveAndReloadScene->selectionNodeAfterClear");
}

//-----------------------------------------------------------------------------
bool testImportScene(const char* sceneFilePath)
{
  std::cout << "test import scene..." << std::endl;
  vtkNew<vtkMRMLScene> scene;
  vtkNew<vtkMRMLSelectionNode> selectionNode;
  scene->AddNode(selectionNode.GetPointer());

  vtkNew<vtkSlicerUnitsLogic> logic;
  logic->SetMRMLScene(scene.GetPointer());

  std::cout << "  ...import" << std::endl;
  scene->SetURL(sceneFilePath);
  scene->Import();

  // Test scene
  std::vector<vtkMRMLNode*> nodes;
  scene->GetNodesByClass("vtkMRMLUnitNode", nodes);
  if (!areValidNodes(nodes, "testImportScene-sceneAfterImport"))
    {
    return false;
    }
  nodes.clear();

  std::vector<vtkMRMLUnitNode*> unitNodes;
  selectionNode->GetUnitNodes(unitNodes);

  if (!areValidNodes(unitNodes, "testImportScene-selectionNodeAfterImport"))
    {
    return false;
    }
  unitNodes.clear();

  // logic should ensure units stay in the selection node even after a Clear().
  std::cout << "  ...clear" << std::endl;
  scene->Clear(0);

  scene->GetNodesByClass("vtkMRMLUnitNode", nodes);
  if (!areValidNodes(nodes, "testImportScene-sceneAfterClear"))
    {
    return false;
    }
  nodes.clear();

  selectionNode->GetUnitNodes(unitNodes);
  return areValidNodes(unitNodes, "testImportScene->selectionNodeAfterClear");
}

} // end namespace
