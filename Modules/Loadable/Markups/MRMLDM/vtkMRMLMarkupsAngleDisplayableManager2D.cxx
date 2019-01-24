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
#include <vtkMRMLMarkupsAngleNode.h>
#include <vtkMRMLMarkupsNode.h>
#include <vtkMRMLMarkupsDisplayNode.h>

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsAngleDisplayableManager2D.h"

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
#include <vtkSlicerAngleWidget.h>
#include <vtkSlicerAngleRepresentation2D.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>

// STD includes
#include <sstream>
#include <string>

//---------------------------------------------------------------------------
vtkStandardNewMacro (vtkMRMLMarkupsAngleDisplayableManager2D);

//---------------------------------------------------------------------------
// vtkMRMLMarkupsAngleDisplayableManager2D Callback
/// \ingroup Slicer_QtModules_Markups
class vtkMarkupsAngleWidgetCallback2D : public vtkCommand
{
public:
  static vtkMarkupsAngleWidgetCallback2D *New()
  { return new vtkMarkupsAngleWidgetCallback2D; }

  vtkMarkupsAngleWidgetCallback2D()
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
// vtkMRMLMarkupsAngleDisplayableManager2D methods

//---------------------------------------------------------------------------
void vtkMRMLMarkupsAngleDisplayableManager2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->Helper->PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
/// Create a new seed widget.
vtkSlicerAbstractWidget * vtkMRMLMarkupsAngleDisplayableManager2D::CreateWidget(vtkMRMLMarkupsNode* node)
{
  if (!node)
    {
    vtkErrorMacro("CreateWidget: Node not set!")
    return nullptr;
    }

  // 2d glyphs and text need to be scaled by 1/60 to show up properly in the 2d slice windows
  this->SetScaleFactor2D(0.01667);

  vtkMRMLMarkupsAngleNode* angleNode = vtkMRMLMarkupsAngleNode::SafeDownCast(node);
  if (!angleNode)
    {
    return nullptr;
    }

  // widget
  vtkSlicerAngleWidget * slicerAngleWidget = vtkSlicerAngleWidget::New();

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
  vtkNew<vtkSlicerAngleRepresentation2D> rep;
  rep->SetRenderer(this->GetRenderer());
  rep->SetSliceNode(this->GetMRMLSliceNode());
  rep->SetMarkupsNode(angleNode);
  slicerAngleWidget->SetRepresentation(rep);
  slicerAngleWidget->On();

  return slicerAngleWidget;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsAngleDisplayableManager2D::OnMRMLMarkupsNthPointModifiedEvent(vtkMRMLNode *node, int n)
{
  vtkDebugMacro("OnMRMLMarkupsPointModifiedEvent");
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
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsAngleDisplayableManager2D::OnMRMLMarkupsPointAddedEvent(vtkMRMLNode *node, int vtkNotUsed(n))
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

  // angle widgets have only one Markup/Representation
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
/// Tear down the widget creation
void vtkMRMLMarkupsAngleDisplayableManager2D::OnWidgetCreated(vtkSlicerAbstractWidget * widget, vtkMRMLMarkupsNode * node)
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
  vtkMarkupsAngleWidgetCallback2D *myCallback = vtkMarkupsAngleWidgetCallback2D::New();
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
/// Create a markupsMRMLnode
void vtkMRMLMarkupsAngleDisplayableManager2D::OnClickInRenderWindow(double x, double y, const char *associatedNodeID)
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
    activeAngleNode->SetName(this->GetMRMLScene()->GetUniqueNameByString("L"));
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
  if (slicerWidget->GetWidgetState() == vtkSlicerAngleWidget::Manipulate &&
      activeAngleNode->GetNumberOfPoints() < 2)
    {
    slicerWidget->SetWidgetState(vtkSlicerAngleWidget::Define);
    slicerWidget->SetFollowCursor(true);
    slicerWidget->SetManagesCursor(false);
    }

  if (slicerWidget->GetWidgetState() == vtkSlicerAngleWidget::Manipulate)
    {
    activeAngleNode = vtkMRMLMarkupsAngleNode::SafeDownCast
      (this->GetMRMLScene()->AddNewNodeByClass("vtkMRMLMarkupsAngleNode"));
    activeAngleNode->SetName(this->GetMRMLScene()->GetUniqueNameByString("L"));
    activeAngleNode->AddDefaultStorageNode();
    activeAngleNode->CreateDefaultDisplayNodes();
    selectionNode->SetActivePlaceNodeID(activeAngleNode->GetID());
    slicerWidget = vtkSlicerAngleWidget::SafeDownCast
      (this->Helper->GetWidget(activeAngleNode));
    }

  // save for undo and add the node to the scene after any reset of the
  // interaction node so that don't end up back in place mode
  this->GetMRMLScene()->SaveStateForUndo();

  int pointIndex = this->AddControlPoint(activeAngleNode, worldCoordinates);
  // is there a node associated with this?
  if (associatedNodeID)
    {
    activeAngleNode->SetNthPointAssociatedNodeID(pointIndex, associatedNodeID);
    }

  // if this was a one time place, go back to view transform mode
  if (interactionNode->GetPlaceModePersistence() == 0 &&
      slicerWidget->GetWidgetState() == vtkSlicerAngleWidget::Manipulate)
    {
    interactionNode->SwitchToViewTransformMode();
    }

  // force update of widgets on other views
  activeAngleNode->GetMarkupsDisplayNode()->Modified();
}

//---------------------------------------------------------------------------
/// observe key press events
void vtkMRMLMarkupsAngleDisplayableManager2D::AdditionnalInitializeStep()
{
  // don't add the key press event, as it triggers a crash on start up
  //vtkDebugMacro("Adding an observer on the key press event");
  this->AddInteractorStyleObservableEvent(vtkCommand::KeyPressEvent);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsAngleDisplayableManager2D::OnInteractorStyleEvent(int eventid)
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
void vtkMRMLMarkupsAngleDisplayableManager2D::OnMRMLSceneEndClose()
{
  // make sure to delete widgets and projections
  this->Superclass::OnMRMLSceneEndClose();

  // clear out the map of glyph types
  this->Helper->ClearNodeGlyphTypes();
}

//---------------------------------------------------------------------------
int vtkMRMLMarkupsAngleDisplayableManager2D::AddControlPoint(vtkMRMLMarkupsAngleNode *markupsNode,
                                                             double worldCoordinates[4])
{
  vtkSlicerAngleWidget *slicerWidget = vtkSlicerAngleWidget::SafeDownCast
    (this->Helper->GetWidget(markupsNode));
  if (slicerWidget == nullptr)
    {
    return -1;
    }

  slicerWidget->AddPointToRepresentationFromWorldCoordinate(worldCoordinates);

  return markupsNode->GetNumberOfPoints() - 1;
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsAngleDisplayableManager2D::IsPointDisplayableOnSlice(vtkMRMLMarkupsNode *node, int pointIndex)
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
  vtkMRMLMarkupsAngleNode *angleNode = vtkMRMLMarkupsAngleNode::SafeDownCast(node);
  if (!angleNode)
    {
    vtkErrorMacro("IsWidgetDisplayableOnSlice: Could not get the controlpoints node.")
    return 0;
    }

  double transformedWorldCoordinates[4];
  angleNode->GetNthControlPointPositionWorld(pointIndex, transformedWorldCoordinates);

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
    int lightboxIndex = this->GetLightboxIndex(angleNode, pointIndex);
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
      int lightboxIndex = this->GetLightboxIndex(angleNode, pointIndex);
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
void vtkMRMLMarkupsAngleDisplayableManager2D::OnMRMLDisplayableNodeModifiedEvent(vtkObject *caller)
{
  this->Superclass::OnMRMLDisplayableNodeModifiedEvent(caller);

  if (!this->GetMRMLScene())
    {
    return;
    }

  vtkSmartPointer<vtkCollection> anglesMarkupsNodes = vtkSmartPointer<vtkCollection>::Take
        (this->GetMRMLScene()->GetNodesByClass("vtkMRMLMarkupsAngleNode"));

  for (int angleMarkupsIndex = 0; angleMarkupsIndex < anglesMarkupsNodes->GetNumberOfItems(); angleMarkupsIndex++)
    {
    vtkMRMLMarkupsAngleNode* angleMarkupsNode = vtkMRMLMarkupsAngleNode::SafeDownCast
          (anglesMarkupsNodes->GetItemAsObject(angleMarkupsIndex));
    if (!angleMarkupsNode)
      {
      continue;
      }

    vtkSlicerAbstractWidget *widget = this->Helper->GetWidget(angleMarkupsNode);
    if (!widget)
      {
      continue;
      }

    // angle widgets have only one Markup/Representation
    vtkSlicerAbstractRepresentation2D *rep = vtkSlicerAbstractRepresentation2D::SafeDownCast
      (widget->GetRepresentation());
    if (!rep)
      {
      continue;
      }

    for (int PointIndex = 0; PointIndex < angleMarkupsNode->GetNumberOfPoints(); PointIndex++)
      {
      bool visibility = this->IsPointDisplayableOnSlice(angleMarkupsNode, PointIndex);
      rep->SetNthPointSliceVisibility(PointIndex, visibility);
      }

    widget->BuildRepresentation();
    }

  this->RequestRender();
}
