/*=========================================================================

 Copyright (c) ProxSim ltd., Kwun Tong, Hong Kong. All Rights Reserved.

 See COPYRIGHT.txt
 or http://www.slicer.org/copyright/copyright.txt for details.

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

 This file was originally developed by Davide Punzo, punzodavide@hotmail.it,
 and development was supported by ProxSim ltd.

=========================================================================*/

// MarkupsModule/MRML includes
#include <vtkMRMLMarkupsAngleNode.h>
#include <vtkMRMLMarkupsNode.h>
#include <vtkMRMLMarkupsDisplayNode.h>

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsAngleDisplayableManager3D.h"

// MarkupsModule/VTKWidgets includes
#include <vtkMarkupsGlyphSource2D.h>

// MRMLDisplayableManager includes
#include <vtkSliceViewInteractorStyle.h>

// MRML includes
#include <vtkMRMLInteractionNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLViewNode.h>

// VTK includes
#include <vtkSlicerAbstractWidget.h>
#include <vtkFollower.h>
#include <vtkHandleRepresentation.h>
#include <vtkInteractorStyle.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkOrientedPolygonalHandleRepresentation3D.h>
#include <vtkPickingManager.h>
#include <vtkProperty2D.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSlicerAngleWidget.h>
#include <vtkSmartPointer.h>
#include <vtkSlicerAngleRepresentation3D.h>
#include <vtkSphereSource.h>
#include <vtkTextProperty.h>

// STD includes
#include <sstream>
#include <string>

//---------------------------------------------------------------------------
vtkStandardNewMacro (vtkMRMLMarkupsAngleDisplayableManager3D);

//---------------------------------------------------------------------------
// vtkMRMLMarkupsAngleDisplayableManager3D Callback
/// \ingroup Slicer_QtModules_Markups
class vtkMarkupsAngleWidgetCallback3D : public vtkCommand
{
public:
  static vtkMarkupsAngleWidgetCallback3D *New()
  { return new vtkMarkupsAngleWidgetCallback3D; }

  vtkMarkupsAngleWidgetCallback3D()
    : Widget(nullptr)
    , Node(nullptr)
    , DisplayableManager(nullptr)
    , LastInteractionEventMarkupIndex(-1)
    , SelectionButton(0)
    , PointMovedSinceStartInteraction(false)
  {
  }

  virtual void Execute (vtkObject *vtkNotUsed(caller), unsigned long event, void *callData)
  {
    // sanity checks
    if (!this->DisplayableManager)
      {
      return;
      }
    if (!this->Node)
      {
      return;
      }
    if (!this->Widget)
      {
      return;
      }

    // If calldata is NULL, invoking an event may cause a crash (e.g., Python observer
    // tries to dereference the NULL pointer), therefore it's important to always pass a valid pointer
    // and indicate invalidity with value (-1).
    this->LastInteractionEventMarkupIndex = (callData ? *(reinterpret_cast<int *>(callData)) : -1);
    if (event ==  vtkCommand::PlacePointEvent)
      {
        this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointAddedEvent, &this->LastInteractionEventMarkupIndex);
      }
    else if (event == vtkCommand::DeletePointEvent)
      {
      this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointRemovedEvent, &this->LastInteractionEventMarkupIndex);
      }
    else if (event == vtkCommand::StartInteractionEvent)
      {
      // save the state of the node when starting interaction
      if (this->Node->GetScene())
        {
        this->Node->GetScene()->SaveStateForUndo();
        }

      this->PointMovedSinceStartInteraction = false;
      this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointStartInteractionEvent, &this->LastInteractionEventMarkupIndex);
      vtkSlicerAngleWidget* slicerAngleWidget = vtkSlicerAngleWidget::SafeDownCast(this->Widget);
      if (slicerAngleWidget)
        {
        this->SelectionButton = slicerAngleWidget->GetSelectionButton();
        }
      // no need to propagate to MRML, just notify external observers that the user selected a markup
      return;
      }
    else if (event == vtkCommand::EndInteractionEvent)
      {
      this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointEndInteractionEvent, &this->LastInteractionEventMarkupIndex);
      if (!this->PointMovedSinceStartInteraction)
        {
        if (this->SelectionButton == vtkSlicerAngleWidget::LeftButton)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointLeftClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerAngleWidget::MiddleButton)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointMiddleClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerAngleWidget::RightButton)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointRightClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerAngleWidget::LeftButtonDoubleClick)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointLeftDoubleClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerAngleWidget::MiddleButtonDoubleClick)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointMiddleDoubleClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerAngleWidget::RightButtonDoubleClick)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointRightDoubleClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        }
      this->LastInteractionEventMarkupIndex = -1;
      }
    else if (event == vtkCommand::InteractionEvent)
      {
      this->PointMovedSinceStartInteraction = true;
      this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointModifiedEvent, &this->LastInteractionEventMarkupIndex);
      }
    else if (event == vtkCommand::PlacePointEvent)
      {
      this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointAddedEvent, &this->LastInteractionEventMarkupIndex);
      }
  }

  void SetWidget(vtkSlicerAbstractWidget *w)
    {
    this->Widget = w;
    }
  void SetNode(vtkMRMLMarkupsNode *n)
    {
    this->Node = n;
    }
  void SetDisplayableManager(vtkMRMLMarkupsDisplayableManager3D * dm)
    {
    this->DisplayableManager = dm;
    }

  vtkSlicerAbstractWidget * Widget;
  vtkMRMLMarkupsNode * Node;
  vtkMRMLMarkupsDisplayableManager3D * DisplayableManager;
  int LastInteractionEventMarkupIndex;
  int SelectionButton;
  bool PointMovedSinceStartInteraction;
};

//---------------------------------------------------------------------------
// vtkMRMLMarkupsAngleDisplayableManager3D methods

//---------------------------------------------------------------------------
void vtkMRMLMarkupsAngleDisplayableManager3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->Helper->PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
vtkSlicerAbstractWidget * vtkMRMLMarkupsAngleDisplayableManager3D::CreateWidget(vtkMRMLMarkupsNode* node)
{
  if (!node)
    {
    vtkErrorMacro("CreateWidget: Node not set!")
    return nullptr;
    }

  vtkMRMLMarkupsAngleNode* angleNode = vtkMRMLMarkupsAngleNode::SafeDownCast(node);
  if (!angleNode)
    {
    return nullptr;
    }

  //SlicerAngle widget
  vtkSlicerAngleWidget *slicerAngleWidget = vtkSlicerAngleWidget::New();

  if (this->GetInteractor()->GetPickingManager())
    {
    if (!(this->GetInteractor()->GetPickingManager()->GetEnabled()))
      {
      // Managed picking is on by default on the widget, but the interactor
      // will need to have it's picking manager turned on once widgets are
      // going to be used to avoid dragging points that are behind others.
      // Enabling it before setting the interactor on the widget seems to
      // work better with tests of two angles.
      this->GetInteractor()->GetPickingManager()->EnabledOn();
      }
    }

  slicerAngleWidget->SetInteractor(this->GetInteractor());
  slicerAngleWidget->SetCurrentRenderer(this->GetRenderer());
  vtkMRMLInteractionNode *interactionNode = this->GetInteractionNode();
  if (interactionNode)
    {
    if (interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place)
      {
      slicerAngleWidget->SetManagesCursor(false);
      }
    else
      {
      slicerAngleWidget->SetManagesCursor(true);
      }
    }
  vtkDebugMacro("Fids CreateWidget: Created widget for node " << angleNode->GetID() << " with a representation");

  // Add the Representation
  vtkNew<vtkSlicerAngleRepresentation3D> rep;
  rep->SetRenderer(this->GetRenderer());
  rep->SetMarkupsNode(angleNode);
  slicerAngleWidget->SetRepresentation(rep);
  slicerAngleWidget->On();

  return slicerAngleWidget;
  }

//---------------------------------------------------------------------------
void vtkMRMLMarkupsAngleDisplayableManager3D::OnWidgetCreated(vtkSlicerAbstractWidget * widget, vtkMRMLMarkupsNode * node)
{
  if (!widget)
    {
    vtkErrorMacro("OnWidgetCreated: Widget was null!")
    return;
    }

  if (!node)
    {
    vtkErrorMacro("OnWidgetCreated: MRML node was null!")
    return;
    }

  // add the callback
  vtkMarkupsAngleWidgetCallback3D *myCallback = vtkMarkupsAngleWidgetCallback3D::New();
  myCallback->SetNode(node);
  myCallback->SetWidget(widget);
  myCallback->SetDisplayableManager(this);
  widget->AddObserver(vtkCommand::StartInteractionEvent,myCallback);
  widget->AddObserver(vtkCommand::EndInteractionEvent, myCallback);
  widget->AddObserver(vtkCommand::InteractionEvent, myCallback);
  widget->AddObserver(vtkCommand::PlacePointEvent, myCallback);
  widget->AddObserver(vtkCommand::DeletePointEvent, myCallback);
  myCallback->Delete();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsAngleDisplayableManager3D::AdditionnalInitializeStep()
{
  // don't add the key press event, as it triggers a crash on start up
  //vtkDebugMacro("Adding an observer on the key press event");
  this->AddInteractorStyleObservableEvent(vtkCommand::KeyPressEvent);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsAngleDisplayableManager3D::OnInteractorStyleEvent(int eventid)
{
  this->Superclass::OnInteractorStyleEvent(eventid);

  if (this->GetDisableInteractorStyleEventsProcessing())
    {
    vtkWarningMacro("OnInteractorStyleEvent: Processing of events was disabled.")
    return;
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
        vtkDebugMacro("Angle DisplayableManager: key press p, but not in Place mode! Returning.");
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
void vtkMRMLMarkupsAngleDisplayableManager3D::OnClickInRenderWindow(double x, double y,
                                                                    const char *associatedNodeID,
                                                                    int action /*= 0 */)
{
  if (!this->IsCorrectDisplayableManager())
    {
    // jump out
    vtkDebugMacro("OnClickInRenderWindow: x = " << x << ", y = " << y << ", incorrect displayable manager, focus = " << this->Focus << ", jumping out");
    return;
    }

  // Get World coordinates from the display ones
  double displayCoordinates[2], worldCoordinates[4];
  displayCoordinates[0] = x;
  displayCoordinates[1] = y;

  this->GetDisplayToWorldCoordinates(displayCoordinates, worldCoordinates);

  // Is there an active markups node that's a angle node?
  vtkMRMLMarkupsAngleNode *activeAngleNode = nullptr;
  vtkMRMLSelectionNode *selectionNode = this->GetSelectionNode();
  if (selectionNode)
    {
    const char *activeMarkupsID = selectionNode->GetActivePlaceNodeID();
    vtkMRMLNode *mrmlNode = this->GetMRMLScene()->GetNodeByID(activeMarkupsID);
    if (mrmlNode &&
        mrmlNode->IsA("vtkMRMLMarkupsAngleNode"))
      {
      activeAngleNode = vtkMRMLMarkupsAngleNode::SafeDownCast(mrmlNode);
      }
    else
      {
      vtkDebugMacro("OnClickInRenderWindow: active markup id = "
            << (activeMarkupsID ? activeMarkupsID : "null")
            << ", mrml node is "
            << (mrmlNode ? mrmlNode->GetID() : "null")
            << ", not a vtkMRMLMarkupsAngleNode");
      }
    }

  if (!activeAngleNode)
    {
    // create the MRML node
    activeAngleNode = vtkMRMLMarkupsAngleNode::SafeDownCast
      (this->GetMRMLScene()->AddNewNodeByClass("vtkMRMLMarkupsAngleNode"));
    activeAngleNode->SetName(this->GetMRMLScene()->GetUniqueNameByString("A"));
    activeAngleNode->AddDefaultStorageNode();
    activeAngleNode->CreateDefaultDisplayNodes();
    selectionNode->SetActivePlaceNodeID(activeAngleNode->GetID());
    }

  vtkSlicerAngleWidget *slicerWidget = vtkSlicerAngleWidget::SafeDownCast
    (this->Helper->GetWidget(activeAngleNode));
  if (slicerWidget == nullptr)
    {
    return;
    }

  vtkMRMLInteractionNode *interactionNode = this->GetInteractionNode();
  if (!interactionNode)
    {
    return;
    }

  // Check if the widget angle has been already place
  // if yes, create a new node.
  if (interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place &&
      slicerWidget->GetWidgetState() == vtkSlicerAngleWidget::Manipulate &&
      activeAngleNode->GetNumberOfPoints() < 3)
    {
    slicerWidget->SetWidgetState(vtkSlicerAngleWidget::Define);
    slicerWidget->SetFollowCursor(true);
    slicerWidget->SetManagesCursor(false);
    }

  if (interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place &&
      slicerWidget->GetWidgetState() == vtkSlicerAngleWidget::Manipulate)
    {
    activeAngleNode = vtkMRMLMarkupsAngleNode::SafeDownCast
      (this->GetMRMLScene()->AddNewNodeByClass("vtkMRMLMarkupsAngleNode"));
    activeAngleNode->SetName(this->GetMRMLScene()->GetUniqueNameByString("A"));
    activeAngleNode->AddDefaultStorageNode();
    activeAngleNode->CreateDefaultDisplayNodes();
    selectionNode->SetActivePlaceNodeID(activeAngleNode->GetID());
    slicerWidget = vtkSlicerAngleWidget::SafeDownCast
      (this->Helper->GetWidget(activeAngleNode));
    if (slicerWidget == nullptr)
      {
      return;
      }
    }

  // save for undo
  this->GetMRMLScene()->SaveStateForUndo();

  if (action == vtkMRMLMarkupsAngleDisplayableManager3D::AddPoint)
    {
    int pointIndex = slicerWidget->AddPointToRepresentationFromWorldCoordinate(worldCoordinates);
    // is there a node associated with this?
    if (associatedNodeID)
      {
      activeAngleNode->SetNthPointAssociatedNodeID(pointIndex, associatedNodeID);
      }
    }
  else if (action == vtkMRMLMarkupsAngleDisplayableManager3D::AddPreview)
    {
    int pointIndex = slicerWidget->AddPreviewPointToRepresentationFromWorldCoordinate(worldCoordinates);
    // is there a node associated with this?
    if (associatedNodeID)
      {
      activeAngleNode->SetNthPointAssociatedNodeID(pointIndex, associatedNodeID);
      }
    }
  else if (action == vtkMRMLMarkupsAngleDisplayableManager3D::RemovePreview)
    {
    slicerWidget->RemoveLastPreviewPointToRepresentation();
    }

  // if this was a one time place, go back to view transform mode
  if (interactionNode->GetPlaceModePersistence() == 0 &&
      slicerWidget->GetWidgetState() == vtkSlicerAngleWidget::Manipulate)
    {
    interactionNode->SwitchToViewTransformMode();
    }

  // if persistence and last widget is placed, add new markups and a previewPoint
  if (interactionNode->GetPlaceModePersistence() == 1 &&
      interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place &&
      action == vtkMRMLMarkupsAngleDisplayableManager3D::AddPoint &&
      slicerWidget->GetWidgetState() == vtkSlicerAngleWidget::Manipulate)
    {
    activeAngleNode = vtkMRMLMarkupsAngleNode::SafeDownCast
      (this->GetMRMLScene()->AddNewNodeByClass("vtkMRMLMarkupsAngleNode"));
    activeAngleNode->SetName(this->GetMRMLScene()->GetUniqueNameByString("A"));
    activeAngleNode->AddDefaultStorageNode();
    activeAngleNode->CreateDefaultDisplayNodes();
    selectionNode->SetActivePlaceNodeID(activeAngleNode->GetID());
    slicerWidget = vtkSlicerAngleWidget::SafeDownCast
      (this->Helper->GetWidget(activeAngleNode));
    if (slicerWidget == nullptr)
      {
      return;
      }
    int pointIndex = slicerWidget->AddPreviewPointToRepresentationFromWorldCoordinate(worldCoordinates);
    // is there a node associated with this?
    if (associatedNodeID)
      {
      activeAngleNode->SetNthPointAssociatedNodeID(pointIndex, associatedNodeID);
      }
    }

  // force update of widgets on other views
  activeAngleNode->GetMarkupsDisplayNode()->Modified();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsAngleDisplayableManager3D::OnMRMLSceneEndClose()
{
  // clear out the map of glyph types
  this->Helper->ClearNodeGlyphTypes();
}
