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
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLMarkupsLineNode.h>
#include <vtkMRMLMarkupsNode.h>
#include <vtkMRMLMarkupsDisplayNode.h>

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsFiducialDisplayableManager2D.h"

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
#include <vtkSlicerPointsWidget.h>
#include <vtkSlicerPointsRepresentation2D.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>

#include <vtkSlicerLineWidget.h>
#include <vtkSlicerLineRepresentation2D.h>

// STD includes
#include <sstream>
#include <string>

//---------------------------------------------------------------------------
vtkStandardNewMacro (vtkMRMLMarkupsFiducialDisplayableManager2D);

//---------------------------------------------------------------------------
// vtkMRMLMarkupsFiducialDisplayableManager2D Callback
/// \ingroup Slicer_QtModules_Markups
class vtkMarkupsFiducialWidgetCallback2D : public vtkCommand
{
public:
  static vtkMarkupsFiducialWidgetCallback2D *New()
  { return new vtkMarkupsFiducialWidgetCallback2D; }

  vtkMarkupsFiducialWidgetCallback2D()
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
      if (this->Widget)
        {
        this->SelectionButton = this->Widget->GetSelectionButton();
        }
      // no need to propagate to MRML, just notify external observers that the user selected a markup
      return;
      }
    else if (event == vtkCommand::EndInteractionEvent)
      {
      this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointEndInteractionEvent, &this->LastInteractionEventMarkupIndex);
      if (!this->PointMovedSinceStartInteraction)
        {
        if (this->SelectionButton == vtkSlicerAbstractWidget::LeftButton)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointLeftClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerAbstractWidget::MiddleButton)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointMiddleClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerAbstractWidget::RightButton)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointRightClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerAbstractWidget::LeftButtonDoubleClick)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointLeftDoubleClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerAbstractWidget::MiddleButtonDoubleClick)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointMiddleDoubleClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerAbstractWidget::RightButtonDoubleClick)
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
// vtkMRMLMarkupsFiducialDisplayableManager2D methods

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->Helper->PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
/// Create a new seed widget.
vtkSlicerAbstractWidget * vtkMRMLMarkupsFiducialDisplayableManager2D::CreateWidget(vtkMRMLMarkupsNode* markupsNode)
{
  if (!markupsNode)
    {
    vtkErrorMacro("CreateWidget: Node not set!")
    return nullptr;
    }

  // 2d glyphs and text need to be scaled by 1/60 to show up properly in the 2d slice windows
  this->SetScaleFactor2D(0.01667);

  vtkMRMLMarkupsFiducialNode* fiducialNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(markupsNode);
  vtkMRMLMarkupsLineNode* lineNode = vtkMRMLMarkupsLineNode::SafeDownCast(markupsNode);

  vtkSlicerAbstractWidget* widget = NULL;
  vtkSlicerAbstractRepresentation2D* rep = NULL;
  if (fiducialNode)
    {
    widget = vtkSlicerPointsWidget::New();
    rep = vtkSlicerPointsRepresentation2D::New();
    }
  else if (lineNode)
    {
    widget = vtkSlicerLineWidget::New();
    rep = vtkSlicerLineRepresentation2D::New();
    }
  else
    {
    return nullptr;
    }

  //Set up widget
  widget->SetInteractor(this->GetInteractor());
  widget->SetCurrentRenderer(this->GetRenderer());
  vtkMRMLInteractionNode *interactionNode = this->GetInteractionNode();
  if (interactionNode)
    {
    if (interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place)
      {
      widget->SetManagesCursor(false);
      }
    else
      {
      widget->SetManagesCursor(true);
      }
    }
  vtkDebugMacro("Fids CreateWidget: Created widget for node " << markupsNode->GetID() << " with a representation");

  // Set up representation
  rep->SetRenderer(this->GetRenderer());
  rep->SetSliceNode(this->GetMRMLSliceNode());
  rep->SetMarkupsNode(markupsNode);
  widget->SetRepresentation(rep);
  widget->On();

  return widget;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager2D::OnWidgetCreated(vtkSlicerAbstractWidget * widget, vtkMRMLMarkupsNode * node)
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
  vtkMarkupsFiducialWidgetCallback2D *myCallback = vtkMarkupsFiducialWidgetCallback2D::New();
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
vtkMRMLMarkupsNode* vtkMRMLMarkupsFiducialDisplayableManager2D::CreateNewMarkupsNode(
  const std::string &markupsNodeClassName)
{
  // create the MRML node
  std::string nodeName = "M";
  if (markupsNodeClassName == "vtkMRMLMarkupsFiducialNode")
    {
    nodeName = "F";
    }
  else if (markupsNodeClassName == "vtkMRMLMarkupsLineNode")
    {
    nodeName = "L";
    }
  vtkMRMLMarkupsNode* markupsNode = vtkMRMLMarkupsNode::SafeDownCast(
    this->GetMRMLScene()->AddNewNodeByClass(markupsNodeClassName, nodeName));
  markupsNode->AddDefaultStorageNode();
  markupsNode->CreateDefaultDisplayNodes();
  return markupsNode;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager2D::OnClickInRenderWindow(double x, double y,
                                                                       const char *associatedNodeID,
                                                                       int action /*= 0 */)
{
  if (!this->IsCorrectDisplayableManager())// &&
      //action != vtkMRMLMarkupsFiducialDisplayableManager3D::RemovePreview)
    {
    // jump out
    vtkDebugMacro("OnClickInRenderWindow: x = " << x << ", y = " << y << ", incorrect displayable manager, focus = " << this->FocusStr << ", jumping out");
    return;
    }

  vtkMRMLInteractionNode *interactionNode = this->GetInteractionNode();
  vtkMRMLSelectionNode *selectionNode = this->GetSelectionNode();
  if (!interactionNode || !selectionNode)
    {
    return;
    }

  std::string placeNodeClassName = (selectionNode->GetActivePlaceNodeClassName() ? selectionNode->GetActivePlaceNodeClassName() : nullptr);
  if (!this->IsManageable(placeNodeClassName.c_str()))
    {
    return;
    }

  // place the seed where the user clicked
  vtkDebugMacro("OnClickInRenderWindow: placing seed at " << x << ", " << y);

  // Get World coordinates from the display ones
  double displayCoordinates[2], worldCoordinates[4];
  displayCoordinates[0] = x;
  displayCoordinates[1] = y;

  this->GetDisplayToWorldCoordinates(displayCoordinates, worldCoordinates);

  // Is there an active markups node that's a fiducial node?
  //vtkMRMLMarkupsFiducialNode *activeFiducialNode = nullptr;
  vtkMRMLMarkupsNode *activeMarkupNode = nullptr;
  const char *activeMarkupsID = selectionNode->GetActivePlaceNodeID();
  vtkMRMLNode *mrmlNode = this->GetMRMLScene()->GetNodeByID(activeMarkupsID);
  if (mrmlNode && mrmlNode->GetClassName() == placeNodeClassName)
    {
    activeMarkupNode = vtkMRMLMarkupsNode::SafeDownCast(mrmlNode);
    }
  else
    {
    vtkDebugMacro("OnClickInRenderWindow: active markup id = "
          << (activeMarkupsID ? activeMarkupsID : "null")
          << ", mrml node is "
          << (mrmlNode ? mrmlNode->GetID() : "null")
          << ", not a supported node");
    }

  // If there is no active markups node then create a new one
  if (interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place && !activeMarkupNode)
    {
    activeMarkupNode = this->CreateNewMarkupsNode(placeNodeClassName);
    selectionNode->SetActivePlaceNodeID(activeMarkupNode->GetID());
    }
  if (!activeMarkupNode)
    {
    return;
    }
  vtkSlicerAbstractWidget *slicerWidget = this->Helper->GetWidget(activeMarkupNode);

  // If we reached the maximum number of points that can be added for a widget then create a new node
  if (interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place)
    {
    int maxNumberOfPoints = 0;
    if (placeNodeClassName == "vtkMRMLMarkupsFiducialNode")
      {
      maxNumberOfPoints = INT_MAX;
      }
    else if (placeNodeClassName == "vtkMRMLMarkupsLineNode")
      {
      maxNumberOfPoints = 2;
      }
    if (slicerWidget && slicerWidget->GetWidgetState() == vtkSlicerAbstractWidget::Manipulate
      && activeMarkupNode->GetNumberOfControlPoints() >= maxNumberOfPoints)
      {
      activeMarkupNode = this->CreateNewMarkupsNode(placeNodeClassName);
      selectionNode->SetActivePlaceNodeID(activeMarkupNode->GetID());
      slicerWidget = this->Helper->GetWidget(activeMarkupNode);
      }
    }

  if (slicerWidget == nullptr)
    {
    return;
    }

  // Check if the widget has been already placed
  // if yes, set again to define
  if (interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place
      && slicerWidget->GetWidgetState() == vtkSlicerAbstractWidget::Manipulate)
    {
    slicerWidget->SetWidgetState(vtkSlicerAbstractWidget::Define);
    slicerWidget->SetFollowCursor(true);
    slicerWidget->SetManagesCursor(false);
    }

  // save for undo
  this->GetMRMLScene()->SaveStateForUndo();

  if (action == vtkMRMLMarkupsFiducialDisplayableManager2D::AddPoint)
    {
    int pointIndex = slicerWidget->AddPointToRepresentationFromWorldCoordinate(worldCoordinates, interactionNode->GetPlaceModePersistence());
    // is there a node associated with this?
    if (associatedNodeID)
      {
      activeMarkupNode->SetNthControlPointAssociatedNodeID(pointIndex, associatedNodeID);
      }
    }
  else if (action == vtkMRMLMarkupsFiducialDisplayableManager2D::AddPreview)
    {
    int pointIndex = slicerWidget->AddPreviewPointToRepresentationFromWorldCoordinate(worldCoordinates);
    // is there a node associated with this?
    if (associatedNodeID)
      {
      activeMarkupNode->SetNthControlPointAssociatedNodeID(pointIndex, associatedNodeID);
      }
    }
  else if (action == vtkMRMLMarkupsFiducialDisplayableManager2D::RemovePreview)
    {
    slicerWidget->RemoveLastPreviewPointToRepresentation();
    }

  // if this was a one time place, go back to view transform mode
  if (interactionNode->GetPlaceModePersistence() == 0 &&
      slicerWidget->GetWidgetState() == vtkSlicerAbstractWidget::Manipulate)
    {
    interactionNode->SwitchToViewTransformMode();
    }

  // if persistence and last point is placed, add new markups node and a previewPoint
  if (interactionNode->GetPlaceModePersistence() == 1 &&
    interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place &&
    action == vtkMRMLMarkupsDisplayableManager2D::AddPoint &&
    slicerWidget->GetWidgetState() == vtkSlicerLineWidget::Manipulate)
    {
    activeMarkupNode = this->CreateNewMarkupsNode(placeNodeClassName);
    selectionNode->SetActivePlaceNodeID(activeMarkupNode->GetID());
    slicerWidget = this->Helper->GetWidget(activeMarkupNode);
    if (slicerWidget)
      {
      int pointIndex = slicerWidget->AddPreviewPointToRepresentationFromWorldCoordinate(worldCoordinates);
      // is there a node associated with this?
      if (associatedNodeID)
        {
        activeMarkupNode->SetNthControlPointAssociatedNodeID(pointIndex, associatedNodeID);
        }
      }
    }

  // force update of widgets on other views
  activeMarkupNode->GetMarkupsDisplayNode()->Modified();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager2D::OnMRMLSceneEndClose()
{
  // make sure to delete widgets and projections
  this->Superclass::OnMRMLSceneEndClose();

  // clear out the map of glyph types
  this->Helper->ClearNodeGlyphTypes();
}
