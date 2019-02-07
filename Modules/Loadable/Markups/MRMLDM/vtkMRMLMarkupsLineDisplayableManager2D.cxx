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
#include <vtkMRMLMarkupsLineNode.h>
#include <vtkMRMLMarkupsNode.h>
#include <vtkMRMLMarkupsDisplayNode.h>

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsLineDisplayableManager2D.h"

// MarkupsModule/VTKWidgets includes
#include <vtkMarkupsGlyphSource2D.h>

// MRMLDisplayableManager includes
#include <vtkSliceViewInteractorStyle.h>

// MRML includes
#include <vtkMRMLApplicationLogic.h>
#include <vtkMRMLInteractionNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLSliceLogic.h>
#include <vtkMRMLViewNode.h>

// VTK includes
#include <vtkAbstractWidget.h>
#include <vtkFollower.h>
#include <vtkHandleRepresentation.h>
#include <vtkInteractorStyle.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkOrientedPolygonalHandleRepresentation3D.h>
#include <vtkPickingManager.h>
#include <vtkPointHandleRepresentation2D.h>
#include <vtkProperty2D.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSlicerLineWidget.h>
#include <vtkSlicerLineRepresentation2D.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>

// STD includes
#include <sstream>
#include <string>

//---------------------------------------------------------------------------
vtkStandardNewMacro (vtkMRMLMarkupsLineDisplayableManager2D);

//---------------------------------------------------------------------------
// vtkMRMLMarkupsLineDisplayableManager2D Callback
/// \ingroup Slicer_QtModules_Markups
class vtkMarkupsLineWidgetCallback2D : public vtkCommand
{
public:
  static vtkMarkupsLineWidgetCallback2D *New()
  { return new vtkMarkupsLineWidgetCallback2D; }

  vtkMarkupsLineWidgetCallback2D()
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
  }

  void SetWidget(vtkSlicerAbstractWidget *w)
    {
    this->Widget = w;
    }
  void SetNode(vtkMRMLMarkupsNode *n)
    {
    this->Node = n;
    }
  void SetDisplayableManager(vtkMRMLMarkupsDisplayableManager2D * dm)
    {
    this->DisplayableManager = dm;
    }

  vtkSlicerAbstractWidget * Widget;
  vtkMRMLMarkupsNode * Node;
  vtkMRMLMarkupsDisplayableManager2D * DisplayableManager;
  int LastInteractionEventMarkupIndex;
  int SelectionButton;
  bool PointMovedSinceStartInteraction;
};

//---------------------------------------------------------------------------
// vtkMRMLMarkupsLineDisplayableManager2D methods

//---------------------------------------------------------------------------
void vtkMRMLMarkupsLineDisplayableManager2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->Helper->PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
vtkSlicerAbstractWidget * vtkMRMLMarkupsLineDisplayableManager2D::CreateWidget(vtkMRMLMarkupsNode* node)
{
  if (!node)
    {
    vtkErrorMacro("CreateWidget: Node not set!")
    return nullptr;
    }

  // 2d glyphs and text need to be scaled by 1/60 to show up properly in the 2d slice windows
  this->SetScaleFactor2D(0.01667);

  vtkMRMLMarkupsLineNode* lineNode = vtkMRMLMarkupsLineNode::SafeDownCast(node);
  if (!lineNode)
    {
    return nullptr;
    }

  //SlicerPoints widget
  vtkSlicerLineWidget * slicerLineWidget = vtkSlicerLineWidget::New();

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
  vtkNew<vtkSlicerLineRepresentation2D> rep;
  rep->SetRenderer(this->GetRenderer());
  rep->SetSliceNode(this->GetMRMLSliceNode());
  rep->SetMarkupsNode(lineNode);
  slicerLineWidget->SetRepresentation(rep);
  slicerLineWidget->On();

  return slicerLineWidget;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsLineDisplayableManager2D::OnWidgetCreated(vtkSlicerAbstractWidget * widget, vtkMRMLMarkupsNode * node)
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
  vtkMarkupsLineWidgetCallback2D *myCallback = vtkMarkupsLineWidgetCallback2D::New();
  myCallback->SetNode(node);
  myCallback->SetWidget(widget);
  myCallback->SetDisplayableManager(this);
  widget->AddObserver(vtkCommand::StartInteractionEvent, myCallback);
  widget->AddObserver(vtkCommand::EndInteractionEvent, myCallback);
  widget->AddObserver(vtkCommand::InteractionEvent,myCallback);
  widget->AddObserver(vtkCommand::PlacePointEvent, myCallback);
  widget->AddObserver(vtkCommand::DeletePointEvent, myCallback);
  myCallback->Delete();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsLineDisplayableManager2D::OnClickInRenderWindow(double x, double y,
                                                                   const char *associatedNodeID,
                                                                   int action /*= 0 */)
{
  if (!this->IsCorrectDisplayableManager())
    {
    // jump out
    vtkDebugMacro("OnClickInRenderWindow: x = " << x << ", y = " << y << ", incorrect displayable manager, focus = " << this->FocusStr << ", jumping out");
    return;
    }

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

  vtkMRMLInteractionNode *interactionNode = this->GetInteractionNode();
  if (!interactionNode)
    {
    return;
    }

  if (!activeLineNode &&
      interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place)
    {
    // create the MRML node
    activeLineNode = vtkMRMLMarkupsLineNode::SafeDownCast
      (this->GetMRMLScene()->AddNewNodeByClass("vtkMRMLMarkupsLineNode"));
    activeLineNode->SetName(this->GetMRMLScene()->GetUniqueNameByString("L"));
    activeLineNode->AddDefaultStorageNode();
    activeLineNode->CreateDefaultDisplayNodes();
    selectionNode->SetActivePlaceNodeID(activeLineNode->GetID());
    }

  vtkSlicerLineWidget *slicerWidget = vtkSlicerLineWidget::SafeDownCast
    (this->Helper->GetWidget(activeLineNode));
  if (slicerWidget == nullptr)
    {
    return;
    }

  // Check if the widget line has been already place
  // if yes, create a new node.
  if (interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place &&
      slicerWidget->GetWidgetState() == vtkSlicerLineWidget::Manipulate &&
      activeLineNode->GetNumberOfPoints() < 2)
    {
    slicerWidget->SetWidgetState(vtkSlicerLineWidget::Define);
    slicerWidget->SetFollowCursor(true);
    slicerWidget->SetManagesCursor(false);
    }

  if (interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place &&
      slicerWidget->GetWidgetState() == vtkSlicerLineWidget::Manipulate)
    {
    activeLineNode = vtkMRMLMarkupsLineNode::SafeDownCast
      (this->GetMRMLScene()->AddNewNodeByClass("vtkMRMLMarkupsLineNode"));
    activeLineNode->SetName(this->GetMRMLScene()->GetUniqueNameByString("L"));
    activeLineNode->AddDefaultStorageNode();
    activeLineNode->CreateDefaultDisplayNodes();
    selectionNode->SetActivePlaceNodeID(activeLineNode->GetID());
    slicerWidget = vtkSlicerLineWidget::SafeDownCast
      (this->Helper->GetWidget(activeLineNode));
    if (slicerWidget == nullptr)
      {
      return;
      }
    }

  // save for undo
  this->GetMRMLScene()->SaveStateForUndo();

  if (action == vtkMRMLMarkupsLineDisplayableManager2D::AddPoint)
    {
    int pointIndex = slicerWidget->AddPointToRepresentationFromWorldCoordinate(worldCoordinates);
    // is there a node associated with this?
    if (associatedNodeID)
      {
      activeLineNode->SetNthPointAssociatedNodeID(pointIndex, associatedNodeID);
      }
    }
  else if (action == vtkMRMLMarkupsLineDisplayableManager2D::AddPreview)
    {
    int pointIndex = slicerWidget->AddPreviewPointToRepresentationFromWorldCoordinate(worldCoordinates);
    // is there a node associated with this?
    if (associatedNodeID)
      {
      activeLineNode->SetNthPointAssociatedNodeID(pointIndex, associatedNodeID);
      }
    }
  else if (action == vtkMRMLMarkupsLineDisplayableManager2D::RemovePreview)
    {
    slicerWidget->RemoveLastPreviewPointToRepresentation();
    }

  // if this was a one time place, go back to view transform mode
  if (interactionNode->GetPlaceModePersistence() == 0 &&
      slicerWidget->GetWidgetState() == vtkSlicerLineWidget::Manipulate)
    {
    interactionNode->SwitchToViewTransformMode();
    }

  // if persistence and last widget is placed, add new markups and a previewPoint
  if (interactionNode->GetPlaceModePersistence() == 1 &&
      interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place &&
      action == vtkMRMLMarkupsLineDisplayableManager2D::AddPoint &&
      slicerWidget->GetWidgetState() == vtkSlicerLineWidget::Manipulate)
    {
    activeLineNode = vtkMRMLMarkupsLineNode::SafeDownCast
      (this->GetMRMLScene()->AddNewNodeByClass("vtkMRMLMarkupsLineNode"));
    activeLineNode->SetName(this->GetMRMLScene()->GetUniqueNameByString("L"));
    activeLineNode->AddDefaultStorageNode();
    activeLineNode->CreateDefaultDisplayNodes();
    selectionNode->SetActivePlaceNodeID(activeLineNode->GetID());
    slicerWidget = vtkSlicerLineWidget::SafeDownCast
      (this->Helper->GetWidget(activeLineNode));
    if (slicerWidget == nullptr)
      {
      return;
      }
    int pointIndex = slicerWidget->AddPreviewPointToRepresentationFromWorldCoordinate(worldCoordinates);
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
void vtkMRMLMarkupsLineDisplayableManager2D::OnMRMLSceneEndClose()
{
  // make sure to delete widgets and projections
  this->Superclass::OnMRMLSceneEndClose();

  // clear out the map of glyph types
  this->Helper->ClearNodeGlyphTypes();
}
