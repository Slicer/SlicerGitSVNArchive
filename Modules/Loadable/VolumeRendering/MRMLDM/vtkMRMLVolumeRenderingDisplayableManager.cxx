/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care
  and CANARIE.

==============================================================================*/

// Volume Rendering includes
#include "vtkMRMLVolumeRenderingDisplayableManager.h"

#include "vtkSlicerVolumeRenderingLogic.h"
#include "vtkMRMLCPURayCastVolumeRenderingDisplayNode.h"
#include "vtkMRMLGPURayCastVolumeRenderingDisplayNode.h"

// MRML includes
#include "vtkMRMLAnnotationROINode.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLTransformNode.h"
#include "vtkMRMLViewNode.h"
#include "vtkMRMLVolumePropertyNode.h"
#include "vtkEventBroker.h"

// VTK includes
#include <vtkCallbackCommand.h>
#include "vtkFixedPointVolumeRayCastMapper.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkInteractorStyle.h"
#include "vtkMatrix4x4.h"
#include <vtkNew.h>
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPlanes.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkDoubleArray.h"

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkMRMLVolumeRenderingDisplayableManager);

//---------------------------------------------------------------------------
int vtkMRMLVolumeRenderingDisplayableManager::DefaultGPUMemorySize = 256;

//---------------------------------------------------------------------------
class vtkMRMLVolumeRenderingDisplayableManager::vtkInternal
{
public:
  vtkInternal(vtkMRMLVolumeRenderingDisplayableManager* external);
  ~vtkInternal();

  class Pipeline
  {
  public:
    Pipeline()
      {
      this->VolumeActor = vtkSmartPointer<vtkVolume>::New();
      this->IJKToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
      this->RayCastMapperCPU = vtkSmartPointer<vtkFixedPointVolumeRayCastMapper>::New();
      this->RayCastMapperGPU = vtkSmartPointer<vtkGPUVolumeRayCastMapper>::New();
      }

    vtkSmartPointer<vtkVolume> VolumeActor;
    vtkSmartPointer<vtkMatrix4x4> IJKToWorldMatrix;
    vtkVolumeMapper* GetVolumeMapper(vtkMRMLVolumeRenderingDisplayNode* displayNode)const
    {
      if (!displayNode)
        {
        return NULL;
        }
      if (displayNode->IsA("vtkMRMLCPURayCastVolumeRenderingDisplayNode"))
        {
        return this->RayCastMapperCPU;
        }
      else if (displayNode->IsA("vtkMRMLGPURayCastVolumeRenderingDisplayNode"))
        {
        return this->RayCastMapperGPU;
        }
      return NULL;
    };

  private:
    vtkSmartPointer<vtkFixedPointVolumeRayCastMapper> RayCastMapperCPU;
    vtkSmartPointer<vtkGPUVolumeRayCastMapper> RayCastMapperGPU;
  };

  typedef std::map < vtkMRMLVolumeRenderingDisplayNode*, const Pipeline* > PipelinesCacheType;
  PipelinesCacheType DisplayPipelines;

  typedef std::map < vtkMRMLVolumeNode*, std::set< vtkMRMLVolumeRenderingDisplayNode* > > VolumeToDisplayCacheType;
  VolumeToDisplayCacheType VolumeToDisplayNodes;

  // Volumes
  void AddVolumeNode(vtkMRMLVolumeNode* displayableNode);
  void RemoveVolumeNode(vtkMRMLVolumeNode* displayableNode);

  // Transforms
  void UpdatePipelineTransforms(vtkMRMLVolumeNode *node);
  bool GetVolumeTransformToWorld(vtkMRMLVolumeNode* node, vtkMatrix4x4* ijkToWorldMatrix);

  // ROIs
  void UpdatePipelineROIs(vtkMRMLVolumeRenderingDisplayNode* displayNode, const Pipeline* pipeline);

  // Display Nodes
  void AddDisplayNode(vtkMRMLVolumeNode*, vtkMRMLVolumeRenderingDisplayNode*);
  void RemoveDisplayNode(vtkMRMLVolumeRenderingDisplayNode* displayNode);
  void UpdateDisplayNode(vtkMRMLVolumeRenderingDisplayNode* displayNode);
  void UpdateDisplayNodePipeline(vtkMRMLVolumeRenderingDisplayNode* displayNode, const Pipeline* pipeline);

  double GetSampleDistance(vtkMRMLVolumeRenderingDisplayNode* displayNode);
  double GetFramerate(vtkMRMLVolumeRenderingDisplayNode* displayNode);
  vtkIdType GetMaxMemoryInBytes(vtkMRMLVolumeRenderingDisplayNode* displayNode);
  void UpdateDesiredUpdateRate(vtkMRMLVolumeRenderingDisplayNode* displayNode);

  // Observations
  void AddObservations(vtkMRMLVolumeNode* node);
  void RemoveObservations(vtkMRMLVolumeNode* node);
  bool IsNodeObserved(vtkMRMLVolumeNode* node);

  // Helper functions
  bool IsVisible(vtkMRMLVolumeRenderingDisplayNode* displayNode);
  bool UseDisplayNode(vtkMRMLVolumeRenderingDisplayNode* displayNode);
  bool UseDisplayableNode(vtkMRMLVolumeNode* node);
  void ClearDisplayableNodes();

public:
  vtkMRMLVolumeRenderingDisplayableManager* External;

  /// Flag indicating whether adding volume node is in progress
  bool AddingVolumeNode;

  /// Original desired update rate of renderer. Restored when volume hidden
  double OriginalDesiredUpdateRate;

  /// Observed events for the display nodes
  vtkIntArray* DisplayObservedEvents;

  /// When interaction is >0, we are in interactive mode (low level of detail)
  int Interaction;
};

//---------------------------------------------------------------------------
// vtkInternal methods

//---------------------------------------------------------------------------
vtkMRMLVolumeRenderingDisplayableManager::vtkInternal::vtkInternal(vtkMRMLVolumeRenderingDisplayableManager* external)
: External(external)
, AddingVolumeNode(false)
, OriginalDesiredUpdateRate(0.0) // 0 fps is a special value that means it hasn't been set
, Interaction(0)
{
  this->DisplayObservedEvents = vtkIntArray::New();
  this->DisplayObservedEvents->InsertNextValue(vtkCommand::StartEvent);
  this->DisplayObservedEvents->InsertNextValue(vtkCommand::EndEvent);
  this->DisplayObservedEvents->InsertNextValue(vtkCommand::ModifiedEvent);
  this->DisplayObservedEvents->InsertNextValue(vtkCommand::StartInteractionEvent);
  this->DisplayObservedEvents->InsertNextValue(vtkCommand::InteractionEvent);
  this->DisplayObservedEvents->InsertNextValue(vtkCommand::EndInteractionEvent);
}

//---------------------------------------------------------------------------
vtkMRMLVolumeRenderingDisplayableManager::vtkInternal::~vtkInternal()
{
  this->ClearDisplayableNodes();

  if (this->DisplayObservedEvents)
    {
    this->DisplayObservedEvents->Delete();
    this->DisplayObservedEvents = NULL;
    }
}

//---------------------------------------------------------------------------
bool vtkMRMLVolumeRenderingDisplayableManager::vtkInternal::UseDisplayNode(vtkMRMLVolumeRenderingDisplayNode* displayNode)
{
  // Allow volumes to appear only in designated viewers
  if (displayNode && !displayNode->IsDisplayableInView(this->External->GetMRMLViewNode()->GetID()))
    {
    return false;
    }

  // Check whether display node can be shown in this view
  vtkMRMLVolumeRenderingDisplayNode* volRenDispNode = vtkMRMLVolumeRenderingDisplayNode::SafeDownCast(displayNode);
  if ( !volRenDispNode
    || !volRenDispNode->GetVolumeNodeID()
    || !volRenDispNode->GetROINodeID()
    || !volRenDispNode->GetVolumePropertyNodeID() )
    {
    return false;
    }

  return true;
}

//---------------------------------------------------------------------------
bool vtkMRMLVolumeRenderingDisplayableManager::vtkInternal::IsVisible(vtkMRMLVolumeRenderingDisplayNode* displayNode)
{
  return displayNode && displayNode->GetVisibility();
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::vtkInternal::AddVolumeNode(vtkMRMLVolumeNode* node)
{
  if (this->AddingVolumeNode)
    {
    return;
    }
  // Check if node should be used
  if (!this->UseDisplayableNode(node))
    {
    return;
    }

  this->AddingVolumeNode = true;

  // Add Display Nodes
  int numDisplayNodes = node->GetNumberOfDisplayNodes();

  this->AddObservations(node);

  for (int i=0; i<numDisplayNodes; i++)
    {
    vtkMRMLVolumeRenderingDisplayNode *displayNode = vtkMRMLVolumeRenderingDisplayNode::SafeDownCast(node->GetNthDisplayNode(i));
    if (this->UseDisplayNode(displayNode))
      {
      this->VolumeToDisplayNodes[node].insert(displayNode);
      this->AddDisplayNode(node, displayNode);
      }
    }
  this->AddingVolumeNode = false;
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::vtkInternal::RemoveVolumeNode(vtkMRMLVolumeNode* node)
{
  if (!node)
    {
    return;
    }
  vtkInternal::VolumeToDisplayCacheType::iterator displayableIt = this->VolumeToDisplayNodes.find(node);
  if (displayableIt == this->VolumeToDisplayNodes.end())
    {
    return;
    }

  std::set<vtkMRMLVolumeRenderingDisplayNode *> dnodes = displayableIt->second;
  std::set<vtkMRMLVolumeRenderingDisplayNode *>::iterator diter;
  for (diter = dnodes.begin(); diter != dnodes.end(); ++diter)
    {
    this->RemoveDisplayNode(*diter);
    }
  this->RemoveObservations(node);
  this->VolumeToDisplayNodes.erase(displayableIt);
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::vtkInternal::AddDisplayNode(vtkMRMLVolumeNode* mNode, vtkMRMLVolumeRenderingDisplayNode* displayNode)
{
  if (!mNode || !displayNode)
    {
    return;
    }

  // Do not add the display node if it is already associated with a pipeline object.
  // This happens when a segmentation node already associated with a display node
  // is copied into an other (using vtkMRMLNode::Copy()) and is added to the scene afterward.
  // Related issue are #3428 and #2608
  PipelinesCacheType::iterator it = this->DisplayPipelines.find(displayNode);
  if (it != this->DisplayPipelines.end())
    {
    return;
    }

  // Create pipeline for volume
  Pipeline* pipeline = new Pipeline();
  // Add actor to Renderer and local cache
  this->External->GetRenderer()->AddVolume(pipeline->VolumeActor);

  this->DisplayPipelines.insert( std::make_pair(displayNode, pipeline) );

  this->External->GetMRMLNodesObserverManager()->AddObjectEvents(displayNode, this->DisplayObservedEvents);

  // Update cached matrix. Calls UpdateDisplayNodePipeline
  this->UpdatePipelineTransforms(mNode);
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::vtkInternal::RemoveDisplayNode(vtkMRMLVolumeRenderingDisplayNode* displayNode)
{
  PipelinesCacheType::iterator pipelineIt = this->DisplayPipelines.find(displayNode);
  if (pipelineIt == this->DisplayPipelines.end())
    {
    return;
    }

  const Pipeline* pipeline = pipelineIt->second;
  this->External->GetRenderer()->RemoveVolume(pipeline->VolumeActor);
  delete pipeline;
  this->DisplayPipelines.erase(pipelineIt);
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::vtkInternal::UpdatePipelineTransforms(vtkMRMLVolumeNode* mNode)
{
  // Update the pipeline for all tracked DisplayableNode
  PipelinesCacheType::iterator pipelineIt;
  std::set< vtkMRMLVolumeRenderingDisplayNode* > displayNodes = this->VolumeToDisplayNodes[mNode];
  std::set< vtkMRMLVolumeRenderingDisplayNode* >::iterator displayNodeIt;
  for (displayNodeIt = displayNodes.begin(); displayNodeIt != displayNodes.end(); displayNodeIt++)
    {
    if (((pipelineIt = this->DisplayPipelines.find(*displayNodeIt)) != this->DisplayPipelines.end()))
      {
      vtkMRMLVolumeRenderingDisplayNode* currentDisplayNode = pipelineIt->first;
      const Pipeline* currentPipeline = pipelineIt->second;
      this->UpdateDisplayNodePipeline(currentDisplayNode, currentPipeline);

      // Calculate and apply transform matrix
      this->GetVolumeTransformToWorld(mNode, currentPipeline->IJKToWorldMatrix);
      currentPipeline->VolumeActor->SetUserMatrix(currentPipeline->IJKToWorldMatrix.GetPointer());
      }
    }
}

//---------------------------------------------------------------------------
bool vtkMRMLVolumeRenderingDisplayableManager::vtkInternal::GetVolumeTransformToWorld(
  vtkMRMLVolumeNode* volumeNode, vtkMatrix4x4* outputIjkToWorldMatrix)
{
  if (volumeNode == NULL)
    {
    vtkErrorWithObjectMacro(this->External, "GetVolumeTransformToWorld: Invalid volume node");
    return false;
    }

  // Check if we have a transform node
  vtkMRMLTransformNode* transformNode = volumeNode->GetParentTransformNode();
  if (transformNode == NULL)
    {
    volumeNode->GetIJKToRASMatrix(outputIjkToWorldMatrix);
    return true;
    }

  // IJK to RAS
  vtkMatrix4x4* ijkToRasMatrix = vtkMatrix4x4::New();
  volumeNode->GetIJKToRASMatrix(ijkToRasMatrix);

  // Parent transforms
  vtkMatrix4x4* nodeToWorldMatrix = vtkMatrix4x4::New();
  int success = transformNode->GetMatrixTransformToWorld(nodeToWorldMatrix);
  if (!success)
    {
    vtkWarningWithObjectMacro(this->External, "GetVolumeTransformToWorld: Non-linear parent transform found for volume node " << volumeNode->GetName());
    outputIjkToWorldMatrix->Identity();
    return false;
    }

  // Transform world to RAS
  vtkMatrix4x4::Multiply4x4(nodeToWorldMatrix, ijkToRasMatrix, outputIjkToWorldMatrix);
  outputIjkToWorldMatrix->Modified(); // Needed because Multiply4x4 does not invoke Modified

  ijkToRasMatrix->Delete();
  nodeToWorldMatrix->Delete();
  return true;
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::vtkInternal::UpdateDisplayNode(vtkMRMLVolumeRenderingDisplayNode* displayNode)
{
  // If the display node already exists, just update. Otherwise, add as new node
  if (!displayNode)
    {
    return;
    }
  PipelinesCacheType::iterator displayNodeIt;
  displayNodeIt = this->DisplayPipelines.find(displayNode);
  if (displayNodeIt != this->DisplayPipelines.end())
    {
    this->UpdateDisplayNodePipeline(displayNode, displayNodeIt->second);
    }
  else
    {
    this->AddVolumeNode( vtkMRMLVolumeNode::SafeDownCast(displayNode->GetDisplayableNode()) );
    }
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::vtkInternal::UpdateDisplayNodePipeline(
  vtkMRMLVolumeRenderingDisplayNode* displayNode, const Pipeline* pipeline)
{
  if (!displayNode || !pipeline)
    {
    vtkErrorWithObjectMacro(this->External, "UpdateDisplayNodePipeline: Display node or pipeline is invalid");
    return;
    }
  bool displayNodeVisible = displayNode->GetVisibility() && displayNode->GetOpacity() > 0
    && displayNode->GetVisibility(this->External->GetMRMLViewNode()->GetID());

  // Get volume node
  vtkMRMLVolumeNode* volumeNode = displayNode ? displayNode->GetVolumeNode() : NULL;
  if (!volumeNode)
    {
    return;
    }

  // Set volume visibility, return if hidden
  pipeline->VolumeActor->SetVisibility(displayNodeVisible);
  if (!displayNodeVisible)
    {
    return;
    }

  // Get generic volume mapper
  vtkVolumeMapper* mapper = pipeline->GetVolumeMapper(displayNode);
  if (!mapper)
    {
    vtkErrorWithObjectMacro(this->External, "UpdateDisplayNodePipeline: Unable to get volume mapper");
    return;
    }

  // Update specific volume mapper
  if (displayNode->IsA("vtkMRMLCPURayCastVolumeRenderingDisplayNode"))
    {
    vtkFixedPointVolumeRayCastMapper* cpuMapper = vtkFixedPointVolumeRayCastMapper::SafeDownCast(mapper);

    const bool maximumQuality = displayNode->GetPerformanceControl() == vtkMRMLVolumeRenderingDisplayNode::MaximumQuality;
    cpuMapper->SetAutoAdjustSampleDistances(maximumQuality ? 0 : 1);
    double sampleDistance = this->GetSampleDistance(displayNode);
    cpuMapper->SetSampleDistance(sampleDistance);
    cpuMapper->SetInteractiveSampleDistance(sampleDistance);
    cpuMapper->SetImageSampleDistance(maximumQuality ? 0.5 : 1.);
    }
  else if (displayNode->IsA("vtkMRMLGPURayCastVolumeRenderingDisplayNode"))
    {
    vtkMRMLGPURayCastVolumeRenderingDisplayNode* gpuDisplayNode =
      vtkMRMLGPURayCastVolumeRenderingDisplayNode::SafeDownCast(displayNode);
    vtkGPUVolumeRayCastMapper* gpuMapper = vtkGPUVolumeRayCastMapper::SafeDownCast(mapper);

    const bool maximumQuality = displayNode->GetPerformanceControl() == vtkMRMLVolumeRenderingDisplayNode::MaximumQuality;
    if (maximumQuality)
      {
      gpuMapper->SetAutoAdjustSampleDistances(0);
      gpuMapper->SetLockSampleDistanceToInputSpacing(0);
      gpuMapper->SetUseJittering(0);
      }
    else
      {
      const bool lockSampleDistance = (bool)gpuDisplayNode->GetLockSampleDistanceToInputSpacing();
      // AutoAdjustSampleDistances disables LockSampleDistanceToInputSpacing, so if
      // LockSampleDistanceToInputSpacing is on then disable AutoAdjustSampleDistances
      gpuMapper->SetAutoAdjustSampleDistances(maximumQuality || lockSampleDistance ? 0 : 1);
      gpuMapper->SetLockSampleDistanceToInputSpacing(lockSampleDistance);
      gpuMapper->SetUseJittering(gpuDisplayNode->GetUseJittering());
      }
    gpuMapper->SetSampleDistance(this->GetSampleDistance(gpuDisplayNode));
    gpuMapper->SetImageSampleDistance(1.0);
    gpuMapper->SetMaxMemoryInBytes(this->GetMaxMemoryInBytes(gpuDisplayNode));
    }
  else
    {
    vtkErrorWithObjectMacro(this->External, "UpdateDisplayNodePipeline: Display node type " << displayNode->GetNodeTagName() << " is not supported");
    return;
    }

  // Set raycast technique
  switch (displayNode->GetRaycastTechnique())
    {
    case vtkMRMLVolumeRenderingDisplayNode::MaximumIntensityProjection:
      mapper->SetBlendMode(vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND);
      break;
    case vtkMRMLVolumeRenderingDisplayNode::MinimumIntensityProjection:
      mapper->SetBlendMode(vtkVolumeMapper::MINIMUM_INTENSITY_BLEND);
      break;
    case vtkMRMLVolumeRenderingDisplayNode::Composite:
    default:
      mapper->SetBlendMode(vtkVolumeMapper::COMPOSITE_BLEND);
      break;
    }

  // Update ROI clipping planes
  this->UpdatePipelineROIs(displayNode, pipeline);

  // Make sure the correct mapper is set to the volume
  pipeline->VolumeActor->SetMapper(mapper);
  // Make sure the correct volume is set to the mapper
  mapper->SetInputConnection(0, volumeNode->GetImageDataConnection());

  // Set volume property
  vtkVolumeProperty* volumeProperty = displayNode->GetVolumePropertyNode() ? displayNode->GetVolumePropertyNode()->GetVolumeProperty() : NULL;
  pipeline->VolumeActor->SetProperty(volumeProperty);

  this->UpdateDesiredUpdateRate(displayNode);
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::vtkInternal::UpdatePipelineROIs(
  vtkMRMLVolumeRenderingDisplayNode* displayNode, const Pipeline* pipeline)
{
  if (!pipeline)
    {
    return;
    }
  vtkVolumeMapper* volumeMapper = pipeline->GetVolumeMapper(displayNode);
  if (!volumeMapper)
    {
    vtkErrorWithObjectMacro(this->External, "UpdatePipelineROIs: Unable to get volume mapper");
    return;
    }
  if (!displayNode || displayNode->GetROINode() == NULL || !displayNode->GetCroppingEnabled())
    {
    volumeMapper->RemoveAllClippingPlanes();
    return;
    }

  vtkNew<vtkPlanes> planes;
  displayNode->GetROINode()->GetTransformedPlanes(planes);
  //TODO: Workaround for bug that broke cropping: normals were inverted, need to invert them
  //      (the reason is not the use of SetUserMatrix instead of PokeMatrix when setting the transform to the mapper)
  vtkDoubleArray* normals = vtkDoubleArray::SafeDownCast(planes->GetNormals());
  for (int i=0; i<normals->GetNumberOfValues(); ++i)
    {
    normals->SetValue(i, (-1.0)*normals->GetValue(i));
    }
  volumeMapper->SetClippingPlanes(planes);
}

//---------------------------------------------------------------------------
double vtkMRMLVolumeRenderingDisplayableManager::vtkInternal::GetSampleDistance(
  vtkMRMLVolumeRenderingDisplayNode* displayNode)
{
  vtkMRMLVolumeNode* volumeNode = displayNode ? displayNode->GetVolumeNode() : NULL;
  if (!volumeNode)
    {
    vtkErrorWithObjectMacro(this->External, "GetSampleDistance: Failed to access volume node");
    return displayNode->GetEstimatedSampleDistance();
    }
  const double minSpacing = volumeNode->GetMinSpacing() > 0 ? volumeNode->GetMinSpacing() : 1.;
  double sampleDistance = minSpacing / displayNode->GetEstimatedSampleDistance();
  if (displayNode->GetPerformanceControl() == vtkMRMLVolumeRenderingDisplayNode::MaximumQuality)
    {
    sampleDistance = minSpacing / 10.; // =10x smaller than pixel is high quality
    }
  return sampleDistance;
}

//---------------------------------------------------------------------------
double vtkMRMLVolumeRenderingDisplayableManager::vtkInternal::GetFramerate(vtkMRMLVolumeRenderingDisplayNode* displayNode)
{
  double framerate = displayNode ? displayNode->GetExpectedFPS() : 15.;
  framerate = std::max(framerate, 0.0001);
  if (displayNode->GetPerformanceControl() == vtkMRMLVolumeRenderingDisplayNode::MaximumQuality)
    {
    framerate = 0.0; // special value meaning full quality
    }
  return framerate;
}

//---------------------------------------------------------------------------
vtkIdType vtkMRMLVolumeRenderingDisplayableManager::vtkInternal::GetMaxMemoryInBytes(
  vtkMRMLVolumeRenderingDisplayNode* displayNode)
{
  int gpuMemorySizeMB = vtkMRMLVolumeRenderingDisplayableManager::DefaultGPUMemorySize;
  if (displayNode && displayNode->GetGPUMemorySize() > 0)
    {
    gpuMemorySizeMB = displayNode->GetGPUMemorySize();
    }

  // Special case: for GPU volume raycast mapper, round up to nearest 128MB
  if (displayNode->IsA("vtkMRMLGPURayCastVolumeRenderingDisplayNode"))
    {
    if (gpuMemorySizeMB < 128)
      {
      gpuMemorySizeMB = 128;
      }
    else
      {
      gpuMemorySizeMB = ((gpuMemorySizeMB - 1) / 128 + 1) * 128;
      }
    }

  vtkIdType gpuMemorySizeB = vtkIdType(gpuMemorySizeMB) * vtkIdType(1024 * 1024);
  return gpuMemorySizeB;
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::vtkInternal::UpdateDesiredUpdateRate(vtkMRMLVolumeRenderingDisplayNode* displayNode)
{
  vtkRenderWindow* renderWindow = this->External->GetRenderer()->GetRenderWindow();
  vtkRenderWindowInteractor* renderWindowInteractor = renderWindow ? renderWindow->GetInteractor() : 0;
  if (!renderWindowInteractor)
    {
    return;
    }
  double fps = this->GetFramerate(displayNode);
  if (displayNode->GetVisibility())
    {
    if (this->OriginalDesiredUpdateRate == 0.0)
      {
      // Save the DesiredUpdateRate before it is changed.
      // It will then be restored when the volume rendering is hidden
      this->OriginalDesiredUpdateRate = renderWindowInteractor->GetDesiredUpdateRate();
      }
    renderWindowInteractor->SetDesiredUpdateRate(fps);
    }
  else if (this->OriginalDesiredUpdateRate != 0.0)
    {
    // Restore the DesiredUpdateRate to its original value.
    renderWindowInteractor->SetDesiredUpdateRate(this->OriginalDesiredUpdateRate);
    this->OriginalDesiredUpdateRate = 0.0;
    }
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::vtkInternal::AddObservations(vtkMRMLVolumeNode* node)
{
  vtkEventBroker* broker = vtkEventBroker::GetInstance();
  if (!broker->GetObservationExist(node, vtkCommand::ModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand()))
    {
    broker->AddObservation(node, vtkCommand::ModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand());
    }
  if (!broker->GetObservationExist(node, vtkMRMLDisplayableNode::TransformModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand()))
    {
    broker->AddObservation(node, vtkMRMLDisplayableNode::TransformModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() );
    }
  if (!broker->GetObservationExist(node, vtkMRMLDisplayableNode::DisplayModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() ))
    {
    broker->AddObservation(node, vtkMRMLDisplayableNode::DisplayModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() );
    }
  if (!broker->GetObservationExist(node, vtkMRMLVolumeNode::ImageDataModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() ))
    {
    broker->AddObservation(node, vtkMRMLVolumeNode::ImageDataModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() );
    }
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::vtkInternal::RemoveObservations(vtkMRMLVolumeNode* node)
{
  vtkEventBroker* broker = vtkEventBroker::GetInstance();
  vtkEventBroker::ObservationVector observations;
  observations = broker->GetObservations(node, vtkCommand::ModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand());
  broker->RemoveObservations(observations);
  observations = broker->GetObservations(node, vtkMRMLTransformableNode::TransformModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand());
  broker->RemoveObservations(observations);
  observations = broker->GetObservations(node, vtkMRMLDisplayableNode::DisplayModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() );
  broker->RemoveObservations(observations);
  observations = broker->GetObservations(node, vtkMRMLVolumeNode::ImageDataModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() );
  broker->RemoveObservations(observations);
}

//---------------------------------------------------------------------------
bool vtkMRMLVolumeRenderingDisplayableManager::vtkInternal::IsNodeObserved(vtkMRMLVolumeNode* node)
{
  vtkEventBroker* broker = vtkEventBroker::GetInstance();
  vtkCollection* observations = broker->GetObservationsForSubject(node);
  if (observations->GetNumberOfItems() > 0)
    {
    return true;
    }
  else
    {
    return false;
    }
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::vtkInternal::ClearDisplayableNodes()
{
  while (this->VolumeToDisplayNodes.size() > 0)
    {
    this->RemoveVolumeNode(this->VolumeToDisplayNodes.begin()->first);
    }
}

//---------------------------------------------------------------------------
bool vtkMRMLVolumeRenderingDisplayableManager::vtkInternal::UseDisplayableNode(vtkMRMLVolumeNode* node)
{
  bool use = node && node->IsA("vtkMRMLVolumeNode");
  return use;
}


//---------------------------------------------------------------------------
// vtkMRMLVolumeRenderingDisplayableManager methods

//---------------------------------------------------------------------------
vtkMRMLVolumeRenderingDisplayableManager::vtkMRMLVolumeRenderingDisplayableManager()
{
  this->Internal = new vtkInternal(this);

  this->RemoveInteractorStyleObservableEvent(vtkCommand::LeftButtonPressEvent);
  this->RemoveInteractorStyleObservableEvent(vtkCommand::LeftButtonReleaseEvent);
  this->RemoveInteractorStyleObservableEvent(vtkCommand::RightButtonPressEvent);
  this->RemoveInteractorStyleObservableEvent(vtkCommand::RightButtonReleaseEvent);
  this->RemoveInteractorStyleObservableEvent(vtkCommand::MiddleButtonPressEvent);
  this->RemoveInteractorStyleObservableEvent(vtkCommand::MiddleButtonReleaseEvent);
  this->RemoveInteractorStyleObservableEvent(vtkCommand::MouseWheelBackwardEvent);
  this->RemoveInteractorStyleObservableEvent(vtkCommand::MouseWheelForwardEvent);
  this->RemoveInteractorStyleObservableEvent(vtkCommand::EnterEvent);
  this->RemoveInteractorStyleObservableEvent(vtkCommand::LeaveEvent);
  this->AddInteractorStyleObservableEvent(vtkCommand::StartInteractionEvent);
  this->AddInteractorStyleObservableEvent(vtkCommand::EndInteractionEvent);
}

//---------------------------------------------------------------------------
vtkMRMLVolumeRenderingDisplayableManager::~vtkMRMLVolumeRenderingDisplayableManager()
{
  delete this->Internal;
  this->Internal=NULL;
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::PrintSelf( ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf ( os, indent );
  os << indent << "vtkMRMLVolumeRenderingDisplayableManager: " << this->GetClassName() << "\n";
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::Create()
{
  Superclass::Create();
  this->ObserveGraphicalResourcesCreatedEvent();
  this->SetUpdateFromMRMLRequested(1);
}

//----------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::ObserveGraphicalResourcesCreatedEvent()
{
  vtkMRMLViewNode* viewNode = this->GetMRMLViewNode();
  if (viewNode == NULL)
    {
    vtkErrorMacro("OnCreate: Failed to access view node");
    return;
    }
  if (!vtkIsObservedMRMLNodeEventMacro(viewNode, vtkMRMLViewNode::GraphicalResourcesCreatedEvent))
    {
    vtkNew<vtkIntArray> events;
    events->InsertNextValue(vtkMRMLViewNode::GraphicalResourcesCreatedEvent);
    vtkObserveMRMLNodeEventsMacro(viewNode, events.GetPointer());
    }
}

//---------------------------------------------------------------------------
int vtkMRMLVolumeRenderingDisplayableManager::ActiveInteractionModes()
{
  // Observe all the modes
  return ~0;
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::UnobserveMRMLScene()
{
  this->Internal->ClearDisplayableNodes();
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::OnMRMLSceneStartClose()
{
  this->Internal->ClearDisplayableNodes();
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::OnMRMLSceneEndClose()
{
  this->SetUpdateFromMRMLRequested(1);
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::OnMRMLSceneEndBatchProcess()
{
  this->SetUpdateFromMRMLRequested(1);
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::OnMRMLSceneEndImport()
{
  // UpdateFromMRML will be executed only if there has been some actions
  // during the import that requested it (don't call
  // SetUpdateFromMRMLRequested(1) here, it should be done somewhere else
  // maybe in OnMRMLSceneNodeAddedEvent, OnMRMLSceneNodeRemovedEvent or
  // OnMRMLDisplayableModelNodeModifiedEvent).
  this->ObserveGraphicalResourcesCreatedEvent();
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::OnMRMLSceneEndRestore()
{
  // UpdateFromMRML will be executed only if there has been some actions
  // during the restoration that requested it (don't call
  // SetUpdateFromMRMLRequested(1) here, it should be done somewhere else
  // maybe in OnMRMLSceneNodeAddedEvent, OnMRMLSceneNodeRemovedEvent or
  // OnMRMLDisplayableModelNodeModifiedEvent).
  this->ObserveGraphicalResourcesCreatedEvent();
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if ( !node->IsA("vtkMRMLVolumeNode") )
    {
    return;
    }

  // Escape if the scene is being closed, imported or connected
  if (this->GetMRMLScene()->IsBatchProcessing())
    {
    this->SetUpdateFromMRMLRequested(1);
    return;
    }

  this->Internal->AddVolumeNode(vtkMRMLVolumeNode::SafeDownCast(node));
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if ( node
    && (!node->IsA("vtkMRMLVolumeNode"))
    && (!node->IsA("vtkMRMLVolumeRenderingDisplayNode")) )
    {
    return;
    }

  vtkMRMLVolumeNode* volumeNode = NULL;
  vtkMRMLVolumeRenderingDisplayNode* displayNode = NULL;

  bool modified = false;
  if ( (volumeNode = vtkMRMLVolumeNode::SafeDownCast(node)) )
    {
    this->Internal->RemoveVolumeNode(volumeNode);
    modified = true;
    }
  else if ( (displayNode = vtkMRMLVolumeRenderingDisplayNode::SafeDownCast(node)) )
    {
    this->Internal->RemoveDisplayNode(displayNode);
    modified = true;
    }
  if (modified)
    {
    this->RequestRender();
    }
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::ProcessMRMLNodesEvents(vtkObject* caller, unsigned long event, void* callData)
{
  vtkMRMLScene* scene = this->GetMRMLScene();

  if (scene->IsBatchProcessing())
    {
    return;
    }

  vtkMRMLVolumeNode* volumeNode = vtkMRMLVolumeNode::SafeDownCast(caller);
  if (volumeNode)
    {
    if (event == vtkMRMLDisplayableNode::DisplayModifiedEvent)
      {
      vtkMRMLNode* callDataNode = reinterpret_cast<vtkMRMLDisplayNode *> (callData);
      vtkMRMLVolumeRenderingDisplayNode* displayNode = vtkMRMLVolumeRenderingDisplayNode::SafeDownCast(callDataNode);
      if (displayNode)
        {
        // Don't update if we are in an interaction mode
        // (vtkCommand::InteractionEvent will be fired, so we can ignore Modified events)
        if (this->Internal->Interaction == 0)
          {
          this->Internal->UpdateDisplayNode(displayNode);
          this->RequestRender();
          }
        }
      }
    else if ( (event == vtkMRMLDisplayableNode::TransformModifiedEvent)
           || (event == vtkMRMLTransformableNode::TransformModifiedEvent) )
      {
      this->Internal->UpdatePipelineTransforms(volumeNode);

      // Reset ROI
      std::set< vtkMRMLVolumeRenderingDisplayNode* > displayNodes = this->Internal->VolumeToDisplayNodes[volumeNode];
      std::set< vtkMRMLVolumeRenderingDisplayNode* >::iterator displayNodeIt;
      for (displayNodeIt = displayNodes.begin(); displayNodeIt != displayNodes.end(); displayNodeIt++)
        {
        this->VolumeRenderingLogic->FitROIToVolume(*displayNodeIt);
        }

      this->RequestRender();
      }
    }
  else if (event == vtkCommand::StartEvent ||
           event == vtkCommand::StartInteractionEvent)
    {
    ++this->Internal->Interaction;
    // We request the interactive mode, we might have nested interactions
    // so we just start the mode for the first time.
    if (this->Internal->Interaction == 1)
      {
      vtkInteractorStyle* interactorStyle = vtkInteractorStyle::SafeDownCast(
        this->GetInteractor()->GetInteractorStyle());
      if (interactorStyle->GetState() == VTKIS_NONE)
        {
        interactorStyle->StartState(VTKIS_VOLUME_PROPS);
        }
      }
    }
  else if (event == vtkCommand::EndEvent ||
           event == vtkCommand::EndInteractionEvent)
    {
    --this->Internal->Interaction;
    if (this->Internal->Interaction == 0)
      {
      vtkInteractorStyle* interactorStyle = vtkInteractorStyle::SafeDownCast(
        this->GetInteractor()->GetInteractorStyle());
      if (interactorStyle->GetState() == VTKIS_VOLUME_PROPS)
        {
        interactorStyle->StopState();
        }
      if (caller->IsA("vtkMRMLVolumeRenderingDisplayNode"))
        {
        this->Internal->UpdateDisplayNode(vtkMRMLVolumeRenderingDisplayNode::SafeDownCast(caller));
        }
      }
    }
  else if (event == vtkCommand::InteractionEvent)
    {
    if (caller->IsA("vtkMRMLVolumeRenderingDisplayNode"))
      {
      this->Internal->UpdateDisplayNode(vtkMRMLVolumeRenderingDisplayNode::SafeDownCast(caller));
      this->RequestRender();
      }
    }
  else if (event == vtkMRMLScalarVolumeNode::ImageDataModifiedEvent)
    {
    int numDisplayNodes = volumeNode->GetNumberOfDisplayNodes();
    for (int i=0; i<numDisplayNodes; i++)
      {
      vtkMRMLVolumeRenderingDisplayNode* displayNode = vtkMRMLVolumeRenderingDisplayNode::SafeDownCast(volumeNode->GetNthDisplayNode(i));
      if (this->Internal->UseDisplayNode(displayNode))
        {
        this->Internal->UpdateDisplayNode(displayNode);
        this->RequestRender();
        }
      }
    }
  else if (event == vtkMRMLViewNode::GraphicalResourcesCreatedEvent)
    {
    this->UpdateFromMRML();
    }
  else
    {
    this->Superclass::ProcessMRMLNodesEvents(caller, event, callData);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::OnInteractorStyleEvent(int eventID)
{
  switch (eventID)
    {
    case vtkCommand::EndInteractionEvent:
    case vtkCommand::StartInteractionEvent:
    {
      vtkInternal::VolumeToDisplayCacheType::iterator displayableIt;
      for ( displayableIt = this->Internal->VolumeToDisplayNodes.begin();
            displayableIt!=this->Internal->VolumeToDisplayNodes.end(); ++displayableIt )
        {
        this->Internal->UpdatePipelineTransforms(displayableIt->first);
        }
      break;
    }
    default:
      break;
    }
  this->Superclass::OnInteractorStyleEvent(eventID);
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::UpdateFromMRML()
{
  this->SetUpdateFromMRMLRequested(0);

  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene)
    {
    vtkDebugMacro( "vtkMRMLVolumeRenderingDisplayableManager::UpdateFromMRML: Scene is not set")
    return;
    }
  this->Internal->ClearDisplayableNodes();

  vtkMRMLVolumeNode* volumeNode = NULL;
  std::vector<vtkMRMLNode*> volumeNodes;
  int numOfVolumeNodes = scene ? scene->GetNodesByClass("vtkMRMLVolumeNode", volumeNodes) : 0;
  for (int i=0; i<numOfVolumeNodes; i++)
    {
    volumeNode = vtkMRMLVolumeNode::SafeDownCast(volumeNodes[i]);
    if (volumeNode && this->Internal->UseDisplayableNode(volumeNode))
      {
      this->Internal->AddVolumeNode(volumeNode);
      }
    }
  this->RequestRender();
}
