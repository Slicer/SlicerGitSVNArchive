
// AnnotationModule/Logic includes
#include <vtkSlicerAnnotationModuleLogic.h>

// AnnotationModule/MRML includes
#include <vtkMRMLAnnotationRulerNode.h>
#include <vtkMRMLAnnotationNode.h>
#include <vtkMRMLAnnotationDisplayableManager.h>
#include <vtkMRMLAnnotationPointDisplayNode.h>
#include <vtkMRMLAnnotationLineDisplayNode.h>
#include <vtkMRMLAnnotationTextDisplayNode.h>

// AnnotationModule/MRMLDisplayableManager includes
#include "MRMLDisplayableManager/vtkMRMLAnnotationRulerDisplayableManager.h"

// AnnotationModule/VTKWidgets includes
#include <vtkAnnotationRulerRepresentation.h>
#include <vtkAnnotationRulerRepresentation3D.h>
#include <vtkAnnotationRulerWidget.h>

// MRML includes
#include <vtkMRMLInteractionNode.h>

// VTK includes
#include <vtkObject.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkProperty.h>
#include <vtkProperty2D.h>
#include <vtkTextProperty.h>
#include <vtkMath.h>
#include <vtkRenderer.h>
#include <vtkAxisActor2D.h>
#include <vtkHandleRepresentation.h>
#include <vtkInteractorEventRecorder.h>
#include <vtkAbstractWidget.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderWindow.h>
#include <vtkPointHandleRepresentation2D.h>
#include <vtkPointHandleRepresentation3D.h>
#include <vtkLineRepresentation.h>
#include <vtkGlyph3D.h>
#include <vtkCubeSource.h>

// std includes
#include <string>

// Convenient macro
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

//---------------------------------------------------------------------------
vtkStandardNewMacro (vtkMRMLAnnotationRulerDisplayableManager);
vtkCxxRevisionMacro (vtkMRMLAnnotationRulerDisplayableManager, "$Revision: 1.0 $");

//---------------------------------------------------------------------------
// vtkMRMLAnnotationRulerDisplayableManager Callback
/// \ingroup Slicer_QtModules_Annotation
class vtkAnnotationRulerWidgetCallback : public vtkCommand
{
public:
  static vtkAnnotationRulerWidgetCallback *New()
  { return new vtkAnnotationRulerWidgetCallback; }

  vtkAnnotationRulerWidgetCallback(){}

  virtual void Execute (vtkObject *vtkNotUsed(caller), unsigned long event, void*)
  {

    if ((event == vtkCommand::EndInteractionEvent) || (event == vtkCommand::InteractionEvent))
      {

      // sanity checks
      if (!this->m_DisplayableManager)
        {
        return;
        }
      if (!this->m_Node)
        {
        return;
        }
      if (!this->m_Widget)
        {
        return;
        }
      // sanity checks end

      if (this->m_DisplayableManager->GetSliceNode())
        {

        // if this is a 2D SliceView displayableManager, restrict the widget to the renderer

        // we need the widgetRepresentation
        vtkAnnotationRulerRepresentation* representation = vtkAnnotationRulerRepresentation::SafeDownCast(this->m_Widget->GetRepresentation());

        double displayCoordinates1[4];
        double displayCoordinates2[4];

        // first, we get the current displayCoordinates of the points
        representation->GetPoint1DisplayPosition(displayCoordinates1);
        representation->GetPoint2DisplayPosition(displayCoordinates2);

        // second, we copy these to restrictedDisplayCoordinates
        double restrictedDisplayCoordinates1[4] = {displayCoordinates1[0], displayCoordinates1[1], displayCoordinates1[2], displayCoordinates1[3]};
        double restrictedDisplayCoordinates2[4] = {displayCoordinates2[0], displayCoordinates2[1], displayCoordinates2[2], displayCoordinates2[3]};

        // modify restrictedDisplayCoordinates 1 and 2, if these are outside the viewport of the current renderer
        this->m_DisplayableManager->RestrictDisplayCoordinatesToViewport(restrictedDisplayCoordinates1);
        this->m_DisplayableManager->RestrictDisplayCoordinatesToViewport(restrictedDisplayCoordinates2);

        // only if we had to restrict the coordinates aka. if the coordinates changed, we update the positions
        if (this->m_DisplayableManager->GetDisplayCoordinatesChanged(displayCoordinates1,restrictedDisplayCoordinates1))
          {
          representation->SetPoint1DisplayPosition(restrictedDisplayCoordinates1);
          }


        if (this->m_DisplayableManager->GetDisplayCoordinatesChanged(displayCoordinates2,restrictedDisplayCoordinates2))
          {
          representation->SetPoint2DisplayPosition(restrictedDisplayCoordinates2);
          }

        }

      // the interaction with the widget ended, now propagate the changes to MRML
      this->m_DisplayableManager->PropagateWidgetToMRML(this->m_Widget, this->m_Node);

      }
  }

  void SetWidget(vtkAbstractWidget *w)
  {
    this->m_Widget = w;
  }
  void SetNode(vtkMRMLAnnotationNode *n)
  {
    this->m_Node = n;
  }
  void SetDisplayableManager(vtkMRMLAnnotationDisplayableManager * dm)
  {
    this->m_DisplayableManager = dm;
  }

  vtkAbstractWidget * m_Widget;
  vtkMRMLAnnotationNode * m_Node;
  vtkMRMLAnnotationDisplayableManager * m_DisplayableManager;
};

//---------------------------------------------------------------------------
// vtkMRMLAnnotationRulerDisplayableManager methods

//---------------------------------------------------------------------------
void vtkMRMLAnnotationRulerDisplayableManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
/// Create a new text widget.
vtkAbstractWidget * vtkMRMLAnnotationRulerDisplayableManager::CreateWidget(vtkMRMLAnnotationNode* node)
{

  if (!node)
    {
    vtkErrorMacro("CreateWidget: Node not set!")
    return 0;
    }

  vtkMRMLAnnotationRulerNode* rulerNode = vtkMRMLAnnotationRulerNode::SafeDownCast(node);

  if (!rulerNode)
    {
    vtkErrorMacro("CreateWidget: Could not get ruler node!")
    return 0;
    }

  vtkAnnotationRulerWidget * rulerWidget = vtkAnnotationRulerWidget::New();

  rulerWidget->SetInteractor(this->GetInteractor());
  rulerWidget->SetCurrentRenderer(this->GetRenderer());

  if (this->GetSliceNode())
    {

    // this is a 2D displayableManager
    VTK_CREATE(vtkPointHandleRepresentation2D, handle);
//    handle->GetProperty()->SetColor(1,0,0);

    VTK_CREATE(vtkAnnotationRulerRepresentation, dRep);
    dRep->SetHandleRepresentation(handle);
    dRep->InstantiateHandleRepresentation();
    dRep->GetAxis()->SetNumberOfMinorTicks(4);
    dRep->GetAxis()->SetTickLength(5);
    dRep->GetAxis()->SetTitlePosition(0.2);
    dRep->RulerModeOn();
    dRep->SetRulerDistance(20);

    rulerWidget->SetRepresentation(dRep);

    bool showWidget = true;
    showWidget = this->IsWidgetDisplayable(this->GetSliceNode(), node);

    rulerWidget->SetWidgetStateToManipulate();

    if (showWidget)
      {
      rulerWidget->On();
      }

    }
  else
    {

    // this is a 3D displayableManager
    VTK_CREATE(vtkPointHandleRepresentation3D, handle2);
//    handle2->GetProperty()->SetColor(1,1,0);

    VTK_CREATE(vtkAnnotationRulerRepresentation3D, dRep2);
    dRep2->SetHandleRepresentation(handle2);
    dRep2->InstantiateHandleRepresentation();
    dRep2->RulerModeOn();
    dRep2->SetRulerDistance(10);

    // change ticks to a stretched cube
    VTK_CREATE(vtkCubeSource, cubeSource);
    cubeSource->SetXLength(1.0);
    cubeSource->SetYLength(0.1);
    cubeSource->SetZLength(1.0);
    cubeSource->Update();
    //dRep2->UpdateGlyphPolyData(cubeSource->GetOutput());

    rulerWidget->SetRepresentation(dRep2);

    rulerWidget->SetWidgetStateToManipulate();
    rulerWidget->On();

    }

  this->PropagateMRMLToWidget(rulerNode, rulerWidget);

  vtkDebugMacro("CreateWidget: Widget was set up")

  return rulerWidget;

}

//---------------------------------------------------------------------------
/// Tear down the widget creation
void vtkMRMLAnnotationRulerDisplayableManager::OnWidgetCreated(vtkAbstractWidget * widget, vtkMRMLAnnotationNode * node)
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

  vtkAnnotationRulerWidget * rulerWidget = vtkAnnotationRulerWidget::SafeDownCast(widget);

  if (!widget)
    {
    vtkErrorMacro("OnWidgetCreated: Could not get ruler widget")
    return;
    }

  vtkMRMLAnnotationRulerNode * rulerNode = vtkMRMLAnnotationRulerNode::SafeDownCast(node);

  if (!rulerNode)
    {
    vtkErrorMacro("OnWidgetCreated: Could not get rulerNode node")
    return;
    }




  // widget thinks the interaction ended, now we can place the points from MRML
  double worldCoordinates1[4]={0,0,0,1};
  rulerNode->GetControlPointWorldCoordinates(0, worldCoordinates1);

  double worldCoordinates2[4]={0,0,0,1};
  rulerNode->GetControlPointWorldCoordinates(1, worldCoordinates2);

  //vtkAnnotationRulerRepresentation::SafeDownCast(rulerWidget->GetRepresentation())->SetPositionsForDistanceCalculation(worldCoordinates1, worldCoordinates2);

  if (this->GetSliceNode())
    {

    double displayCoordinates1[4]={0,0,0,1};
    double displayCoordinates2[4]={0,0,0,1};

    this->GetWorldToDisplayCoordinates(worldCoordinates1,displayCoordinates1);
    this->GetWorldToDisplayCoordinates(worldCoordinates2,displayCoordinates2);

    vtkAnnotationRulerRepresentation::SafeDownCast(rulerWidget->GetRepresentation())->SetPoint1DisplayPosition(displayCoordinates1);
    vtkAnnotationRulerRepresentation::SafeDownCast(rulerWidget->GetRepresentation())->SetPoint2DisplayPosition(displayCoordinates2);

    // set a specific format for the measurement text
    vtkAnnotationRulerRepresentation::SafeDownCast(rulerWidget->GetRepresentation())->SetLabelFormat("%.1f");

    vtkAnnotationRulerRepresentation::SafeDownCast(rulerWidget->GetRepresentation())->SetDistance(sqrt(vtkMath::Distance2BetweenPoints(worldCoordinates1,worldCoordinates2)));

    }
  else
    {

    vtkAnnotationRulerRepresentation3D::SafeDownCast(rulerWidget->GetRepresentation())->SetPoint1WorldPosition(worldCoordinates1);
    vtkAnnotationRulerRepresentation3D::SafeDownCast(rulerWidget->GetRepresentation())->SetPoint2WorldPosition(worldCoordinates2);

    // set a specific format for the measurement text
    vtkAnnotationRulerRepresentation3D::SafeDownCast(rulerWidget->GetRepresentation())->SetLabelFormat("%.1f");

    vtkAnnotationRulerRepresentation3D::SafeDownCast(rulerWidget->GetRepresentation())->SetDistance(sqrt(vtkMath::Distance2BetweenPoints(worldCoordinates1,worldCoordinates2)));

    }



  // add observer for end interaction
  vtkAnnotationRulerWidgetCallback *myCallback = vtkAnnotationRulerWidgetCallback::New();
  myCallback->SetNode(node);
  myCallback->SetWidget(widget);
  myCallback->SetDisplayableManager(this);
  widget->AddObserver(vtkCommand::EndInteractionEvent,myCallback);
  widget->AddObserver(vtkCommand::InteractionEvent,myCallback);
  myCallback->Delete();

  node->SaveView();

  //this->m_Updating = 0;
  //this->PropagateWidgetToMRML(widget, node);

}

//---------------------------------------------------------------------------
/// Propagate properties of MRML node to widget.
void vtkMRMLAnnotationRulerDisplayableManager::PropagateMRMLToWidget(vtkMRMLAnnotationNode* node, vtkAbstractWidget * widget)
{

  if (!widget)
    {
    vtkErrorMacro("PropagateMRMLToWidget: Widget was null!")
    return;
    }

  if (!node)
    {
    vtkErrorMacro("PropagateMRMLToWidget: MRML node was null!")
    return;
    }

  // cast to the specific widget
  vtkAnnotationRulerWidget* rulerWidget = vtkAnnotationRulerWidget::SafeDownCast(widget);

  if (!rulerWidget)
    {
    vtkErrorMacro("PropagateMRMLToWidget: Could not get ruler widget!")
    return;
    }

  // cast to the specific mrml node
  vtkMRMLAnnotationRulerNode* rulerNode = vtkMRMLAnnotationRulerNode::SafeDownCast(node);

  if (!rulerNode)
    {
    vtkErrorMacro("PropagateMRMLToWidget: Could not get ruler node!")
    return;
    }

  // disable processing of modified events
  this->m_Updating = 1;


  // get the points, to calculate the distance between them
  double worldCoordinates1[4]={0,0,0,1};
  rulerNode->GetControlPointWorldCoordinates(0, worldCoordinates1);

  double worldCoordinates2[4]={0,0,0,1};
  rulerNode->GetControlPointWorldCoordinates(1, worldCoordinates2);

  vtkMRMLAnnotationTextDisplayNode *textDisplayNode = rulerNode->GetAnnotationTextDisplayNode();
  vtkMRMLAnnotationPointDisplayNode *pointDisplayNode = rulerNode->GetAnnotationPointDisplayNode();
  vtkMRMLAnnotationLineDisplayNode *lineDisplayNode = rulerNode->GetAnnotationLineDisplayNode();
  
  // update the location
  if (this->Is2DDisplayableManager())
    {

    // now get the widget properties (coordinates, measurement etc.) and if the mrml node has changed, propagate the changes
    vtkAnnotationRulerRepresentation * rep = vtkAnnotationRulerRepresentation::SafeDownCast(rulerWidget->GetRepresentation());

    if (textDisplayNode)
      {
      // TODO: get this working
      /*
      float textScale = textDisplayNode->GetTextScale();
      std::cout << "Setting title text property 2d, scale to " << textScale << std::endl;
      vtkTextProperty *textProp = rep->GetAxis()->GetTitleTextProperty();
      textProp->SetFontSize(textScale);
      std::cout << "\ttitle font size now = " <<
      rep->GetAxis()->GetTitleTextProperty()->GetFontSize() << std::endl;
      */
      }
    // update the distance measurement
    rep->SetDistance(sqrt(vtkMath::Distance2BetweenPoints(worldCoordinates1,worldCoordinates2)));

    // set the color
    vtkHandleRepresentation *pointrep1 = rep->GetPoint1Representation();
    vtkHandleRepresentation *pointrep2 = rep->GetPoint2Representation();
    vtkPointHandleRepresentation2D *handle1 = NULL;
    vtkPointHandleRepresentation2D *handle2 = NULL;
    if (pointrep1 && pointrep2)
      {
      handle1 = vtkPointHandleRepresentation2D::SafeDownCast(pointrep1);
      handle2 = vtkPointHandleRepresentation2D::SafeDownCast(pointrep2);
      }
    if (handle1 && handle2 && pointDisplayNode)
      {
      if (rulerNode->GetSelected())
        {
        handle1->GetProperty()->SetColor(pointDisplayNode->GetSelectedColor());
        handle2->GetProperty()->SetColor(pointDisplayNode->GetSelectedColor());
        }
      else
        {
        handle1->GetProperty()->SetColor(pointDisplayNode->GetColor());
        handle2->GetProperty()->SetColor(pointDisplayNode->GetColor());
        }
      }
    if (textDisplayNode)
      {
      
      // TODO: get this working
      /*
      rep->GetAxis()->GetTitleTextProperty()->SetFontSize(textDisplayNode->GetTextScale());
      
      if (rep->GetTitleTextMapper())
        {
        rep->GetTitleTextMapper()->GetTextProperty()->ShallowCopy(rep->GetAxis()->GetTitleTextProperty());
        }
      */
      if (rulerNode->GetSelected())
        {
        rep->GetAxis()->GetTitleTextProperty()->SetColor(textDisplayNode->GetSelectedColor());
        }
      else
        {
        rep->GetAxis()->GetTitleTextProperty()->SetColor(textDisplayNode->GetColor());
        }
      // TODO: get this working
      rep->GetAxis()->GetTitleTextProperty()->SetFontSize(textDisplayNode->GetTextScale());
      }
    if (lineDisplayNode)
      {
      if (rep && rep->GetLineProperty())
        {
        if (rulerNode->GetSelected())
          {
          rep->GetLineProperty()->SetColor(lineDisplayNode->GetSelectedColor());
          }
        else
          {
          rep->GetLineProperty()->SetColor(lineDisplayNode->GetColor());
          }
        rep->GetLineProperty()->SetLineWidth(lineDisplayNode->GetLineThickness());
        rep->GetLineProperty()->SetOpacity(lineDisplayNode->GetOpacity());
//        rep->GetLineProperty()->SetAmbient(lineDisplayNode->GetAmbient());
//        rep->GetLineProperty()->SetDiffuse(lineDisplayNode->GetDiffuse());
//        rep->GetLineProperty()->SetSpecular(lineDisplayNode->GetSpecular());
        }
      rep->GetAxis()->SetTitlePosition(lineDisplayNode->GetLabelPosition());
      rep->GetAxis()->SetTickLength(lineDisplayNode->GetTickSpacing());
      rep->GetAxis()->SetTitleVisibility(lineDisplayNode->GetLabelVisibility());
      }
    rep->NeedToRenderOn();
    }
  else
    {
    /// 3d case
    
    // now get the widget properties (coordinates, measurement etc.) and if the mrml node has changed, propagate the changes
    vtkAnnotationRulerRepresentation3D * rep = vtkAnnotationRulerRepresentation3D::SafeDownCast(rulerWidget->GetRepresentation());

    // update the distance measurement
    rep->SetDistance(sqrt(vtkMath::Distance2BetweenPoints(worldCoordinates1,worldCoordinates2)));

    if (lineDisplayNode)
      {
      rep->SetLabelPosition(lineDisplayNode->GetLabelPosition());
      }
    if (textDisplayNode)
      {
      double textScale = textDisplayNode->GetTextScale();
      rep->SetLabelScale(textScale,textScale,textScale);
      }
    // set the color
    vtkHandleRepresentation *pointrep1 = rep->GetPoint1Representation();
    vtkHandleRepresentation *pointrep2 = rep->GetPoint2Representation();
    vtkPointHandleRepresentation3D *handle1 = NULL;
    vtkPointHandleRepresentation3D *handle2 = NULL;
    if (pointrep1 && pointrep2)
      {
      handle1 = vtkPointHandleRepresentation3D::SafeDownCast(pointrep1);
      handle2 = vtkPointHandleRepresentation3D::SafeDownCast(pointrep2);
      }
    if (handle1 && handle2 && pointDisplayNode)
      {
      if (rulerNode->GetSelected())
        {
        handle1->GetProperty()->SetColor(pointDisplayNode->GetSelectedColor());
        handle2->GetProperty()->SetColor(pointDisplayNode->GetSelectedColor());
        }
      else
        {
        handle1->GetProperty()->SetColor(pointDisplayNode->GetColor());
        handle2->GetProperty()->SetColor(pointDisplayNode->GetColor());
        }
      }
    if (pointDisplayNode)
      {
      // use this scale for the ticks
      rep->SetGlyphScale(pointDisplayNode->GetGlyphScale());
      }
    if (lineDisplayNode)
      {
      if (rulerNode->GetSelected())
        {
        rep->GetGlyphActor()->GetProperty()->SetColor(lineDisplayNode->GetSelectedColor());
        }
      else
        {
        rep->GetGlyphActor()->GetProperty()->SetColor(lineDisplayNode->GetColor());
        }
      if (textDisplayNode)
        {
        if (rulerNode->GetSelected())
          {
          rep->GetLabelProperty()->SetColor(textDisplayNode->GetSelectedColor());
          }
        else
          {
          rep->GetLabelProperty()->SetColor(textDisplayNode->GetColor());
          }
        // if the line node says not to show the label, use the label
        // property to set it invisible, otherwise, use the text display
        // node's opacity setting.
        if (!lineDisplayNode->GetLabelVisibility())
          {
          rep->GetLabelProperty()->SetOpacity(0);
          }
        else
          {
          rep->GetLabelProperty()->SetOpacity(textDisplayNode->GetOpacity());
          }
        }
      if (rep->GetLineProperty())
        {
        if (rulerNode->GetSelected())
          {
          rep->GetLineProperty()->SetColor(lineDisplayNode->GetSelectedColor());
          }
        else
          {
          rep->GetLineProperty()->SetColor(lineDisplayNode->GetColor());
          }
        rep->GetLineProperty()->SetLineWidth(lineDisplayNode->GetLineThickness());
        rep->GetLineProperty()->SetOpacity(lineDisplayNode->GetOpacity());
        // vtkProperty2D only defines opacity, color and line width
        //rep->GetLineProperty()->SetAmbient(lineDisplayNode->GetAmbient());
        //rep->GetLineProperty()->SetDiffuse(lineDisplayNode->GetDiffuse());
        //rep->GetLineProperty()->SetSpecular(lineDisplayNode->GetSpecular());
        }
      //double thickness = lineDisplayNode->GetLineThickness();
      rep->SetRulerDistance(lineDisplayNode->GetTickSpacing());
      rep->SetMaxTicks(lineDisplayNode->GetMaxTicks());
      }
    //vtkProperty *labelProperty = rep->GetLabelProperty();

    rep->NeedToRenderOn();
    }
  // update the label format
  vtkDistanceRepresentation *rep = vtkDistanceRepresentation::SafeDownCast(rulerWidget->GetRepresentation());
  if (rep)
    {
    if (rulerNode->GetNumberOfTexts() > 0)
      {
      std::string format = std::string("%-#6.3g\n");
      for (int i = 0; i < rulerNode->GetNumberOfTexts(); i++)
        {
        format += std::string(rulerNode->GetText(i));
        }
      rep->SetLabelFormat(format.c_str());
      }
    else
      {
      rep->SetLabelFormat("%-#6.3g");
      }
    }

  
  // update the position
  this->UpdatePosition(widget, node);

  rulerWidget->Modified();

  // enable processing of modified events
  this->m_Updating = 0;

}

//---------------------------------------------------------------------------
/// Propagate properties of widget to MRML node.
void vtkMRMLAnnotationRulerDisplayableManager::PropagateWidgetToMRML(vtkAbstractWidget * widget, vtkMRMLAnnotationNode* node)
{

  if (!widget)
    {
    vtkErrorMacro("PropagateWidgetToMRML: Widget was null!")
    return;
    }

  if (!node)
    {
    vtkErrorMacro("PropagateWidgetToMRML: MRML node was null!")
    return;
    }

  // cast to the specific widget
  vtkAnnotationRulerWidget* rulerWidget = vtkAnnotationRulerWidget::SafeDownCast(widget);

  if (!rulerWidget)
    {
    vtkErrorMacro("PropagateWidgetToMRML: Could not get ruler widget!")
    return;
    }

  // cast to the specific mrml node
  vtkMRMLAnnotationRulerNode* rulerNode = vtkMRMLAnnotationRulerNode::SafeDownCast(node);

  if (!rulerNode)
    {
    vtkErrorMacro("PropagateWidgetToMRML: Could not get ruler node!")
    return;
    }

  // disable processing of modified events
  this->m_Updating = 1;
  rulerNode->DisableModifiedEventOn();

  double worldCoordinates1[4]={0,0,0,1};
  double worldCoordinates2[4]={0,0,0,1};

  bool allowMovement = true;

  double distance;

  if (this->GetSliceNode())
    {
    // 2D widget was changed

    // now get the widget properties (coordinates, measurement etc.) and save it to the mrml node
    vtkAnnotationRulerRepresentation * rep = vtkAnnotationRulerRepresentation::SafeDownCast(rulerWidget->GetRepresentation());


    double displayCoordinates1[4]={0,0,0,1};
    double displayCoordinates2[4]={0,0,0,1};
    rep->GetPoint1DisplayPosition(displayCoordinates1);
    rep->GetPoint2DisplayPosition(displayCoordinates2);

    this->GetDisplayToWorldCoordinates(displayCoordinates1,worldCoordinates1);
    this->GetDisplayToWorldCoordinates(displayCoordinates2,worldCoordinates2);

    if (displayCoordinates1[0] < 0 || displayCoordinates1[0] > this->GetInteractor()->GetRenderWindow()->GetSize()[0])
      {
      allowMovement = false;
      }

    if (displayCoordinates1[1] < 0 || displayCoordinates1[1] > this->GetInteractor()->GetRenderWindow()->GetSize()[1])
      {
      allowMovement = false;
      }

    if (displayCoordinates2[0] < 0 || displayCoordinates2[0] > this->GetInteractor()->GetRenderWindow()->GetSize()[0])
      {
      allowMovement = false;
      }

    if (displayCoordinates2[1] < 0 || displayCoordinates2[1] > this->GetInteractor()->GetRenderWindow()->GetSize()[1])
      {
      allowMovement = false;
      }

    // Compute distance and update representation
    distance = sqrt(vtkMath::Distance2BetweenPoints(worldCoordinates1,worldCoordinates2));
    rep->SetDistance(distance);

    }
  else
    {
    // now get the widget properties (coordinates, measurement etc.) and save it to the mrml node
    vtkAnnotationRulerRepresentation3D * rep = vtkAnnotationRulerRepresentation3D::SafeDownCast(rulerWidget->GetRepresentation());

    rep->GetPoint1WorldPosition(worldCoordinates1);
    rep->GetPoint2WorldPosition(worldCoordinates2);

    // Compute distance and update representation
    distance = sqrt(vtkMath::Distance2BetweenPoints(worldCoordinates1,worldCoordinates2));
    rep->SetDistance(distance);

    }

  // if movement is not allowed, jump out
  if (!allowMovement)
    {
    return;
    }

  // save worldCoordinates to MRML if no change
  double p1[4]={0,0,0,1};
  double p2[4]={0,0,0,1};
  rulerNode->GetPositionWorldCoordinates1(p1);
  rulerNode->GetPositionWorldCoordinates2(p2);
  if (this->GetWorldCoordinatesChanged(worldCoordinates1, p1))
    {
    rulerNode->SetPositionWorldCoordinates1(worldCoordinates1);
    }
  if (this->GetWorldCoordinatesChanged(worldCoordinates2, p2))
    {
    rulerNode->SetPositionWorldCoordinates2(worldCoordinates2);
    }
  // save distance to mrml
  rulerNode->SetDistanceMeasurement(distance);

  // save the current view
  rulerNode->SaveView();

  // enable processing of modified events
  rulerNode->DisableModifiedEventOff();

  rulerNode->Modified();
  rulerNode->GetScene()->InvokeEvent(vtkCommand::ModifiedEvent, rulerNode);

  // This displayableManager should now consider ModifiedEvent again
  this->m_Updating = 0;
}

//---------------------------------------------------------------------------
/// Create a annotationMRMLnode
void vtkMRMLAnnotationRulerDisplayableManager::OnClickInRenderWindow(double x, double y)
{

  if (!this->IsCorrectDisplayableManager())
    {
    // jump out
    return;
    }

  // place the seed where the user clicked
  this->PlaceSeed(x,y);

  if (this->m_ClickCounter->HasEnoughClicks(2))
    {

    // switch to updating state to avoid events mess
    this->m_Updating = 1;


    vtkHandleWidget *h1 = this->GetSeed(0);
    vtkHandleWidget *h2 = this->GetSeed(1);

    // convert the coordinates
    double* displayCoordinates1 = vtkHandleRepresentation::SafeDownCast(h1->GetRepresentation())->GetDisplayPosition();
    double* displayCoordinates2 = vtkHandleRepresentation::SafeDownCast(h2->GetRepresentation())->GetDisplayPosition();

    double worldCoordinates1[4] = {0,0,0,1};
    double worldCoordinates2[4] = {0,0,0,1};

    this->GetDisplayToWorldCoordinates(displayCoordinates1[0],displayCoordinates1[1],worldCoordinates1);
    this->GetDisplayToWorldCoordinates(displayCoordinates2[0],displayCoordinates2[1],worldCoordinates2);

    double distance = sqrt(vtkMath::Distance2BetweenPoints(worldCoordinates1,worldCoordinates2));

    vtkMRMLAnnotationRulerNode *rulerNode = vtkMRMLAnnotationRulerNode::New();

    rulerNode->SetPositionWorldCoordinates1(worldCoordinates1);
    rulerNode->SetPositionWorldCoordinates2(worldCoordinates2);
    rulerNode->SetDistanceMeasurement(distance);

    rulerNode->SetName(this->GetMRMLScene()->GetUniqueNameByString("AnnotationRuler"));

    rulerNode->Initialize(this->GetMRMLScene());

    rulerNode->Delete();

    // reset updating state
    this->m_Updating = 0;

    // if this was a one time place, go back to view transform mode
     vtkMRMLInteractionNode *interactionNode = this->GetInteractionNode();
     if (interactionNode && interactionNode->GetPlaceModePersistence() != 1)
       {
       interactionNode->SetCurrentInteractionMode(vtkMRMLInteractionNode::ViewTransform);
       }
    }

  }

//---------------------------------------------------------------------------
void vtkMRMLAnnotationRulerDisplayableManager::UpdatePosition(vtkAbstractWidget *widget, vtkMRMLNode *node)
{
    if (!widget)
    {
    vtkErrorMacro("UpdatePosition: Widget was null!")
    return;
    }

  if (!node)
    {
    vtkErrorMacro("UpdatePosition: MRML node was null!")
    return;
    }

  // cast to the specific widget
  vtkAnnotationRulerWidget* rulerWidget = vtkAnnotationRulerWidget::SafeDownCast(widget);

  if (!rulerWidget)
    {
    vtkErrorMacro("UpdatePosition: Could not get ruler widget!")
    return;
    }

  // cast to the specific mrml node
    // cast to the specific mrml node
  vtkMRMLAnnotationRulerNode* rulerNode = vtkMRMLAnnotationRulerNode::SafeDownCast(node);

  if (!rulerNode)
    {
    vtkErrorMacro("UpdatePosition: Could not get ruler node!")
    return;
    }

  // disable processing of modified events
  this->m_Updating = 1;

  double worldCoordinates1[4]={0,0,0,1};
  rulerNode->GetControlPointWorldCoordinates(0, worldCoordinates1);

  double worldCoordinates2[4]={0,0,0,1};
  rulerNode->GetControlPointWorldCoordinates(1, worldCoordinates2);


  // update the location
  if (this->Is2DDisplayableManager())
    {
    // get the 2d representation
    vtkAnnotationRulerRepresentation * rep = vtkAnnotationRulerRepresentation::SafeDownCast(rulerWidget->GetRepresentation());
    
    // change the 2D location
    double displayCoordinates1[4]={0,0,0,1};
    double displayCoordinates2[4]={0,0,0,1};
    double displayCoordinatesBuffer1[4]={0,0,0,1};
    double displayCoordinatesBuffer2[4]={0,0,0,1};

    this->GetWorldToDisplayCoordinates(worldCoordinates1,displayCoordinates1);
    this->GetWorldToDisplayCoordinates(worldCoordinates2,displayCoordinates2);

    // only update the position, if coordinates really change
    rep->GetPoint1DisplayPosition(displayCoordinatesBuffer1);
    rep->GetPoint2DisplayPosition(displayCoordinatesBuffer2);

    if (this->GetDisplayCoordinatesChanged(displayCoordinates1,displayCoordinatesBuffer1))
      {
      rep->SetPoint1DisplayPosition(displayCoordinates1);
      }
    if (this->GetDisplayCoordinatesChanged(displayCoordinates2,displayCoordinatesBuffer2))
      {
      rep->SetPoint2DisplayPosition(displayCoordinates2);
      }
    }
  else
    {
    /// 3d case
    // now get the widget properties (coordinates, measurement etc.) and if the mrml node has changed, propagate the changes
    vtkAnnotationRulerRepresentation3D * rep = vtkAnnotationRulerRepresentation3D::SafeDownCast(rulerWidget->GetRepresentation());
    // change the 3D location
    rep->SetPoint1WorldPosition(worldCoordinates1);
    rep->SetPoint2WorldPosition(worldCoordinates2);
    
    rep->NeedToRenderOn();
    }
  
  rulerWidget->Modified();
  
  // enable processing of modified events
  this->m_Updating = 0;
}
