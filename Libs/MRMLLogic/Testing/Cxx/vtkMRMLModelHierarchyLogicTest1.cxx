/*==============================================================================

  Program: 3D Slicer

  Copyright (c) 2010 Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// MRMLLogic includes
#include "vtkMRMLModelHierarchyLogic.h"

// MRML includes
#include "vtkMRMLModelHierarchyNode.h"
#include "vtkMRMLModelDisplayNode.h"
#include "vtkMRMLModelNode.h"

// VTK includes

// STD includes

#include "TestingMacros.h"

int vtkMRMLModelHierarchyLogicTest1(int , char * [] )
{
  vtkSmartPointer<vtkMRMLScene> scene = vtkSmartPointer<vtkMRMLScene>::New();
  vtkMRMLModelHierarchyLogic* modelHierarchyLogic = vtkMRMLModelHierarchyLogic::New();
  modelHierarchyLogic->SetDebug(1);

  modelHierarchyLogic->SetMRMLScene(scene);

  vtkMRMLModelHierarchyNode *hnode = modelHierarchyLogic->GetModelHierarchyNode(0);
  if (hnode)
    {
    std::cerr << "GetModelHierarchyNode with null id returned a node!";
    return EXIT_FAILURE;
    }

  std::cout << "Number of models in Hierarchy = " << modelHierarchyLogic->GetNumberOfModelsInHierarchy() << std::endl;

  vtkMRMLModelHierarchyNodeList nodeList = modelHierarchyLogic->GetHierarchyChildrenNodes(0);
  if (nodeList.size() != 0)
    {
    std::cerr << "Getting hierarchy children nodes failed on null, returned size of " << nodeList.size() << std::endl;
    return EXIT_FAILURE;
    }
  
  modelHierarchyLogic->HierarchyIsModified();

  std::cout << "Now adding a hierarchy and a node" << std::endl;
  
  // now test with some nodes
  vtkSmartPointer<vtkMRMLModelHierarchyNode> mhnode = vtkSmartPointer<vtkMRMLModelHierarchyNode>::New();
  scene->AddNode(mhnode);

  vtkSmartPointer<vtkMRMLModelDisplayNode> mdnode = vtkSmartPointer<vtkMRMLModelDisplayNode>::New();
  scene->AddNode(mdnode);
  mhnode->SetAndObserveDisplayNodeID(mdnode->GetID());

  vtkSmartPointer<vtkMRMLModelNode> modelNode = vtkSmartPointer<vtkMRMLModelNode>::New();
  scene->AddNode(modelNode);
  mhnode->SetModelNodeID(modelNode->GetID());

  int numModels = modelHierarchyLogic->GetNumberOfModelsInHierarchy();
  std::cout << "Number of models in Hierarchy after adding a model hierarchy = " << numModels << std::endl;
  if (numModels != 1)
    {
    std::cerr << "Have " << numModels << ", should be 1" << std::endl;
    return EXIT_FAILURE;
    }

  hnode = modelHierarchyLogic->GetModelHierarchyNode(modelNode->GetID());
  if (!hnode)
    {
    std::cerr << "Error getting hierarchy node for model " << modelNode->GetID() << std::endl;
    return EXIT_FAILURE;
    }

  nodeList = modelHierarchyLogic->GetHierarchyChildrenNodes(hnode);
  int hNumChildrenNodes = hnode->GetNumberOfChildrenNodes();
  if (nodeList.size() != static_cast<unsigned int>(hNumChildrenNodes))
    {
    std::cerr << "Getting hierarchy children nodes failed on " << hnode->GetID() << ", returned size of " << nodeList.size() << " instead of " << hNumChildrenNodes << ", as the hierarchy node reports" << std::endl;
    return EXIT_FAILURE;
    }
  
  modelHierarchyLogic->Delete();

  return EXIT_SUCCESS;
}

