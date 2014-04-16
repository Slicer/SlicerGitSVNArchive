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

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// Volumes logic
#include "vtkSlicerVolumesLogic.h"

// MRML includes
#include <vtkMRMLScalarVolumeNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkDataSetAttributes.h>
#include <vtkImageData.h>
#include <vtkNew.h>

// ITK includes
#include <itkConfigure.h>
#include <itkFactoryRegistration.h>

//-----------------------------------------------------------------------------
bool isImageDataValid(vtkImageData* imageData)
{
  if (!imageData)
    {
    return false;
    }
  imageData->GetProducerPort();
  vtkInformation* info = imageData->GetPipelineInformation();
  if (!info)
    {
    return false;
    }
  vtkInformation *scalarInfo = vtkDataObject::GetActiveFieldInformation(info,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
  if (!scalarInfo)
    {
    return false;
    }
  return true;
}

//-----------------------------------------------------------------------------
int vtkSlicerVolumesLogicTest1( int argc, char * argv[] )
{
  itk::itkFactoryRegistration();

  vtkNew<vtkMRMLScene> scene;
  vtkNew<vtkSlicerVolumesLogic> logic;

  if (argc < 2)
    {
    std::cerr << "Usage: vtkSlicerVolumesLogicTest1 volumeName [-I]" << std::endl;
    return EXIT_FAILURE;
    }

  logic->SetMRMLScene(scene.GetPointer());

  vtkMRMLVolumeNode* volume =
    logic->AddArchetypeVolume(argv[1], "volume", 0);

  if (!volume ||
      !isImageDataValid(volume->GetImageData()))
    {
    std::cerr << "Failed to load scalar image." << std::endl;
    return EXIT_FAILURE;
    }

  if (volume && volume->GetImageData())
    {
    volume->GetImageData()->Print(std::cerr);
    }

  return EXIT_SUCCESS;
}
