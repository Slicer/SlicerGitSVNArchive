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
#include "vtkMRMLMarkupsDisplayableManager3D.h"

// MRMLDisplayableManager includes
#include <vtkMRMLModelDisplayableManager.h>
#include <vtkMRMLDisplayableManagerGroup.h>

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
#include <vtkSlicerAbstractWidget.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkGeneralTransform.h>
#include <vtkMarkupsGlyphSource2D.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPropCollection.h>
#include <vtkProperty.h>
#include <vtkTextProperty.h>
#include <vtkRendererCollection.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSlicerAbstractRepresentation.h>
#include <vtkSlicerAbstractRepresentation3D.h>
#include <vtkSlicerAbstractWidget.h>
#include <vtkSphereSource.h>
#include <vtkWidgetRepresentation.h>

// STD includes
#include <algorithm>
#include <map>
#include <vector>

typedef void (*fp)(void);

#define NUMERIC_ZERO 0.001

//---------------------------------------------------------------------------
vtkStandardNewMacro (vtkMRMLMarkupsDisplayableManager3D);

//---------------------------------------------------------------------------
vtkMRMLMarkupsDisplayableManager3D::vtkMRMLMarkupsDisplayableManager3D()
{
  this->Helper = vtkMRMLMarkupsDisplayableManagerHelper::New();
  this->DisableInteractorStyleEventsProcessing = 0;

  this->Focus = "vtkMRMLMarkupsNode";

  this->LastClickWorldCoordinates[0]=0.0;
  this->LastClickWorldCoordinates[1]=0.0;
  this->LastClickWorldCoordinates[2]=0.0;
  this->LastClickWorldCoordinates[3]=1.0;
}

//---------------------------------------------------------------------------
vtkMRMLMarkupsDisplayableManager3D::~vtkMRMLMarkupsDisplayableManager3D()
{
  this->DisableInteractorStyleEventsProcessing = 0;
  this->Focus = nullptr;

  this->Helper->Delete();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "DisableInteractorStyleEventsProcessing = " << this->DisableInteractorStyleEventsProcessing << std::endl;
  if (this->Focus)
    {
    os << indent << "Focus = " << this->Focus << std::endl;
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager3D::SetAndObserveNode(vtkMRMLMarkupsNode *markupsNode)
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

  vtkUnObserveMRMLNodeMacro(markupsNode);
  vtkObserveMRMLNodeEventsMacro(markupsNode, nodeEvents.GetPointer());
}
//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager3D::SetAndObserveNodes()
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
void vtkMRMLMarkupsDisplayableManager3D::RequestRender()
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
void vtkMRMLMarkupsDisplayableManager3D::RemoveMRMLObservers()
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
void vtkMRMLMarkupsDisplayableManager3D::UpdateFromMRML()
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
        vtkSlicerAbstractWidget *widget = this->AddWidget(markupsNode);
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
void vtkMRMLMarkupsDisplayableManager3D::SetMRMLSceneInternal(vtkMRMLScene* newScene)
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
void vtkMRMLMarkupsDisplayableManager3D
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
void vtkMRMLMarkupsDisplayableManager3D::OnMRMLSceneEndClose()
{
  vtkDebugMacro("OnMRMLSceneEndClose: remove observers?");
  // run through all nodes and remove node and widget
  this->Helper->RemoveAllWidgetsAndNodes();

  this->SetUpdateFromMRMLRequested(1);
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager3D::OnMRMLSceneEndImport()
{
 this->SetUpdateFromMRMLRequested(1);
 this->UpdateFromMRMLScene();
 this->Helper->SetAllWidgetsToManipulate();
 this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager3D::UpdateFromMRMLScene()
{
  this->UpdateFromMRML();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager3D::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
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

  vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(node);
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
  vtkSlicerAbstractWidget* newWidget = this->AddWidget(markupsNode);
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
void vtkMRMLMarkupsDisplayableManager3D::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
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

  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager3D::OnMRMLMarkupsNodeModifiedEvent(vtkMRMLNode* node)
{
  vtkDebugMacro("OnMRMLMarkupsNodeModifiedEvent");

  vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(node);
  if (!markupsNode)
    {
    vtkErrorMacro("OnMRMLMarkupsNodeModifiedEvent: Can not access node.")
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
void vtkMRMLMarkupsDisplayableManager3D::OnMRMLMarkupsDisplayNodeModifiedEvent(vtkMRMLNode* node)
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
    vtkSlicerAbstractRepresentation3D *rep =
      vtkSlicerAbstractRepresentation3D::SafeDownCast(widget->GetRepresentation());
    if (rep)
      {
      if (markupsDisplayNode->GetVisibility() && markupsDisplayNode->IsDisplayableInView(this->GetMRMLViewNode()->GetID()))
        {
        rep->VisibilityOn();
        vtkProperty *prop = rep->GetProperty();
        if (prop)
          {
          prop->SetColor(markupsDisplayNode->GetColor());
          prop->SetAmbient(markupsDisplayNode->GetAmbient());
          prop->SetDiffuse(markupsDisplayNode->GetDiffuse());
          prop->SetSpecular(markupsDisplayNode->GetSpecular());
          prop->SetShading(markupsDisplayNode->GetShading());
          prop->SetSpecularPower(markupsDisplayNode->GetPower());
          prop->SetOpacity(markupsDisplayNode->GetOpacity());
          }
        vtkProperty *selectedProp = rep->GetSelectedProperty();
        if (selectedProp)
          {
          selectedProp->SetColor(markupsDisplayNode->GetSelectedColor());
          // do not use selected ambient and specular
          selectedProp->SetAmbient(markupsDisplayNode->GetAmbient());
          selectedProp->SetDiffuse(markupsDisplayNode->GetDiffuse());
          selectedProp->SetSpecular(markupsDisplayNode->GetSpecular());
          selectedProp->SetShading(markupsDisplayNode->GetShading());
          selectedProp->SetSpecularPower(markupsDisplayNode->GetPower());
          selectedProp->SetOpacity(markupsDisplayNode->GetOpacity());
          }
        vtkProperty *activeProp = rep->GetActiveProperty();
        if (activeProp)
          {
          // bright green
          activeProp->SetColor(0.4, 1.0, 0.);
          // do not use selected ambient and specular
          activeProp->SetAmbient(markupsDisplayNode->GetAmbient());
          activeProp->SetDiffuse(markupsDisplayNode->GetDiffuse());
          activeProp->SetSpecular(markupsDisplayNode->GetSpecular());
          activeProp->SetShading(markupsDisplayNode->GetShading());
          activeProp->SetSpecularPower(markupsDisplayNode->GetPower());
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

        rep->SetHandleSize(markupsDisplayNode->GetGlyphScale());

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
void vtkMRMLMarkupsDisplayableManager3D::OnMRMLMarkupsNthPointModifiedEvent(vtkMRMLNode *node, int vtkNotUsed(n))
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

  vtkSlicerAbstractWidget *widget = this->Helper->GetWidget(markupsNode);
  if (widget)
    {
    // Rebuild representation
    widget->BuildRepresentation();
    this->RequestRender();
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager3D::OnMRMLMarkupsPointAddedEvent(vtkMRMLNode *node, int vtkNotUsed(n))
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

  // Rebuild representation
  widget->BuildRepresentation();
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager3D::OnMRMLMarkupsPointRemovedEvent(vtkMRMLNode *node, int vtkNotUsed(n))
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
void vtkMRMLMarkupsDisplayableManager3D::OnMRMLMarkupsAllPointsRemovedEvent(vtkMRMLNode *node)
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
void vtkMRMLMarkupsDisplayableManager3D::OnMRMLMarkupsNodeTransformModifiedEvent(vtkMRMLNode* node)
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

  vtkSlicerAbstractWidget *widget = this->Helper->GetWidget(markupsNode);
  if (widget)
    {
    // Rebuild representation
    widget->BuildRepresentation();
    this->RequestRender();
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager3D::OnMRMLMarkupsNodeLockModifiedEvent(vtkMRMLNode* node)
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
void vtkMRMLMarkupsDisplayableManager3D::OnMRMLDisplayableNodeModifiedEvent(vtkObject* caller)
{

  vtkDebugMacro("OnMRMLDisplayableNodeModifiedEvent");

  if (!caller)
    {
    vtkErrorMacro("OnMRMLDisplayableNodeModifiedEvent: Could not get caller.")
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
void vtkMRMLMarkupsDisplayableManager3D::OnInteractorStyleEvent(int eventid)
{  
  if (this->GetDisableInteractorStyleEventsProcessing())
    {
    vtkWarningMacro("OnInteractorStyleEvent: Processing of events was disabled.")
    return;
    }

  if (!this->IsCorrectDisplayableManager())
    {
    //std::cout << "Markups DisplayableManger: OnInteractorStyleEvent : " << this->Focus << ", not correct displayable manager, returning" << std::endl;
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
      this->OnClickInRenderWindowGetCoordinates(vtkMRMLMarkupsDisplayableManager3D::AddPreview);
      }
    this->RequestRender();
    }
  else if (eventid == vtkCommand::LeaveEvent)
    {
    if (this->GetInteractionNode()->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place)
      {
      this->OnClickInRenderWindowGetCoordinates(vtkMRMLMarkupsDisplayableManager3D::RemovePreview);
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
void vtkMRMLMarkupsDisplayableManager3D::AddObserversToInteractionNode()
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
void vtkMRMLMarkupsDisplayableManager3D::RemoveObserversFromInteractionNode()
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
vtkSlicerAbstractWidget * vtkMRMLMarkupsDisplayableManager3D::GetWidget(vtkMRMLMarkupsNode * node)
{
  return this->Helper->GetWidget(node);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager3D::OnClickInRenderWindowGetCoordinates(int action /*= 0 */)
{

  double x = this->GetInteractor()->GetEventPosition()[0];
  double y = this->GetInteractor()->GetEventPosition()[1];

  double windowWidth = this->GetInteractor()->GetRenderWindow()->GetSize()[0];
  double windowHeight = this->GetInteractor()->GetRenderWindow()->GetSize()[1];

  if ((x < windowWidth && y < windowHeight) ||
       action != vtkMRMLMarkupsDisplayableManager3D::RemovePreview)
    {
    const char *associatedNodeID = nullptr;
    // it's a 3D displayable manager and the click could have been on a node
    vtkMRMLModelDisplayableManager * modelDisplayableManager =
      vtkMRMLModelDisplayableManager::SafeDownCast(this->GetMRMLDisplayableManagerGroup()->GetDisplayableManagerByClassName("vtkMRMLModelDisplayableManager"));
    double yNew = windowHeight - y - 1;
    if (modelDisplayableManager &&
        modelDisplayableManager->Pick(static_cast<int>(x), static_cast<int>(yNew)) &&
        strcmp(modelDisplayableManager->GetPickedNodeID(),"") != 0)
      {
      // find the node id, the picked node name is probably the display node
      const char *pickedNodeID = modelDisplayableManager->GetPickedNodeID();
      vtkDebugMacro("Click was on model " << pickedNodeID);
      vtkMRMLNode *mrmlNode = this->GetMRMLScene()->GetNodeByID(pickedNodeID);
      vtkMRMLDisplayNode *displayNode = nullptr;
      if (mrmlNode)
        {
        vtkDebugMacro("Got a mrml node by name, id = " << mrmlNode->GetID());
        displayNode = vtkMRMLDisplayNode::SafeDownCast(mrmlNode);
        }
      else
        {
        vtkDebugMacro("couldn't find a mrml node with ID " << pickedNodeID);
        }
      if (displayNode)
        {
        vtkDebugMacro("Got display node for picked node name " << displayNode->GetID());
        vtkMRMLDisplayableNode *displayableNode = displayNode->GetDisplayableNode();
        if (displayableNode)
          {
          // start with the assumption that it's a generic displayable node,
          // then look for it to be a slice node that has the string
          // CompositeID in it's Description with a valid mrml node id after it
          associatedNodeID = displayableNode->GetID();
          // it might be a slice node, check the Description field for the string CompositeID
          if (displayableNode->GetDescription())
            {
            std::string desc = displayableNode->GetDescription();
            size_t ptr = desc.find("CompositeID");
            // does it have the string CompositeID in the description with
            // something after it?
            vtkDebugMacro("Desc len = " << desc.length() << ", ptr = " << ptr);
            if (ptr != std::string::npos &&
                (desc.length() > (ptr + 12)))
              {
              std::string compID = desc.substr(ptr + 12);
              vtkDebugMacro("Found composite node id = " << compID.c_str());
              vtkMRMLNode *mrmlNode = this->GetMRMLScene()->GetNodeByID(compID.c_str());
              // was this a valid composite node id?
              if (mrmlNode)
                {
                vtkMRMLSliceCompositeNode* sliceCompositeNode = vtkMRMLSliceCompositeNode::SafeDownCast(mrmlNode);
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
                  vtkDebugMacro("Markups was placed on a 3d slice model, found the volume id " << associatedNodeID);
                  }
                }
              }
            }
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
void vtkMRMLMarkupsDisplayableManager3D::GetDisplayToWorldCoordinates(double x, double y, double * worldCoordinates)
{

  // for 3D, we want to convert the coordinates using the pick function

  // ModelDisplayableManager is expected to be instantiated !
  vtkMRMLModelDisplayableManager * modelDisplayableManager =
      vtkMRMLModelDisplayableManager::SafeDownCast(
        this->GetMRMLDisplayableManagerGroup()->GetDisplayableManagerByClassName(
          "vtkMRMLModelDisplayableManager"));
  assert(modelDisplayableManager);

  double windowHeight = this->GetInteractor()->GetRenderWindow()->GetSize()[1];

  double yNew = windowHeight - y - 1;

  if (modelDisplayableManager->Pick(static_cast<int>(x), static_cast<int>(yNew)))
    {
    double* pickedWorldCoordinates = modelDisplayableManager->GetPickedRAS();
    worldCoordinates[0] = pickedWorldCoordinates[0];
    worldCoordinates[1] = pickedWorldCoordinates[1];
    worldCoordinates[2] = pickedWorldCoordinates[2];
    worldCoordinates[3] = 1;
    }
  else
    {
    // we could not pick so just convert to world coordinates
    vtkInteractorObserver::ComputeDisplayToWorld(this->GetRenderer(),x,y,0,worldCoordinates);
    }

}

//---------------------------------------------------------------------------
/// Convert display to world coordinates
void vtkMRMLMarkupsDisplayableManager3D::GetDisplayToWorldCoordinates(double * displayCoordinates, double * worldCoordinates)
{

  this->GetDisplayToWorldCoordinates(displayCoordinates[0], displayCoordinates[1], worldCoordinates);

}

//---------------------------------------------------------------------------
/// Convert world to display coordinates
void vtkMRMLMarkupsDisplayableManager3D::GetWorldToDisplayCoordinates(double r, double a, double s, double * displayCoordinates)
{
  vtkInteractorObserver::ComputeWorldToDisplay(this->GetRenderer(),r,a,s,displayCoordinates);
}

//---------------------------------------------------------------------------
/// Convert world to display coordinates
void vtkMRMLMarkupsDisplayableManager3D::GetWorldToDisplayCoordinates(double * worldCoordinates, double * displayCoordinates)
{
  if (worldCoordinates == nullptr)
    {
    return;
    }

  this->GetWorldToDisplayCoordinates(worldCoordinates[0], worldCoordinates[1], worldCoordinates[2], displayCoordinates);
}

//---------------------------------------------------------------------------
/// Convert display to viewport coordinates
void vtkMRMLMarkupsDisplayableManager3D::GetDisplayToViewportCoordinates(double x, double y, double * viewportCoordinates)
{
  if (viewportCoordinates == nullptr)
    {
    return;
    }
  double windowWidth = this->GetInteractor()->GetRenderWindow()->GetSize()[0];
  double windowHeight = this->GetInteractor()->GetRenderWindow()->GetSize()[1];

  if (windowWidth != 0.0)
    {
    viewportCoordinates[0] = x/windowWidth;
    }
  if (windowHeight != 0.0)
    {
    viewportCoordinates[1] = y/windowHeight;
    }
  vtkDebugMacro("GetDisplayToViewportCoordinates: x = " << x
        << ", y = " << y
        << ", returning viewport = "
        << viewportCoordinates[0] << ", " << viewportCoordinates[1]);
}

//---------------------------------------------------------------------------
/// Convert display to viewport coordinates
void vtkMRMLMarkupsDisplayableManager3D::GetDisplayToViewportCoordinates(double * displayCoordinates, double * viewportCoordinates)
{
  if (displayCoordinates && viewportCoordinates)
    {
    this->GetDisplayToViewportCoordinates(displayCoordinates[0], displayCoordinates[1], viewportCoordinates);
    }
}

//---------------------------------------------------------------------------
/// Check if there are real changes between two sets of displayCoordinates
bool vtkMRMLMarkupsDisplayableManager3D::GetDisplayCoordinatesChanged(double * displayCoordinates1, double * displayCoordinates2)
{
  bool changed = false;

  if (sqrt((displayCoordinates1[0] - displayCoordinates2[0]) * (displayCoordinates1[0] - displayCoordinates2[0])
           + (displayCoordinates1[1] - displayCoordinates2[1]) * (displayCoordinates1[1] - displayCoordinates2[1]))>1.0)
    {
    changed = true;
    }

  return changed;
}

//---------------------------------------------------------------------------
/// Check if there are real changes between two sets of displayCoordinates
bool vtkMRMLMarkupsDisplayableManager3D::GetWorldCoordinatesChanged(double * worldCoordinates1, double * worldCoordinates2)
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
bool vtkMRMLMarkupsDisplayableManager3D::IsCorrectDisplayableManager()
{
  vtkMRMLSelectionNode *selectionNode = this->GetMRMLApplicationLogic()->GetSelectionNode();
  if (selectionNode == nullptr)
    {
    vtkErrorMacro ("IsCorrectDisplayableManager: No selection node in the scene.");
    return false;
    }
  if (selectionNode->GetActivePlaceNodeClassName() == nullptr)
    {
    //vtkErrorMacro ("IsCorrectDisplayableManager: no active markups");
    return false;
    }
  // the purpose of the displayableManager is hardcoded
  return this->IsManageable(selectionNode->GetActivePlaceNodeClassName());
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsDisplayableManager3D::IsManageable(vtkMRMLNode* node)
{
  return node->IsA(this->Focus);
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsDisplayableManager3D::IsManageable(const char* nodeClassName)
{
  return nodeClassName && !strcmp(nodeClassName, this->Focus);
}

//---------------------------------------------------------------------------
// Functions to overload!
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager3D::OnClickInRenderWindow(double vtkNotUsed(x), double vtkNotUsed(y),
                                                               const char * vtkNotUsed(associatedNodeID),
                                                               int vtkNotUsed(onlyPreview))
{
  // The user clicked in the renderWindow
  vtkErrorMacro("OnClickInRenderWindow should be overloaded!");
}

//---------------------------------------------------------------------------
vtkSlicerAbstractWidget* vtkMRMLMarkupsDisplayableManager3D::CreateWidget(vtkMRMLMarkupsNode* vtkNotUsed(node))
{
  // A widget should be created here.
  vtkErrorMacro("CreateWidget should be overloaded!");
  return nullptr;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager3D::OnWidgetCreated(vtkSlicerAbstractWidget* vtkNotUsed(widget), vtkMRMLMarkupsNode* vtkNotUsed(node))
{
  // Actions after a widget was created should be executed here.
  vtkErrorMacro("OnWidgetCreated should be overloaded!");
}

//---------------------------------------------------------------------------
/// Convert world coordinates to local using mrml parent transform
void vtkMRMLMarkupsDisplayableManager3D::GetWorldToLocalCoordinates(vtkMRMLMarkupsNode *node,
                                                                     double *worldCoordinates,
                                                                     double *localCoordinates)
{
  if (node == nullptr)
    {
    vtkErrorMacro("GetWorldToLocalCoordinates: node is null");
    return;
    }

  for (int i = 0; i < 3; i++)
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
vtkSlicerAbstractWidget * vtkMRMLMarkupsDisplayableManager3D::AddWidget(vtkMRMLMarkupsNode *markupsNode)
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
