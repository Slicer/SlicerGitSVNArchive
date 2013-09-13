/*=auto=========================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Yuzheng Zhou, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

=========================================================================auto=*/

// MRML includes
#include "vtkMRMLCoreTestingMacros.h"
#include "vtkMRMLLinearTransformNode.h"
#include "vtkMRMLTransformableNode.h"
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkGeneralTransform.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>

namespace
{

void populateScene(vtkMRMLScene* scene);

} // end of anonymous namespace

//---------------------------------------------------------------------------
int vtkMRMLTransformableNodeOnNodeReferenceAddTest(int , char * [] )
{
  // Save a scene containing a viewnode and a sceneview node.
  vtkNew<vtkMRMLScene> scene;
  populateScene(scene.GetPointer());

  scene->SetSaveToXMLString(1);
  scene->Commit();
  std::string xmlScene = scene->GetSceneXMLString();
  std::cout << xmlScene << std::endl;

  // Load the saved scene
  vtkNew<vtkMRMLScene> scene2;

  scene2->SetLoadFromXMLString(1);
  scene2->SetSceneXMLString(xmlScene);
  scene2->Import();

  // Check transform node IDs
  vtkMRMLNode* transformNode =
    scene2->GetNthNodeByClass(0, "vtkMRMLLinearTransformNode");
  if (!transformNode || strcmp(transformNode->GetID(), "vtkMRMLLinearTransformNode1") != 0)
    {
    std::cerr << __LINE__ << ": import failed." << std::endl
              << "    transform node ID is " << transformNode->GetID()
              << " instead of vtkMRMLLinearTransformNode1." << std::endl;

    return EXIT_FAILURE;
    }

  // Check references
  vtkMRMLTransformableNode* trnsformableNode =
    vtkMRMLTransformableNode::SafeDownCast(scene2->GetNthNodeByClass(0, "vtkMRMLTransformableNode"));

  if (strcmp(trnsformableNode->GetTransformNodeID(),
             transformNode->GetID()) != 0)
    {
    std::cerr << __LINE__ << ": import failed." << std::endl
              << "    Transformable node references are not updated. "
              << "Transform node ID reference is "
              << trnsformableNode->GetTransformNodeID()
              << " instead of " << transformNode->GetID() << std::endl;
    return EXIT_FAILURE;
    }

  // Test vtkMRMLTransformableNode::OnNodeReferenceAdded()
  vtkMRMLLinearTransformNode* linearTransformNode = vtkMRMLLinearTransformNode::SafeDownCast(scene2->GetNthNodeByClass(0, "vtkMRMLLinearTransformNode"));
  vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(scene2->GetNthNodeByClass(0, "vtkMRMLScalarVolumeNode"));
  vtkNew<vtkMRMLNodeCallback> callback;
  volumeNode->AddObserver(vtkCommand::AnyEvent, callback.GetPointer());

  linearTransformNode->GetMatrixTransformToParent()->Modified();
  if (!callback->GetErrorString().empty() ||
      callback->GetNumberOfModified() != 0 ||
      callback->GetNumberOfEvents(vtkMRMLTransformNode::TransformModifiedEvent) != 1)
    {
    std::cerr << "vtkMRMLTransformableNode::OnNodeReferenceAdded failed."
              << callback->GetErrorString().c_str() << " "
              << "Number of ModifiedEvent: " << callback->GetNumberOfModified() << " "
              << "Number of TransformModifiedEvent: "
              << callback->GetNumberOfEvents(vtkMRMLTransformNode::TransformModifiedEvent)
              << std::endl;
    return EXIT_FAILURE;
    }
  callback->ResetNumberOfEvents();
  return EXIT_SUCCESS;
}

namespace
{

//---------------------------------------------------------------------------
void populateScene(vtkMRMLScene* scene)
{

  vtkNew<vtkMRMLScalarVolumeNode> transformableableNode;
  scene->AddNode(transformableableNode.GetPointer());

  vtkNew<vtkMRMLLinearTransformNode> transformNode;
  scene->AddNode(transformNode.GetPointer());

  transformableableNode->SetAndObserveTransformNodeID(transformNode->GetID());

}


}
