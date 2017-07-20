// MRMLLogic includes
#include "vtkMRMLPlotLayoutLogic.h"

// MRML includes
#include "vtkMRMLPlotLayoutNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkCollection.h>
#include <vtkEventBroker.h>
#include <vtkFloatArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMRMLPlotLayoutLogic);

//----------------------------------------------------------------------------
vtkMRMLPlotLayoutLogic::vtkMRMLPlotLayoutLogic()
{
}

//----------------------------------------------------------------------------
vtkMRMLPlotLayoutLogic::~vtkMRMLPlotLayoutLogic()
{
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  // List of events the slice logics should listen
  vtkNew<vtkIntArray> events;
  vtkNew<vtkFloatArray> priorities;

  float normalPriority = 0.0;

  // Events that use the default priority.  Don't care the order they
  // are triggered
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  priorities->InsertNextValue(normalPriority);

  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer(), priorities.GetPointer());

  this->ProcessMRMLSceneEvents(newScene, vtkCommand::ModifiedEvent, 0);
}

//----------------------------------------------------------------------------
void vtkMRMLPlotLayoutLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node)
    {
    return;
    }

  if (!node->IsA("vtkMRMLPlotNode"))
    {
    return;
    }

  vtkSmartPointer<vtkCollection> listPlotLayoutNode = vtkSmartPointer<vtkCollection>::Take(
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLPlotLayoutNode"));

  for (int listIndex = 0; listIndex < listPlotLayoutNode->GetNumberOfItems() ; listIndex++)
    {
    vtkMRMLPlotLayoutNode* PlotLayoutNode = vtkMRMLPlotLayoutNode::SafeDownCast
      (listPlotLayoutNode->GetItemAsObject(listIndex));

    if (PlotLayoutNode)
      {
      PlotLayoutNode->UpdateObservation(node->GetID());
      }
  }
}
