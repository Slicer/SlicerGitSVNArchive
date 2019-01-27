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
#include <vtkMRMLMarkupsClosedCurveNode.h>
#include <vtkMRMLMarkupsNode.h>
#include <vtkMRMLMarkupsDisplayNode.h>

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsClosedCurveDisplayableManager3D.h"

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
#include <vtkSlicerClosedCurveWidget.h>
#include <vtkSmartPointer.h>
#include <vtkSlicerCurveRepresentation3D.h>
#include <vtkSphereSource.h>
#include <vtkTextProperty.h>

// STD includes
#include <sstream>
#include <string>

//---------------------------------------------------------------------------
vtkStandardNewMacro (vtkMRMLMarkupsClosedCurveDisplayableManager3D);

//---------------------------------------------------------------------------
// vtkMRMLMarkupsClosedCurveDisplayableManager3D Callback
/// \ingroup Slicer_QtModules_Markups
class vtkMarkupsCurveWidgetCallback3D : public vtkCommand
{
public:
  static vtkMarkupsCurveWidgetCallback3D *New()
  { return new vtkMarkupsCurveWidgetCallback3D; }

  vtkMarkupsCurveWidgetCallback3D()
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
      vtkSlicerClosedCurveWidget* slicerCurveWidget = vtkSlicerClosedCurveWidget::SafeDownCast(this->Widget);
      if (slicerCurveWidget)
        {
        this->SelectionButton = slicerCurveWidget->GetSelectionButton();
        }
      // no need to propagate to MRML, just notify external observers that the user selected a markup
      return;
      }
    else if (event == vtkCommand::EndInteractionEvent)
      {
      this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointEndInteractionEvent, &this->LastInteractionEventMarkupIndex);
      if (!this->PointMovedSinceStartInteraction)
        {
        if (this->SelectionButton == vtkSlicerClosedCurveWidget::LeftButton)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointLeftClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerClosedCurveWidget::MiddleButton)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointMiddleClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerClosedCurveWidget::RightButton)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointRightClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerClosedCurveWidget::LeftButtonDoubleClick)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointLeftDoubleClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerClosedCurveWidget::MiddleButtonDoubleClick)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointMiddleDoubleClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerClosedCurveWidget::RightButtonDoubleClick)
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
// vtkMRMLMarkupsClosedCurveDisplayableManager3D methods

//---------------------------------------------------------------------------
void vtkMRMLMarkupsClosedCurveDisplayableManager3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->Helper->PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
vtkSlicerAbstractWidget * vtkMRMLMarkupsClosedCurveDisplayableManager3D::CreateWidget(vtkMRMLMarkupsNode* node)
{
  if (!node)
    {
    vtkErrorMacro("CreateWidget: Node not set!")
    return nullptr;
    }

  vtkMRMLMarkupsClosedCurveNode* curveNode = vtkMRMLMarkupsClosedCurveNode::SafeDownCast(node);
  if (!curveNode)
    {
    return nullptr;
    }

  //SlicerCurve widget
  vtkSlicerClosedCurveWidget *slicerCurveWidget = vtkSlicerClosedCurveWidget::New();

  if (this->GetInteractor()->GetPickingManager())
    {
    if (!(this->GetInteractor()->GetPickingManager()->GetEnabled()))
      {
      // Managed picking is on by default on the widget, but the interactor
      // will need to have it's picking manager turned on once widgets are
      // going to be used to avoid dragging points that are behind others.
      // Enabling it before setting the interactor on the widget seems to
      // work better with tests of two curves.
      this->GetInteractor()->GetPickingManager()->EnabledOn();
      }
    }

  slicerCurveWidget->SetInteractor(this->GetInteractor());
  slicerCurveWidget->SetCurrentRenderer(this->GetRenderer());
  vtkMRMLInteractionNode *interactionNode = this->GetInteractionNode();
  if (interactionNode)
    {
    if (interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place)
      {
      slicerCurveWidget->SetManagesCursor(false);
      }
    else
      {
      slicerCurveWidget->SetManagesCursor(true);
      }
    }
  vtkDebugMacro("Fids CreateWidget: Created widget for node " << curveNode->GetID() << " with a representation");

  // Add the Representation
  vtkNew<vtkSlicerCurveRepresentation3D> rep;
  rep->SetRenderer(this->GetRenderer());
  rep->SetMarkupsNode(curveNode);
  rep->SetClosedLoop(true);
  slicerCurveWidget->SetRepresentation(rep);
  slicerCurveWidget->On();

  return slicerCurveWidget;
  }

//---------------------------------------------------------------------------
void vtkMRMLMarkupsClosedCurveDisplayableManager3D::OnWidgetCreated(vtkSlicerAbstractWidget * widget, vtkMRMLMarkupsNode * node)
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
  vtkMarkupsCurveWidgetCallback3D *myCallback = vtkMarkupsCurveWidgetCallback3D::New();
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
void vtkMRMLMarkupsClosedCurveDisplayableManager3D::OnClickInRenderWindow(double x, double y,
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

  // Is there an active markups node that's a curve node?
  vtkMRMLMarkupsClosedCurveNode *activeCurveNode = nullptr;
  vtkMRMLSelectionNode *selectionNode = this->GetSelectionNode();
  if (selectionNode)
    {
    const char *activeMarkupsID = selectionNode->GetActivePlaceNodeID();
    vtkMRMLNode *mrmlNode = this->GetMRMLScene()->GetNodeByID(activeMarkupsID);
    if (mrmlNode &&
        mrmlNode->IsA("vtkMRMLMarkupsClosedCurveNode"))
      {
      activeCurveNode = vtkMRMLMarkupsClosedCurveNode::SafeDownCast(mrmlNode);
      }
    else
      {
      vtkDebugMacro("OnClickInRenderWindow: active markup id = "
            << (activeMarkupsID ? activeMarkupsID : "null")
            << ", mrml node is "
            << (mrmlNode ? mrmlNode->GetID() : "null")
            << ", not a vtkMRMLMarkupsClosedCurveNode");
      }
    }

  if (!activeCurveNode)
    {
    // create the MRML node
    activeCurveNode = vtkMRMLMarkupsClosedCurveNode::SafeDownCast
      (this->GetMRMLScene()->AddNewNodeByClass("vtkMRMLMarkupsClosedCurveNode"));
    activeCurveNode->SetName(this->GetMRMLScene()->GetUniqueNameByString("C"));
    activeCurveNode->AddDefaultStorageNode();
    activeCurveNode->CreateDefaultDisplayNodes();
    selectionNode->SetActivePlaceNodeID(activeCurveNode->GetID());
    }

  vtkSlicerClosedCurveWidget *slicerWidget = vtkSlicerClosedCurveWidget::SafeDownCast
    (this->Helper->GetWidget(activeCurveNode));
  if (slicerWidget == nullptr)
    {
    return;
    }

  vtkMRMLInteractionNode *interactionNode = this->GetInteractionNode();
  if (!interactionNode)
    {
    return;
    }

  // Check if the widget has been already place
  // if yes, set again to define
  if (interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place &&
      slicerWidget->GetWidgetState() == vtkSlicerClosedCurveWidget::Manipulate)
    {
    slicerWidget->SetWidgetState(vtkSlicerClosedCurveWidget::Define);
    slicerWidget->SetFollowCursor(true);
    slicerWidget->SetManagesCursor(false);
    }

  // save for undo
  this->GetMRMLScene()->SaveStateForUndo();

  if (action == vtkMRMLMarkupsClosedCurveDisplayableManager3D::AddPoint)
    {
    int pointIndex = slicerWidget->AddPointToRepresentationFromWorldCoordinate(worldCoordinates, interactionNode->GetPlaceModePersistence());
    // is there a node associated with this?
    if (associatedNodeID)
      {
      activeCurveNode->SetNthPointAssociatedNodeID(pointIndex, associatedNodeID);
      }
    }
  else if (action == vtkMRMLMarkupsClosedCurveDisplayableManager3D::AddPreview)
    {
    int pointIndex = slicerWidget->AddPreviewPointToRepresentationFromWorldCoordinate(worldCoordinates);
    // is there a node associated with this?
    if (associatedNodeID)
      {
      activeCurveNode->SetNthPointAssociatedNodeID(pointIndex, associatedNodeID);
      }
    }
  else if (action == vtkMRMLMarkupsClosedCurveDisplayableManager3D::RemovePreview)
    {
    slicerWidget->RemoveLastPreviewPointToRepresentation();
    }

  // if this was a one time place, go back to view transform mode
  if (interactionNode->GetPlaceModePersistence() == 0 &&
      slicerWidget->GetWidgetState() == vtkSlicerClosedCurveWidget::Manipulate)
    {
    interactionNode->SwitchToViewTransformMode();
    }

  // force update of widgets on other views
  activeCurveNode->GetMarkupsDisplayNode()->Modified();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsClosedCurveDisplayableManager3D::OnMRMLSceneEndClose()
{
  // clear out the map of glyph types
  this->Helper->ClearNodeGlyphTypes();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsClosedCurveDisplayableManager3D::OnInteractorStyleEvent(int eventid)
{
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
    if (this->GetInteractionNode()->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place)
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
