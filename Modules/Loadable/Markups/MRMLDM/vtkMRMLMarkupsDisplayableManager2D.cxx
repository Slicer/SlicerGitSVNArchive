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

// MarkupsModule/MRML includes
#include <vtkMRMLMarkupsDisplayNode.h>
#include <vtkMRMLMarkupsNode.h>

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsDisplayableManager2D.h"

// MRMLDisplayableManager includes
#include <vtkMRMLDisplayableManagerGroup.h>
#include <vtkMRMLModelDisplayableManager.h>

// MRML includes
#include <vtkMRMLApplicationLogic.h>
#include <vtkMRMLInteractionNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLSliceCompositeNode.h>
#include <vtkMRMLSliceLogic.h>
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLViewNode.h>

// VTK includes
#include <vtkAbstractWidget.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkGeneralTransform.h>
#include <vtkMarkupsGlyphSource2D.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPropCollection.h>
#include <vtkProperty2D.h>
#include <vtkRendererCollection.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSlicerAbstractRepresentation.h>
#include <vtkSlicerAbstractRepresentation2D.h>
#include <vtkSlicerAbstractWidget.h>
#include <vtkSphereSource.h>
#include <vtkTextProperty.h>
#include <vtkWidgetRepresentation.h>

// STD includes
#include <algorithm>
#include <map>
#include <vector>

typedef void (*fp)(void);

#define NUMERIC_ZERO 0.001

//---------------------------------------------------------------------------
vtkStandardNewMacro (vtkMRMLMarkupsDisplayableManager2D);

//---------------------------------------------------------------------------
vtkMRMLMarkupsDisplayableManager2D::vtkMRMLMarkupsDisplayableManager2D()
{
  this->Helper = vtkMRMLMarkupsDisplayableManagerHelper::New();
  this->DisableInteractorStyleEventsProcessing = 0;

  this->Focus = "vtkMRMLMarkupsNode";

  // by default, this displayableManager handles a 2d view, so the SliceNode
  // must be set when it's assigned to a viewer
  this->SliceNode = nullptr;

  // by default, multiply the display node scale by this when setting scale on elements in 2d windows
  this->ScaleFactor2D = 0.01667;

  this->LastClickWorldCoordinates[0]=0.0;
  this->LastClickWorldCoordinates[1]=0.0;
  this->LastClickWorldCoordinates[2]=0.0;
  this->LastClickWorldCoordinates[3]=1.0;
}

//---------------------------------------------------------------------------
vtkMRMLMarkupsDisplayableManager2D::~vtkMRMLMarkupsDisplayableManager2D()
{
  this->DisableInteractorStyleEventsProcessing = 0;
  this->Focus = nullptr;

  this->Helper->Delete();

  this->SliceNode = nullptr;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "DisableInteractorStyleEventsProcessing = " << this->DisableInteractorStyleEventsProcessing << std::endl;
  if (this->SliceNode &&
      this->SliceNode->GetID())
    {
    os << indent << "Slice node id = " << this->SliceNode->GetID() << std::endl;
    }
  else
    {
    os << indent << "No slice node" << std::endl;
    }
  if (this->Focus)
    {
    os << indent << "Focus = " << this->Focus << std::endl;
    }
  os << indent << "ScaleFactor2D = " << this->ScaleFactor2D << std::endl;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::SetAndObserveNode(vtkMRMLMarkupsNode *markupsNode)
{
  if (!markupsNode)
    {
    return;
    }
  vtkNew<vtkIntArray> nodeEvents;
  nodeEvents->InsertNextValue(vtkCommand::ModifiedEvent);
  nodeEvents->InsertNextValue(vtkMRMLMarkupsNode::PointModifiedEvent);
  nodeEvents->InsertNextValue(vtkMRMLMarkupsNode::PointAddedEvent);
  nodeEvents->InsertNextValue(vtkMRMLMarkupsNode::PointRemovedEvent);
  nodeEvents->InsertNextValue(vtkMRMLMarkupsNode::AllPointsRemovedEvent);
  nodeEvents->InsertNextValue(vtkMRMLMarkupsNode::LockModifiedEvent);
  nodeEvents->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
  nodeEvents->InsertNextValue(vtkMRMLDisplayableNode::DisplayModifiedEvent);

 if (markupsNode)// && !markupsNode->HasObserver(vtkMRMLTransformableNode::TransformModifiedEvent))
   {
   vtkUnObserveMRMLNodeMacro(markupsNode);
   vtkObserveMRMLNodeEventsMacro(markupsNode, nodeEvents.GetPointer());
   }
}
//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::SetAndObserveNodes()
{
  // run through all associated nodes
  vtkMRMLMarkupsDisplayableManagerHelper::MarkupsNodeListIt it;
  for(it = this->Helper->MarkupsNodeList.begin();
      it != this->Helper->MarkupsNodeList.end();
      ++it)
    {
    vtkMRMLMarkupsNode* markupsNode = vtkMRMLMarkupsNode::SafeDownCast((*it));
    this->SetAndObserveNode(markupsNode);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::RequestRender()
{
  if (!this->GetMRMLScene())
    {
    return;
    }
  if (!this->GetMRMLScene()->IsBatchProcessing())
    {
    this->Superclass::RequestRender();
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::RemoveMRMLObservers()
{
  // run through all associated nodes
  vtkMRMLMarkupsDisplayableManagerHelper::MarkupsNodeListIt it;
  it = this->Helper->MarkupsNodeList.begin();
  while(it != this->Helper->MarkupsNodeList.end())
    {
    vtkUnObserveMRMLNodeMacro(*it);
    ++it;
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::UpdateFromMRML()
{
  // this gets called from RequestRender, so make sure to jump out quickly if possible
  if (this->GetMRMLScene() == nullptr || this->Focus == nullptr)
    {
    return;
    }

  std::vector<vtkMRMLNode*> nodes;
  this->GetMRMLScene()->GetNodesByClass(this->Focus, nodes);

  // check if there are any of these nodes in the scene
  if (nodes.size() < 1)
    {
    return;
    }

  // turn off update from mrml requested, as we're doing it now, and create
  // widget requests a render which checks this flag before calling update
  // from mrml again
  this->SetUpdateFromMRMLRequested(0);

  // loop over the nodes for which this manager provides widgets
  for (std::vector< vtkMRMLNode* >::iterator nodeIt = nodes.begin(); nodeIt != nodes.end(); ++nodeIt)
    {
    vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(*nodeIt);
    if (markupsNode)
      {
      // do we  have a widget for it?
      if (this->GetWidget(markupsNode) == nullptr)
        {
        vtkDebugMacro("UpdateFromMRML: creating a widget for node " << markupsNode->GetID());
        vtkAbstractWidget *widget = this->AddWidget(markupsNode);
        if (!widget)
          {
          vtkErrorMacro("UpdateFromMRML: failed to create a widget for node " << markupsNode->GetID());
          }
        }
      this->OnMRMLMarkupsDisplayNodeModifiedEvent(markupsNode->GetDisplayNode());
      }
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::SetMRMLSceneInternal(vtkMRMLScene* newScene)
{
  Superclass::SetMRMLSceneInternal(newScene);

  // after a new scene got associated, we want to make sure everything old is gone
  this->OnMRMLSceneEndClose();

  if (newScene)
    {
    this->AddObserversToInteractionNode();
    }
  else
    {
    // there's no scene to get the interaction node from, so this won't do anything
    this->RemoveObserversFromInteractionNode();
    }
  vtkDebugMacro("SetMRMLSceneInternal: add observer on interaction node now?");
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D
::ProcessMRMLNodesEvents(vtkObject *caller,unsigned long event,void *callData)
{
  vtkMRMLMarkupsNode * markupsNode = vtkMRMLMarkupsNode::SafeDownCast(caller);
  vtkMRMLInteractionNode * interactionNode = vtkMRMLInteractionNode::SafeDownCast(caller);
  int *nPtr = nullptr;
  int n = -1;
  if (callData != nullptr)
    {
    nPtr = reinterpret_cast<int *>(callData);
    if (nPtr)
      {
      n = *nPtr;
      }
    }
  if (markupsNode)
    {
    switch(event)
      {
      case vtkCommand::ModifiedEvent:
        this->OnMRMLMarkupsNodeModifiedEvent(markupsNode);
        break;
      case vtkMRMLMarkupsNode::PointModifiedEvent:
        this->OnMRMLMarkupsNthPointModifiedEvent(markupsNode, n);
        break;
      case vtkMRMLMarkupsNode::PointAddedEvent:
        this->OnMRMLMarkupsPointAddedEvent(markupsNode, n);
        break;
      case vtkMRMLMarkupsNode::PointRemovedEvent:
        this->OnMRMLMarkupsPointRemovedEvent(markupsNode, n);
        break;
      case vtkMRMLMarkupsNode::AllPointsRemovedEvent:
        this->OnMRMLMarkupsAllPointsRemovedEvent(markupsNode);
        break;
      case vtkMRMLTransformableNode::TransformModifiedEvent:
        this->OnMRMLMarkupsNodeTransformModifiedEvent(markupsNode);
        break;
      case vtkMRMLMarkupsNode::LockModifiedEvent:
        this->OnMRMLMarkupsNodeLockModifiedEvent(markupsNode);
        break;
      case vtkMRMLDisplayableNode::DisplayModifiedEvent:
        // get the display node and process the change
        vtkMRMLNode *displayNode = markupsNode->GetDisplayNode();
        this->OnMRMLMarkupsDisplayNodeModifiedEvent(displayNode);
        break;
      }
    }
  else if (interactionNode)
    {
    if (event == vtkMRMLInteractionNode::InteractionModeChangedEvent)
      {
      this->Helper->UpdateAllWidgetsFromInteractionNode(interactionNode);
      }
    }
  else
    {
    this->Superclass::ProcessMRMLNodesEvents(caller, event, callData);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::OnMRMLSceneEndClose()
{
  vtkDebugMacro("OnMRMLSceneEndClose: remove observers?");
  // run through all nodes and remove node and widget
  this->Helper->RemoveAllWidgetsAndNodes();

  this->SetUpdateFromMRMLRequested(1);
  this->RequestRender();

}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::OnMRMLSceneEndImport()
{
  this->SetUpdateFromMRMLRequested(1);
  this->UpdateFromMRMLScene();
  this->Helper->SetAllWidgetsToManipulate();
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::UpdateFromMRMLScene()
{
  if (this->GetMRMLSliceNode())
    {
    this->UpdateFromMRML();
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
    {
    return;
    }

  vtkDebugMacro("OnMRMLSceneNodeAddedEvent");

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
    {
    return;
    }

  if (node->IsA("vtkMRMLInteractionNode"))
    {
    this->AddObserversToInteractionNode();
    return;
    }

  if (node->IsA("vtkMRMLMarkupsDisplayNode"))
    {
    // have a display node, need to observe it
    vtkObserveMRMLNodeMacro(node);
    return;
    }

  vtkMRMLMarkupsNode * markupsNode = vtkMRMLMarkupsNode::SafeDownCast(node);
  if (!markupsNode)
    {
    return;
    }

  vtkDebugMacro("OnMRMLSceneNodeAddedEvent:  node " << node->GetID());

  // Node added should not be already managed
  vtkMRMLMarkupsDisplayableManagerHelper::MarkupsNodeListIt it = std::find(
      this->Helper->MarkupsNodeList.begin(),
      this->Helper->MarkupsNodeList.end(),
      markupsNode);
  if (it != this->Helper->MarkupsNodeList.end())
    {
    vtkErrorMacro("OnMRMLSceneNodeAddedEvent: This node is already associated to the displayable manager!")
    return;
    }

  // There should not be a widget for the new node
  if (this->Helper->GetWidget(markupsNode) != nullptr)
    {
    vtkErrorMacro("OnMRMLSceneNodeAddedEvent: A widget is already associated to this node!");
    return;
    }

  // Create the Widget and add it to the list.
  vtkAbstractWidget* newWidget = this->AddWidget(markupsNode);
  if (!newWidget)
    {
    return;
    }
  else
    {
    vtkDebugMacro("OnMRMLSceneNodeAddedEvent: widget was created, saved to helper Widgets map");
    }

  // and render again
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::AddObserversToInteractionNode()
{
  if (!this->GetMRMLScene())
    {
    return;
    }
  // also observe the interaction node for changes
  vtkMRMLInteractionNode *interactionNode = this->GetInteractionNode();
  if (interactionNode)
    {
    vtkDebugMacro("AddObserversToInteractionNode: interactionNode found");
    vtkNew<vtkIntArray> interactionEvents;
    interactionEvents->InsertNextValue(vtkMRMLInteractionNode::InteractionModeChangedEvent);
    interactionEvents->InsertNextValue(vtkMRMLInteractionNode::InteractionModePersistenceChangedEvent);
    interactionEvents->InsertNextValue(vtkMRMLInteractionNode::EndPlacementEvent);
    vtkObserveMRMLNodeEventsMacro(interactionNode, interactionEvents.GetPointer());
    }
  else { vtkDebugMacro("AddObserversToInteractionNode: No interaction node!"); }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::RemoveObserversFromInteractionNode()
{
  if (!this->GetMRMLScene())
    {
    return;
    }

  // find the interaction node
  vtkMRMLInteractionNode *interactionNode =  this->GetInteractionNode();
  if (interactionNode)
    {
    vtkUnObserveMRMLNodeMacro(interactionNode);
  }
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsDisplayableManager2D::IsPointDisplayableOnSlice(vtkMRMLMarkupsNode *node, int pointIndex)
{
  if (this->IsInLightboxMode())
    {
    // TBD: issue 1690: disable angle in light box mode as they appear
    // in the wrong location
    return false;
    }

  vtkMRMLSliceNode* sliceNode = this->GetMRMLSliceNode();
  // if no slice node, it doesn't constrain the visibility, so return that
  // it's visible
  if (!sliceNode)
    {
    vtkErrorMacro("IsWidgetDisplayableOnSlice: Could not get the sliceNode.")
    return 1;
    }

  // if there's no node, it's not visible
  if (!node)
    {
    vtkErrorMacro("IsWidgetDisplayableOnSlice: Could not get the markups node.")
    return 0;
    }

  bool showPoint = true;

  // allow annotations to appear only in designated viewers
  vtkMRMLDisplayNode *displayNode = node->GetDisplayNode();
  if (displayNode && !displayNode->IsDisplayableInView(sliceNode->GetID()))
    {
    return 0;
    }

  // down cast the node as a controlpoints node to get the coordinates
  vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(node);
  if (!markupsNode)
    {
    vtkErrorMacro("IsWidgetDisplayableOnSlice: Could not get the controlpoints node.")
    return 0;
    }

  double transformedWorldCoordinates[4];
  markupsNode->GetNthControlPointPositionWorld(pointIndex, transformedWorldCoordinates);

  // now get the displayCoordinates for the transformed worldCoordinates
  double displayCoordinates[4];
  this->GetWorldToDisplayCoordinates(transformedWorldCoordinates,displayCoordinates);

  if (this->IsInLightboxMode())
    {
    //
    // Lightbox specific code
    //
    // get the corresponding lightbox index for this display coordinate and
    // check if it's in the range of the current number of light boxes being
    // displayed in the grid rows/columns.
    int lightboxIndex = this->GetLightboxIndex(markupsNode, pointIndex);
    int numberOfLightboxes = sliceNode->GetLayoutGridColumns() * sliceNode->GetLayoutGridRows();
    if (lightboxIndex < 0 ||
        lightboxIndex >= numberOfLightboxes)
      {
      showPoint = false;
      }
    }
  // check if the point is close enough to the slice to be shown
  if (showPoint)
    {
    if (this->IsInLightboxMode())
      {
      // get the volume's spacing to determine the distance between the slice
      // location and the markup
      // default to spacing 1.0 in case can't get volume slice spacing from
      // the logic as that will be a multiplicative no-op
      double spacing = 1.0;
      vtkMRMLSliceLogic *sliceLogic = nullptr;
      vtkMRMLApplicationLogic *mrmlAppLogic = this->GetMRMLApplicationLogic();
      if (mrmlAppLogic)
        {
        sliceLogic = mrmlAppLogic->GetSliceLogic(sliceNode);
        }
      if (sliceLogic)
        {
        double *volumeSliceSpacing = sliceLogic->GetLowestVolumeSliceSpacing();
        if (volumeSliceSpacing != nullptr)
          {
          vtkDebugMacro("Slice node " << sliceNode->GetName()
                        << ": volumeSliceSpacing = "
                        << volumeSliceSpacing[0] << ", "
                        << volumeSliceSpacing[1] << ", "
                        << volumeSliceSpacing[2]);
          spacing = volumeSliceSpacing[2];
          }
        }
      vtkDebugMacro("displayCoordinates: "
                    << displayCoordinates[0] << ","
                    << displayCoordinates[1] << ","
                    << displayCoordinates[2] << "\n\tworld coords: "
                    << transformedWorldCoordinates[0] << ","
                    << transformedWorldCoordinates[1] << ","
                    << transformedWorldCoordinates[2]);
      // calculate the distance from the point in world space to the
      // plane defined by the slice node normal and origin (using same
      // convention as the vtkMRMLThreeDReformatDisplayableManager)
      vtkMatrix4x4 *sliceToRAS = sliceNode->GetSliceToRAS();
      double slicePlaneNormal[3], slicePlaneOrigin[3];
      slicePlaneNormal[0] = sliceToRAS->GetElement(0,2);
      slicePlaneNormal[1] = sliceToRAS->GetElement(1,2);
      slicePlaneNormal[2] = sliceToRAS->GetElement(2,2);
      slicePlaneOrigin[0] = sliceToRAS->GetElement(0,3);
      slicePlaneOrigin[1] = sliceToRAS->GetElement(1,3);
      slicePlaneOrigin[2] = sliceToRAS->GetElement(2,3);
      double distanceToPlane = slicePlaneNormal[0]*(transformedWorldCoordinates[0]-slicePlaneOrigin[0]) +
        slicePlaneNormal[1]*(transformedWorldCoordinates[1]-slicePlaneOrigin[1]) +
        slicePlaneNormal[2]*(transformedWorldCoordinates[2]-slicePlaneOrigin[2]);
      // this gives the distance to light box plane 0, but have to offset by
      // number of light box planes (as determined by the light box index) times the volume
      // slice spacing
      int lightboxIndex = this->GetLightboxIndex(markupsNode, pointIndex);
      double lightboxOffset = lightboxIndex * spacing;
      double distanceToSlice = distanceToPlane - lightboxOffset;
      double maxDistance = 0.5;
      vtkDebugMacro("\n\tdistance to plane = " << distanceToPlane
                    << "\n\tlightboxIndex = " << lightboxIndex
                    << "\n\tlightboxOffset = " << lightboxOffset
                    << "\n\tdistance to slice = " << distanceToSlice);
      // check that it's within 0.5mm
      if (distanceToSlice < -0.5 || distanceToSlice >= maxDistance)
        {
        vtkDebugMacro("Distance to slice is greater than max distance, not showing the widget");
        showPoint = false;
        }
      }
    else
      {
      // the third coordinate of the displayCoordinates is the distance to the slice
      double distanceToSlice = displayCoordinates[2];
      double maxDistance = 0.5 + (sliceNode->GetDimensions()[2] - 1);
      vtkDebugMacro("Slice node " << sliceNode->GetName()
                    << ": distance to slice = " << distanceToSlice
                    << ", maxDistance = " << maxDistance
                    << "\n\tslice node dimensions[2] = "
                    << sliceNode->GetDimensions()[2]);
      if (distanceToSlice < -0.5 || distanceToSlice >= maxDistance)
        {
        // if the distance to the slice is more than 0.5mm, we know that at least one coordinate of the widget is outside the current activeSlice
        // hence, we do not want to show this widget
        showPoint = false;
        }
      }
    }

  return showPoint;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  vtkDebugMacro("OnMRMLSceneNodeRemovedEvent");
  vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(node);
  if (!markupsNode)
    {
    return;
    }

  // Remove the widget and the MRMLnode from the internal lists.
  this->Helper->RemoveWidgetAndNode(markupsNode);

  // Refresh observers
  vtkUnObserveMRMLNodeMacro(markupsNode);

  // and render again
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::OnMRMLMarkupsNodeModifiedEvent(vtkMRMLNode* node)
{
  vtkDebugMacro("OnMRMLMarkupsNodeModifiedEvent");

  vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(node);
  if (!markupsNode)
    {
    vtkErrorMacro("OnMRMLMarkupsNodeModifiedEvent: Can not access node.")
    return;
    }

  vtkSlicerAbstractWidget *widget = this->Helper->GetWidget(markupsNode);
  if (!widget)
    {
    return;
    }

  // Points widgets have only one Markup/Representation
  vtkSlicerAbstractRepresentation2D *rep = vtkSlicerAbstractRepresentation2D::SafeDownCast
    (widget->GetRepresentation());
  if (!rep)
    {
    return;
    }

  for (int PointIndex = 0; PointIndex < markupsNode->GetNumberOfControlPoints(); PointIndex++)
    {
    bool visibility = this->IsPointDisplayableOnSlice(markupsNode, PointIndex);
    rep->SetNthPointSliceVisibility(PointIndex, visibility);
    }

  widget->BuildRepresentation();

  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::OnMRMLMarkupsDisplayNodeModifiedEvent(vtkMRMLNode* node)
{
  if (!node)
    {
    return;
    }

  vtkMRMLMarkupsDisplayNode *markupsDisplayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast(node);
  if (!markupsDisplayNode)
    {
    vtkErrorMacro("OnMRMLMarkupsDisplayNodeModifiedEvent: Can not access node.")
    return;
    }

  // find the markups node that has this display node
  vtkMRMLMarkupsNode *markupsNode = this->Helper->GetMarkupsNodeFromDisplayNode(markupsDisplayNode);
  if (!markupsNode)
    {
    return;
    }

  vtkDebugMacro("OnMRMLMarkupsDisplayNodeModifiedEvent: found the markups node "
                << markupsNode->GetID()
                << " associated with the modified display node "
                << markupsDisplayNode->GetID());

  vtkSlicerAbstractWidget * widget = this->Helper->GetWidget(markupsNode);

  if (widget)
    {
    vtkSlicerAbstractRepresentation2D *rep =
      vtkSlicerAbstractRepresentation2D::SafeDownCast(widget->GetRepresentation());
    if (rep)
      {
      if (markupsDisplayNode->GetVisibility() && markupsDisplayNode->IsDisplayableInView(this->GetMRMLSliceNode()->GetID()))
        {
        rep->VisibilityOn();
        vtkProperty2D *prop = rep->GetProperty();
        if (prop)
          {
          prop->SetColor(markupsDisplayNode->GetColor());
          prop->SetOpacity(markupsDisplayNode->GetOpacity());
          }
        vtkProperty2D *selectedProp = rep->GetSelectedProperty();
        if (selectedProp)
          {
          selectedProp->SetColor(markupsDisplayNode->GetSelectedColor());
          selectedProp->SetOpacity(markupsDisplayNode->GetOpacity());
          }
        vtkProperty2D *activeProp = rep->GetActiveProperty();
        if (activeProp)
          {
          // bright green
          activeProp->SetColor(0.4, 1.0, 0.);
          activeProp->SetOpacity(markupsDisplayNode->GetOpacity());
          }

        if (markupsDisplayNode->GlyphTypeIs3D())
          {
          vtkNew<vtkSphereSource> ss;
          ss->SetRadius(0.5);
          ss->Update();
          rep->SetCursorShape(ss->GetOutput());
          rep->SetSelectedCursorShape(ss->GetOutput());
          rep->SetActiveCursorShape(ss->GetOutput());
          }
        else
          {
          vtkNew<vtkMarkupsGlyphSource2D> glyphSource;
          glyphSource->SetGlyphType(markupsDisplayNode->GetGlyphType());
          glyphSource->Update();
          rep->SetCursorShape(glyphSource->GetOutput());
          rep->SetSelectedCursorShape(glyphSource->GetOutput());
          rep->SetActiveCursorShape(glyphSource->GetOutput());
          }

        rep->SetHandleSize(markupsDisplayNode->GetGlyphScale() * this->ScaleFactor2D);

        vtkTextProperty *textProp = rep->GetTextProperty();
        if (textProp)
          {
          textProp->SetColor(markupsDisplayNode->GetColor());
          textProp->SetOpacity(markupsDisplayNode->GetOpacity());
          textProp->SetFontSize(static_cast<int>(5. * markupsDisplayNode->GetTextScale()));
          }
        vtkTextProperty *selectedTextProp = rep->GetSelectedTextProperty();
        if (selectedTextProp)
          {
          selectedTextProp->SetColor(markupsDisplayNode->GetSelectedColor());
          selectedTextProp->SetOpacity(markupsDisplayNode->GetOpacity());
          selectedTextProp->SetFontSize(static_cast<int>(5. * markupsDisplayNode->GetTextScale()));
          }
        vtkTextProperty *activeTextProp = rep->GetActiveTextProperty();
        if (activeTextProp)
          {
          // bright green
          activeTextProp->SetColor(0.4, 1.0, 0.);
          activeTextProp->SetOpacity(markupsDisplayNode->GetOpacity());
          activeTextProp->SetFontSize(static_cast<int>(5. * markupsDisplayNode->GetTextScale()));
          }
        }
      else
        {
        rep->VisibilityOff();
        }
      }
    // Rebuild representation
    widget->BuildRepresentation();
    this->RequestRender();
  }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::OnMRMLMarkupsNthPointModifiedEvent(vtkMRMLNode *node, int n)
{
  vtkDebugMacro("OnMRMLMarkupsNthPointModifiedEvent");
  if (!node)
    {
    return;
    }

  vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(node);
  if (!markupsNode)
    {
    return;
    }

  vtkSlicerAbstractWidget * widget = this->Helper->GetWidget(markupsNode);
  if (!widget)
    {
    return;
    }

  // Check if the Point is on the slice
  bool visibility = this->IsPointDisplayableOnSlice(markupsNode, n);
  vtkSlicerAbstractRepresentation2D *rep = vtkSlicerAbstractRepresentation2D::SafeDownCast
    (widget->GetRepresentation());
  if (rep && rep->GetVisibility())
    {
    rep->SetNthPointSliceVisibility(n, visibility);
    }

  // Rebuild representation
  widget->BuildRepresentation();
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::OnMRMLMarkupsPointAddedEvent(vtkMRMLNode *node, int vtkNotUsed(n))
{
  vtkDebugMacro("OnMRMLMarkupsPointAddedEvent");
  if (!node)
    {
    return;
    }

  vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(node);
  if (!markupsNode)
    {
    return;
    }

  vtkSlicerAbstractWidget *widget = this->Helper->GetWidget(markupsNode);
  if (!widget)
    {
    return;
    }

  if (this->GetInteractionNode()->GetCurrentInteractionMode() != vtkMRMLInteractionNode::Place)
    {
    // The point has not been added by clicks.
    // If the user has never interacted with the widget:
    // set the widget to manipulate and the placing ended for the markups.
    widget->SetWidgetState(vtkSlicerAbstractWidget::Manipulate);
    }

  // widgets have only one Markup/Representation
  vtkSlicerAbstractRepresentation2D *rep = vtkSlicerAbstractRepresentation2D::SafeDownCast
    (widget->GetRepresentation());
  if (!rep)
    {
    return;
    }

  for (int PointIndex = 0; PointIndex < markupsNode->GetNumberOfControlPoints(); PointIndex++)
    {
    bool visibility = this->IsPointDisplayableOnSlice(markupsNode, PointIndex);
    rep->SetNthPointSliceVisibility(PointIndex, visibility);
    }

  // Rebuild representation
  widget->BuildRepresentation();
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::OnMRMLMarkupsPointRemovedEvent(vtkMRMLNode *node, int vtkNotUsed(n))
{
  vtkDebugMacro("OnMRMLMarkupsPointRemovedEvent");
  if (!node)
    {
    return;
    }

  vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(node);
  if (!markupsNode)
    {
    return;
    }

  vtkSlicerAbstractWidget *widget = this->Helper->GetWidget(markupsNode);
  if (widget)
    {
    // Rebuild representation
    widget->BuildRepresentation();
    this->RequestRender();
  }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::OnMRMLMarkupsAllPointsRemovedEvent(vtkMRMLNode *node)
{
  vtkDebugMacro("OnMRMLMarkupsAllPointsRemovedEvent");
  if (!node)
    {
    return;
    }

  vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(node);
  if (!markupsNode)
    {
    return;
    }

  vtkSlicerAbstractWidget *widget = this->Helper->GetWidget(markupsNode);
  if (widget)
    {
    // Rebuild representation
    widget->BuildRepresentation();
    this->RequestRender();
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::OnMRMLMarkupsNodeTransformModifiedEvent(vtkMRMLNode* node)
{
  vtkDebugMacro("OnMRMLMarkupsNodeTransformModifiedEvent");
  if (!node)
    {
    return;
    }
  vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(node);
  if (!markupsNode)
    {
    vtkErrorMacro("OnMRMLMarkupsNodeTransformModifiedEvent - Can not access node.")
    return;
    }

  vtkSlicerAbstractWidget * widget = this->Helper->GetWidget(markupsNode);

  if (widget)
    {
    // Rebuild representation
    widget->BuildRepresentation();
    this->RequestRender();
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::OnMRMLMarkupsNodeLockModifiedEvent(vtkMRMLNode* node)
{
  vtkDebugMacro("OnMRMLMarkupsNodeLockModifiedEvent");
  vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(node);
  if (!markupsNode)
    {
    vtkErrorMacro("OnMRMLMarkupsNodeLockModifiedEvent - Can not access node.")
    return;
    }
  // Update the standard settings of all widgets.
  this->Helper->UpdateLocked(markupsNode);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::OnMRMLDisplayableNodeModifiedEvent(vtkObject* caller)
{
  vtkDebugMacro("OnMRMLDisplayableNodeModifiedEvent");

  if (!caller)
    {
    vtkErrorMacro("OnMRMLDisplayableNodeModifiedEvent: Could not get caller.")
    return;
    }

  vtkMRMLSliceNode * sliceNode = vtkMRMLSliceNode::SafeDownCast(caller);
  if (sliceNode)
    {
    // the associated renderWindow is a 2D SliceView
    // this is the entry point for all events fired by one of the three sliceviews
    // (f.e. change slice number, zoom etc.)

    // we remember that this instance of the displayableManager deals with 2D
    // this is important for widget creation etc. and save the actual SliceNode
    // because during Slicer startup the SliceViews fire events, it will be always set correctly
    this->SliceNode = sliceNode;

    // now we call the handle for specific sliceNode actions
    this->OnMRMLSliceNodeModifiedEvent();

    // and exit
    return;
    }

  vtkMRMLViewNode * viewNode = vtkMRMLViewNode::SafeDownCast(caller);
  if (viewNode)
    {
    // the associated renderWindow is a 3D View
    vtkDebugMacro("OnMRMLDisplayableNodeModifiedEvent: This displayableManager handles a ThreeD view.")
    return;
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::OnMRMLSliceNodeModifiedEvent()
{
  // run through all markup nodes in the helper
  vtkMRMLMarkupsDisplayableManagerHelper::MarkupsNodeListIt it;
  it = this->Helper->MarkupsNodeList.begin();
  bool requestRender = false;
  while(it != this->Helper->MarkupsNodeList.end())
    {
    // we loop through all nodes
    vtkMRMLMarkupsNode * markupsNode = *it;

    vtkSlicerAbstractWidget* widget = this->Helper->GetWidget(markupsNode);
    if (widget)
      {
      // Rebuild representation
      vtkSlicerAbstractRepresentation2D *rep = vtkSlicerAbstractRepresentation2D::SafeDownCast
        (widget->GetRepresentation());
      if (!rep)
        {
        return;
        }

      for (int PointIndex = 0; PointIndex < markupsNode->GetNumberOfControlPoints(); PointIndex++)
        {
        bool visibility = this->IsPointDisplayableOnSlice(markupsNode, PointIndex);
        rep->SetNthPointSliceVisibility(PointIndex, visibility);
        }
      widget->BuildRepresentation();
      requestRender = true;
      }

    ++it;
    }

  if (requestRender)
    {
    this->RequestRender();
    }
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsDisplayableManager2D::IsWidgetDisplayableOnSlice(vtkMRMLMarkupsNode* node)
{
  if (this->IsInLightboxMode())
    {
    // TBD: issue 1690: disable fiducials in light box mode as they appear
    // in the wrong location
    return false;
    }

  vtkMRMLSliceNode* sliceNode = this->GetMRMLSliceNode();
  // if no slice node, it doesn't constrain the visibility, so return that
  // it's visible
  if (!sliceNode)
    {
    vtkErrorMacro("IsWidgetDisplayableOnSlice: Could not get the sliceNode.")
    return 1;
    }

  // if there's no node, it's not visible
  if (!node)
    {
    vtkErrorMacro("IsWidgetDisplayableOnSlice: Could not get the markups node.")
    return 0;
    }

  bool showWidget = true;
  bool inViewport = false;

  // allow annotations to appear only in designated viewers
  vtkMRMLDisplayNode *displayNode = node->GetDisplayNode();
  if (displayNode && !displayNode->IsDisplayableInView(sliceNode->GetID()))
    {
    return 0;
    }

  // down cast the node as a controlpoints node to get the coordinates
  vtkMRMLMarkupsNode * controlPointsNode = vtkMRMLMarkupsNode::SafeDownCast(node);

  if (!controlPointsNode)
    {
    vtkErrorMacro("IsWidgetDisplayableOnSlice: Could not get the controlpoints node.")
    return 0;
    }

  int numberOfControlPoints = controlPointsNode->GetNumberOfControlPoints();
  for (int i=0; i < numberOfControlPoints; i++)
    {
    // we loop through all controlpoints of each node
    double transformedWorldCoordinates[4];
    controlPointsNode->GetNthControlPointPositionWorld(i, transformedWorldCoordinates);

    // now get the displayCoordinates for the transformed worldCoordinates
    double displayCoordinates[4];
    this->GetWorldToDisplayCoordinates(transformedWorldCoordinates,displayCoordinates);

    if (this->IsInLightboxMode())
      {
      //
      // Lightbox specific code
      //
      // get the corresponding lightbox index for this display coordinate and
      // check if it's in the range of the current number of light boxes being
      // displayed in the grid rows/columns.
      int lightboxIndex = this->GetLightboxIndex(controlPointsNode, i);
      int numberOfLightboxes = sliceNode->GetLayoutGridColumns() * sliceNode->GetLayoutGridRows();
      if (lightboxIndex < 0 ||
          lightboxIndex >= numberOfLightboxes)
        {
        showWidget = false;
        }
      }
    // check if the markup is close enough to the slice to be shown
    if (showWidget)
      {
      if (this->IsInLightboxMode())
        {
        // get the volume's spacing to determine the distance between the slice
        // location and the markup
        // default to spacing 1.0 in case can't get volume slice spacing from
        // the logic as that will be a multiplicative no-op
        double spacing = 1.0;
        vtkMRMLSliceLogic *sliceLogic = nullptr;
        vtkMRMLApplicationLogic *mrmlAppLogic = this->GetMRMLApplicationLogic();
        if (mrmlAppLogic)
          {
          sliceLogic = mrmlAppLogic->GetSliceLogic(sliceNode);
          }
        if (sliceLogic)
          {
          double *volumeSliceSpacing = sliceLogic->GetLowestVolumeSliceSpacing();
          if (volumeSliceSpacing != nullptr)
            {
            vtkDebugMacro("Slice node " << sliceNode->GetName()
                          << ": volumeSliceSpacing = "
                          << volumeSliceSpacing[0] << ", "
                          << volumeSliceSpacing[1] << ", "
                          << volumeSliceSpacing[2]);
            spacing = volumeSliceSpacing[2];
            }
          }
        vtkDebugMacro("displayCoordinates: "
                      << displayCoordinates[0] << ","
                      << displayCoordinates[1] << ","
                      << displayCoordinates[2] << "\n\tworld coords: "
                      << transformedWorldCoordinates[0] << ","
                      << transformedWorldCoordinates[1] << ","
                      << transformedWorldCoordinates[2]);
        // calculate the distance from the point in world space to the
        // plane defined by the slice node normal and origin (using same
        // convention as the vtkMRMLThreeDReformatDisplayableManager)
        vtkMatrix4x4 *sliceToRAS = sliceNode->GetSliceToRAS();
        double slicePlaneNormal[3], slicePlaneOrigin[3];
        slicePlaneNormal[0] = sliceToRAS->GetElement(0,2);
        slicePlaneNormal[1] = sliceToRAS->GetElement(1,2);
        slicePlaneNormal[2] = sliceToRAS->GetElement(2,2);
        slicePlaneOrigin[0] = sliceToRAS->GetElement(0,3);
        slicePlaneOrigin[1] = sliceToRAS->GetElement(1,3);
        slicePlaneOrigin[2] = sliceToRAS->GetElement(2,3);
        double distanceToPlane = slicePlaneNormal[0]*(transformedWorldCoordinates[0]-slicePlaneOrigin[0]) +
          slicePlaneNormal[1]*(transformedWorldCoordinates[1]-slicePlaneOrigin[1]) +
          slicePlaneNormal[2]*(transformedWorldCoordinates[2]-slicePlaneOrigin[2]);
        // this gives the distance to light box plane 0, but have to offset by
        // number of light box planes (as determined by the light box index) times the volume
        // slice spacing
        int lightboxIndex = this->GetLightboxIndex(controlPointsNode, i);
        double lightboxOffset = lightboxIndex * spacing;
        double distanceToSlice = distanceToPlane - lightboxOffset;
        double maxDistance = 0.5;
        vtkDebugMacro("\n\tdistance to plane = " << distanceToPlane
                      << "\n\tlightboxIndex = " << lightboxIndex
                      << "\n\tlightboxOffset = " << lightboxOffset
                      << "\n\tdistance to slice = " << distanceToSlice);
        // check that it's within 0.5mm
        if (distanceToSlice < -0.5 || distanceToSlice >= maxDistance)
          {
          vtkDebugMacro("Distance to slice is greater than max distance, not showing the widget");
          showWidget = false;
          break;
          }
        }
      else
        {
        // the third coordinate of the displayCoordinates is the distance to the slice
        double distanceToSlice = displayCoordinates[2];
        double maxDistance = 0.5 + (sliceNode->GetDimensions()[2] - 1);
        vtkDebugMacro("Slice node " << sliceNode->GetName()
                      << ": distance to slice = " << distanceToSlice
                      << ", maxDistance = " << maxDistance
                      << "\n\tslice node dimensions[2] = "
                      << sliceNode->GetDimensions()[2]);
        if (distanceToSlice < -0.5 || distanceToSlice >= maxDistance)
          {
          // if the distance to the slice is more than 0.5mm, we know that at least one coordinate of the widget is outside the current activeSlice
          // hence, we do not want to show this widget
          showWidget = false;
          // we don't even need to continue parsing the controlpoints, because we know the widget will not be shown
          break;
          }
        }
      }

    // -----------------------------------------
    // special cases when the slices get panned:

    // if all of the controlpoints are outside the viewport coordinates, the widget should not be shown
    // if one controlpoint is inside the viewport coordinates, the widget should be shown

    // we need to check if we are inside the viewport
    double coords[2] = {displayCoordinates[0], displayCoordinates[1]};

    vtkRenderer* pokedRenderer = this->GetInteractor()->
      FindPokedRenderer(static_cast<int> (coords[0]), static_cast<int> (coords[1]));
    if (!pokedRenderer)
      {
      vtkErrorMacro("IsWidgetDisplayableOnSlice: Could not find the poked renderer!")
      return false;
      }

    pokedRenderer->DisplayToNormalizedDisplay(coords[0],coords[1]);
    pokedRenderer->NormalizedDisplayToViewport(coords[0],coords[1]);
    pokedRenderer->ViewportToNormalizedViewport(coords[0],coords[1]);

    if ((coords[0]>0.0) && (coords[0]<1.0) && (coords[1]>0.0) && (coords[1]<1.0))
      {
      // current point is inside of view
      inViewport = true;
      }

    } // end of for loop through control points

  return showWidget && inViewport;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::OnInteractorStyleEvent(int eventid)
{
  Superclass::OnInteractorStyleEvent(eventid);
  if (this->GetDisableInteractorStyleEventsProcessing())
    {
    vtkWarningMacro("OnInteractorStyleEvent: Processing of events was disabled.")
    return;
    }

  if (!this->IsCorrectDisplayableManager())
    {
    //std::cout << "Markups DisplayableManger: OnInteractorStyleEvent : "
    // << this->Focus << ", not correct displayable manager, returning"
    // << std::endl;
    return;
    }
  vtkDebugMacro("OnInteractorStyleEvent " << this->Focus << " " << eventid);

  if (eventid == vtkCommand::LeftButtonReleaseEvent)
    {
    if (this->GetInteractionNode()->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place)
      {
      this->OnClickInRenderWindowGetCoordinates();
      }
    }
  else if (eventid == vtkCommand::RightButtonReleaseEvent)
    {
    // if we're in persistent place mode, go back to view transform mode, but
    // leave the persistent flag on
    if (this->GetInteractionNode()->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place &&
        this->GetInteractionNode()->GetPlaceModePersistence() == 1)
      {
      // add point after is switched to no place
      // the manager helper will take care to set the right status on the widgets
      this->GetInteractionNode()->SwitchToViewTransformMode();
      this->OnClickInRenderWindowGetCoordinates();
      }
    }
  else if (eventid == vtkCommand::EnterEvent)
    {
    if (this->GetInteractionNode()->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place)
      {
      this->OnClickInRenderWindowGetCoordinates(vtkMRMLMarkupsDisplayableManager2D::AddPreview);
      }
    this->RequestRender();
    }
  else if (eventid == vtkCommand::LeaveEvent)
    {
    if (this->GetInteractionNode()->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place)
      {
      this->OnClickInRenderWindowGetCoordinates(vtkMRMLMarkupsDisplayableManager2D::RemovePreview);
      }
    this->RequestRender();
    }
  if (eventid == vtkCommand::KeyPressEvent)
    {
    char *keySym = this->GetInteractor()->GetKeySym();
    vtkDebugMacro("OnInteractorStyleEvent 3D: key press event position = "
              << this->GetInteractor()->GetEventPosition()[0] << ", "
              << this->GetInteractor()->GetEventPosition()[1]
              << ", key sym = " << (keySym == NULL ? "null" : keySym));
    if (!keySym)
      {
      return;
      }
    if (strcmp(keySym, "p") == 0)
      {
      if (this->GetInteractionNode()->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place)
        {
        this->OnClickInRenderWindowGetCoordinates();
        }
      else
        {
        vtkDebugMacro("Line DisplayableManager: key press p, but not in Place mode! Returning.");
        return;
        }
      }
    }
  else if (eventid == vtkCommand::KeyReleaseEvent)
    {
    vtkDebugMacro("Got a key release event");
    }
}

//---------------------------------------------------------------------------
vtkAbstractWidget * vtkMRMLMarkupsDisplayableManager2D::GetWidget(vtkMRMLMarkupsNode * node)
{
  return this->Helper->GetWidget(node);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::OnClickInRenderWindowGetCoordinates(int action /*= 0 */)
{

  double x = this->GetInteractor()->GetEventPosition()[0];
  double y = this->GetInteractor()->GetEventPosition()[1];

  double windowWidth = this->GetInteractor()->GetRenderWindow()->GetSize()[0];
  double windowHeight = this->GetInteractor()->GetRenderWindow()->GetSize()[1];

  if ((x < windowWidth && y < windowHeight) ||
       action != vtkMRMLMarkupsDisplayableManager2D::RemovePreview)
    {
    const char *associatedNodeID = nullptr;
    // is there a volume in the background?
    if (this->GetMRMLSliceNode())
      {
      // find the slice composite node in the scene with the matching layout
      // name
      vtkMRMLSliceLogic *sliceLogic = nullptr;
      vtkMRMLSliceCompositeNode* sliceCompositeNode = nullptr;
      vtkMRMLApplicationLogic *mrmlAppLogic = this->GetMRMLApplicationLogic();
      if (mrmlAppLogic)
        {
        sliceLogic = mrmlAppLogic->GetSliceLogic(this->GetMRMLSliceNode());
        }
      if (sliceLogic)
        {
        sliceCompositeNode = sliceLogic->GetSliceCompositeNode(this->GetMRMLSliceNode());
        }
      if (sliceCompositeNode)
        {
        if (sliceCompositeNode->GetBackgroundVolumeID())
          {
          associatedNodeID = sliceCompositeNode->GetBackgroundVolumeID();
          }
        else if (sliceCompositeNode->GetForegroundVolumeID())
          {
          associatedNodeID = sliceCompositeNode->GetForegroundVolumeID();
          }
        else if (sliceCompositeNode->GetLabelVolumeID())
          {
          associatedNodeID = sliceCompositeNode->GetLabelVolumeID();
          }
        }
      }
    vtkDebugMacro("associatedNodeID set to " << (associatedNodeID ? associatedNodeID : "NULL"));
    this->OnClickInRenderWindow(x, y, associatedNodeID, action);
    }
}

//---------------------------------------------------------------------------
// Coordinate conversions
//---------------------------------------------------------------------------
/// Convert display to world coordinates
void vtkMRMLMarkupsDisplayableManager2D::GetDisplayToWorldCoordinates(double x, double y, double * worldCoordinates)
{

  // we will get the transformation matrix to convert display coordinates to RAS

  vtkRenderer* pokedRenderer = this->GetInteractor()->
    FindPokedRenderer(static_cast<int> (x), static_cast<int> (y));

  vtkMatrix4x4 * xyToRasMatrix = this->GetMRMLSliceNode()->GetXYToRAS();

  double displayCoordinates[4];
  displayCoordinates[0] = x - pokedRenderer->GetOrigin()[0];
  displayCoordinates[1] = y - pokedRenderer->GetOrigin()[1];
  displayCoordinates[2] = 0;
  displayCoordinates[3] = 1;

  xyToRasMatrix->MultiplyPoint(displayCoordinates, worldCoordinates);

}

//---------------------------------------------------------------------------
/// Convert display to world coordinates
void vtkMRMLMarkupsDisplayableManager2D::GetDisplayToWorldCoordinates(double * displayCoordinates, double * worldCoordinates)
{

  this->GetDisplayToWorldCoordinates(displayCoordinates[0], displayCoordinates[1], worldCoordinates);

}

//---------------------------------------------------------------------------
/// Convert world to display coordinates
void vtkMRMLMarkupsDisplayableManager2D::GetWorldToDisplayCoordinates(double r, double a, double s, double * displayCoordinates)
{
  if (!this->GetMRMLSliceNode())
    {
    vtkErrorMacro("GetWorldToDisplayCoordinates: no slice node!");
    return;
    }

  // we will get the transformation matrix to convert world coordinates to the display coordinates of the specific sliceNode

  vtkMatrix4x4 * xyToRasMatrix = this->GetMRMLSliceNode()->GetXYToRAS();
  vtkNew<vtkMatrix4x4> rasToXyMatrix;

  // we need to invert this matrix
  xyToRasMatrix->Invert(xyToRasMatrix, rasToXyMatrix.GetPointer());

  double worldCoordinates[4];
  worldCoordinates[0] = r;
  worldCoordinates[1] = a;
  worldCoordinates[2] = s;
  worldCoordinates[3] = 1;

  rasToXyMatrix->MultiplyPoint(worldCoordinates,displayCoordinates);
  xyToRasMatrix = nullptr;
}

//---------------------------------------------------------------------------
// Convert world to display coordinates
void vtkMRMLMarkupsDisplayableManager2D::GetWorldToDisplayCoordinates(double * worldCoordinates, double * displayCoordinates)
{
  if (worldCoordinates == nullptr)
    {
    return;
    }

  this->GetWorldToDisplayCoordinates(worldCoordinates[0], worldCoordinates[1], worldCoordinates[2], displayCoordinates);
}

//---------------------------------------------------------------------------
// Convert display to viewport coordinates
void vtkMRMLMarkupsDisplayableManager2D::GetDisplayToViewportCoordinates(double x, double y, double * viewportCoordinates)
{

  if (viewportCoordinates == nullptr)
    {
    return;
    }
  if (!this->GetInteractor())
    {
    vtkErrorMacro("GetDisplayToViewportCoordinates: No interactor!");
    return;
    }

  double displayCoordinates[4];
  this->ConvertDeviceToXYZ(x, y, displayCoordinates);
  displayCoordinates[3] = 1;

  double windowWidth = this->GetInteractor()->GetRenderWindow()->GetSize()[0];
  double windowHeight = this->GetInteractor()->GetRenderWindow()->GetSize()[1];
  if (windowWidth != 0.0)
    {
    viewportCoordinates[0] = displayCoordinates[0]/windowWidth;
    }
  if (windowHeight != 0.0)
    {
    viewportCoordinates[1] = displayCoordinates[1]/windowHeight;
    }
  vtkDebugMacro("GetDisplayToViewportCoordinates: x = " << x << ", y = " << y
                << ", display coords calc as "
                << displayCoordinates[0] << ", " << displayCoordinates[1]
                << ", returning viewport = "
                << viewportCoordinates[0] << ", " << viewportCoordinates[1]);
}

//---------------------------------------------------------------------------
/// Convert display to viewport coordinates
void vtkMRMLMarkupsDisplayableManager2D::GetDisplayToViewportCoordinates(double * displayCoordinates, double * viewportCoordinates)
{
  if (displayCoordinates && viewportCoordinates)
    {
    this->GetDisplayToViewportCoordinates(displayCoordinates[0], displayCoordinates[1], viewportCoordinates);
    }
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsDisplayableManager2D::RestrictDisplayCoordinatesToViewport(double* displayCoordinates)
{
  double coords[2] = {displayCoordinates[0], displayCoordinates[1]};
  bool restricted = false;

  vtkRenderer* pokedRenderer = this->GetInteractor()->
    FindPokedRenderer(static_cast<int> (coords[0]), static_cast<int> (coords[1]));
  if (!pokedRenderer)
    {
    vtkErrorMacro("RestrictDisplayCoordinatesToViewport: Could not find the poked renderer!")
    return restricted;
    }

  pokedRenderer->DisplayToNormalizedDisplay(coords[0],coords[1]);
  pokedRenderer->NormalizedDisplayToViewport(coords[0],coords[1]);
  pokedRenderer->ViewportToNormalizedViewport(coords[0],coords[1]);

  if (coords[0]<0.001)
    {
    coords[0] = 0.001;
    restricted = true;
    }
  else if (coords[0]>0.999)
    {
    coords[0] = 0.999;
    restricted = true;
    }

  if (coords[1]<0.001)
    {
    coords[1] = 0.001;
    restricted = true;
    }
  else if (coords[1]>0.999)
    {
    coords[1] = 0.999;
    restricted = true;
    }

  pokedRenderer->NormalizedViewportToViewport(coords[0],coords[1]);
  pokedRenderer->ViewportToNormalizedDisplay(coords[0],coords[1]);
  pokedRenderer->NormalizedDisplayToDisplay(coords[0],coords[1]);

  displayCoordinates[0] = coords[0];
  displayCoordinates[1] = coords[1];

  return restricted;
}

//---------------------------------------------------------------------------
/// Check if there are real changes between two sets of displayCoordinates
bool vtkMRMLMarkupsDisplayableManager2D::GetDisplayCoordinatesChanged(double * displayCoordinates1, double * displayCoordinates2)
{
  bool changed = false;

  if (sqrt((displayCoordinates1[0] - displayCoordinates2[0]) * (displayCoordinates1[0] - displayCoordinates2[0])
           + (displayCoordinates1[1] - displayCoordinates2[1]) * (displayCoordinates1[1] - displayCoordinates2[1]))>1.0)
    {
    changed = true;
    }
  else
    {
    // if in lightbox mode, the third element in the vector may have changed
    if (this->IsInLightboxMode())
      {
      // one of the arguments may be coming from a widget, the other should be
      // the index into the light box array
      double dist = sqrt((displayCoordinates1[2] - displayCoordinates2[2]) * (displayCoordinates1[2] - displayCoordinates2[2]));
      if (dist > 1.0)
        {
        changed = true;
        }
      }
    }
  return changed;
}

//---------------------------------------------------------------------------
/// Check if there are real changes between two sets of displayCoordinates
bool vtkMRMLMarkupsDisplayableManager2D::GetWorldCoordinatesChanged(double * worldCoordinates1, double * worldCoordinates2)
{
  bool changed = false;

  double distance = sqrt(vtkMath::Distance2BetweenPoints(worldCoordinates1,worldCoordinates2));

  // TODO find a better value?
  // - use a smaller number to make fiducial seeding more smooth
  if (distance > VTK_DBL_EPSILON)
    {
    changed = true;
    }

  return changed;
}

//---------------------------------------------------------------------------
/// Check if it is the correct displayableManager
//---------------------------------------------------------------------------
bool vtkMRMLMarkupsDisplayableManager2D::IsCorrectDisplayableManager()
{

  vtkMRMLSelectionNode *selectionNode = this->GetMRMLApplicationLogic()->GetSelectionNode();
  if (selectionNode == nullptr)
    {
    vtkErrorMacro ("IsCorrectDisplayableManager: No selection node in the scene.");
    return false;
    }
  if (selectionNode->GetActivePlaceNodeClassName() == nullptr)
    {
    return false;
    }
  // the purpose of the displayableManager is hardcoded
  return this->IsManageable(selectionNode->GetActivePlaceNodeClassName());

}
//---------------------------------------------------------------------------
bool vtkMRMLMarkupsDisplayableManager2D::IsManageable(vtkMRMLNode* node)
{
  return node->IsA(this->Focus);
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsDisplayableManager2D::IsManageable(const char* nodeClassName)
{
  return nodeClassName && !strcmp(nodeClassName, this->Focus);
}

//---------------------------------------------------------------------------
// Functions to overload!
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::OnClickInRenderWindow(double vtkNotUsed(x), double vtkNotUsed(y),
                                                               const char * vtkNotUsed(associatedNodeID),
                                                               int vtkNotUsed(action))
{
  // The user clicked in the renderWindow
  vtkErrorMacro("OnClickInRenderWindow should be overloaded!");
}

//---------------------------------------------------------------------------
vtkSlicerAbstractWidget* vtkMRMLMarkupsDisplayableManager2D::CreateWidget(vtkMRMLMarkupsNode* vtkNotUsed(node))
{
  // A widget should be created here.
  vtkErrorMacro("CreateWidget should be overloaded!");
  return nullptr;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager2D::OnWidgetCreated(vtkSlicerAbstractWidget* vtkNotUsed(widget), vtkMRMLMarkupsNode* vtkNotUsed(node))
{
  // Actions after a widget was created should be executed here.
  vtkErrorMacro("OnWidgetCreated should be overloaded!");
}

//---------------------------------------------------------------------------
/// Convert world coordinates to local using mrml parent transform
void vtkMRMLMarkupsDisplayableManager2D::GetWorldToLocalCoordinates(vtkMRMLMarkupsNode *node,
                                                                     double *worldCoordinates,
                                                                     double *localCoordinates)
{
  if (node == nullptr)
    {
    vtkErrorMacro("GetWorldToLocalCoordinates: node is null");
    return;
    }

  for (int i  =0; i < 3; i++)
    {
    localCoordinates[i] = worldCoordinates[i];
    }

  vtkMRMLTransformNode* tnode = node->GetParentTransformNode();

  vtkGeneralTransform *transformToWorld = vtkGeneralTransform::New();
  transformToWorld->Identity();
  if (tnode != nullptr && !tnode->IsTransformToWorldLinear())
    {
    tnode->GetTransformFromWorld(transformToWorld);
    }
  else if (tnode != nullptr && tnode->IsTransformToWorldLinear())
    {
    vtkNew<vtkMatrix4x4> matrixTransformToWorld;
    matrixTransformToWorld->Identity();
    tnode->GetMatrixTransformToWorld(matrixTransformToWorld.GetPointer());
    matrixTransformToWorld->Invert();
    transformToWorld->Concatenate(matrixTransformToWorld.GetPointer());
  }
  double p[4];
  p[3] = 1;
  int i;
  for (i=0; i<3; i++)
    {
    p[i] = worldCoordinates[i];
    }
  double *xyz = transformToWorld->TransformDoublePoint(p);
  for (i=0; i<3; i++)
    {
    localCoordinates[i] = xyz[i];
    }
  transformToWorld->Delete();
}

//---------------------------------------------------------------------------
/// Create a new widget for this markups node and save it to the helper.
vtkSlicerAbstractWidget * vtkMRMLMarkupsDisplayableManager2D::AddWidget(vtkMRMLMarkupsNode *markupsNode)
{
  vtkDebugMacro("AddWidget: calling create widget");
  vtkSlicerAbstractWidget* newWidget = this->CreateWidget(markupsNode);
  if (!newWidget)
    {
    return nullptr;
    }

  // record the mapping between node and widget in the helper
  this->Helper->RecordWidgetForNode(newWidget,markupsNode);

  vtkDebugMacro("AddWidget: saved to helper ");

  // Refresh observers
  this->SetAndObserveNode(markupsNode);

  this->RequestRender();
  this->OnWidgetCreated(newWidget, markupsNode);

  // Build representation
  newWidget->BuildRepresentation();

  return newWidget;
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsDisplayableManager2D::IsInLightboxMode()
{
  bool flag = false;
  vtkMRMLSliceNode *sliceNode = this->GetMRMLSliceNode();
  if (sliceNode)
    {
    int numberOfColumns = sliceNode->GetLayoutGridColumns();
    int numberOfRows = sliceNode->GetLayoutGridRows();
    if (numberOfColumns > 1 ||
        numberOfRows > 1)
      {
      flag = true;
      }
    }
  return flag;
}

//---------------------------------------------------------------------------
int  vtkMRMLMarkupsDisplayableManager2D::GetLightboxIndex(vtkMRMLMarkupsNode *node, int pointIndex)
{
  int index = -1;

  if (!node || !this->IsInLightboxMode() ||
      !node->ControlPointExists(pointIndex))
    {
    return index;
    }

  double transformedWorldCoordinates[4], displayCoordinates[4];
  node->GetNthControlPointPositionWorld(pointIndex, transformedWorldCoordinates);
  this->GetWorldToDisplayCoordinates(transformedWorldCoordinates,displayCoordinates);

  return static_cast<int> (floor(displayCoordinates[2]+0.5));
}
