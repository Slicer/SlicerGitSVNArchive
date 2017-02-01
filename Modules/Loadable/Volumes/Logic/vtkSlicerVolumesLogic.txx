/*==============================================================================

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  Program:   3D Slicer

==============================================================================*/

#ifndef __vtkSlicerVolumesLogic_txx
#define __vtkSlicerVolumesLogic_txx

// MRML includes
#include "vtkMRMLDisplayNode.h"
#include "vtkMRMLVolumeNode.h"

// VTK includes
#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>

//----------------------------------------------------------------------------
template<typename MRMLVolumeNodeType>
MRMLVolumeNodeType*
vtkSlicerVolumesLogic::
CloneVolume (vtkMRMLScene *scene, MRMLVolumeNodeType *volumeNode, const char *name, bool cloneImageData/*=true*/)
{
  if ( scene == NULL || volumeNode == NULL )
    {
    // no valid object is available, so we cannot log error
    return NULL;
    }

  // clone the volume node
  vtkSmartPointer<MRMLVolumeNodeType> clonedVolumeNode;
  clonedVolumeNode.TakeReference((MRMLVolumeNodeType*)scene->CreateNodeByClass(volumeNode->GetClassName()));
  if ( !clonedVolumeNode.GetPointer() )
    {
    vtkErrorWithObjectMacro(volumeNode, "Could not clone volume!");
    return NULL;
    }
  clonedVolumeNode->CopyWithScene(volumeNode);

  // remove storage nodes
  clonedVolumeNode->SetAndObserveStorageNodeID(NULL);

  // remove display nodes (but not the first one)
  while (clonedVolumeNode->GetNumberOfDisplayNodes() > 1)
    {
    clonedVolumeNode->RemoveNthDisplayNodeID(1); // always remove at index 1 since they will shift
    }

  // clone the 1st display node if possible
  vtkMRMLDisplayNode* originalDisplayNode = volumeNode->GetDisplayNode();
  vtkSmartPointer<vtkMRMLDisplayNode> clonedDisplayNode;
  if (originalDisplayNode)
    {
    clonedDisplayNode.TakeReference((vtkMRMLDisplayNode*)scene->CreateNodeByClass(originalDisplayNode->GetClassName()));
    }
  if (clonedDisplayNode.GetPointer())
    {
    clonedDisplayNode->CopyWithScene(originalDisplayNode);
    scene->AddNode(clonedDisplayNode);
    clonedVolumeNode->SetAndObserveDisplayNodeID(clonedDisplayNode->GetID());
    }
  else
    {
    clonedVolumeNode->SetAndObserveDisplayNodeID(NULL);
    }

  // update name
  std::string uname = scene->GetUniqueNameByString(name);
  clonedVolumeNode->SetName(uname.c_str());

  if (cloneImageData)
    {
    // copy over the volume's data
    if (volumeNode->GetImageData())
      {
      vtkNew<vtkImageData> clonedVolumeData;
      clonedVolumeData->DeepCopy(volumeNode->GetImageData());
      clonedVolumeNode->SetAndObserveImageData( clonedVolumeData.GetPointer() );
      }
    else
      {
      vtkErrorWithObjectMacro(scene, "CloneVolume: The ImageData of VolumeNode with ID "
                              << volumeNode->GetID() << " is null !");
      }
    }
  else
    {
    clonedVolumeNode->SetAndObserveImageData(NULL);
    }

  // add the cloned volume to the scene
  scene->AddNode(clonedVolumeNode.GetPointer());

  return clonedVolumeNode.GetPointer();
}

#endif
