/*=========================================================================

  Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL: http://svn.slicer.org/Slicer4/trunk/Base/GUI/vtkMRMLTransformsDisplayableManager3D.cxx $
  Date:      $Date: 2010-05-27 14:32:23 -0400 (Thu, 27 May 2010) $
  Version:   $Revision: 13525 $

==========================================================================*/

// MRMLLogic includes

// MRMLDisplayableManager includes
#include "vtkMRMLTransformsDisplayableManager3D.h"
#include "TransformsDisplayableManagerHelper.h"

// MRML includes
#include <vtkEventBroker.h>
#include <vtkMRMLDisplayableNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLProceduralColorNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLTransformDisplayNode.h>
#include <vtkMRMLViewNode.h>

// VTK includes
#include <vtkColorTransferFunction.h>
#include <vtkDataSetAttributes.h>
#include <vtkLookupTable.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

// STD includes
#include <cassert>

//---------------------------------------------------------------------------
vtkStandardNewMacro ( vtkMRMLTransformsDisplayableManager3D );

//---------------------------------------------------------------------------
class vtkMRMLTransformsDisplayableManager3D::vtkInternal
{
public:

  vtkInternal(vtkMRMLTransformsDisplayableManager3D * external);
  ~vtkInternal();

  struct Pipeline
    {
    vtkSmartPointer<vtkActor> Actor;
    vtkSmartPointer<vtkPolyData> InputPolyData;
    };

  typedef std::map < vtkMRMLTransformDisplayNode*, const Pipeline* > PipelinesCacheType;
  PipelinesCacheType DisplayPipelines;

  typedef std::map < vtkMRMLTransformNode*, std::set< vtkMRMLTransformDisplayNode* > > ModelToDisplayCacheType;
  ModelToDisplayCacheType ModelToDisplayNodes;

  // Transforms
  void UpdateDisplayableTransforms(vtkMRMLTransformNode *node);

  // Slice Node
  //void SetSliceNode(vtkMRMLSliceNode* sliceNode);
  //void UpdateSliceNode();

  // Display Nodes
  void AddDisplayNode(vtkMRMLTransformNode*, vtkMRMLTransformDisplayNode*);
  void UpdateDisplayNode(vtkMRMLTransformDisplayNode* displayNode);
  void UpdateDisplayNodePipeline(vtkMRMLTransformDisplayNode*, const Pipeline*);
  void RemoveDisplayNode(vtkMRMLTransformDisplayNode* displayNode);

  // Observations
  void AddObservations(vtkMRMLTransformNode* node);
  void RemoveObservations(vtkMRMLTransformNode* node);
  bool IsNodeObserved(vtkMRMLTransformNode* node);

  // Helper functions
  bool IsVisible(vtkMRMLTransformDisplayNode* displayNode);
  bool UseDisplayNode(vtkMRMLTransformDisplayNode* displayNode);
  bool UseDisplayableNode(vtkMRMLTransformNode* node);
  void ClearDisplayableNodes();

private:
  vtkMRMLTransformsDisplayableManager3D* External;
};

//---------------------------------------------------------------------------
// vtkInternal methods

//---------------------------------------------------------------------------
vtkMRMLTransformsDisplayableManager3D::vtkInternal::vtkInternal(vtkMRMLTransformsDisplayableManager3D * external)
: External(external)
{
}

//---------------------------------------------------------------------------
vtkMRMLTransformsDisplayableManager3D::vtkInternal::~vtkInternal()
{
  this->ClearDisplayableNodes();
}

//---------------------------------------------------------------------------
bool vtkMRMLTransformsDisplayableManager3D::vtkInternal::UseDisplayNode(vtkMRMLTransformDisplayNode* displayNode)
{
   // allow annotations to appear only in designated viewers
  if (displayNode && !displayNode->IsDisplayableInView(this->External->GetMRMLViewNode()->GetID()))
    {
    return false;
    }

  // Check whether DisplayNode should be shown in this view
  bool use = displayNode && displayNode->IsA("vtkMRMLTransformDisplayNode");

  return use;
}

//---------------------------------------------------------------------------
bool vtkMRMLTransformsDisplayableManager3D::vtkInternal::IsVisible(vtkMRMLTransformDisplayNode* displayNode)
{
  return displayNode && (displayNode->GetVisibility() != 0);
}
/*
//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::vtkInternal::UpdateAllDisplayableNodePipelines()
{
  // Update the Slice node transform

  PipelinesCacheType::iterator it;
  for (it = this->DisplayPipelines.begin(); it != this->DisplayPipelines.end(); ++it)
    {
    this->UpdateDisplayNodePipeline(it->first, it->second);
    }
}
*/

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::vtkInternal::UpdateDisplayableTransforms(vtkMRMLTransformNode* mNode)
{
  // Update the pipeline for all tracked DisplayableNode

  PipelinesCacheType::iterator pipelinesIter;
  std::set<vtkMRMLTransformDisplayNode *> displayNodes = this->ModelToDisplayNodes[mNode];
  std::set<vtkMRMLTransformDisplayNode *>::iterator dnodesIter;
  for ( dnodesIter = displayNodes.begin(); dnodesIter != displayNodes.end(); dnodesIter++ )
    {
    if ( ((pipelinesIter = this->DisplayPipelines.find(*dnodesIter)) != this->DisplayPipelines.end()) )
      {
      this->UpdateDisplayNodePipeline(pipelinesIter->first, pipelinesIter->second);
      }
    }
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::vtkInternal::RemoveDisplayNode(vtkMRMLTransformDisplayNode* displayNode)
{
  PipelinesCacheType::iterator actorsIt = this->DisplayPipelines.find(displayNode);
  if(actorsIt == this->DisplayPipelines.end())
    {
    return;
    }
  const Pipeline* pipeline = actorsIt->second;
  this->External->GetRenderer()->RemoveActor(pipeline->Actor);
  delete pipeline;
  this->DisplayPipelines.erase(actorsIt);
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::vtkInternal::AddDisplayNode(vtkMRMLTransformNode* mNode, vtkMRMLTransformDisplayNode* displayNode)
{
  if (!mNode || !displayNode)
    {
    return;
    }

  // Do not add the display node if it is already associated with a pipeline object.
  // This happens when a model node already associated with a display node
  // is copied into an other (using vtkMRMLNode::Copy()) and is added to the scene afterward.
  // Related issue are #3428 and #2608
  PipelinesCacheType::iterator it;
  it = this->DisplayPipelines.find(displayNode);
  if (it != this->DisplayPipelines.end())
    {
    return;
    }

  // Create pipeline
  Pipeline* pipeline = new Pipeline();

  pipeline->Actor = vtkSmartPointer<vtkActor>::New();
  vtkNew<vtkPolyDataMapper> mapper;
  pipeline->Actor->SetMapper(mapper.GetPointer());
  pipeline->Actor->SetVisibility(false);
  pipeline->InputPolyData = vtkSmartPointer<vtkPolyData>::New();
  mapper->SetInput(pipeline->InputPolyData);

  // Add actor to Renderer and local cache
  this->External->GetRenderer()->AddActor( pipeline->Actor );
  this->DisplayPipelines.insert( std::make_pair(displayNode, pipeline) );

  // Update cached matrices. Calls UpdateDisplayNodePipeline
  this->UpdateDisplayableTransforms(mNode);
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::vtkInternal::UpdateDisplayNode(vtkMRMLTransformDisplayNode* displayNode)
{
  // If the DisplayNode already exists, just update.
  //   otherwise, add as new node

  if (!displayNode)
    {
    return;
    }
  PipelinesCacheType::iterator it;
  it = this->DisplayPipelines.find(displayNode);
  if (it != this->DisplayPipelines.end())
    {
    this->UpdateDisplayNodePipeline(displayNode, it->second);
    }
  else
    {
    this->External->AddDisplayableNode( vtkMRMLTransformNode::SafeDownCast(displayNode->GetDisplayableNode()) );
    }
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::vtkInternal::UpdateDisplayNodePipeline(vtkMRMLTransformDisplayNode* displayNode, const Pipeline* pipeline)
{
  // Sets visibility, set pipeline polydata input, update color
  //   calculate and set pipeline transforms.

  if (!displayNode || !pipeline)
    {
    return;
    }

  vtkMRMLTransformNode* transformNode=vtkMRMLTransformNode::SafeDownCast(displayNode->GetDisplayableNode());
  if (transformNode==NULL)
    {
    pipeline->Actor->SetVisibility(false);
    return;
    }

  vtkMRMLNode* regionNode=displayNode->GetRegionNode();
  if (regionNode==NULL)
    {
    pipeline->Actor->SetVisibility(false);
    return;
    }

  // Update visibility
  bool visible = this->IsVisible(displayNode);
  pipeline->Actor->SetVisibility(visible);
  if (!visible)
    {
    return;
    }

  vtkNew<vtkMatrix4x4> ijkToRAS;
  int regionSize_IJK[3]={0};
  vtkMRMLSliceNode* sliceNode=vtkMRMLSliceNode::SafeDownCast(regionNode);
  vtkMRMLDisplayableNode* displayableNode=vtkMRMLDisplayableNode::SafeDownCast(regionNode);
  if (sliceNode!=NULL)
   {
    double pointSpacing=displayNode->GetGlyphSpacingMm();

    vtkMatrix4x4* sliceToRAS=sliceNode->GetSliceToRAS();
    double* fieldOfViewSize=sliceNode->GetFieldOfView();
    double* fieldOfViewOrigin=sliceNode->GetXYZOrigin();

    int numOfPointsX=floor(fieldOfViewSize[0]/pointSpacing+0.5);
    int numOfPointsY=floor(fieldOfViewSize[1]/pointSpacing+0.5);
    double xOfs = -fieldOfViewSize[0]/2+fieldOfViewOrigin[0];
    double yOfs = -fieldOfViewSize[1]/2+fieldOfViewOrigin[1];

    ijkToRAS->DeepCopy(sliceToRAS);
    vtkNew<vtkMatrix4x4> ijkOffset;
    ijkOffset->Element[0][3]=xOfs;
    ijkOffset->Element[1][3]=yOfs;
    vtkMatrix4x4::Multiply4x4(ijkToRAS.GetPointer(),ijkOffset.GetPointer(),ijkToRAS.GetPointer());
    vtkNew<vtkMatrix4x4> voxelSpacing;
    voxelSpacing->Element[0][0]=pointSpacing;
    voxelSpacing->Element[1][1]=pointSpacing;
    voxelSpacing->Element[2][2]=pointSpacing;
    vtkMatrix4x4::Multiply4x4(ijkToRAS.GetPointer(),voxelSpacing.GetPointer(),ijkToRAS.GetPointer());

    regionSize_IJK[0]=numOfPointsX;
    regionSize_IJK[1]=numOfPointsY;
    regionSize_IJK[2]=0;
    }
  else if (displayableNode!=NULL)
    {
    double bounds_RAS[6]={0};
    displayableNode->GetRASBounds(bounds_RAS);
    ijkToRAS->SetElement(0,3,bounds_RAS[0]);
    ijkToRAS->SetElement(1,3,bounds_RAS[2]);
    ijkToRAS->SetElement(2,3,bounds_RAS[4]);
    regionSize_IJK[0]=floor(bounds_RAS[1]-bounds_RAS[0]);
    regionSize_IJK[1]=floor(bounds_RAS[3]-bounds_RAS[2]);
    regionSize_IJK[2]=floor(bounds_RAS[5]-bounds_RAS[4]);
    }
  else
   {
    vtkWarningWithObjectMacro(displayNode, "Failed to show transform in 3D: unsupported ROI type");
    pipeline->Actor->SetVisibility(false);
    return;
    }
  TransformsDisplayableManagerHelper::GetVisualization3d(displayNode, pipeline->InputPolyData, ijkToRAS.GetPointer(), regionSize_IJK);

  //pipeline->Actor->Update();

  if (pipeline->InputPolyData->GetNumberOfPoints()==0)
    {
    pipeline->Actor->SetVisibility(false);
    return;
    }

  // Update pipeline actor
  this->External->SetModelDisplayProperty(displayNode, pipeline->Actor);
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::vtkInternal::AddObservations(vtkMRMLTransformNode* node)
{
  vtkEventBroker* broker = vtkEventBroker::GetInstance();
  if (!broker->GetObservationExist(node, vtkMRMLDisplayableNode::TransformModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() ))
    {
    broker->AddObservation(node, vtkMRMLDisplayableNode::TransformModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() );
    }
  if (!broker->GetObservationExist(node, vtkMRMLDisplayableNode::DisplayModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() ))
    {
    broker->AddObservation(node, vtkMRMLDisplayableNode::DisplayModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() );
    }
  if (!broker->GetObservationExist(node, vtkMRMLTransformableNode::TransformModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() ))
    {
    broker->AddObservation(node, vtkMRMLTransformableNode::TransformModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() );
    }
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::vtkInternal::RemoveObservations(vtkMRMLTransformNode* node)
{
  vtkEventBroker* broker = vtkEventBroker::GetInstance();
  vtkEventBroker::ObservationVector observations;
  observations = broker->GetObservations(node, vtkMRMLTransformableNode::TransformModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() );
  broker->RemoveObservations(observations);
  observations = broker->GetObservations(node, vtkMRMLDisplayableNode::DisplayModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() );
  broker->RemoveObservations(observations);
  observations = broker->GetObservations(node, vtkMRMLDisplayableNode::TransformModifiedEvent, this->External, this->External->GetMRMLNodesCallbackCommand() );
  broker->RemoveObservations(observations);
}

//---------------------------------------------------------------------------
bool vtkMRMLTransformsDisplayableManager3D::vtkInternal::IsNodeObserved(vtkMRMLTransformNode* node)
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
void vtkMRMLTransformsDisplayableManager3D::vtkInternal::ClearDisplayableNodes()
{
  while(this->ModelToDisplayNodes.size() > 0)
    {
    this->External->RemoveDisplayableNode(this->ModelToDisplayNodes.begin()->first);
    }
}

//---------------------------------------------------------------------------
bool vtkMRMLTransformsDisplayableManager3D::vtkInternal::UseDisplayableNode(vtkMRMLTransformNode* node)
{
  bool use = node && node->IsA("vtkMRMLTransformNode");
  return use;
}

//---------------------------------------------------------------------------
// vtkMRMLTransformsDisplayableManager3D methods

//---------------------------------------------------------------------------
vtkMRMLTransformsDisplayableManager3D::vtkMRMLTransformsDisplayableManager3D()
{
  this->Internal = new vtkInternal(this);
  this->AddingDisplayableNode = false;
}

//---------------------------------------------------------------------------
vtkMRMLTransformsDisplayableManager3D::~vtkMRMLTransformsDisplayableManager3D()
{
  delete this->Internal;
  this->Internal=NULL;
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::PrintSelf ( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf ( os, indent );
  os << indent << "vtkMRMLTransformsDisplayableManager3D: " << this->GetClassName() << "\n";
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::AddDisplayableNode(vtkMRMLTransformNode* node)
{
  if (this->AddingDisplayableNode)
    {
    return;
    }
  // Check if node should be used
  if (!this->Internal->UseDisplayableNode(node))
    {
    return;
    }

  this->AddingDisplayableNode = true;
  // Add Display Nodes
  int nnodes = node->GetNumberOfDisplayNodes();

  this->Internal->AddObservations(node);

  for (int i=0; i<nnodes; i++)
    {
    vtkMRMLTransformDisplayNode *dnode = vtkMRMLTransformDisplayNode::SafeDownCast(node->GetNthDisplayNode(i));
    if ( this->Internal->UseDisplayNode(dnode) )
      {
      this->Internal->ModelToDisplayNodes[node].insert(dnode);
      this->Internal->AddDisplayNode( node, dnode );
      }
    }
  this->AddingDisplayableNode = false;
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::RemoveDisplayableNode(vtkMRMLTransformNode* node)
{
  if (!node)
    {
    return;
    }
  vtkInternal::ModelToDisplayCacheType::iterator displayableIt =
    this->Internal->ModelToDisplayNodes.find(node);
  if(displayableIt == this->Internal->ModelToDisplayNodes.end())
    {
    return;
    }

  std::set<vtkMRMLTransformDisplayNode *> dnodes = displayableIt->second;
  std::set<vtkMRMLTransformDisplayNode *>::iterator diter;
  for ( diter = dnodes.begin(); diter != dnodes.end(); ++diter)
    {
    this->Internal->RemoveDisplayNode(*diter);
    }
  this->Internal->RemoveObservations(node);
  this->Internal->ModelToDisplayNodes.erase(displayableIt);
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if ( !node->IsA("vtkMRMLTransformNode") )
    {
    return;
    }

  // Escape if the scene a scene is being closed, imported or connected
  if (this->GetMRMLScene()->IsBatchProcessing())
    {
    this->SetUpdateFromMRMLRequested(1);
    return;
    }

  this->AddDisplayableNode(vtkMRMLTransformNode::SafeDownCast(node));
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if ( node
    && (!node->IsA("vtkMRMLTransformNode"))
    && (!node->IsA("vtkMRMLTransformDisplayNode")) )
    {
    return;
    }

  vtkMRMLTransformNode* modelNode = NULL;
  vtkMRMLTransformDisplayNode* displayNode = NULL;

  bool modified = false;
  if ( (modelNode = vtkMRMLTransformNode::SafeDownCast(node)) )
    {
    this->RemoveDisplayableNode(modelNode);
    modified = true;
    }
  else if ( (displayNode = vtkMRMLTransformDisplayNode::SafeDownCast(node)) )
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
void vtkMRMLTransformsDisplayableManager3D::ProcessMRMLNodesEvents(vtkObject *caller,
                                                           unsigned long event,
                                                           void *callData)
{
  vtkMRMLScene* scene = this->GetMRMLScene();

  if ( scene->IsBatchProcessing() )
    {
    return;
    }

  vtkMRMLTransformNode* displayableNode = vtkMRMLTransformNode::SafeDownCast(caller);

  if ( displayableNode )
    {
    vtkMRMLNode* callDataNode = reinterpret_cast<vtkMRMLDisplayNode *> (callData);
    vtkMRMLTransformDisplayNode* displayNode = vtkMRMLTransformDisplayNode::SafeDownCast(callDataNode);

    if ( displayNode && (event == vtkMRMLDisplayableNode::DisplayModifiedEvent) )
      {
      this->Internal->UpdateDisplayNode(displayNode);
      this->RequestRender();
      }
    else if ( (event == vtkMRMLDisplayableNode::TransformModifiedEvent)
             || (event == vtkMRMLTransformableNode::TransformModifiedEvent))
      {
      this->Internal->UpdateDisplayableTransforms(displayableNode);
      this->RequestRender();
      }
    }
  /*
  else if ( vtkMRMLSliceNode::SafeDownCast(caller) )
      {
      this->Internal->UpdateSliceNode();
      this->RequestRender();
      }
  */
  else
    {
    this->Superclass::ProcessMRMLNodesEvents(caller, event, callData);
    }


/*

  if (this->GetMRMLScene() == 0)
    {
    return;
    }
  if ( this->GetInteractor() &&
     this->GetInteractor()->GetRenderWindow() &&
     this->GetInteractor()->GetRenderWindow()->CheckInRenderStatus())
    {
    vtkDebugMacro("skipping ProcessMRMLNodesEvents during render");
    return;
    }

  bool isUpdating = this->GetMRMLScene()->IsBatchProcessing();
  if (vtkMRMLTransformNode::SafeDownCast(caller))
    {
    // There is no need to request a render (which can be expensive if the
    // volume rendering is on) if nothing visible has changed.
    bool requestRender = true;
    vtkMRMLTransformNode* displayableNode = vtkMRMLTransformNode::SafeDownCast(caller);
    switch (event)
      {
       case vtkMRMLDisplayableNode::DisplayModifiedEvent:
         // don't go any further if the modified display node is not a transform
         if (!this->IsTransformDisplayable(displayableNode) &&
             !this->IsTransformDisplayable(reinterpret_cast<vtkMRMLTransformDisplayNode*>(callData)))
          {
          requestRender = false;
          break;
          }
      case vtkCommand::ModifiedEvent:
      case vtkMRMLModelNode::PolyDataModifiedEvent:
        requestRender = this->OnMRMLDisplayableNodeModifiedEvent(displayableNode);
        break;
      default:
        this->SetUpdateFromMRMLRequested(1);
        break;
      }
    if (!isUpdating && requestRender)
      {
      this->RequestRender();
      }
    }
  else
    {
    this->Superclass::ProcessMRMLNodesEvents(caller, event, callData);
    }
    */
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::UpdateFromMRML()
{
  this->SetUpdateFromMRMLRequested(0);

  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene)
    {
    vtkDebugMacro( "vtkMRMLTransformsDisplayableManager3D->UpdateFromMRML: Scene is not set.")
    return;
    }
  this->Internal->ClearDisplayableNodes();

  vtkMRMLTransformNode* mNode = NULL;
  std::vector<vtkMRMLNode *> mNodes;
  int nnodes = scene ? scene->GetNodesByClass("vtkMRMLTransformNode", mNodes) : 0;
  for (int i=0; i<nnodes; i++)
    {
    mNode  = vtkMRMLTransformNode::SafeDownCast(mNodes[i]);
    if (mNode && this->Internal->UseDisplayableNode(mNode))
      {
      this->AddDisplayableNode(mNode);
      }
    }
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::UnobserveMRMLScene()
{
  this->Internal->ClearDisplayableNodes();
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::OnMRMLSceneStartClose()
{
  this->Internal->ClearDisplayableNodes();
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::OnMRMLSceneEndClose()
{
  // Clean
  /*
  this->RemoveProps();
  this->RemoveDisplayableObservers(1);

  this->SetUpdateFromMRMLRequested(1);
  this->RequestRender();
  */
  this->SetUpdateFromMRMLRequested(1);
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::OnMRMLSceneEndBatchProcess()
{
  this->SetUpdateFromMRMLRequested(1);
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::SetModelDisplayProperty(vtkMRMLTransformDisplayNode *displayNode, vtkActor* actor)
{
  bool visible = this->Internal->IsVisible(displayNode);
  actor->SetVisibility(visible);

  if (actor)
    {
    actor->GetMapper()->SetScalarVisibility(displayNode->GetScalarVisibility());
    // if the scalars are visible, set active scalars, try to get the lookup
    // table
    if (displayNode->GetScalarVisibility())
      {
      if (displayNode->GetColorNode() != 0)
        {
        vtkMRMLProceduralColorNode* proceduralColorNode =
          vtkMRMLProceduralColorNode::SafeDownCast(displayNode->GetColorNode());
        if (proceduralColorNode &&
            proceduralColorNode->GetColorTransferFunction() != 0)
          {
          // \tbd maybe the trick below should be applied here too
          vtkNew<vtkColorTransferFunction> ctf;
          ctf->DeepCopy(proceduralColorNode->GetColorTransferFunction());
          actor->GetMapper()->SetLookupTable(ctf.GetPointer());
          }
        else if (displayNode->GetColorNode()->GetLookupTable() != 0)
          {
          // \tbd: Could slow down if done too often
          // copy lut so that they are not shared between the mappers
          // vtk sets scalar range on lut while rendering
          // that may cause performance problem if lut's are shared
          vtkNew<vtkLookupTable> lut;
          lut->DeepCopy( displayNode->GetColorNode()->GetLookupTable());
          actor->GetMapper()->SetLookupTable(lut.GetPointer());
          }
        }

      actor->GetMapper()->SelectColorArray(TransformsDisplayableManagerHelper::GetDisplacementMagnitudeScalarName());
      // set the scalar range
      actor->GetMapper()->SetScalarRange(displayNode->GetScalarRange());

      actor->GetMapper()->SetScalarModeToUsePointData();
      //actor->GetMapper()->SetColorModeToMapScalars();

      actor->GetMapper()->UseLookupTableScalarRangeOff();
      actor->GetMapper()->SetScalarRange(displayNode->GetScalarRange());
      }
      //// }
    actor->GetProperty()->SetRepresentation(displayNode->GetRepresentation());
    actor->GetProperty()->SetPointSize(displayNode->GetPointSize());
    actor->GetProperty()->SetLineWidth(displayNode->GetLineWidth());
    actor->GetProperty()->SetLighting(displayNode->GetLighting());
    actor->GetProperty()->SetInterpolation(displayNode->GetInterpolation());
    actor->GetProperty()->SetShading(displayNode->GetShading());
    actor->GetProperty()->SetFrontfaceCulling(displayNode->GetFrontfaceCulling());
    actor->GetProperty()->SetBackfaceCulling(displayNode->GetBackfaceCulling());

    if (displayNode->GetSelected())
      {
      vtkDebugMacro("Model display node " << displayNode->GetName() << " is selected...");
      actor->GetProperty()->SetColor(displayNode->GetSelectedColor());
      actor->GetProperty()->SetAmbient(displayNode->GetSelectedAmbient());
      actor->GetProperty()->SetSpecular(displayNode->GetSelectedSpecular());
      }
    else
      {
      //vtkWarningMacro("Model display node " << displayNode->GetName() << " is not selected...");
      actor->GetProperty()->SetColor(displayNode->GetColor());
      actor->GetProperty()->SetAmbient(displayNode->GetAmbient());
      actor->GetProperty()->SetSpecular(displayNode->GetSpecular());
      }
    actor->GetProperty()->SetOpacity(displayNode->GetOpacity());
    actor->GetProperty()->SetDiffuse(displayNode->GetDiffuse());
    actor->GetProperty()->SetSpecularPower(displayNode->GetPower());
    actor->GetProperty()->SetEdgeVisibility(displayNode->GetEdgeVisibility());
    actor->GetProperty()->SetEdgeColor(displayNode->GetEdgeColor());

    actor->SetTexture(0);
    }
}

/*

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::UpdateFromMRMLScene()
{
  // UpdateFromMRML will be executed only if there has been some actions
  // during the import that requested it (don't call
  // SetUpdateFromMRMLRequested(1) here, it should be done somewhere else
  // maybe in OnMRMLSceneNodeAddedEvent, OnMRMLSceneNodeRemovedEvent or
  // OnMRMLDisplayableNodeModifiedEvent).
  this->RequestRender();
}

//---------------------------------------------------------------------------
bool vtkMRMLTransformsDisplayableManager3D::IsTransformDisplayable(vtkMRMLTransformNode* node)const
{
  vtkMRMLTransformNode* transformNode = vtkMRMLTransformNode::SafeDownCast(node);
  if (!node)
    {
    return false;
    }
  bool displayable = false;
  for (int i = 0; i < node->GetNumberOfDisplayNodes(); ++i)
    {
    displayable |= this->IsTransformDisplayable(node->GetNthDisplayNode(i));
    if (displayable)
      {// Optimization: no need to search any further.
      break;
      }
    }
  return displayable;
}

//---------------------------------------------------------------------------
bool vtkMRMLTransformsDisplayableManager3D::IsTransformDisplayable(vtkMRMLTransformDisplayNode* node)const
{
  vtkMRMLTransformDisplayNode* displayNode = vtkMRMLTransformDisplayNode::SafeDownCast(node);
  if (!displayNode)
    {
    return false;
    }

  vtkPolyData* cachedPolyData=this->Internal->DisplayPipelines[displayNode->GetID()].InputPolyData;
  return TransformsDisplayableManagerHelper::GetOutputPolyData(displayNode, cachedPolyData) ? true : false;
}

//---------------------------------------------------------------------------
bool vtkMRMLTransformsDisplayableManager3D::OnMRMLDisplayableNodeModifiedEvent(vtkMRMLTransformNode * transformNode)
{
  if (!this->IsTransformDisplayable(transformNode))
    {
    return false;
    }
  // If the node is already cached with an actor process only this one
  // If it was not visible and is still not visible do nothing
  int ndnodes = transformNode->GetNumberOfDisplayNodes();
  bool updateModel = false;
  bool updateMRML = false;
  for (int i=0; i<ndnodes; i++)
    {
    vtkMRMLTransformDisplayNode *dnode = transformNode->GetNthDisplayNode(i);
    assert(dnode);
    bool visible = (dnode->GetVisibility()) && this->IsTransformDisplayable(dnode);
    bool hasActor = this->Internal->DisplayPipelines.find(dnode->GetID()) != this->Internal->DisplayPipelines.end();
    // If the displayNode is visible and doesn't have actors yet, then request
    // an updated
    if (visible && !hasActor)
      {
      updateMRML = true;
      break;
      }
    // If the displayNode visibility has changed or displayNode is visible, then
    // update the model.
    if (!(!visible && this->GetDisplayedModelsVisibility(dnode) == 0))
      {
      updateModel = true;
      break;
      }
    }
  if (updateModel)
    {
    this->UpdateModifiedModel(transformNode);
    this->SetUpdateFromMRMLRequested(1);
    }
  if (updateMRML)
    {
    this->SetUpdateFromMRMLRequested(1);
    }
  return updateModel || updateMRML;
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::UpdateFromMRML()
{
  if ( this->GetInteractor() &&
       this->GetInteractor()->GetRenderWindow() &&
       this->GetInteractor()->GetRenderWindow()->CheckInRenderStatus())
    {
    vtkDebugMacro("skipping update during render");
    return;
    }

  this->RemoveProps();

  this->UpdateModelsFromMRML();

  this->SetUpdateFromMRMLRequested(0);
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::UpdateModelsFromMRML()
{
  vtkMRMLScene *scene = this->GetMRMLScene();

  // find volume slices
  bool clearDisplayedModels = scene ? false : true;

  std::vector<vtkMRMLNode *> dnodes;
  int nnodes = scene ? scene->GetNodesByClass("vtkMRMLTransformNode", dnodes) : 0;
  for (int n=0; n<nnodes; n++)
    {
    vtkMRMLNode *node = dnodes[n];
    vtkMRMLTransformNode *model = vtkMRMLTransformNode::SafeDownCast(node);
    }

  if (clearDisplayedModels)
    {
    std::map<std::string, vtkProp3D *>::iterator iter;
    for (iter = this->Internal->DisplayedActors.begin(); iter != this->Internal->DisplayedActors.end(); iter++)
      {
      this->GetRenderer()->RemoveViewProp(iter->second);
      }
    this->RemoveDisplayableObservers(1);
    this->Internal->DisplayedActors.clear();
    this->Internal->DisplayedNodes.clear();
    this->Internal->DisplayedVisibility.clear();
    }

  for (int n=0; n<nnodes; n++)
    {
    vtkMRMLTransformNode *model = vtkMRMLTransformNode::SafeDownCast(dnodes[n]);
    if (model)
      {
      this->UpdateModifiedModel(model);
      }
    }
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::UpdateModifiedModel(vtkMRMLTransformNode *model)
{
  this->UpdateModel(model);
  this->SetModelDisplayProperty(model);
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::UpdateModelPolyData(vtkMRMLTransformNode *displayableNode)
{
  int ndnodes = displayableNode->GetNumberOfDisplayNodes();

  // if no transform display nodes found, return
  int transformDisplayNodeCount = 0;
  for (int i=0; i<ndnodes; i++)
    {
    vtkMRMLTransformDisplayNode *dNode = displayableNode->GetNthDisplayNode(i);
    if (vtkMRMLTransformDisplayNode::SafeDownCast(dNode) != NULL)
      {
      transformDisplayNodeCount++;
      }
    }
  if (transformDisplayNodeCount == 0)
    {
    return;
    }

  vtkMRMLTransformNode* transformNode = vtkMRMLTransformNode::SafeDownCast(displayableNode);

  bool hasNonLinearTransform = false;
  vtkMRMLTransformNode* tnode = displayableNode->GetParentTransformNode();

  for (int i=0; i<ndnodes; i++)
    {
    vtkMRMLTransformDisplayNode *displayNode = vtkMRMLTransformDisplayNode::SafeDownCast(displayableNode->GetNthDisplayNode(i));
    if (displayNode == NULL)
      {
      continue;
      }

    int visibility = displayNode->GetVisibility();
    vtkPolyData *polyData = NULL;
    if (this->IsTransformDisplayable(displayNode))
      {
      vtkPolyData* cachedPolyData=this->Internal->DisplayPipelines[displayNode->GetID()].InputPolyData
      polyData = TransformsDisplayableManagerHelper::GetOutputPolyData(displayNode, cachedPolyData);
      }
    if (polyData == 0)
      {
      continue;
      }

    vtkProp3D* prop = 0;
    std::map<std::string, vtkProp3D *>::iterator ait;
    ait = this->Internal->DisplayedActors.find(displayNode->GetID());
    if (ait == this->Internal->DisplayedActors.end() )
      {
      if (!prop)
        {
        prop = vtkActor::New();
        }
      }
    else
      {
      prop = (*ait).second;
      }

    vtkActor * actor = vtkActor::SafeDownCast(prop);
    if(actor)
      {
      vtkNew<vtkPolyDataMapper> mapper;
      mapper->SetInput(polyData);
      actor->SetMapper(mapper.GetPointer());
      }

    if (ait == this->Internal->DisplayedActors.end())
      {
      this->GetRenderer()->AddViewProp(prop);
      this->Internal->DisplayedActors[displayNode->GetID()] = prop;
      this->Internal->DisplayedNodes[std::string(displayNode->GetID())] = displayNode;

      if (displayNode)
        {
        this->Internal->DisplayedVisibility[displayNode->GetID()] = visibility;
        }
      else
        {
        this->Internal->DisplayedVisibility[displayNode->GetID()] = 1;
        }

      prop->Delete();
      }
    else if (polyData == 0)
      {
      prop->Delete();
      }
    }
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::UpdateModel(vtkMRMLTransformNode *model)
{
  this->UpdateModelPolyData(model);

  vtkEventBroker *broker = vtkEventBroker::GetInstance();
  vtkEventBroker::ObservationVector observations;
  // observe polydata;
  if (!broker->GetObservationExist(model, vtkMRMLModelNode::PolyDataModifiedEvent,
                                         this, this->GetMRMLNodesCallbackCommand()))
    {
    broker->AddObservation(model, vtkMRMLModelNode::PolyDataModifiedEvent,
                           this, this->GetMRMLNodesCallbackCommand());
    this->Internal->DisplayableNodes[model->GetID()] = model;
    }
  // observe display node
  if (!broker->GetObservationExist(model, vtkMRMLDisplayableNode::DisplayModifiedEvent,
                                         this, this->GetMRMLNodesCallbackCommand()))
    {
    broker->AddObservation(model, vtkMRMLDisplayableNode::DisplayModifiedEvent,
                           this, this->GetMRMLNodesCallbackCommand());
    }

  if (!broker->GetObservationExist(model, vtkMRMLTransformableNode::TransformModifiedEvent,
                                         this, this->GetMRMLNodesCallbackCommand()))
    {
    broker->AddObservation(model, vtkMRMLTransformableNode::TransformModifiedEvent,
                           this, this->GetMRMLNodesCallbackCommand());
    }
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::RemoveProps()
{
  std::map<std::string, vtkProp3D *>::iterator iter;
  std::map<std::string, int>::iterator clipIter;
  std::vector<std::string> removedIDs;
  for(iter=this->Internal->DisplayedActors.begin(); iter != this->Internal->DisplayedActors.end(); iter++)
    {
    vtkMRMLTransformDisplayNode *displayNode = vtkMRMLTransformDisplayNode::SafeDownCast(
      this->GetMRMLScene() ? this->GetMRMLScene()->GetNodeByID(iter->first) : 0);
    if (displayNode == 0)
      {
      this->GetRenderer()->RemoveViewProp(iter->second);
      removedIDs.push_back(iter->first);
      }
    }
  for (unsigned int i=0; i< removedIDs.size(); i++)
    {
    this->RemoveDispalyedID(removedIDs[i]);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::RemoveDisplayable(vtkMRMLTransformNode* model)
{
  if (!model)
    {
    return;
    }
  const int ndnodes = model->GetNumberOfDisplayNodes();
  std::vector<std::string> removedIDs;
  for (int i=0; i<ndnodes; i++)
    {
    const char* displayNodeIDToRemove = model->GetNthDisplayNodeID(i);
    std::map<std::string, vtkProp3D *>::iterator iter =
      this->Internal->DisplayedActors.find(displayNodeIDToRemove);
    if (iter != this->Internal->DisplayedActors.end())
      {
      this->GetRenderer()->RemoveViewProp(iter->second);
      removedIDs.push_back(iter->first);
      }
    }

  for (unsigned int i=0; i< removedIDs.size(); i++)
    {
    this->RemoveDispalyedID(removedIDs[i]);
    }
  this->RemoveDisplayableNodeObservers(model);
  this->Internal->DisplayableNodes.erase(model->GetID());
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::RemoveDispalyedID(std::string &id)
{
  std::map<std::string, vtkMRMLTransformDisplayNode *>::iterator modelIter;
  this->Internal->DisplayedActors.erase(id);
  this->Internal->DisplayedVisibility.erase(id);
  modelIter = this->Internal->DisplayedNodes.find(id);
  if(modelIter != this->Internal->DisplayedNodes.end())
    {
    //this->RemoveDisplayableObservers(modelIter->second);
    this->Internal->DisplayedNodes.erase(modelIter->first);
    }
}

//---------------------------------------------------------------------------
int vtkMRMLTransformsDisplayableManager3D::GetDisplayedModelsVisibility(vtkMRMLTransformDisplayNode *model)
{
  int visibility = 0;

  std::map<std::string, int>::iterator iter;
  iter = this->Internal->DisplayedVisibility.find(model->GetID());
  if (iter != this->Internal->DisplayedVisibility.end())
    {
    visibility = iter->second;
    }

  return visibility;
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::RemoveMRMLObservers()
{
  this->RemoveDisplayableObservers(1);

  this->Superclass::RemoveMRMLObservers();
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::RemoveDisplayableObservers(int clearCache)
{
  std::map<std::string, vtkMRMLTransformNode *>::iterator iter;

  for (iter=this->Internal->DisplayableNodes.begin();
       iter!=this->Internal->DisplayableNodes.end();
       iter++)
    {
    this->RemoveDisplayableNodeObservers(iter->second);
    }
  if (clearCache)
    {
    this->Internal->DisplayableNodes.clear();
    this->Internal->DisplayedActors.clear();
    this->Internal->DisplayedNodes.clear();
    this->Internal->DisplayedVisibility.clear();
    }
}

//---------------------------------------------------------------------------
void vtkMRMLTransformsDisplayableManager3D::RemoveDisplayableNodeObservers(vtkMRMLTransformNode *model)
{
  vtkEventBroker *broker = vtkEventBroker::GetInstance();
  vtkEventBroker::ObservationVector observations;
  if (model != 0)
    {
    observations = broker->GetObservations(
      model, vtkMRMLModelNode::PolyDataModifiedEvent, this, this->GetMRMLNodesCallbackCommand() );
    broker->RemoveObservations(observations);
    observations = broker->GetObservations(
      model, vtkMRMLDisplayableNode::DisplayModifiedEvent, this, this->GetMRMLNodesCallbackCommand() );
    broker->RemoveObservations(observations);
    observations = broker->GetObservations(
      model, vtkMRMLTransformableNode::TransformModifiedEvent, this, this->GetMRMLNodesCallbackCommand() );
    broker->RemoveObservations(observations);
    }
}



*/