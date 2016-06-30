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

==============================================================================*/

// MRMLDisplayableManager includes
#include "vtkMRMLSegmentationsDisplayableManager2D.h"

// MRML includes
#include <vtkMRMLProceduralColorNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSliceCompositeNode.h>
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLSegmentationDisplayNode.h>
#include <vtkMRMLSegmentationNode.h>
#include <vtkMRMLTransformNode.h>

// MRML logic includes
#include "vtkImageLabelOutline.h"

// SegmentationCore includes
#include "vtkSegmentation.h"
#include "vtkOrientedImageData.h"
#include "vtkOrientedImageDataResample.h"

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkCallbackCommand.h>
#include <vtkEventBroker.h>
#include <vtkActor2D.h>
#include <vtkMatrix4x4.h>
#include <vtkPlane.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkProperty2D.h>
#include <vtkRenderer.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkPointLocator.h>
#include <vtkGeneralTransform.h>
#include <vtkPointData.h>
#include <vtkDataSetAttributes.h>
#include <vtkCutter.h>
#include <vtkImageReslice.h>
#include <vtkImageMapper.h>
#include <vtkImageMapToRGBA.h>
#include <vtkLookupTable.h>
#include <vtkStripper.h>
#include <vtkTriangleFilter.h>
#include <vtkCleanPolyData.h>
#include <vtkCellArray.h>

// STD includes
#include <algorithm>
#include <set>
#include <map>

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkMRMLSegmentationsDisplayableManager2D );

//---------------------------------------------------------------------------
// Convert a linear transform that is almost exactly a permute transform
// to an exact permute transform.
// vtkImageReslice works about 10-20% faster if it reslices along an axis
// (transformation is just a permutation). However, vtkImageReslice
// checks for strict (floatValue!=0) to consider a matrix element zero.
// Here we set a small floatValue to 0 if it is several magnitudes
// (controlled by SUPPRESSION_FACTOR parameter) smaller than the
// maximum norm of the axis.
//----------------------------------------------------------------------------
void SnapToPermuteMatrix(vtkTransform* transform)
{
  const double SUPPRESSION_FACTOR = 1e-3;
  vtkHomogeneousTransform* linearTransform = vtkHomogeneousTransform::SafeDownCast(transform);
  if (!linearTransform)
    {
    // it is not a simple linear transform, so it cannot be snapped to a permute matrix
    return;
    }
  bool modified = false;
  vtkNew<vtkMatrix4x4> transformMatrix;
  linearTransform->GetMatrix(transformMatrix.GetPointer());
  for (int c=0; c<3; c++)
    {
    double absValues[3] = {fabs(transformMatrix->Element[0][c]), fabs(transformMatrix->Element[1][c]), fabs(transformMatrix->Element[2][c])};
    double maxValue = std::max(absValues[0], std::max(absValues[1], absValues[2]));
    double zeroThreshold = SUPPRESSION_FACTOR * maxValue;
    for (int r=0; r<3; r++)
      {
      if (absValues[r]!=0 && absValues[r]<zeroThreshold)
        {
        transformMatrix->Element[r][c]=0;
        modified = true;
        }
      }
    }
  if (modified)
    {
    transform->SetMatrix(transformMatrix.GetPointer());
    }
}

//---------------------------------------------------------------------------
class vtkMRMLSegmentationsDisplayableManager2D::vtkInternal
{
public:

  vtkInternal( vtkMRMLSegmentationsDisplayableManager2D* external );
  ~vtkInternal();

  struct Pipeline
    {
    std::string SegmentID;
    vtkSmartPointer<vtkTransform> WorldToSliceTransform;
    vtkSmartPointer<vtkGeneralTransform> NodeToWorldTransform;
    vtkSmartPointer<vtkGeneralTransform> WorldToNodeTransform;

    vtkSmartPointer<vtkActor2D> PolyDataOutlineActor;
    vtkSmartPointer<vtkActor2D> PolyDataFillActor;
    vtkSmartPointer<vtkTransformPolyDataFilter> ModelWarper;
    vtkSmartPointer<vtkPlane> Plane;
    vtkSmartPointer<vtkCutter> Cutter;
    vtkSmartPointer<vtkStripper> Stripper;
    vtkSmartPointer<vtkCleanPolyData> Cleaner;

    vtkSmartPointer<vtkActor2D> ImageOutlineActor;
    vtkSmartPointer<vtkActor2D> ImageFillActor;
    vtkSmartPointer<vtkImageReslice> Reslice;
    vtkSmartPointer<vtkGeneralTransform> SliceToImageTransform;
    vtkSmartPointer<vtkImageLabelOutline> LabelOutline;
    vtkSmartPointer<vtkLookupTable> LookupTableOutline;
    vtkSmartPointer<vtkLookupTable> LookupTableFill;
    };

  typedef std::map<std::string, const Pipeline*> PipelineMapType;
  typedef std::map < vtkMRMLSegmentationDisplayNode*, PipelineMapType > PipelinesCacheType;
  PipelinesCacheType DisplayPipelines;

  typedef std::map < vtkMRMLSegmentationNode*, std::set< vtkMRMLSegmentationDisplayNode* > > SegmentationToDisplayCacheType;
  SegmentationToDisplayCacheType SegmentationToDisplayNodes;

  // Segmentations
  void AddSegmentationNode(vtkMRMLSegmentationNode* displayableNode);
  void RemoveSegmentationNode(vtkMRMLSegmentationNode* displayableNode);

  // Transforms
  void UpdateDisplayableTransforms(vtkMRMLSegmentationNode *node);
  void GetNodeTransformToWorld(vtkMRMLTransformableNode* node, vtkGeneralTransform* transformToWorld, vtkGeneralTransform* transformFromWorld);

  // Slice Node
  void SetSliceNode(vtkMRMLSliceNode* sliceNode);
  void UpdateSliceNode();
  void SetSlicePlaneFromMatrix(vtkMatrix4x4* matrix, vtkPlane* plane);

  // Display Nodes
  void AddDisplayNode(vtkMRMLSegmentationNode*, vtkMRMLSegmentationDisplayNode*);
  Pipeline* CreateSegmentPipeline(std::string segmentID);
  void UpdateDisplayNode(vtkMRMLSegmentationDisplayNode* displayNode);
  void UpdateSegmentPipelines(vtkMRMLSegmentationDisplayNode*, PipelineMapType&);
  void UpdateDisplayNodePipeline(vtkMRMLSegmentationDisplayNode*, PipelineMapType);
  void RemoveDisplayNode(vtkMRMLSegmentationDisplayNode* displayNode);

  // Observations
  void AddObservations(vtkMRMLSegmentationNode* node);
  void RemoveObservations(vtkMRMLSegmentationNode* node);
  bool IsNodeObserved(vtkMRMLSegmentationNode* node);

  // Helper functions
  bool IsVisible(vtkMRMLSegmentationDisplayNode* displayNode);
  bool UseDisplayNode(vtkMRMLSegmentationDisplayNode* displayNode);
  bool UseDisplayableNode(vtkMRMLSegmentationNode* node);
  void ClearDisplayableNodes();

private:
  vtkSmartPointer<vtkMatrix4x4> SliceXYToRAS;
  vtkMRMLSegmentationsDisplayableManager2D* External;
  bool AddingSegmentationNode;
  vtkSmartPointer<vtkMRMLSliceNode> SliceNode;
};

//---------------------------------------------------------------------------
// vtkInternal methods

//---------------------------------------------------------------------------
vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::vtkInternal(vtkMRMLSegmentationsDisplayableManager2D* external)
: External(external)
, AddingSegmentationNode(false)
{
  this->SliceXYToRAS = vtkSmartPointer<vtkMatrix4x4>::New();
  this->SliceXYToRAS->Identity();
}

//---------------------------------------------------------------------------
vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::~vtkInternal()
{
  this->ClearDisplayableNodes();
  this->SliceNode = NULL;
}

//---------------------------------------------------------------------------
bool vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::UseDisplayNode(vtkMRMLSegmentationDisplayNode* displayNode)
{
   // allow annotations to appear only in designated viewers
  if (displayNode && !displayNode->IsDisplayableInView(this->SliceNode->GetID()))
    {
    return false;
    }

  // Check whether DisplayNode should be shown in this view
  bool use = displayNode && displayNode->IsA("vtkMRMLSegmentationDisplayNode");

  return use;
}

//---------------------------------------------------------------------------
bool vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::IsVisible(vtkMRMLSegmentationDisplayNode* displayNode)
{
  return displayNode
      && displayNode->GetVisibility(this->External->GetMRMLSliceNode()->GetID());
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::SetSliceNode(vtkMRMLSliceNode* sliceNode)
{
  if (!sliceNode || this->SliceNode == sliceNode)
    {
    return;
    }
  this->SliceNode=sliceNode;
  this->UpdateSliceNode();
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::UpdateSliceNode()
{
  // Update the Slice node transform then update the DisplayNode pipelines to account for plane location
  this->SliceXYToRAS->DeepCopy( this->SliceNode->GetXYToRAS() );
  PipelinesCacheType::iterator displayNodeIt;
  for (displayNodeIt = this->DisplayPipelines.begin(); displayNodeIt != this->DisplayPipelines.end(); ++displayNodeIt)
    {
    this->UpdateDisplayNodePipeline(displayNodeIt->first, displayNodeIt->second);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::SetSlicePlaneFromMatrix(vtkMatrix4x4* sliceMatrix, vtkPlane* plane)
{
  double normal[3] = {0.0,0.0,0.0};
  double origin[3] = {0.0,0.0,0.0};

  // +/-1: orientation of the normal
  const int planeOrientation = 1;
  for (int i = 0; i < 3; i++)
    {
    normal[i] = planeOrientation * sliceMatrix->GetElement(i,2);
    origin[i] = sliceMatrix->GetElement(i,3);
    }

  plane->SetNormal(normal);
  plane->SetOrigin(origin);
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::AddSegmentationNode(vtkMRMLSegmentationNode* node)
{
  if (this->AddingSegmentationNode)
    {
    return;
    }
  // Check if node should be used
  if (!this->UseDisplayableNode(node))
    {
    return;
    }

  this->AddingSegmentationNode = true;
  // Add Display Nodes
  int nnodes = node->GetNumberOfDisplayNodes();

  this->AddObservations(node);

  for (int i=0; i<nnodes; i++)
    {
    vtkMRMLSegmentationDisplayNode *dnode = vtkMRMLSegmentationDisplayNode::SafeDownCast(node->GetNthDisplayNode(i));
    if ( this->UseDisplayNode(dnode) )
      {
      this->SegmentationToDisplayNodes[node].insert(dnode);
      this->AddDisplayNode( node, dnode );
      }
    }
  this->AddingSegmentationNode = false;
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::RemoveSegmentationNode(vtkMRMLSegmentationNode* node)
{
  if (!node)
    {
    return;
    }
  vtkInternal::SegmentationToDisplayCacheType::iterator displayableIt =
    this->SegmentationToDisplayNodes.find(node);
  if(displayableIt == this->SegmentationToDisplayNodes.end())
    {
    return;
    }

  std::set< vtkMRMLSegmentationDisplayNode* > dnodes = displayableIt->second;
  std::set< vtkMRMLSegmentationDisplayNode* >::iterator diter;
  for ( diter = dnodes.begin(); diter != dnodes.end(); ++diter)
    {
    this->RemoveDisplayNode(*diter);
    }
  this->RemoveObservations(node);
  this->SegmentationToDisplayNodes.erase(displayableIt);
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::GetNodeTransformToWorld(vtkMRMLTransformableNode* node, vtkGeneralTransform* transformToWorld, vtkGeneralTransform* transformFromWorld)
{
  if (!node || !transformToWorld)
    {
    return;
    }

  vtkMRMLTransformNode* tnode = node->GetParentTransformNode();

  transformToWorld->Identity();
  transformFromWorld->Identity();
  if (tnode)
    {
    tnode->GetTransformToWorld(transformToWorld);
    // Need inverse of the transform for image resampling
    tnode->GetTransformFromWorld(transformFromWorld);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::UpdateDisplayableTransforms(vtkMRMLSegmentationNode* mNode)
{
  // Update the NodeToWorld matrix for all tracked DisplayableNode
  PipelinesCacheType::iterator pipelinesIter;
  std::set<vtkMRMLSegmentationDisplayNode *> displayNodes = this->SegmentationToDisplayNodes[mNode];
  std::set<vtkMRMLSegmentationDisplayNode *>::iterator dnodesIter;
  for ( dnodesIter = displayNodes.begin(); dnodesIter != displayNodes.end(); dnodesIter++ )
    {
    if ( ((pipelinesIter = this->DisplayPipelines.find(*dnodesIter)) != this->DisplayPipelines.end()) )
      {
      for (PipelineMapType::iterator pipelineIt=pipelinesIter->second.begin(); pipelineIt!=pipelinesIter->second.end(); ++pipelineIt)
        {
        const Pipeline* currentPipeline = pipelineIt->second;
        this->GetNodeTransformToWorld(mNode, currentPipeline->NodeToWorldTransform, currentPipeline->WorldToNodeTransform);
        }
      this->UpdateDisplayNodePipeline(pipelinesIter->first, pipelinesIter->second);
      }
    }
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::RemoveDisplayNode(vtkMRMLSegmentationDisplayNode* displayNode)
{
  PipelinesCacheType::iterator pipelinesIter = this->DisplayPipelines.find(displayNode);
  if (pipelinesIter == this->DisplayPipelines.end())
    {
    return;
    }
  PipelineMapType::iterator pipelineIt;
  for (pipelineIt = pipelinesIter->second.begin(); pipelineIt != pipelinesIter->second.end(); ++pipelineIt)
    {
    const Pipeline* pipeline = pipelineIt->second;
    this->External->GetRenderer()->RemoveActor(pipeline->PolyDataOutlineActor);
    this->External->GetRenderer()->RemoveActor(pipeline->PolyDataFillActor);
    this->External->GetRenderer()->RemoveActor(pipeline->ImageOutlineActor);
    this->External->GetRenderer()->RemoveActor(pipeline->ImageFillActor);
    delete pipeline;
    }
  this->DisplayPipelines.erase(pipelinesIter);
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::AddDisplayNode(vtkMRMLSegmentationNode* mNode, vtkMRMLSegmentationDisplayNode* displayNode)
{
  if (!mNode || !displayNode)
    {
    return;
    }

  // Do not add the display node if displayNodeIt is already associated with a pipeline object.
  // This happens when a segmentation node already associated with a display node
  // is copied into an other (using vtkMRMLNode::Copy()) and is added to the scene afterward.
  // Related issue are #3428 and #2608
  PipelinesCacheType::iterator displayNodeIt;
  displayNodeIt = this->DisplayPipelines.find(displayNode);
  if (displayNodeIt != this->DisplayPipelines.end())
    {
    return;
    }

  // Create pipelines for each segment
  vtkSegmentation* segmentation = mNode->GetSegmentation();
  if (!segmentation)
    {
    return;
    }
  PipelineMapType pipelineVector;
  vtkSegmentation::SegmentMap segmentMap = segmentation->GetSegments();
  for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt)
    {
    pipelineVector[segmentIt->first] = this->CreateSegmentPipeline(segmentIt->first);
    }

  this->DisplayPipelines.insert( std::make_pair(displayNode, pipelineVector) );

  // Update cached matrices. Calls UpdateDisplayNodePipeline
  this->UpdateDisplayableTransforms(mNode);
}

//---------------------------------------------------------------------------
vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::Pipeline*
vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::CreateSegmentPipeline(std::string segmentID)
{
  Pipeline* pipeline = new Pipeline();
  pipeline->SegmentID = segmentID;
  pipeline->WorldToSliceTransform = vtkSmartPointer<vtkTransform>::New();
  pipeline->NodeToWorldTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  pipeline->WorldToNodeTransform = vtkSmartPointer<vtkGeneralTransform>::New();

  // Create poly data pipeline
  pipeline->PolyDataOutlineActor = vtkSmartPointer<vtkActor2D>::New();
  pipeline->PolyDataFillActor = vtkSmartPointer<vtkActor2D>::New();
  pipeline->Cutter = vtkSmartPointer<vtkCutter>::New();
  pipeline->ModelWarper = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  pipeline->Plane = vtkSmartPointer<vtkPlane>::New();
  pipeline->Stripper = vtkSmartPointer<vtkStripper>::New();
  pipeline->Cleaner = vtkSmartPointer<vtkCleanPolyData>::New();

  // Set up poly data outline pipeline
  pipeline->Cutter->SetInputConnection(pipeline->ModelWarper->GetOutputPort());
  pipeline->Cutter->SetCutFunction(pipeline->Plane);
  pipeline->Cutter->SetGenerateCutScalars(0);
  vtkSmartPointer<vtkTransformPolyDataFilter> polyDataOutlineTransformer = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  polyDataOutlineTransformer->SetInputConnection(pipeline->Cutter->GetOutputPort());
  polyDataOutlineTransformer->SetTransform(pipeline->WorldToSliceTransform);
  vtkSmartPointer<vtkPolyDataMapper2D> polyDataOutlineMapper = vtkSmartPointer<vtkPolyDataMapper2D>::New();
  polyDataOutlineMapper->SetInputConnection(polyDataOutlineTransformer->GetOutputPort());
  pipeline->PolyDataOutlineActor->SetMapper(polyDataOutlineMapper);
  pipeline->PolyDataOutlineActor->SetVisibility(0);

  // Set up poly data fill pipeline
  pipeline->Stripper->SetInputConnection(pipeline->Cutter->GetOutputPort());
  pipeline->Cleaner->SetInputConnection(NULL); // This will be modified in the UpdateDisplayNodePipeline function
  vtkSmartPointer<vtkTriangleFilter> triangleFilter = vtkSmartPointer<vtkTriangleFilter>::New();
  triangleFilter->SetInputConnection(pipeline->Cleaner->GetOutputPort());
  vtkSmartPointer<vtkTransformPolyDataFilter> polyDataFillTransformer = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  polyDataFillTransformer->SetInputConnection(triangleFilter->GetOutputPort());
  polyDataFillTransformer->SetTransform(pipeline->WorldToSliceTransform);
  vtkSmartPointer<vtkPolyDataMapper2D> polyDataFillMapper = vtkSmartPointer<vtkPolyDataMapper2D>::New();
  polyDataFillMapper->SetInputConnection(polyDataFillTransformer->GetOutputPort());
  pipeline->PolyDataFillActor->SetMapper(polyDataFillMapper);
  pipeline->PolyDataFillActor->SetVisibility(0);

  // Create image pipeline
  pipeline->ImageOutlineActor = vtkSmartPointer<vtkActor2D>::New();
  pipeline->ImageFillActor = vtkSmartPointer<vtkActor2D>::New();
  pipeline->Reslice = vtkSmartPointer<vtkImageReslice>::New();
  pipeline->SliceToImageTransform = vtkSmartPointer<vtkGeneralTransform>::New();
  pipeline->LabelOutline = vtkSmartPointer<vtkImageLabelOutline>::New();
  pipeline->LookupTableOutline = vtkSmartPointer<vtkLookupTable>::New();
  pipeline->LookupTableFill = vtkSmartPointer<vtkLookupTable>::New();

  // Set up image pipeline
  pipeline->Reslice->SetBackgroundColor(0.0, 0.0, 0.0, 0.0);
  pipeline->Reslice->AutoCropOutputOff();
  pipeline->Reslice->SetOptimization(1);
  pipeline->Reslice->SetOutputOrigin(0.0, 0.0, 0.0);
  pipeline->Reslice->SetOutputSpacing(1.0, 1.0, 1.0);
  pipeline->Reslice->SetOutputDimensionality(3);
  pipeline->Reslice->GenerateStencilOutputOn();

  pipeline->SliceToImageTransform->PostMultiply();

  pipeline->LookupTableOutline->SetRampToLinear();
  pipeline->LookupTableOutline->SetNumberOfTableValues(2);
  pipeline->LookupTableOutline->SetTableRange(0, 1);
  pipeline->LookupTableOutline->SetTableValue(0,  0, 0, 0,  0);
  pipeline->LookupTableOutline->SetTableValue(1,  0, 0, 0,  0);
  pipeline->LookupTableFill->SetRampToLinear();
  pipeline->LookupTableFill->SetNumberOfTableValues(2);
  pipeline->LookupTableFill->SetTableRange(0, 1);
  pipeline->LookupTableFill->SetTableValue(0,  0, 0, 0,  0);
  pipeline->LookupTableFill->SetTableValue(1,  0, 0, 0,  0);

  // Image outline
  pipeline->LabelOutline->SetInputConnection(pipeline->Reslice->GetOutputPort());
  vtkSmartPointer<vtkImageMapToRGBA> outlineColorMapper = vtkSmartPointer<vtkImageMapToRGBA>::New();
  outlineColorMapper->SetInputConnection(pipeline->LabelOutline->GetOutputPort());
  outlineColorMapper->SetOutputFormatToRGBA();
  outlineColorMapper->SetLookupTable(pipeline->LookupTableOutline);
  vtkSmartPointer<vtkImageMapper> imageOutlineMapper = vtkSmartPointer<vtkImageMapper>::New();
  imageOutlineMapper->SetInputConnection(outlineColorMapper->GetOutputPort());
  imageOutlineMapper->SetColorWindow(255);
  imageOutlineMapper->SetColorLevel(127.5);
  pipeline->ImageOutlineActor->SetMapper(imageOutlineMapper);
  pipeline->ImageOutlineActor->SetVisibility(0);

  // Image fill
  vtkSmartPointer<vtkImageMapToRGBA> fillColorMapper = vtkSmartPointer<vtkImageMapToRGBA>::New();
  fillColorMapper->SetInputConnection(pipeline->Reslice->GetOutputPort());
  fillColorMapper->SetOutputFormatToRGBA();
  fillColorMapper->SetLookupTable(pipeline->LookupTableFill);
  vtkSmartPointer<vtkImageMapper> imageFillMapper = vtkSmartPointer<vtkImageMapper>::New();
  imageFillMapper->SetInputConnection(fillColorMapper->GetOutputPort());
  imageFillMapper->SetColorWindow(255);
  imageFillMapper->SetColorLevel(127.5);
  pipeline->ImageFillActor->SetMapper(imageFillMapper);
  pipeline->ImageFillActor->SetVisibility(0);

  // Add actors to Renderer
  this->External->GetRenderer()->AddActor( pipeline->PolyDataOutlineActor );
  this->External->GetRenderer()->AddActor( pipeline->PolyDataFillActor );
  this->External->GetRenderer()->AddActor( pipeline->ImageOutlineActor );
  this->External->GetRenderer()->AddActor( pipeline->ImageFillActor );

  return pipeline;
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::UpdateDisplayNode(vtkMRMLSegmentationDisplayNode* displayNode)
{
  // If the DisplayNode already exists, just update. Otherwise, add as new node
  if (!displayNode)
    {
    return;
    }
  PipelinesCacheType::iterator displayNodeIt = this->DisplayPipelines.find(displayNode);
  if (displayNodeIt != this->DisplayPipelines.end())
    {
    this->UpdateSegmentPipelines(displayNode, displayNodeIt->second);
    this->UpdateDisplayNodePipeline(displayNode, displayNodeIt->second);
    }
  else
    {
    this->AddSegmentationNode( vtkMRMLSegmentationNode::SafeDownCast(displayNode->GetDisplayableNode()) );
    }
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::UpdateSegmentPipelines(vtkMRMLSegmentationDisplayNode* displayNode, PipelineMapType &pipelines)
{
  // Get segmentation
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(displayNode->GetDisplayableNode());
  if (!segmentationNode)
    {
    return;
    }
  vtkSegmentation* segmentation = segmentationNode->GetSegmentation();
  if (!segmentation)
    {
    return;
    }

  // Get segments
  vtkSegmentation::SegmentMap segmentMap = segmentation->GetSegments();

  bool requestTransformUpdate = false;
  // Make sure each segment has a pipeline
  for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt)
    {
    // If segment does not have a pipeline, create one
    PipelineMapType::iterator pipelineIt = pipelines.find(segmentIt->first);
    if (pipelineIt == pipelines.end())
      {
      pipelines[segmentIt->first] = this->CreateSegmentPipeline(segmentIt->first);
      requestTransformUpdate = true;
      }
    }

  // Make sure each pipeline belongs to an existing segment
  PipelineMapType::iterator pipelineIt = pipelines.begin();
  while (pipelineIt != pipelines.end())
    {
    const Pipeline* pipeline = pipelineIt->second;
    vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.find(pipeline->SegmentID);
    if (segmentIt == segmentMap.end())
      {
      PipelineMapType::iterator erasedIt = pipelineIt;
      ++pipelineIt;
      pipelines.erase(erasedIt);
      this->External->GetRenderer()->RemoveActor(pipeline->PolyDataOutlineActor);
      this->External->GetRenderer()->RemoveActor(pipeline->PolyDataFillActor);
      this->External->GetRenderer()->RemoveActor(pipeline->ImageOutlineActor);
      this->External->GetRenderer()->RemoveActor(pipeline->ImageFillActor);
      delete pipeline;
      }
    else
      {
      ++pipelineIt;
      }
    }

  // Update cached matrices. Calls UpdateDisplayNodePipeline
  if (requestTransformUpdate)
  {
    this->UpdateDisplayableTransforms(segmentationNode);
  }
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::UpdateDisplayNodePipeline(vtkMRMLSegmentationDisplayNode* displayNode, PipelineMapType pipelines)
{
  // Sets visibility, set pipeline polydata input, update color calculate and set pipeline segments.
  if (!displayNode)
    {
    return;
    }
  bool displayNodeVisible = this->IsVisible(displayNode);

  // Get segmentation display node
  vtkMRMLSegmentationDisplayNode* segmentationDisplayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(displayNode);

  // Determine which representation to show
  std::string shownRepresenatationName = segmentationDisplayNode->GetDisplayRepresentationName2D();
  if (shownRepresenatationName.empty())
    {
    // Hide segmentation if there is no 2D representation to show
    for (PipelineMapType::iterator pipelineIt=pipelines.begin(); pipelineIt!=pipelines.end(); ++pipelineIt)
      {
      pipelineIt->second->PolyDataOutlineActor->SetVisibility(false);
      pipelineIt->second->PolyDataFillActor->SetVisibility(false);
      pipelineIt->second->ImageOutlineActor->SetVisibility(false);
      pipelineIt->second->ImageFillActor->SetVisibility(false);
      }
    return;
    }

  // Get segmentation
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(
    segmentationDisplayNode->GetDisplayableNode() );
  if (!segmentationNode)
    {
    return;
    }
  vtkSegmentation* segmentation = segmentationNode->GetSegmentation();
  if (!segmentation)
    {
    return;
    }
  // Make sure the requested representation exists
  if (!segmentation->CreateRepresentation(shownRepresenatationName))
    {
    return;
    }

  // For all pipelines (pipeline per segment)
  for (PipelineMapType::iterator pipelineIt=pipelines.begin(); pipelineIt!=pipelines.end(); ++pipelineIt)
    {
    const Pipeline* pipeline = pipelineIt->second;

    // Get visibility
    vtkMRMLSegmentationDisplayNode::SegmentDisplayProperties properties;
    displayNode->GetSegmentDisplayProperties(pipeline->SegmentID, properties);
    bool segmentOutlineVisible = displayNodeVisible && properties.Visible && properties.Visible2DOutline;
    bool segmentFillVisible = displayNodeVisible && properties.Visible && properties.Visible2DFill;

    // Get representation to display
    vtkPolyData* polyData = vtkPolyData::SafeDownCast(
      segmentation->GetSegmentRepresentation(pipeline->SegmentID, shownRepresenatationName) );
    vtkOrientedImageData* imageData = vtkOrientedImageData::SafeDownCast(
      segmentation->GetSegmentRepresentation(pipeline->SegmentID, shownRepresenatationName) );
    if (imageData)
    {
      int* imageExtent = imageData->GetExtent();
      if (imageExtent[0]>imageExtent[1] || imageExtent[2]>imageExtent[3] || imageExtent[4]>imageExtent[5])
      {
        // empty image
        imageData = NULL;
      }
    }

    if ( (!segmentOutlineVisible && !segmentFillVisible)
      || ((!polyData || polyData->GetNumberOfPoints() == 0) && !imageData) )
      {
      pipelineIt->second->PolyDataOutlineActor->SetVisibility(false);
      pipelineIt->second->PolyDataFillActor->SetVisibility(false);
      pipelineIt->second->ImageOutlineActor->SetVisibility(false);
      pipelineIt->second->ImageFillActor->SetVisibility(false);
      continue;
      }

    // If shown representation is poly data
    if (polyData)
      {
      // Turn off image visibility when showing poly data
      pipeline->ImageOutlineActor->SetVisibility(false);
      pipeline->ImageFillActor->SetVisibility(false);

      pipeline->ModelWarper->SetInputData(polyData);
      pipeline->ModelWarper->SetTransform(pipeline->NodeToWorldTransform);

      // Set Plane transform
      this->SetSlicePlaneFromMatrix(this->SliceXYToRAS, pipeline->Plane);
      pipeline->Plane->Modified();

      // Set PolyData transform
      vtkNew<vtkMatrix4x4> rasToSliceXY;
      vtkMatrix4x4::Invert(this->SliceXYToRAS, rasToSliceXY.GetPointer());
      pipeline->WorldToSliceTransform->SetMatrix(rasToSliceXY.GetPointer());

      // Optimization for slice to slice intersections which are 1 quad polydatas
      // no need for 50^3 default locator divisions
      if (polyData->GetPoints() != NULL && polyData->GetNumberOfPoints() <= 4 )
        {
        vtkNew<vtkPointLocator> locator;
        double *bounds = polyData->GetBounds();
        locator->SetDivisions(2,2,2);
        locator->InitPointInsertion(polyData->GetPoints(), bounds);
        pipeline->Cutter->SetLocator(locator.GetPointer());
        }

      // Apply trick to create cell from line for poly data fill
      // Omit cells that are not closed (first point is not same as last)
      pipeline->Stripper->Update();
      vtkCellArray* strippedLines = pipeline->Stripper->GetOutput()->GetLines();
      vtkSmartPointer<vtkCellArray> closedCells = vtkSmartPointer<vtkCellArray>::New();
      bool cellsValid = false;
      for (int index=0; index<strippedLines->GetNumberOfCells(); ++index)
        {
        vtkSmartPointer<vtkIdList> pointList = vtkSmartPointer<vtkIdList>::New();
        strippedLines->GetCell(index, pointList);
        if ( pointList->GetNumberOfIds() > 0
          && pointList->GetId(0) == pointList->GetId(pointList->GetNumberOfIds()-1) )
          {
          closedCells->InsertNextCell(pointList);
          cellsValid = true;
          }
        }
      vtkSmartPointer<vtkPolyData> fillPolyData = vtkSmartPointer<vtkPolyData>::New();
      fillPolyData->SetPoints(pipeline->Stripper->GetOutput()->GetPoints());
      fillPolyData->SetPolys(closedCells);
      cellsValid = false; //TODO: Disable polydata fill until a good solution is found
      if (cellsValid)
        {
        pipeline->Cleaner->SetInputData(fillPolyData);
        }
      else
        {
        segmentFillVisible = false;
        }

      // Update pipeline actors
      pipeline->PolyDataOutlineActor->SetVisibility(segmentOutlineVisible);
      pipeline->PolyDataOutlineActor->GetProperty()->SetColor(properties.Color[0], properties.Color[1], properties.Color[2]);
      pipeline->PolyDataOutlineActor->GetProperty()->SetOpacity(properties.Opacity2DOutline * displayNode->GetOpacity());
      pipeline->PolyDataOutlineActor->GetProperty()->SetLineWidth(displayNode->GetSliceIntersectionThickness());
      pipeline->PolyDataOutlineActor->SetPosition(0,0);
      pipeline->PolyDataFillActor->SetVisibility(segmentFillVisible);
      pipeline->PolyDataFillActor->GetProperty()->SetColor(properties.Color[0], properties.Color[1], properties.Color[2]);
      pipeline->PolyDataFillActor->GetProperty()->SetOpacity(properties.Opacity2DFill * displayNode->GetOpacity());
      pipeline->PolyDataFillActor->SetPosition(0,0);
      }
    // If shown representation is image data
    else if (imageData)
      {
      // Turn off poly data visibility when showing image
      pipeline->PolyDataOutlineActor->SetVisibility(false);
      pipeline->PolyDataFillActor->SetVisibility(false);

      // Set segment color
      pipeline->LookupTableOutline->SetTableValue(1,
        properties.Color[0], properties.Color[1], properties.Color[2], properties.Opacity2DOutline * displayNode->GetOpacity());
      pipeline->LookupTableFill->SetTableValue(1,
        properties.Color[0], properties.Color[1], properties.Color[2], properties.Opacity2DFill * displayNode->GetOpacity());

      // Calculate image IJK to world RAS transform
      pipeline->SliceToImageTransform->Identity();
      pipeline->SliceToImageTransform->Concatenate(this->SliceXYToRAS);
      pipeline->SliceToImageTransform->Concatenate(pipeline->WorldToNodeTransform);
      vtkSmartPointer<vtkMatrix4x4> worldToImageMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
      imageData->GetWorldToImageMatrix(worldToImageMatrix);
      pipeline->SliceToImageTransform->Concatenate(worldToImageMatrix);

      // Create temporary copy of the segment image with default origin and spacing
      vtkSmartPointer<vtkImageData> identityImageData = vtkSmartPointer<vtkImageData>::New();
      identityImageData->ShallowCopy(imageData);
      identityImageData->SetOrigin(0.0, 0.0, 0.0);
      identityImageData->SetSpacing(1.0, 1.0, 1.0);

      // Set Reslice transform
      // vtkImageReslice works faster if the input is a linear transform, so try to convert it
      // to a linear transform.
      // Also attempt to make it a permute transform, as it makes reslicing even faster.
      vtkSmartPointer<vtkTransform> linearSliceToImageTransform = vtkSmartPointer<vtkTransform>::New();
      if (vtkMRMLTransformNode::IsGeneralTransformLinear(pipeline->SliceToImageTransform, linearSliceToImageTransform))
        {
        SnapToPermuteMatrix(linearSliceToImageTransform);
        pipeline->Reslice->SetResliceTransform(linearSliceToImageTransform);
        }
      else
        {
        pipeline->Reslice->SetResliceTransform(pipeline->SliceToImageTransform);
        }
      //TODO: Interpolate fractional labelmaps
      pipeline->Reslice->SetInterpolationModeToNearestNeighbor();
      pipeline->Reslice->SetInputData(identityImageData);
      int dimensions[3] = {0,0,0};
      this->SliceNode->GetDimensions(dimensions);
      pipeline->Reslice->SetOutputExtent(0, dimensions[0]-1, 0, dimensions[1]-1, 0, dimensions[2]-1);

      // Set outline properties and turn it off if not shown
      if (segmentOutlineVisible)
        {
        pipeline->LabelOutline->SetInputConnection(pipeline->Reslice->GetOutputPort());
        pipeline->LabelOutline->SetOutline(displayNode->GetSliceIntersectionThickness());
        }
      else
        {
        pipeline->LabelOutline->SetInputConnection(NULL);
        }

      // Update pipeline actors
      pipeline->ImageOutlineActor->SetVisibility(segmentOutlineVisible);
      pipeline->ImageOutlineActor->SetPosition(0,0);
      pipeline->ImageFillActor->SetVisibility(segmentFillVisible);
      pipeline->ImageFillActor->SetPosition(0,0);
      }
    }
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::AddObservations(vtkMRMLSegmentationNode* node)
{
  vtkEventBroker* broker = vtkEventBroker::GetInstance();
  if (!broker->GetObservationExist(node, vtkMRMLTransformableNode::TransformModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() ))
    {
    broker->AddObservation(node, vtkMRMLTransformableNode::TransformModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() );
    }
  if (!broker->GetObservationExist(node, vtkMRMLDisplayableNode::DisplayModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() ))
    {
    broker->AddObservation(node, vtkMRMLDisplayableNode::DisplayModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() );
    }
  if (!broker->GetObservationExist(node, vtkSegmentation::MasterRepresentationModified, this->External, this->External->GetMRMLNodesCallbackCommand() ))
    {
    broker->AddObservation(node, vtkSegmentation::MasterRepresentationModified, this->External, this->External->GetMRMLNodesCallbackCommand() );
    }
  if (!broker->GetObservationExist(node, vtkSegmentation::RepresentationCreated, this->External, this->External->GetMRMLNodesCallbackCommand() ))
    {
    broker->AddObservation(node, vtkSegmentation::RepresentationCreated, this->External, this->External->GetMRMLNodesCallbackCommand() );
    }
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::RemoveObservations(vtkMRMLSegmentationNode* node)
{
  vtkEventBroker* broker = vtkEventBroker::GetInstance();
  vtkEventBroker::ObservationVector observations;
  observations = broker->GetObservations(node, vtkMRMLTransformableNode::TransformModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() );
  broker->RemoveObservations(observations);
  observations = broker->GetObservations(node, vtkMRMLDisplayableNode::DisplayModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() );
  broker->RemoveObservations(observations);
  observations = broker->GetObservations(node, vtkSegmentation::MasterRepresentationModified, this->External, this->External->GetMRMLNodesCallbackCommand() );
  broker->RemoveObservations(observations);
  observations = broker->GetObservations(node, vtkSegmentation::RepresentationCreated, this->External, this->External->GetMRMLNodesCallbackCommand() );
  broker->RemoveObservations(observations);
}

//---------------------------------------------------------------------------
bool vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::IsNodeObserved(vtkMRMLSegmentationNode* node)
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
void vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::ClearDisplayableNodes()
{
  while(this->SegmentationToDisplayNodes.size() > 0)
    {
    this->RemoveSegmentationNode(this->SegmentationToDisplayNodes.begin()->first);
    }
}

//---------------------------------------------------------------------------
bool vtkMRMLSegmentationsDisplayableManager2D::vtkInternal::UseDisplayableNode(vtkMRMLSegmentationNode* node)
{
  bool use = node && node->IsA("vtkMRMLSegmentationNode");
  return use;
}


//---------------------------------------------------------------------------
// vtkMRMLSegmentationsDisplayableManager2D methods

//---------------------------------------------------------------------------
vtkMRMLSegmentationsDisplayableManager2D::vtkMRMLSegmentationsDisplayableManager2D()
{
  this->Internal = new vtkInternal(this);
}

//---------------------------------------------------------------------------
vtkMRMLSegmentationsDisplayableManager2D::~vtkMRMLSegmentationsDisplayableManager2D()
{
  delete this->Internal;
  this->Internal = NULL;
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "vtkMRMLSegmentationsDisplayableManager2D: " << this->GetClassName() << "\n";
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if ( !node->IsA("vtkMRMLSegmentationNode") )
    {
    return;
    }

  // Escape if the scene a scene is being closed, imported or connected
  if (this->GetMRMLScene()->IsBatchProcessing())
    {
    this->SetUpdateFromMRMLRequested(1);
    return;
    }

  this->Internal->AddSegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(node));
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if ( node
    && (!node->IsA("vtkMRMLSegmentationNode"))
    && (!node->IsA("vtkMRMLSegmentationDisplayNode")) )
    {
    return;
    }

  vtkMRMLSegmentationNode* segmentationNode = NULL;
  vtkMRMLSegmentationDisplayNode* displayNode = NULL;

  bool modified = false;
  if ( (segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(node)) )
    {
    this->Internal->RemoveSegmentationNode(segmentationNode);
    modified = true;
    }
  else if ( (displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(node)) )
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
void vtkMRMLSegmentationsDisplayableManager2D::ProcessMRMLNodesEvents(vtkObject* caller, unsigned long event, void* callData)
{
  vtkMRMLScene* scene = this->GetMRMLScene();

  if ( scene->IsBatchProcessing() )
    {
    return;
    }

  vtkMRMLSegmentationNode* displayableNode = vtkMRMLSegmentationNode::SafeDownCast(caller);

  if (displayableNode)
    {
    if (event == vtkMRMLDisplayableNode::DisplayModifiedEvent)
      {
      vtkMRMLNode* callDataNode = reinterpret_cast<vtkMRMLDisplayNode *> (callData);
      vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(callDataNode);
      if (displayNode)
        {
        this->Internal->UpdateDisplayNode(displayNode);
        this->RequestRender();
        }
      }
    else if ( (event == vtkMRMLDisplayableNode::TransformModifiedEvent)
           || (event == vtkMRMLTransformableNode::TransformModifiedEvent)
           || (event == vtkSegmentation::RepresentationCreated) )
      {
      this->Internal->UpdateDisplayableTransforms(displayableNode);
      this->RequestRender();
      }
    }
  else if ( vtkMRMLSliceNode::SafeDownCast(caller) )
      {
      this->Internal->UpdateSliceNode();
      this->RequestRender();
      }
  else
    {
    this->Superclass::ProcessMRMLNodesEvents(caller, event, callData);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::UpdateFromMRML()
{
  this->SetUpdateFromMRMLRequested(0);

  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene)
    {
    vtkDebugMacro( "vtkMRMLSegmentationsDisplayableManager2D->UpdateFromMRML: Scene is not set.")
    return;
    }
  this->Internal->ClearDisplayableNodes();

  vtkMRMLSegmentationNode* mNode = NULL;
  std::vector<vtkMRMLNode *> mNodes;
  int nnodes = scene ? scene->GetNodesByClass("vtkMRMLSegmentationNode", mNodes) : 0;
  for (int i=0; i<nnodes; i++)
    {
    mNode  = vtkMRMLSegmentationNode::SafeDownCast(mNodes[i]);
    if (mNode && this->Internal->UseDisplayableNode(mNode))
      {
      this->Internal->AddSegmentationNode(mNode);
      }
    }
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::UnobserveMRMLScene()
{
  this->Internal->ClearDisplayableNodes();
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::OnMRMLSceneStartClose()
{
  this->Internal->ClearDisplayableNodes();
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::OnMRMLSceneEndClose()
{
  this->SetUpdateFromMRMLRequested(1);
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::OnMRMLSceneEndBatchProcess()
{
  this->SetUpdateFromMRMLRequested(1);
}

//---------------------------------------------------------------------------
void vtkMRMLSegmentationsDisplayableManager2D::Create()
{
  this->Internal->SetSliceNode(this->GetMRMLSliceNode());
  this->SetUpdateFromMRMLRequested(1);
}
