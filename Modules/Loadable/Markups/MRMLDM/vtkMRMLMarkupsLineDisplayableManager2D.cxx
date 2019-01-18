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
      vtkSlicerLineWidget* slicerPointsWidget = vtkSlicerLineWidget::SafeDownCast(this->Widget);
      if (slicerPointsWidget)
        {
        this->SelectionButton = slicerPointsWidget->GetSelectionButton();
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
/// Create a new seed widget.
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
void vtkMRMLMarkupsLineDisplayableManager2D::OnMRMLMarkupsNthPointModifiedEvent(vtkMRMLNode *node, int n)
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
void vtkMRMLMarkupsLineDisplayableManager2D::OnMRMLMarkupsPointAddedEvent(vtkMRMLNode *node, int vtkNotUsed(n))
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

  // line widgets have only one Markup/Representation
  vtkSlicerAbstractRepresentation2D *rep = vtkSlicerAbstractRepresentation2D::SafeDownCast
    (widget->GetRepresentation());
  if (!rep || !rep->GetVisibility())
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
/// Create a markupsMRMLnode
void vtkMRMLMarkupsLineDisplayableManager2D::OnClickInRenderWindow(double x, double y, const char *associatedNodeID)
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
/// observe key press events
void vtkMRMLMarkupsLineDisplayableManager2D::AdditionnalInitializeStep()
{
  // don't add the key press event, as it triggers a crash on start up
  //vtkDebugMacro("Adding an observer on the key press event");
  this->AddInteractorStyleObservableEvent(vtkCommand::KeyPressEvent);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsLineDisplayableManager2D::OnInteractorStyleEvent(int eventid)
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
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsLineDisplayableManager2D::OnMRMLSceneEndClose()
{
  // make sure to delete widgets and projections
  this->Superclass::OnMRMLSceneEndClose();

  // clear out the map of glyph types
  this->Helper->ClearNodeGlyphTypes();
}

//---------------------------------------------------------------------------
int vtkMRMLMarkupsLineDisplayableManager2D::AddControlPoint(vtkMRMLMarkupsLineNode *markupsNode,
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
bool vtkMRMLMarkupsLineDisplayableManager2D::IsPointDisplayableOnSlice(vtkMRMLMarkupsNode *node, int pointIndex)
{
  if (this->IsInLightboxMode())
    {
    // TBD: issue 1690: disable line in light box mode as they appear
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
  bool inViewport = false;

  // allow annotations to appear only in designated viewers
  vtkMRMLDisplayNode *displayNode = node->GetDisplayNode();
  if (displayNode && !displayNode->IsDisplayableInView(sliceNode->GetID()))
    {
    return 0;
    }

  // down cast the node as a controlpoints node to get the coordinates
  vtkMRMLMarkupsNode *controlPointsNode = vtkMRMLMarkupsNode::SafeDownCast(node);

  if (!controlPointsNode)
    {
    vtkErrorMacro("IsWidgetDisplayableOnSlice: Could not get the controlpoints node.")
    return 0;
    }

  double transformedWorldCoordinates[4];
  controlPointsNode->GetNthControlPointPositionWorld(pointIndex, transformedWorldCoordinates);

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
    int lightboxIndex = this->GetLightboxIndex(controlPointsNode, pointIndex);
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
      int lightboxIndex = this->GetLightboxIndex(controlPointsNode, pointIndex);
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

  // -----------------------------------------
  // special cases when the slices get panned:

  // if all of the controlpoints are outside the viewport coordinates, the widget should not be shown
  // if one controlpoint is inside the viewport coordinates, the widget should be shown

  // we need to check if we are inside the viewport
  double coords[2] = {displayCoordinates[0], displayCoordinates[1]};

  vtkRenderer* pokedRenderer = this->GetInteractor()->
    FindPokedRenderer(static_cast<int>(coords[0]), static_cast<int>(coords[1]));
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

  return showPoint && inViewport;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsLineDisplayableManager2D::OnMRMLDisplayableNodeModifiedEvent(vtkObject *caller)
{
  this->Superclass::OnMRMLDisplayableNodeModifiedEvent(caller);

  if (!this->GetMRMLScene())
    {
    return;
    }

  vtkSmartPointer<vtkCollection> linesMarkupsNodes = vtkSmartPointer<vtkCollection>::Take
        (this->GetMRMLScene()->GetNodesByClass("vtkMRMLMarkupsLineNode"));

  for (int lineMarkupsIndex = 0; lineMarkupsIndex < linesMarkupsNodes->GetNumberOfItems(); lineMarkupsIndex++)
    {
    vtkMRMLMarkupsLineNode* lineMarkupsNode = vtkMRMLMarkupsLineNode::SafeDownCast
          (linesMarkupsNodes->GetItemAsObject(lineMarkupsIndex));
    if (!lineMarkupsNode)
      {
      continue;
      }

    vtkSlicerAbstractWidget *widget = this->Helper->GetWidget(lineMarkupsNode);
    if (!widget)
      {
      continue;
      }

    // line widgets have only one Markup/Representation
    vtkSlicerAbstractRepresentation2D *rep = vtkSlicerAbstractRepresentation2D::SafeDownCast
      (widget->GetRepresentation());
    if (!rep || !rep->GetVisibility())
      {
      continue;
      }

    for (int PointIndex = 0; PointIndex < lineMarkupsNode->GetNumberOfPoints(); PointIndex++)
      {
      bool visibility = this->IsPointDisplayableOnSlice(lineMarkupsNode, PointIndex);
      rep->SetNthPointSliceVisibility(PointIndex, visibility);
      }

    widget->BuildRepresentation();
    }

  this->RequestRender();
}
