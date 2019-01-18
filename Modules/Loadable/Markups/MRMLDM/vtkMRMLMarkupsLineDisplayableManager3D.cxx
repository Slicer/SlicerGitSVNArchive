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
#include <vtkMRMLMarkupsLineNode.h>
#include <vtkMRMLMarkupsNode.h>
#include <vtkMRMLMarkupsDisplayNode.h>

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsLineDisplayableManager3D.h"

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
#include <vtkSlicerLineWidget.h>
#include <vtkSmartPointer.h>
#include <vtkSlicerLineRepresentation3D.h>
#include <vtkSphereSource.h>
#include <vtkTextProperty.h>

// STD includes
#include <sstream>
#include <string>

//---------------------------------------------------------------------------
vtkStandardNewMacro (vtkMRMLMarkupsLineDisplayableManager3D);

//---------------------------------------------------------------------------
// vtkMRMLMarkupsLineDisplayableManager3D Callback
/// \ingroup Slicer_QtModules_Markups
class vtkMarkupsLineWidgetCallback3D : public vtkCommand
{
public:
  static vtkMarkupsLineWidgetCallback3D *New()
  { return new vtkMarkupsLineWidgetCallback3D; }

  vtkMarkupsLineWidgetCallback3D()
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
      vtkSlicerLineWidget* slicerLineWidget = vtkSlicerLineWidget::SafeDownCast(this->Widget);
      if (slicerLineWidget)
        {
        this->SelectionButton = slicerLineWidget->GetSelectionButton();
        }
      // no need to propagate to MRML, just notify external observers that the user selected a markup
      return;
      }
    else if (event == vtkCommand::EndInteractionEvent)
      {
      this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointEndInteractionEvent, &this->LastInteractionEventMarkupIndex);
      if (!this->PointMovedSinceStartInteraction)
        {
        if (this->SelectionButton == vtkSlicerLineWidget::LeftButton)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointLeftClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerLineWidget::MiddleButton)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointMiddleClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerLineWidget::RightButton)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointRightClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerLineWidget::LeftButtonDoubleClick)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointLeftDoubleClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerLineWidget::MiddleButtonDoubleClick)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointMiddleDoubleClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerLineWidget::RightButtonDoubleClick)
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
// vtkMRMLMarkupsLineDisplayableManager3D methods

//---------------------------------------------------------------------------
void vtkMRMLMarkupsLineDisplayableManager3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->Helper->PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
/// Create a new widget.
vtkSlicerAbstractWidget * vtkMRMLMarkupsLineDisplayableManager3D::CreateWidget(vtkMRMLMarkupsNode* node)
{
  if (!node)
    {
    vtkErrorMacro("CreateWidget: Node not set!")
    return nullptr;
    }

  vtkMRMLMarkupsLineNode* lineNode = vtkMRMLMarkupsLineNode::SafeDownCast(node);
  if (!lineNode)
    {
    return nullptr;
    }

  //SlicerLine widget
  vtkSlicerLineWidget *slicerLineWidget = vtkSlicerLineWidget::New();

  if (this->GetInteractor()->GetPickingManager())
    {
    if (!(this->GetInteractor()->GetPickingManager()->GetEnabled()))
      {
      // Managed picking is on by default on the widget, but the interactor
      // will need to have it's picking manager turned on once widgets are
      // going to be used to avoid dragging points that are behind others.
      // Enabling it before setting the interactor on the widget seems to
      // work better with tests of two lines.
      this->GetInteractor()->GetPickingManager()->EnabledOn();
      }
    }

  slicerLineWidget->SetInteractor(this->GetInteractor());
  slicerLineWidget->SetCurrentRenderer(this->GetRenderer());
  vtkMRMLInteractionNode *interactionNode = this->GetInteractionNode();
  if (interactionNode)
    {
    if (interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place)
      {
      slicerLineWidget->SetManagesCursor(false);
      }
    else
      {
      slicerLineWidget->SetManagesCursor(true);
      }
    }
  vtkDebugMacro("Fids CreateWidget: Created widget for node " << lineNode->GetID() << " with a representation");

  // Add the Representation
  vtkNew<vtkSlicerLineRepresentation3D> rep;
  rep->SetRenderer(this->GetRenderer());
  rep->SetMarkupsNode(lineNode);
  slicerLineWidget->SetRepresentation(rep);
  slicerLineWidget->On();

  return slicerLineWidget;
  }

//---------------------------------------------------------------------------
/// Tear down the widget creation
void vtkMRMLMarkupsLineDisplayableManager3D::OnWidgetCreated(vtkSlicerAbstractWidget * widget, vtkMRMLMarkupsNode * node)
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
  vtkMarkupsLineWidgetCallback3D *myCallback = vtkMarkupsLineWidgetCallback3D::New();
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
/// observe key press events
void vtkMRMLMarkupsLineDisplayableManager3D::AdditionnalInitializeStep()
{
  // don't add the key press event, as it triggers a crash on start up
  //vtkDebugMacro("Adding an observer on the key press event");
  this->AddInteractorStyleObservableEvent(vtkCommand::KeyPressEvent);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsLineDisplayableManager3D::OnInteractorStyleEvent(int eventid)
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
        vtkDebugMacro("Line DisplayableManager: key press p, but not in Place mode! Returning.");
        return;
        }
      }
    }
  else if (eventid == vtkCommand::KeyReleaseEvent)
    {
    vtkDebugMacro("Got a key release event");
    }
  else if (eventid == vtkCommand::EnterEvent || eventid == vtkCommand::ExitEvent)
    {
    this->RequestRender();
    }
}

//---------------------------------------------------------------------------
/// Create a markupsMRMLnode
void vtkMRMLMarkupsLineDisplayableManager3D::OnClickInRenderWindow(double x, double y, const char *associatedNodeID)
{
  if (!this->IsCorrectDisplayableManager())
    {
    // jump out
    vtkDebugMacro("OnClickInRenderWindow: x = " << x << ", y = " << y << ", incorrect displayable manager, focus = " << this->Focus << ", jumping out");
    return;
    }

  // place the seed where the user clicked
  vtkDebugMacro("OnClickInRenderWindow: placing seed at " << x << ", " << y);

  // Get World coordinates from the display ones
  double displayCoordinates[2], worldCoordinates[4];
  displayCoordinates[0] = x;
  displayCoordinates[1] = y;

  this->GetDisplayToWorldCoordinates(displayCoordinates, worldCoordinates);

  // Is there an active markups node that's a line node?
  vtkMRMLMarkupsLineNode *activeLineNode = nullptr;
  vtkMRMLSelectionNode *selectionNode = this->GetSelectionNode();
  if (selectionNode)
    {
    const char *activeMarkupsID = selectionNode->GetActivePlaceNodeID();
    vtkMRMLNode *mrmlNode = this->GetMRMLScene()->GetNodeByID(activeMarkupsID);
    if (mrmlNode &&
        mrmlNode->IsA("vtkMRMLMarkupsLineNode"))
      {
      activeLineNode = vtkMRMLMarkupsLineNode::SafeDownCast(mrmlNode);
      }
    else
      {
      vtkDebugMacro("OnClickInRenderWindow: active markup id = "
            << (activeMarkupsID ? activeMarkupsID : "null")
            << ", mrml node is "
            << (mrmlNode ? mrmlNode->GetID() : "null")
            << ", not a vtkMRMLMarkupsLineNode");
      }
    }

  if (!activeLineNode)
    {
    // create the MRML node
    activeLineNode = vtkMRMLMarkupsLineNode::SafeDownCast
      (this->GetMRMLScene()->AddNewNodeByClass("vtkMRMLMarkupsLineNode"));
    activeLineNode->SetName(this->GetMRMLScene()->GetUniqueNameByString("L"));
    activeLineNode->AddDefaultStorageNode();
    activeLineNode->CreateDefaultDisplayNodes();
    selectionNode->SetActivePlaceNodeID(activeLineNode->GetID());
    }

  // Check if the widget line has been already place
  // if yes, create a new node.
  if (activeLineNode->GetPlacingEnded() == true &&
      activeLineNode->GetNumberOfPoints() < 2)
    {
    activeLineNode->SetPlacingEnded(false);
    }

  if (activeLineNode->GetPlacingEnded() == true)
    {
    activeLineNode = vtkMRMLMarkupsLineNode::SafeDownCast
      (this->GetMRMLScene()->AddNewNodeByClass("vtkMRMLMarkupsLineNode"));
    activeLineNode->SetName(this->GetMRMLScene()->GetUniqueNameByString("L"));
    activeLineNode->AddDefaultStorageNode();
    activeLineNode->CreateDefaultDisplayNodes();
    selectionNode->SetActivePlaceNodeID(activeLineNode->GetID());
    }

  // Check if activeLineNode has already one point.
  // If yes, place last point and go back to view transform mode
  if (activeLineNode->GetNumberOfPoints() > 0)
    {
    activeLineNode->SetPlacingEnded(true);
    }

  // if this was a one time place, go back to view transform mode
  vtkMRMLInteractionNode *interactionNode = this->GetInteractionNode();
  if (interactionNode && interactionNode->GetPlaceModePersistence() == 0 && activeLineNode->GetPlacingEnded())
    {
    vtkDebugMacro("End of one time place, place mode persistence = " << interactionNode->GetPlaceModePersistence());
    interactionNode->SwitchToViewTransformMode();
    }

  // save for undo and add the node to the scene after any reset of the
  // interaction node so that don't end up back in place mode
  this->GetMRMLScene()->SaveStateForUndo();

  int pointIndex = this->AddControlPoint(activeLineNode, worldCoordinates);
  // is there a node associated with this?
  if (associatedNodeID)
    {
    activeLineNode->SetNthPointAssociatedNodeID(pointIndex, associatedNodeID);
    }

  // Ended the persistence placing, but only one point for the last line.
  // Add a second point at the same coordinates
  if (interactionNode->GetPlaceModePersistence() == 1 &&
      interactionNode->GetCurrentInteractionMode() != vtkMRMLInteractionNode::Place)
    {
    pointIndex = this->AddControlPoint(activeLineNode, worldCoordinates);
    activeLineNode->SetPlacingEnded(true);
    // is there a node associated with this?
    if (associatedNodeID)
      {
      activeLineNode->SetNthPointAssociatedNodeID(pointIndex, associatedNodeID);
      }
    }

  // force update of widgets on other views
  activeLineNode->GetMarkupsDisplayNode()->Modified();
}

//---------------------------------------------------------------------------
int vtkMRMLMarkupsLineDisplayableManager3D::AddControlPoint(vtkMRMLMarkupsLineNode *markupsNode,
                                                            double worldCoordinates[4])
{
  vtkSlicerLineWidget *slicerWidget = vtkSlicerLineWidget::SafeDownCast
    (this->Helper->GetWidget(markupsNode));
  if (slicerWidget == nullptr)
    {
    return -1;
    }

  slicerWidget->AddPointToRepresentationFromWorldCoordinate(worldCoordinates);

  return markupsNode->GetNumberOfPoints() - 1;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsLineDisplayableManager3D::OnMRMLSceneEndClose()
{
  // clear out the map of glyph types
  this->Helper->ClearNodeGlyphTypes();
}
