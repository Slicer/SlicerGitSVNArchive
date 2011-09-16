#include "vtkEMSegmentNodeParametersStep.h"

#include "vtkEMSegmentGUI.h"
#include "vtkEMSegmentLogic.h"
#include "vtkEMSegmentMRMLManager.h"

#include "vtkKWWizardWidget.h"
#include "vtkKWWizardWorkflow.h"
#include "vtkKWCheckButton.h"
#include "vtkKWCheckButtonWithLabel.h"
#include "vtkKWEntry.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWMenuButton.h"
#include "vtkKWMenuButtonWithLabel.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWNotebook.h"
#include "vtkKWTree.h"
#include "vtkKWTreeWithScrollbars.h"
#include "vtkKWScaleWithEntry.h"
#include "vtkKWEntryWithLabel.h"
#include "vtkEMSegmentAnatomicalStructureStep.h"
#include "vtkMath.h"
#include "vtkMRMLEMSGlobalParametersNode.h"

#include <vector>

#define EMSEG_PRINT_FREQUENCY_MAX 20

#include "vtkMRMLEMSWorkingDataNode.h"
#include "vtkMRMLEMSVolumeCollectionNode.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkEMSegmentNodeParametersStep);
vtkCxxRevisionMacro(vtkEMSegmentNodeParametersStep, "$Revision: 1.2 $");

//----------------------------------------------------------------------------
vtkEMSegmentNodeParametersStep::vtkEMSegmentNodeParametersStep()
{
  this->SetName("8/9. Edit Node-based Parameters");
  this->SetDescription("Specify node-based segmentation parameters.");

  this->NodeParametersNotebook                 = NULL;
  this->NodeParametersGlobalPriorScale         = NULL;
  this->NodeParametersSpatialPriorWeightScale  = NULL;
  this->NodeParametersInputChannelWeightFrame  = NULL;
  this->NodeParametersInputChannelWeight.clear();
  this->NodeParametersAlphaScale               = NULL;
  this->StoppingConditionsEMMenuButton         = NULL;
  this->StoppingConditionsEMIterationsEntry    = NULL;
  this->StoppingConditionsEMValueEntry         = NULL;
  this->StoppingConditionsMFAMenuButton        = NULL;
  this->StoppingConditionsMFAIterationsEntry   = NULL;
  this->StoppingConditionsMFAValueEntry        = NULL;
  this->StoppingConditionsBiasIterationsEntry  = NULL;
  this->PrintBasicFrame                        = NULL;
  this->NodeParametersPrintWeightCheckButton   = NULL;
  this->NodeParametersPrintQualityCheckButton  = NULL;
  this->NodeParametersPrintFrequencyScale      = NULL;
  this->NodeParametersPrintBiasCheckButton     = NULL;
  this->NodeParametersPrintLabelMapCheckButton = NULL;
  this->PrintConvergenceFrame                  = NULL;
  this->NodeParametersPrintEMLabelMapConvergenceCheckButton = NULL;
  this->NodeParametersPrintEMWeightsConvergenceCheckButton = NULL;
  this->NodeParametersPrintMFALabelMapConvergenceCheckButton = NULL;
  this->NodeParametersPrintMFAWeightsConvergenceCheckButton = NULL;

  this->NodeParametersInteractionMatricesFrame = NULL;
  this->NodeParametersInteractionMatrices2DCheckButton   = NULL;

  this->NodeParametersPCAFrame                 = NULL;
  this->NodeParametersRegistrationFrame        = NULL;
  this->NodeParametersMiscellaeneousFrame      = NULL;
  this->NodeParametersExcludeIncompleteEStepCheckButton = NULL;
  this->NodeParametersGenerateBackgroundProbabilityCheckButton = NULL;
  this->NodeParametersInhomogeneityFrame       = NULL;
  this->NodeParametersClassOverviewWeightGlobalFrame = NULL;
  this->NodeParametersClassOverviewWeightEntry.clear();
  this->NodeParametersClassOverviewWeightFrame.clear();
  this->NodeParametersClassOverviewWeightAuto.clear();
  this->ClassOverviewWeightAutomaticRecalculateFlag = 0;

  this->NodeParametersLabelTabBasic = NULL;
  this->NodeParametersLabelTabStoppingConditions = NULL;
  this->NodeParametersLabelTabPrint = NULL;
  this->NodeParametersLabelTabAdvanced = NULL;
}

//----------------------------------------------------------------------------
vtkEMSegmentNodeParametersStep::~vtkEMSegmentNodeParametersStep()
{
  if (this->NodeParametersNotebook)
    {
    this->NodeParametersNotebook->Delete();
    this->NodeParametersNotebook = NULL;
    }

  if (this->NodeParametersLabelTabBasic)
    {
    this->NodeParametersLabelTabBasic->Delete();
    this->NodeParametersLabelTabBasic = NULL;
    }

  if (this->NodeParametersLabelTabStoppingConditions)
    {
    this->NodeParametersLabelTabStoppingConditions->Delete();
    this->NodeParametersLabelTabStoppingConditions = NULL;
    }

  if (this->NodeParametersLabelTabPrint)
    {
    this->NodeParametersLabelTabPrint->Delete();
    this->NodeParametersLabelTabPrint = NULL;
    }

  if (this->NodeParametersLabelTabAdvanced)
    {
    this->NodeParametersLabelTabAdvanced->Delete();
    this->NodeParametersLabelTabAdvanced = NULL;
    }

  if (this->NodeParametersGlobalPriorScale)
    {
    this->NodeParametersGlobalPriorScale->Delete();
    this->NodeParametersGlobalPriorScale = NULL;
    }

  if (this->NodeParametersSpatialPriorWeightScale)
    {
    this->NodeParametersSpatialPriorWeightScale->Delete();
    this->NodeParametersSpatialPriorWeightScale = NULL;
    }

  for (int i = 0 ; i < (int) this->NodeParametersInputChannelWeight.size(); i++) 
    {
      if (this->NodeParametersInputChannelWeight[i])
    {
      this->NodeParametersInputChannelWeight[i]->Delete();
      this->NodeParametersInputChannelWeight[i] = NULL;
    }
    }
  this->NodeParametersInputChannelWeight.clear();
    
  if (this->NodeParametersInputChannelWeightFrame)
    {
      this->NodeParametersInputChannelWeightFrame->Delete();
      this->NodeParametersInputChannelWeightFrame = NULL;
    }

  if (this->NodeParametersAlphaScale)
    {
    this->NodeParametersAlphaScale->Delete();
    this->NodeParametersAlphaScale = NULL;
    }

  if (this->StoppingConditionsEMMenuButton)
    {
    this->StoppingConditionsEMMenuButton->Delete();
    this->StoppingConditionsEMMenuButton = NULL;
    }

  if (this->StoppingConditionsEMIterationsEntry)
    {
    this->StoppingConditionsEMIterationsEntry->Delete();
    this->StoppingConditionsEMIterationsEntry = NULL;
    }

  if (this->StoppingConditionsEMValueEntry)
    {
    this->StoppingConditionsEMValueEntry->Delete();
    this->StoppingConditionsEMValueEntry = NULL;
    }

  if (this->StoppingConditionsMFAMenuButton)
    {
    this->StoppingConditionsMFAMenuButton->Delete();
    this->StoppingConditionsMFAMenuButton = NULL;
    }

  if (this->StoppingConditionsMFAIterationsEntry)
    {
    this->StoppingConditionsMFAIterationsEntry->Delete();
    this->StoppingConditionsMFAIterationsEntry = NULL;
    }

  if (this->StoppingConditionsMFAValueEntry)
    {
    this->StoppingConditionsMFAValueEntry->Delete();
    this->StoppingConditionsMFAValueEntry = NULL;
    }

  if (this->StoppingConditionsBiasIterationsEntry)
    {
    this->StoppingConditionsBiasIterationsEntry->Delete();
    this->StoppingConditionsBiasIterationsEntry = NULL;
    }

  if (this->NodeParametersPrintWeightCheckButton)
    {
    this->NodeParametersPrintWeightCheckButton->Delete();
    this->NodeParametersPrintWeightCheckButton = NULL;
    }

  if (this->NodeParametersPrintQualityCheckButton)
    {
    this->NodeParametersPrintQualityCheckButton->Delete();
    this->NodeParametersPrintQualityCheckButton = NULL;
    }

  if (this->NodeParametersPrintFrequencyScale)
    {
    this->NodeParametersPrintFrequencyScale->Delete();
    this->NodeParametersPrintFrequencyScale = NULL;
    }

  if (this->NodeParametersPrintBiasCheckButton)
    {
    this->NodeParametersPrintBiasCheckButton->Delete();
    this->NodeParametersPrintBiasCheckButton = NULL;
    }

  if (this->NodeParametersPrintLabelMapCheckButton)
    {
    this->NodeParametersPrintLabelMapCheckButton->Delete();
    this->NodeParametersPrintLabelMapCheckButton = NULL;
    }
  
  if (this->PrintBasicFrame)
    { 
    this->PrintBasicFrame->Delete();
    this->PrintBasicFrame = NULL;
    }

  if (this->NodeParametersPrintEMLabelMapConvergenceCheckButton)
    {
    this->NodeParametersPrintEMLabelMapConvergenceCheckButton->Delete();
    this->NodeParametersPrintEMLabelMapConvergenceCheckButton = NULL;
    }

  if (this->NodeParametersPrintEMWeightsConvergenceCheckButton)
    {
    this->NodeParametersPrintEMWeightsConvergenceCheckButton->Delete();
    this->NodeParametersPrintEMWeightsConvergenceCheckButton = NULL;
    }

  if(this->NodeParametersPrintMFALabelMapConvergenceCheckButton)
    {
    this->NodeParametersPrintMFALabelMapConvergenceCheckButton->Delete();
    this->NodeParametersPrintMFALabelMapConvergenceCheckButton = NULL;
    }
  
  if(this->NodeParametersPrintMFAWeightsConvergenceCheckButton)
    {
    this->NodeParametersPrintMFAWeightsConvergenceCheckButton->Delete();
    this->NodeParametersPrintMFAWeightsConvergenceCheckButton = NULL;
    }
  
  if (this->PrintConvergenceFrame)
    {
    this->PrintConvergenceFrame->Delete();
    this->PrintConvergenceFrame = NULL; 
    }

  if (this->NodeParametersInteractionMatricesFrame)
    {
    this->NodeParametersInteractionMatricesFrame->Delete();
    this->NodeParametersInteractionMatricesFrame = NULL;
    }

  if (this->NodeParametersInteractionMatrices2DCheckButton)
    {
        this->NodeParametersInteractionMatrices2DCheckButton->Delete();
        this->NodeParametersInteractionMatrices2DCheckButton   = NULL;
    }

  if (this->NodeParametersPCAFrame)
    {
    this->NodeParametersPCAFrame->Delete();
    this->NodeParametersPCAFrame = NULL;
    }

  if (this->NodeParametersRegistrationFrame)
    {
    this->NodeParametersRegistrationFrame->Delete();
    this->NodeParametersRegistrationFrame = NULL;
    }
  
  if(this->NodeParametersExcludeIncompleteEStepCheckButton)
    {
    this->NodeParametersExcludeIncompleteEStepCheckButton->Delete();
    this->NodeParametersExcludeIncompleteEStepCheckButton = NULL;
    }

  if(this->NodeParametersGenerateBackgroundProbabilityCheckButton)
    {
    this->NodeParametersGenerateBackgroundProbabilityCheckButton->Delete();
    this->NodeParametersGenerateBackgroundProbabilityCheckButton = NULL;
    }

  if (this->NodeParametersMiscellaeneousFrame)
    {
    this->NodeParametersMiscellaeneousFrame->Delete();
    this->NodeParametersMiscellaeneousFrame = NULL;
    }

  if (this->NodeParametersInhomogeneityFrame)
    {
    this->NodeParametersInhomogeneityFrame->Delete();
    this->NodeParametersInhomogeneityFrame = NULL;
    }

  if (this->NodeParametersClassOverviewWeightGlobalFrame )
    {
      this->NodeParametersClassOverviewWeightGlobalFrame->Delete();
      this->NodeParametersClassOverviewWeightGlobalFrame = NULL;    
    } 

   for (int i = 0 ; i < (int) this->NodeParametersClassOverviewWeightEntry.size(); i++) 
    {
      if (this->NodeParametersClassOverviewWeightEntry[i])
    {
      this->NodeParametersClassOverviewWeightEntry[i]->Delete();
      this->NodeParametersClassOverviewWeightEntry[i] = NULL;
    }
    }
  this->NodeParametersClassOverviewWeightEntry.clear();

   for (int i = 0 ; i < (int) this->NodeParametersClassOverviewWeightFrame.size(); i++) 
    {
      if (this->NodeParametersClassOverviewWeightFrame[i])
    {
      this->NodeParametersClassOverviewWeightFrame[i]->Delete();
      this->NodeParametersClassOverviewWeightFrame[i] = NULL;
    }
    }
  this->NodeParametersClassOverviewWeightFrame.clear();

   for (int i = 0 ; i < (int) this->NodeParametersClassOverviewWeightAuto.size(); i++) 
    {
      if (this->NodeParametersClassOverviewWeightAuto[i])
    {
      this->NodeParametersClassOverviewWeightAuto[i]->Delete();
      this->NodeParametersClassOverviewWeightAuto[i] = NULL;
    }
    }
  this->NodeParametersClassOverviewWeightAuto.clear();
    
}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::ShowUserInterface()
{
  this->Superclass::ShowUserInterface();

  vtkKWWizardWidget *wizard_widget = this->GetGUI()->GetWizardWidget();
  wizard_widget->GetCancelButton()->SetEnabled(0);

  vtkEMSegmentAnatomicalStructureStep *anat_step = 
    this->GetGUI()->GetAnatomicalStructureStep();
  anat_step->ShowAnatomicalStructureTree();

  vtkKWWidget *parent = wizard_widget->GetClientArea();;

  // Override the tree callbacks for that specific step

  anat_step->GetAnatomicalStructureTree()->GetWidget()->
    SetSelectionChangedCommand(
      this, "DisplaySelectedNodeParametersCallback");

  // Create the notebook

  if (!this->NodeParametersNotebook)
    {
    this->NodeParametersNotebook = vtkKWNotebook::New();
    }
  if (!this->NodeParametersNotebook->IsCreated())
    {
    this->NodeParametersNotebook->SetParent(parent);
    this->NodeParametersNotebook->Create();
    this->NodeParametersNotebook->AddPage("Basic");
    this->NodeParametersNotebook->AddPage("Stopping Conditions");
    this->NodeParametersNotebook->AddPage("Print");
    this->NodeParametersNotebook->AddPage("Advanced");
    }
  vtkKWFrame *basic_page = 
    this->NodeParametersNotebook->GetFrame("Basic");
  vtkKWFrame *stop_cond_page = 
    this->NodeParametersNotebook->GetFrame("Stopping Conditions");
  vtkKWFrame *print_page = 
    this->NodeParametersNotebook->GetFrame("Print");
  vtkKWFrame *advanced_page = 
    this->NodeParametersNotebook->GetFrame("Advanced");

  this->Script(
    "pack %s -side top -anchor nw -fill both -expand y -padx 2 -pady 2", 
    this->NodeParametersNotebook->GetWidgetName());

  // Create label to display selected node for each tab

  if (!this->NodeParametersLabelTabBasic)
    {
    this->NodeParametersLabelTabBasic = vtkKWLabel::New();
    }
  if (!this->NodeParametersLabelTabBasic->IsCreated())
    {
    this->NodeParametersLabelTabBasic->SetParent(basic_page);
    this->NodeParametersLabelTabBasic->Create();
    this->Script("grid %s -column 0 -row 0 -sticky nw -padx 2 -pady 2", this->NodeParametersLabelTabBasic->GetWidgetName());
    }

  // Create the class probability scale

  if (!this->NodeParametersGlobalPriorScale)
    {
    this->NodeParametersGlobalPriorScale = vtkKWScaleWithEntry::New();
    }
  if (!this->NodeParametersGlobalPriorScale->IsCreated())
    {
    this->NodeParametersGlobalPriorScale->SetParent(basic_page);
    this->NodeParametersGlobalPriorScale->PopupModeOn();
    this->NodeParametersGlobalPriorScale->Create();
    this->NodeParametersGlobalPriorScale->SetEntryWidth(4);
    this->NodeParametersGlobalPriorScale->SetLabelText("Class Weight:");
    this->NodeParametersGlobalPriorScale->GetLabel()->
      SetWidth(EMSEG_WIDGETS_LABEL_WIDTH - 9);
    this->NodeParametersGlobalPriorScale->SetRange(0.0, 1.0);
    this->NodeParametersGlobalPriorScale->SetResolution(0.01);
    this->NodeParametersGlobalPriorScale->GetEntry()->
      SetCommandTriggerToAnyChange();
    this->NodeParametersGlobalPriorScale->SetBalloonHelpString("Relative weight of this over other anatomical structures. For example, one can address the overestimation of a specific structure, e.g. grey matter, by reducing the corresponding class weight and increasing the weight of a sibling, such as white matter.The value must be in the range [0,1].  Class weights across siblings must sum to 1.");  
    }

  this->Script("grid %s -column 0 -row 1 -sticky nw -padx 2 -pady 2",  this->NodeParametersGlobalPriorScale->GetWidgetName());

  // Create the spatial prior weight scale

  if (!this->NodeParametersSpatialPriorWeightScale)
    {
    this->NodeParametersSpatialPriorWeightScale = vtkKWScaleWithEntry::New();
    }
  if (!this->NodeParametersSpatialPriorWeightScale->IsCreated())
    {
    this->NodeParametersSpatialPriorWeightScale->SetParent(basic_page);
    this->NodeParametersSpatialPriorWeightScale->PopupModeOn();
    this->NodeParametersSpatialPriorWeightScale->Create();
    this->NodeParametersSpatialPriorWeightScale->SetEntryWidth(4);
    this->NodeParametersSpatialPriorWeightScale->SetLabelText(
      "Atlas Weight:");
    this->NodeParametersSpatialPriorWeightScale->GetLabel()->
      SetWidth(EMSEG_WIDGETS_LABEL_WIDTH - 9);
    this->NodeParametersSpatialPriorWeightScale->SetRange(0.0, 1.0);
    this->NodeParametersSpatialPriorWeightScale->SetResolution(0.01);
    this->NodeParametersSpatialPriorWeightScale->GetEntry()->
      SetCommandTriggerToAnyChange();
    this->NodeParametersSpatialPriorWeightScale->SetBalloonHelpString("Weight of the atlas (spatial prior) versus the image data. The value must be in the range [0,1], where 0 indicates that the atlas is ignored and 1 indicates the maximum atlas weight. For example, when skullstripping an image, one should set the atlas weight to 1 as the algorithm requires spatial information to separate the neck from the brain"); 
    }

  this->Script("grid %s -column 0 -row 2 -sticky nw -padx 2 -pady 2",
    this->NodeParametersSpatialPriorWeightScale->GetWidgetName());

  // Create Atlas Weight  
  this->DefineInputChannelWeightOverviewWindow(basic_page->GetEnabled());

  // Create the alpha scale

  if (!this->NodeParametersAlphaScale)
    {
    this->NodeParametersAlphaScale = vtkKWScaleWithEntry::New();
    }
  if (!this->NodeParametersAlphaScale->IsCreated())
    {
    this->NodeParametersAlphaScale->SetParent(basic_page);
    this->NodeParametersAlphaScale->PopupModeOn();
    this->NodeParametersAlphaScale->Create();
    this->NodeParametersAlphaScale->SetEntryWidth(4);
    this->NodeParametersAlphaScale->SetLabelText("MFA Weight:");
    this->NodeParametersAlphaScale->GetLabel()->
      SetWidth(EMSEG_WIDGETS_LABEL_WIDTH - 9);
    this->NodeParametersAlphaScale->SetRange(0.0, 1.0);
    this->NodeParametersAlphaScale->SetResolution(0.01);
    this->NodeParametersAlphaScale->GetEntry()->SetCommandTriggerToAnyChange();
    }

  this->Script("grid %s -column 0 -row 2 -sticky nw -padx 2 -pady 2", 
               this->NodeParametersAlphaScale->GetWidgetName());

  this->Script("grid columnconfigure %s 1 -weight 1", 
               basic_page->GetWidgetName());


  // Stopping Conditions page

  if (!this->NodeParametersLabelTabStoppingConditions)
    {
    this->NodeParametersLabelTabStoppingConditions = vtkKWLabel::New();
    }
  if (!this->NodeParametersLabelTabStoppingConditions->IsCreated())
    {
    this->NodeParametersLabelTabStoppingConditions->SetParent(stop_cond_page);
    this->NodeParametersLabelTabStoppingConditions->Create();
    this->Script("grid %s -column 0 -row 0 -sticky nw -padx 2 -pady 2", this->NodeParametersLabelTabStoppingConditions->GetWidgetName());
    }

  // Create the EM menu button
  if (!this->StoppingConditionsEMMenuButton)
    {
    this->StoppingConditionsEMMenuButton = vtkKWMenuButtonWithLabel::New();
    }
  if (!this->StoppingConditionsEMMenuButton->IsCreated())
    {
    this->StoppingConditionsEMMenuButton->SetParent(stop_cond_page);
    this->StoppingConditionsEMMenuButton->Create();
    this->StoppingConditionsEMMenuButton->SetLabelText("EM:");
    this->StoppingConditionsEMMenuButton->SetLabelWidth(5);
    this->StoppingConditionsEMMenuButton->GetWidget()->SetWidth(
      EMSEG_MENU_BUTTON_WIDTH);
    }

  this->Script("grid %s -column 0 -row 1 -sticky nw -padx 2 -pady 2",
               this->StoppingConditionsEMMenuButton->GetWidgetName());
  
  // Create the EM iterations entry

  if (!this->StoppingConditionsEMIterationsEntry)
    {
    this->StoppingConditionsEMIterationsEntry = vtkKWEntryWithLabel::New();
    }
  if (!this->StoppingConditionsEMIterationsEntry->IsCreated())
    {
    this->StoppingConditionsEMIterationsEntry->SetParent(stop_cond_page);
    this->StoppingConditionsEMIterationsEntry->Create();
    this->StoppingConditionsEMIterationsEntry->SetLabelText("Iterations:");
    this->StoppingConditionsEMIterationsEntry->SetLabelWidth(15);
    vtkKWEntry *entry = this->StoppingConditionsEMIterationsEntry->GetWidget();
    entry->SetWidth(6);
    entry->SetRestrictValueToInteger();
    entry->SetCommandTriggerToAnyChange();
    }

  this->Script("grid %s -column 1 -row 1 -sticky nw -padx 2 -pady 2",
               this->StoppingConditionsEMIterationsEntry->GetWidgetName());

  // Create the EM value entry

  if (!this->StoppingConditionsEMValueEntry)
    {
    this->StoppingConditionsEMValueEntry = vtkKWEntryWithLabel::New();
    }
  if (!this->StoppingConditionsEMValueEntry->IsCreated())
    {
    this->StoppingConditionsEMValueEntry->SetParent(stop_cond_page);
    this->StoppingConditionsEMValueEntry->Create();
    this->StoppingConditionsEMValueEntry->SetLabelText("Value:");
    this->StoppingConditionsEMValueEntry->SetLabelWidth(15);
    vtkKWEntry *entry = this->StoppingConditionsEMValueEntry->GetWidget();
    entry->SetWidth(6);
    entry->SetRestrictValueToDouble();
    entry->SetCommandTriggerToAnyChange();
    }

  this->Script("grid %s -column 2 -row 1 -sticky nw -padx 2 -pady 2",
               this->StoppingConditionsEMValueEntry->GetWidgetName());

  // Create the MFA menubutton

  if (!this->StoppingConditionsMFAMenuButton)
    {
    this->StoppingConditionsMFAMenuButton = vtkKWMenuButtonWithLabel::New();
    }
  if (!this->StoppingConditionsMFAMenuButton->IsCreated())
    {
    this->StoppingConditionsMFAMenuButton->SetParent(stop_cond_page);
    this->StoppingConditionsMFAMenuButton->Create();
    this->StoppingConditionsMFAMenuButton->SetLabelText("MFA:");
    this->StoppingConditionsMFAMenuButton->SetLabelWidth(5);
    this->StoppingConditionsMFAMenuButton->GetWidget()->SetWidth(
      EMSEG_MENU_BUTTON_WIDTH);
    }

  this->Script("grid %s -column 0 -row 2 -sticky nw -padx 2 -pady 2",
               this->StoppingConditionsMFAMenuButton->GetWidgetName());

  // Create the MFA iterations entry

  if (!this->StoppingConditionsMFAIterationsEntry)
    {
    this->StoppingConditionsMFAIterationsEntry = vtkKWEntryWithLabel::New();
    }
  if (!this->StoppingConditionsMFAIterationsEntry->IsCreated())
    {
    this->StoppingConditionsMFAIterationsEntry->SetParent(stop_cond_page);
    this->StoppingConditionsMFAIterationsEntry->Create();
    this->StoppingConditionsMFAIterationsEntry->SetLabelText("Iterations:");
    this->StoppingConditionsMFAIterationsEntry->SetLabelWidth(15);
    vtkKWEntry *entry = 
      this->StoppingConditionsMFAIterationsEntry->GetWidget();
    entry->SetWidth(6);
    entry->SetRestrictValueToInteger();
    entry->SetCommandTriggerToAnyChange();
    }

  this->Script("grid %s -column 1 -row 2 -sticky nw -padx 2 -pady 2",
               this->StoppingConditionsMFAIterationsEntry->GetWidgetName());

  // Create the MFA value entry

  if (!this->StoppingConditionsMFAValueEntry)
    {
    this->StoppingConditionsMFAValueEntry = vtkKWEntryWithLabel::New();
    }
  if (!this->StoppingConditionsMFAValueEntry->IsCreated())
    {
    this->StoppingConditionsMFAValueEntry->SetParent(stop_cond_page);
    this->StoppingConditionsMFAValueEntry->Create();
    this->StoppingConditionsMFAValueEntry->SetLabelText("Value:");
    this->StoppingConditionsMFAValueEntry->SetLabelWidth(15);
    vtkKWEntry *entry = this->StoppingConditionsMFAValueEntry->GetWidget();
    entry->SetWidth(6);
    entry->SetRestrictValueToDouble();
    entry->SetCommandTriggerToAnyChange();
    }

  this->Script("grid %s -column 2 -row 2 -sticky nw -padx 2 -pady 2",
               this->StoppingConditionsMFAValueEntry->GetWidgetName());
  
  // Create the Bias calculation max iterations entry

  if (!this->StoppingConditionsBiasIterationsEntry)
    {
    this->StoppingConditionsBiasIterationsEntry = vtkKWEntryWithLabel::New();
    }
  if (!this->StoppingConditionsBiasIterationsEntry->IsCreated())
    {
    this->StoppingConditionsBiasIterationsEntry->SetParent(stop_cond_page);
    this->StoppingConditionsBiasIterationsEntry->Create();
    this->StoppingConditionsBiasIterationsEntry->SetLabelText(
      "Bias Iterations:");
    this->StoppingConditionsBiasIterationsEntry->SetLabelWidth(15);
    vtkKWEntry *entry = 
      this->StoppingConditionsBiasIterationsEntry->GetWidget();
    entry->SetWidth(6);
    entry->SetRestrictValueToInteger();
    entry->SetCommandTriggerToAnyChange();
    }

  this->Script("grid %s -column 1 -row 3 -sticky nw -padx 2 -pady 2",
               this->StoppingConditionsBiasIterationsEntry->GetWidgetName());

  this->Script("grid columnconfigure %s 0 -weight 1",
               stop_cond_page->GetWidgetName());
  this->Script("grid columnconfigure %s 1 -weight 1",
               stop_cond_page->GetWidgetName());
  this->Script("grid columnconfigure %s 2 -weight 1",
               stop_cond_page->GetWidgetName());
  



  // print page

  if (!this->NodeParametersLabelTabPrint)
    {
    this->NodeParametersLabelTabPrint = vtkKWLabel::New();
    }
  if (!this->NodeParametersLabelTabPrint->IsCreated())
    {
    this->NodeParametersLabelTabPrint->SetParent(print_page);
    this->NodeParametersLabelTabPrint->Create();
    this->Script("pack %s -side top -anchor nw -padx 2 -pady 2", this->NodeParametersLabelTabPrint->GetWidgetName());
    }

  // Create the print basic frame

  if (!this->PrintBasicFrame)
    {
    this->PrintBasicFrame = vtkKWFrame::New();
    }
  if (!this->PrintBasicFrame->IsCreated())
    {
    this->PrintBasicFrame->SetParent(print_page);
    this->PrintBasicFrame->Create();
    }

  this->Script(
    "pack %s -side left -anchor nw -padx 5 -pady {20 2}", 
    this->PrintBasicFrame->GetWidgetName());


  // Create the print weight checkbutton

  if (!this->NodeParametersPrintWeightCheckButton)
    {
    this->NodeParametersPrintWeightCheckButton = 
      vtkKWCheckButtonWithLabel::New();
    }
  if (!this->NodeParametersPrintWeightCheckButton->IsCreated())
    {
    this->NodeParametersPrintWeightCheckButton->SetParent(this->PrintBasicFrame);
    this->NodeParametersPrintWeightCheckButton->Create();
    this->NodeParametersPrintWeightCheckButton->SetLabelText("Weight:");
    this->NodeParametersPrintWeightCheckButton->SetLabelWidth(
      EMSEG_WIDGETS_LABEL_WIDTH-10);
    }

  this->Script("pack %s -side top -anchor nw -padx 2 -pady 2", 
               this->NodeParametersPrintWeightCheckButton->GetWidgetName());
  
  // Create the print quality checkbutton

  if (!this->NodeParametersPrintQualityCheckButton)
    {
    this->NodeParametersPrintQualityCheckButton = 
      vtkKWCheckButtonWithLabel::New();
    }
  if (!this->NodeParametersPrintQualityCheckButton->IsCreated())
    {
    this->NodeParametersPrintQualityCheckButton->SetParent(this->PrintBasicFrame);
    this->NodeParametersPrintQualityCheckButton->Create();
    this->NodeParametersPrintQualityCheckButton->SetLabelText("Quality:");
    this->NodeParametersPrintQualityCheckButton->SetLabelWidth(
      EMSEG_WIDGETS_LABEL_WIDTH-10);
    }

  // Create the print frequency scale

  if (!this->NodeParametersPrintFrequencyScale)
    {
    this->NodeParametersPrintFrequencyScale = vtkKWScaleWithEntry::New();
    }
  if (!this->NodeParametersPrintFrequencyScale->IsCreated())
    {
    this->NodeParametersPrintFrequencyScale->SetParent(this->PrintBasicFrame);
    this->NodeParametersPrintFrequencyScale->PopupModeOn();
    this->NodeParametersPrintFrequencyScale->Create();
    this->NodeParametersPrintFrequencyScale->SetEntryWidth(4);
    this->NodeParametersPrintFrequencyScale->GetEntry()->
      SetRestrictValueToInteger();
    this->NodeParametersPrintFrequencyScale->SetLabelText("Frequency:");
    this->NodeParametersPrintFrequencyScale->GetLabel()->
      SetWidth(EMSEG_WIDGETS_LABEL_WIDTH-10);
    this->NodeParametersPrintFrequencyScale->SetRange(0, 
      EMSEG_PRINT_FREQUENCY_MAX);
    this->NodeParametersPrintFrequencyScale->SetResolution(1);
    this->NodeParametersPrintFrequencyScale->GetEntry()->
      SetCommandTriggerToAnyChange();
    }

  // Create the print bias checkbutton

  if (!this->NodeParametersPrintBiasCheckButton)
    {
    this->NodeParametersPrintBiasCheckButton = 
      vtkKWCheckButtonWithLabel::New();
    }
  if (!this->NodeParametersPrintBiasCheckButton->IsCreated())
    {
    this->NodeParametersPrintBiasCheckButton->SetParent(this->PrintBasicFrame);
    this->NodeParametersPrintBiasCheckButton->Create();
    this->NodeParametersPrintBiasCheckButton->SetLabelText("Bias:");
    this->NodeParametersPrintBiasCheckButton->SetLabelWidth(
      EMSEG_WIDGETS_LABEL_WIDTH-10);
    }

  // Create the print label map checkbutton

  if (!this->NodeParametersPrintLabelMapCheckButton)
    {
    this->NodeParametersPrintLabelMapCheckButton = 
      vtkKWCheckButtonWithLabel::New();
    }
  if (!this->NodeParametersPrintLabelMapCheckButton->IsCreated())
    {
    this->NodeParametersPrintLabelMapCheckButton->SetParent(this->PrintBasicFrame);
    this->NodeParametersPrintLabelMapCheckButton->Create();
    this->NodeParametersPrintLabelMapCheckButton->SetLabelText("Label Map:");
    this->NodeParametersPrintLabelMapCheckButton->SetLabelWidth(
      EMSEG_WIDGETS_LABEL_WIDTH-10);
    }

  // Create the print convergence frame

  if (!this->PrintConvergenceFrame)
    {
    this->PrintConvergenceFrame = vtkKWFrameWithLabel::New();
    }
  if (!this->PrintConvergenceFrame->IsCreated())
    {
    this->PrintConvergenceFrame->SetParent(print_page);
    this->PrintConvergenceFrame->Create();
    this->PrintConvergenceFrame->SetLabelText(
      "Convergence");
    }

  // Create the print EM label map convergence checkbutton

  if (!this->NodeParametersPrintEMLabelMapConvergenceCheckButton)
    {
    this->NodeParametersPrintEMLabelMapConvergenceCheckButton = 
      vtkKWCheckButtonWithLabel::New();
    }
  if (!this->NodeParametersPrintEMLabelMapConvergenceCheckButton->IsCreated())
    {
    this->NodeParametersPrintEMLabelMapConvergenceCheckButton->SetParent(
      this->PrintConvergenceFrame->GetFrame());
    this->NodeParametersPrintEMLabelMapConvergenceCheckButton->Create();
    this->NodeParametersPrintEMLabelMapConvergenceCheckButton->
      SetLabelText("EM Label Map:");
    this->NodeParametersPrintEMLabelMapConvergenceCheckButton->SetLabelWidth(
      EMSEG_WIDGETS_LABEL_WIDTH - 10);
    }

  this->Script("pack %s -side top -anchor nw -padx 2 -pady 1", 
               this->NodeParametersPrintEMLabelMapConvergenceCheckButton->GetWidgetName());

  // Create the print EM weights convergence checkbutton

  if (!this->NodeParametersPrintEMWeightsConvergenceCheckButton)
    {
    this->NodeParametersPrintEMWeightsConvergenceCheckButton = 
      vtkKWCheckButtonWithLabel::New();
    }
  if (!this->NodeParametersPrintEMWeightsConvergenceCheckButton->IsCreated())
    {
    this->NodeParametersPrintEMWeightsConvergenceCheckButton->SetParent(
      this->PrintConvergenceFrame->GetFrame());
    this->NodeParametersPrintEMWeightsConvergenceCheckButton->Create();
    this->NodeParametersPrintEMWeightsConvergenceCheckButton->SetLabelText(
      "EM Weights:");
    this->NodeParametersPrintEMWeightsConvergenceCheckButton->SetLabelWidth(
      EMSEG_WIDGETS_LABEL_WIDTH - 10);
    }

  this->Script("pack %s -side top -anchor nw -padx 2 -pady 1", 
               this->NodeParametersPrintEMWeightsConvergenceCheckButton->GetWidgetName());

  // Create the print MFA label map convergence checkbutton

  if (!this->NodeParametersPrintMFALabelMapConvergenceCheckButton)
    {
    this->NodeParametersPrintMFALabelMapConvergenceCheckButton = 
      vtkKWCheckButtonWithLabel::New();
    }
  if (!this->NodeParametersPrintMFALabelMapConvergenceCheckButton->IsCreated())
    {
    this->NodeParametersPrintMFALabelMapConvergenceCheckButton->SetParent(
      this->PrintConvergenceFrame->GetFrame());
    this->NodeParametersPrintMFALabelMapConvergenceCheckButton->Create();
    this->NodeParametersPrintMFALabelMapConvergenceCheckButton->SetLabelText(
      "MFA Label Map:");
    this->NodeParametersPrintMFALabelMapConvergenceCheckButton->SetLabelWidth(
      EMSEG_WIDGETS_LABEL_WIDTH - 10);
    }

  this->Script(
    "pack %s -side top -anchor nw -padx 2 -pady 1", 
    this->NodeParametersPrintMFALabelMapConvergenceCheckButton->GetWidgetName());

  // Create the print MFA weights convergence checkbutton

  if (!this->NodeParametersPrintMFAWeightsConvergenceCheckButton)
    {
    this->NodeParametersPrintMFAWeightsConvergenceCheckButton = 
      vtkKWCheckButtonWithLabel::New();
    }
  if (!this->NodeParametersPrintMFAWeightsConvergenceCheckButton->IsCreated())
    {
    this->NodeParametersPrintMFAWeightsConvergenceCheckButton->SetParent(
      this->PrintConvergenceFrame->GetFrame());
    this->NodeParametersPrintMFAWeightsConvergenceCheckButton->Create();
    this->NodeParametersPrintMFAWeightsConvergenceCheckButton->SetLabelText(
      "MFA Weights:");
    this->NodeParametersPrintMFAWeightsConvergenceCheckButton->SetLabelWidth(
      EMSEG_WIDGETS_LABEL_WIDTH - 10);
    }

  this->Script("pack %s -side top -anchor nw -padx 2 -pady 1", 
               this->NodeParametersPrintMFAWeightsConvergenceCheckButton->GetWidgetName());

  // Create the frame

  if (!this->NodeParametersLabelTabAdvanced)
    {
    this->NodeParametersLabelTabAdvanced = vtkKWLabel::New();
    }
  if (!this->NodeParametersLabelTabAdvanced->IsCreated())
    {
    this->NodeParametersLabelTabAdvanced->SetParent(advanced_page);
    this->NodeParametersLabelTabAdvanced->Create();
    this->Script("pack %s -side top -anchor nw -padx 2 -pady 1", this->NodeParametersLabelTabAdvanced->GetWidgetName());
    }

  if (!this->NodeParametersPCAFrame)
    {
    this->NodeParametersPCAFrame = vtkKWFrameWithLabel::New();
    }
  if (!this->NodeParametersPCAFrame->IsCreated())
    {
    this->NodeParametersPCAFrame->SetParent(advanced_page);
    this->NodeParametersPCAFrame->Create();
    this->NodeParametersPCAFrame->SetLabelText(
      "PCA Parameters");
    }

  this->Script(
    "pack %s -side top -anchor nw -fill x -padx 2 -pady 2", 
    this->NodeParametersPCAFrame->GetWidgetName());

  // Create the frame

  if (!this->NodeParametersRegistrationFrame)
    {
    this->NodeParametersRegistrationFrame = vtkKWFrameWithLabel::New();
    }
  if (!this->NodeParametersRegistrationFrame->IsCreated())
    {
    this->NodeParametersRegistrationFrame->SetParent(advanced_page);
    this->NodeParametersRegistrationFrame->Create();
    this->NodeParametersRegistrationFrame->SetLabelText(
      "Registration Parameters");
    }

  this->Script(
    "pack %s -side top -anchor nw -fill x -padx 2 -pady 2", 
    this->NodeParametersRegistrationFrame->GetWidgetName());

  // Create the frame

  if (!this->NodeParametersInteractionMatricesFrame)
    {
    this->NodeParametersInteractionMatricesFrame = vtkKWFrameWithLabel::New();
    }
  if (!this->NodeParametersInteractionMatricesFrame->IsCreated())
    {
    this->NodeParametersInteractionMatricesFrame->SetParent(advanced_page);
    this->NodeParametersInteractionMatricesFrame->Create();
    this->NodeParametersInteractionMatricesFrame->SetLabelText(
      "Mean Field Parameters");
    }

 if (!this->NodeParametersInteractionMatrices2DCheckButton)
    {
    this->NodeParametersInteractionMatrices2DCheckButton = vtkKWCheckButtonWithLabel::New();
    }
  if (!this->NodeParametersInteractionMatrices2DCheckButton->IsCreated())
    {
      this->NodeParametersInteractionMatrices2DCheckButton->SetParent( this->NodeParametersInteractionMatricesFrame->GetFrame());
    this->NodeParametersInteractionMatrices2DCheckButton->Create();
    this->NodeParametersInteractionMatrices2DCheckButton->SetLabelText("2D Neighborhood:");
    this->NodeParametersInteractionMatrices2DCheckButton->SetLabelWidth( EMSEG_WIDGETS_LABEL_WIDTH-10);
    }

  this->Script("pack %s -side top -anchor nw -padx 2 -pady 2", 
               this->NodeParametersInteractionMatrices2DCheckButton->GetWidgetName());



  // Create the frame

  if (!this->NodeParametersInhomogeneityFrame)
    {
    this->NodeParametersInhomogeneityFrame = vtkKWFrameWithLabel::New();
    }
  if (!this->NodeParametersInhomogeneityFrame->IsCreated())
    {
    this->NodeParametersInhomogeneityFrame->SetParent(advanced_page);
    this->NodeParametersInhomogeneityFrame->Create();
    this->NodeParametersInhomogeneityFrame->SetLabelText(
      "Inhomogeneity Parameters");
    }


  // Create the miscellaeneous parameters frame

  if (!this->NodeParametersMiscellaeneousFrame)
    {
    this->NodeParametersMiscellaeneousFrame = vtkKWFrameWithLabel::New();
    }
  if (!this->NodeParametersMiscellaeneousFrame->IsCreated())
    {
    this->NodeParametersMiscellaeneousFrame->SetParent(advanced_page);
    this->NodeParametersMiscellaeneousFrame->Create();
    this->NodeParametersMiscellaeneousFrame->SetLabelText(
      "Miscellaeneous Parameters");
    }

  this->Script(
    "pack %s -side top -anchor nw -fill x -padx 2 -pady 2", 
    this->NodeParametersMiscellaeneousFrame->GetWidgetName());

  // Create the Tree node exclude from incomplete EStep check button

  if (!this->NodeParametersExcludeIncompleteEStepCheckButton)
    {
    this->NodeParametersExcludeIncompleteEStepCheckButton = 
      vtkKWCheckButtonWithLabel::New();
    }
  if (!this->NodeParametersExcludeIncompleteEStepCheckButton->IsCreated())
    {
    this->NodeParametersExcludeIncompleteEStepCheckButton->SetParent(
      this->NodeParametersMiscellaeneousFrame->GetFrame());
    this->NodeParametersExcludeIncompleteEStepCheckButton->Create();
    this->NodeParametersExcludeIncompleteEStepCheckButton->GetLabel()->
      SetWidth(EMSEG_WIDGETS_LABEL_WIDTH);
    this->NodeParametersExcludeIncompleteEStepCheckButton->GetLabel()->
      SetText("Exclude From Incomplete EStep:");
    }

  this->Script(
    "pack %s -side top -anchor nw -padx 2 -pady 2", 
    this->NodeParametersExcludeIncompleteEStepCheckButton->GetWidgetName());


  // Create the Tree node generate background probability check button

  if (!this->NodeParametersGenerateBackgroundProbabilityCheckButton)
    {
    this->NodeParametersGenerateBackgroundProbabilityCheckButton = 
      vtkKWCheckButtonWithLabel::New();
    }
  if (!this->NodeParametersGenerateBackgroundProbabilityCheckButton->IsCreated())
    {
    this->NodeParametersGenerateBackgroundProbabilityCheckButton->SetParent(
      this->NodeParametersMiscellaeneousFrame->GetFrame());
    this->NodeParametersGenerateBackgroundProbabilityCheckButton->Create();
    this->NodeParametersGenerateBackgroundProbabilityCheckButton->GetLabel()->
      SetWidth(EMSEG_WIDGETS_LABEL_WIDTH);
    this->NodeParametersGenerateBackgroundProbabilityCheckButton->GetLabel()->
      SetText("Generate Background Probability:");
    }

   if (!this->NodeParametersClassOverviewWeightGlobalFrame) 
    {
      this->NodeParametersClassOverviewWeightGlobalFrame = vtkKWFrameWithLabel::New();
    }
   if (!this->NodeParametersClassOverviewWeightGlobalFrame->IsCreated())
    {
      this->NodeParametersClassOverviewWeightGlobalFrame->SetParent(parent);
      this->NodeParametersClassOverviewWeightGlobalFrame->Create();
      this->NodeParametersClassOverviewWeightGlobalFrame->SetLabelText(" Overview of Class Weights");
    }
   // Remember that we collapse the frame when going to another step to avoid the flicker bug ! 
   this->NodeParametersClassOverviewWeightGlobalFrame->ExpandFrame();
   this->Script("pack %s -side top -padx 2 -pady 2",this->NodeParametersClassOverviewWeightGlobalFrame ->GetWidgetName());



  this->DisplaySelectedNodeParametersCallback();

}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::DisplaySelectedNodeParametersCallback()
{
  // Update the UI with the proper value, if there is a selection

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  vtkEMSegmentAnatomicalStructureStep *anat_step = 
    this->GetGUI()->GetAnatomicalStructureStep();
  if (!mrmlManager || !anat_step)
    {
    return;
    }
  vtkKWTree *tree = anat_step->GetAnatomicalStructureTree()->GetWidget();
  vtksys_stl::string sel_node;
  vtkIdType sel_vol_id = 0;
  int sel_is_leaf_node = 0;
  int has_valid_selection = tree->HasSelection();
  if (has_valid_selection)
    {
    sel_node = tree->GetSelection();
    sel_vol_id = tree->GetNodeUserDataAsInt(sel_node.c_str());
    sel_is_leaf_node = mrmlManager->GetTreeNodeIsLeaf(sel_vol_id);
    }

  int enabled = tree->GetEnabled();
  char buffer[256];

  // Update the current tree node label for each tab

  if (this->NodeParametersLabelTabBasic)
    {
    if (has_valid_selection)
      {
      this->NodeParametersLabelTabBasic->SetEnabled(enabled);
      std::string nodetext("Class: " + std::string(tree->GetNodeText(sel_node.c_str())) );
      this->NodeParametersLabelTabBasic->SetText( nodetext.c_str() );
      }
    else
      {
      this->NodeParametersLabelTabBasic->SetEnabled(0);
      this->NodeParametersLabelTabBasic->SetText("");
      }
    }

  if (this->NodeParametersLabelTabStoppingConditions)
    {
    if (has_valid_selection)
      {
      this->NodeParametersLabelTabStoppingConditions->SetEnabled(enabled);
      std::string nodetext("Class: " + std::string(tree->GetNodeText(sel_node.c_str())) );
      this->NodeParametersLabelTabStoppingConditions->SetText( nodetext.c_str() );
      }
    else
      {
      this->NodeParametersLabelTabStoppingConditions->SetEnabled(0);
      this->NodeParametersLabelTabStoppingConditions->SetText("");
      }
    }

  if (this->NodeParametersLabelTabPrint)
    {
    if (has_valid_selection)
      {
      this->NodeParametersLabelTabPrint->SetEnabled(enabled);
      std::string nodetext("Class: " + std::string(tree->GetNodeText(sel_node.c_str())) );
      this->NodeParametersLabelTabPrint->SetText( nodetext.c_str() );
      }
    else
      {
      this->NodeParametersLabelTabPrint->SetEnabled(0);
      this->NodeParametersLabelTabPrint->SetText("");
      }
    }

  if (this->NodeParametersLabelTabAdvanced)
    {
    if (has_valid_selection)
      {
      this->NodeParametersLabelTabAdvanced->SetEnabled(enabled);
      std::string nodetext("Class: " + std::string(tree->GetNodeText(sel_node.c_str())) );
      this->NodeParametersLabelTabAdvanced->SetText( nodetext.c_str() );
      }
    else
      {
      this->NodeParametersLabelTabAdvanced->SetEnabled(0);
      this->NodeParametersLabelTabAdvanced->SetText("");
      }
    }

  // Update the class probability scale

  if (this->NodeParametersGlobalPriorScale)
    {
    if (has_valid_selection)
      {
      this->NodeParametersGlobalPriorScale->SetEnabled(enabled);
      sprintf(
        buffer, "NodeParametersGlobalPriorChangedCallback %d", 
        static_cast<int>(sel_vol_id));
      this->NodeParametersGlobalPriorScale->SetEndCommand(this, buffer);
      this->NodeParametersGlobalPriorScale->SetEntryCommand(this, buffer);
      this->NodeParametersGlobalPriorScale->SetValue(mrmlManager->GetTreeNodeClassProbability(sel_vol_id));
      }
    else
      {
      this->NodeParametersGlobalPriorScale->SetEnabled(0);
      this->NodeParametersGlobalPriorScale->SetEndCommand(NULL, NULL);
      this->NodeParametersGlobalPriorScale->SetEntryCommand(NULL, NULL);
      this->NodeParametersGlobalPriorScale->SetValue(0.0);
      }
    }

  // Update the spatial prior weight scale
  if (this->NodeParametersSpatialPriorWeightScale)
    {
    if (has_valid_selection)
      {
      this->NodeParametersSpatialPriorWeightScale->SetEnabled(enabled);
      sprintf(buffer, 
              "NodeParametersSpatialPriorWeightChangedCallback %d",
              static_cast<int>(sel_vol_id));
      this->NodeParametersSpatialPriorWeightScale->SetEndCommand(this, buffer);
      this->NodeParametersSpatialPriorWeightScale->SetEntryCommand(
        this, buffer);
      this->NodeParametersSpatialPriorWeightScale->SetValue(
        mrmlManager->GetTreeNodeSpatialPriorWeight(sel_vol_id));
      }
    else
      {
      this->NodeParametersSpatialPriorWeightScale->SetEnabled(0);
      this->NodeParametersSpatialPriorWeightScale->SetEndCommand(NULL, NULL);
      this->NodeParametersSpatialPriorWeightScale->SetEntryCommand(NULL, NULL);
      this->NodeParametersSpatialPriorWeightScale->SetValue(0.0);
      }
    }

    // Update the input channel weights
  this->DefineInputChannelWeightOverviewWindow(has_valid_selection*enabled); 
    for (int i = 0 ; i < (int) this->NodeParametersInputChannelWeight.size(); i++) 
      {
    if (has_valid_selection)
      {
        sprintf(buffer, "NodeParametersInputChannelWeightChangedCallback %d %d", static_cast<int>(sel_vol_id), i);
        this->NodeParametersInputChannelWeight[i]->SetEndCommand(this, buffer);
        this->NodeParametersInputChannelWeight[i]->SetEntryCommand(this, buffer);
        this->NodeParametersInputChannelWeight[i]->SetValue(mrmlManager->GetTreeNodeInputChannelWeight(sel_vol_id, i));
      }
    else 
      {
        this->NodeParametersInputChannelWeight[i]->SetEnabled(0);
        this->NodeParametersInputChannelWeight[i]->SetEndCommand(NULL, NULL);
        this->NodeParametersInputChannelWeight[i]->SetEntryCommand(NULL, NULL);
        this->NodeParametersInputChannelWeight[i]->SetValue(0.0);
      }
      }


      // Update the alpha scale

  if (this->NodeParametersAlphaScale)
    {
    if (has_valid_selection)
      {
      this->NodeParametersAlphaScale->SetEnabled(enabled);
      sprintf(buffer, "NodeParametersAlphaChangedCallback %d", 
              static_cast<int>(sel_vol_id));
      this->NodeParametersAlphaScale->SetEndCommand(this, buffer);
      this->NodeParametersAlphaScale->SetEntryCommand(this, buffer);
      if (!sel_is_leaf_node) 
    {
           this->NodeParametersAlphaScale->SetValue(mrmlManager->GetTreeNodeAlpha(sel_vol_id));
    }
      }
    else
      {
      this->NodeParametersAlphaScale->SetEnabled(0);
      this->NodeParametersAlphaScale->SetEndCommand(NULL, NULL);
      this->NodeParametersAlphaScale->SetEntryCommand(NULL, NULL);
      this->NodeParametersAlphaScale->SetValue(0.0);
      }
    if (has_valid_selection && !sel_is_leaf_node)
      {
      this->Script("grid %s  -column 0 -row 3 -sticky nw -padx 2 -pady 2", this->NodeParametersAlphaScale->GetWidgetName());
      }
    else
      {
      this->Script("grid remove %s",
                   this->NodeParametersAlphaScale->GetWidgetName());
      }
    }

  // Update the stopping conditions

  if (this->NodeParametersNotebook)
    {
    this->NodeParametersNotebook->SetPageEnabled(
      "Stopping Conditions", has_valid_selection && !sel_is_leaf_node);
    }

  // Update the EM menu button

  if (this->StoppingConditionsEMMenuButton)
    {
    vtkKWMenu *menu = 
      this->StoppingConditionsEMMenuButton->GetWidget()->GetMenu();
    menu->DeleteAllItems();
    if (has_valid_selection && !sel_is_leaf_node)
      {
        this->StoppingConditionsEMMenuButton->SetEnabled(tree->GetEnabled());
        sprintf(buffer, "StoppingConditionsEMCallback %d %d", 
              static_cast<int>(sel_vol_id), 
              vtkEMSegmentMRMLManager::StoppingConditionIterations);
        menu->AddRadioButton("Iterations", this, buffer);
        sprintf(buffer, "StoppingConditionsEMCallback %d %d", 
              static_cast<int>(sel_vol_id),
              vtkEMSegmentMRMLManager::StoppingConditionLabelMapMeasure);
        menu->AddRadioButton("Label Map", this, buffer);
        sprintf(buffer, "StoppingConditionsEMCallback %d %d", 
              static_cast<int>(sel_vol_id), 
              vtkEMSegmentMRMLManager::StoppingConditionWeightsMeasure);
        menu->AddRadioButton("Weights", this, buffer);
        vtksys_stl::string value;
        switch (mrmlManager->GetTreeNodeStoppingConditionEMType(sel_vol_id))
        {
            case vtkEMSegmentMRMLManager::StoppingConditionIterations:
               value = "Iterations";
               break;
            case vtkEMSegmentMRMLManager::StoppingConditionLabelMapMeasure:
              value = "Label Map";
              break;
           case vtkEMSegmentMRMLManager::StoppingConditionWeightsMeasure:
             value = "Weights";
             break;
        }
      this->StoppingConditionsEMMenuButton->GetWidget()->SetValue(value.c_str());
      }
    else
      {
      this->StoppingConditionsEMMenuButton->GetWidget()->SetValue("");
      this->StoppingConditionsEMMenuButton->SetEnabled(0);
      }
    }

  // Update the EM iterations entry

  if (this->StoppingConditionsEMIterationsEntry)
    {
    vtkKWEntry *entry = 
      this->StoppingConditionsEMIterationsEntry->GetWidget();
    if (has_valid_selection && !sel_is_leaf_node)
      {
      this->StoppingConditionsEMIterationsEntry->SetEnabled(enabled);
      sprintf(buffer, "StoppingConditionsEMIterationsCallback %d", 
              static_cast<int>(sel_vol_id));
      entry->SetCommand(this, buffer);
      entry->SetValueAsInt(mrmlManager->GetTreeNodeStoppingConditionEMIterations(sel_vol_id));
      }
    else
      {
      this->StoppingConditionsEMIterationsEntry->SetEnabled(0);
      entry->SetCommand(NULL, NULL);
      entry->SetValue("");
      }
    if (has_valid_selection && !sel_is_leaf_node &&
        mrmlManager->GetTreeNodeStoppingConditionEMType(sel_vol_id) ==
        vtkEMSegmentMRMLManager::StoppingConditionIterations)
      {
      this->Script("grid %s",
                   this->StoppingConditionsEMIterationsEntry->GetWidgetName());
      }
    else
      {
      this->Script("grid remove %s",
                   this->StoppingConditionsEMIterationsEntry->GetWidgetName());
      }
    }

  // Update the EM value entry

  if (this->StoppingConditionsEMValueEntry)
    {
    vtkKWEntry *entry = this->StoppingConditionsEMValueEntry->GetWidget();
    if (has_valid_selection && !sel_is_leaf_node)
      {
      this->StoppingConditionsEMValueEntry->SetEnabled(enabled);
      sprintf(buffer, "StoppingConditionsEMValueCallback %d", 
              static_cast<int>(sel_vol_id));
      entry->SetCommand(this, buffer);
      entry->SetValueAsDouble(
        mrmlManager->GetTreeNodeStoppingConditionEMValue(sel_vol_id));
      }
    else
      {
      this->StoppingConditionsEMValueEntry->SetEnabled(0);
      entry->SetCommand(NULL, NULL);
      entry->SetValue("");
      }
    if (has_valid_selection && !sel_is_leaf_node && 
        (mrmlManager->GetTreeNodeStoppingConditionEMType(sel_vol_id) ==
         vtkEMSegmentMRMLManager::StoppingConditionLabelMapMeasure ||
         mrmlManager->GetTreeNodeStoppingConditionEMType(sel_vol_id) ==
         vtkEMSegmentMRMLManager::StoppingConditionWeightsMeasure))
      {
      this->Script("grid %s",
                   this->StoppingConditionsEMValueEntry->GetWidgetName());
      }
    else
      {
      this->Script("grid remove %s",
                   this->StoppingConditionsEMValueEntry->GetWidgetName());
      }
    }

  // Update the MFA menu button

  if (this->StoppingConditionsMFAMenuButton)
    {
    vtkKWMenu *menu = 
      this->StoppingConditionsMFAMenuButton->GetWidget()->GetMenu();
    menu->DeleteAllItems();
    if (has_valid_selection && !sel_is_leaf_node)
      {
      this->StoppingConditionsMFAMenuButton->SetEnabled(tree->GetEnabled());
      sprintf(buffer, "StoppingConditionsMFACallback %d %d", 
              static_cast<int>(sel_vol_id),
              vtkEMSegmentMRMLManager::StoppingConditionIterations);
      menu->AddRadioButton("Iterations", this, buffer);
      sprintf(buffer, "StoppingConditionsMFACallback %d %d", 
              static_cast<int>(sel_vol_id),
              vtkEMSegmentMRMLManager::StoppingConditionLabelMapMeasure);
      menu->AddRadioButton("Label Map", this, buffer);
      sprintf(buffer, "StoppingConditionsMFACallback %d %d", 
              static_cast<int>(sel_vol_id),
              vtkEMSegmentMRMLManager::StoppingConditionWeightsMeasure);
      menu->AddRadioButton("Weights", this, buffer);
      vtksys_stl::string value;
      switch (mrmlManager->GetTreeNodeStoppingConditionMFAType(sel_vol_id))
        {
        case vtkEMSegmentMRMLManager::StoppingConditionIterations:
          value = "Iterations";
          break;
        case vtkEMSegmentMRMLManager::StoppingConditionLabelMapMeasure:
          value = "Label Map";
          break;
        case vtkEMSegmentMRMLManager::StoppingConditionWeightsMeasure:
          value = "Weights";
          break;
        }
      this->StoppingConditionsMFAMenuButton->GetWidget()->SetValue(
        value.c_str());
      }
    else
      {
      this->StoppingConditionsMFAMenuButton->GetWidget()->SetValue("");
      this->StoppingConditionsMFAMenuButton->SetEnabled(0);
      }
    }

  // Update the MFA iterations entry

  if (this->StoppingConditionsMFAIterationsEntry)
    {
    vtkKWEntry *entry = 
      this->StoppingConditionsMFAIterationsEntry->GetWidget();
    if (has_valid_selection && !sel_is_leaf_node)
      {
      this->StoppingConditionsMFAIterationsEntry->SetEnabled(enabled);
      sprintf(buffer, "StoppingConditionsMFAIterationsCallback %d",
              static_cast<int>(sel_vol_id));
      entry->SetCommand(this, buffer);
      entry->SetValueAsInt(
        mrmlManager->GetTreeNodeStoppingConditionMFAIterations(sel_vol_id));
      }
    else
      {
      this->StoppingConditionsMFAIterationsEntry->SetEnabled(0);
      entry->SetCommand(NULL, NULL);
      entry->SetValue("");
      }
    if (has_valid_selection && !sel_is_leaf_node && 
        mrmlManager->GetTreeNodeStoppingConditionMFAType(sel_vol_id) ==
        vtkEMSegmentMRMLManager::StoppingConditionIterations)
      {
      this->Script("grid %s",
                  this->StoppingConditionsMFAIterationsEntry->GetWidgetName());
      }
    else
      {
      this->Script("grid remove %s",
                  this->StoppingConditionsMFAIterationsEntry->GetWidgetName());
      }
    }

  // Update the MFA value entry

  if (this->StoppingConditionsMFAValueEntry)
    {
    vtkKWEntry *entry = this->StoppingConditionsMFAValueEntry->GetWidget();
    if (has_valid_selection && !sel_is_leaf_node)
      {
      this->StoppingConditionsMFAValueEntry->SetEnabled(enabled);
      sprintf(buffer, "StoppingConditionsMFAValueCallback %d",
              static_cast<int>(sel_vol_id));
      entry->SetCommand(this, buffer);
      entry->SetValueAsDouble(
        mrmlManager->GetTreeNodeStoppingConditionMFAValue(sel_vol_id));
      }
    else
      {
      this->StoppingConditionsMFAValueEntry->SetEnabled(0);
      entry->SetCommand(NULL, NULL);
      entry->SetValue("");
      }
    if (has_valid_selection && !sel_is_leaf_node && 
        (mrmlManager->GetTreeNodeStoppingConditionMFAType(sel_vol_id) ==
         vtkEMSegmentMRMLManager::StoppingConditionLabelMapMeasure ||
         mrmlManager->GetTreeNodeStoppingConditionMFAType(sel_vol_id) ==
         vtkEMSegmentMRMLManager::StoppingConditionWeightsMeasure))
      {
      this->Script("grid %s",
                   this->StoppingConditionsMFAValueEntry->GetWidgetName());
      }
    else
      {
      this->Script("grid remove %s",
                   this->StoppingConditionsMFAValueEntry->GetWidgetName());
      }
    }

  // Update the Bias Calculation Max Iterations entry

  if (this->StoppingConditionsBiasIterationsEntry)
    {
    vtkKWEntry *entry = 
      this->StoppingConditionsBiasIterationsEntry->GetWidget();
    if (has_valid_selection && !sel_is_leaf_node)
      {
      sprintf(buffer,"StoppingConditionsBiasIterationsCallback %d",
              static_cast<int>(sel_vol_id));
      entry->SetCommand(this, buffer);
      entry->SetValueAsInt(
        mrmlManager->GetTreeNodeBiasCalculationMaxIterations(sel_vol_id));
      this->Script(
        "grid %s",
        this->StoppingConditionsBiasIterationsEntry->GetWidgetName());
      }
    else
      {
      entry->SetCommand(NULL, NULL);
      entry->SetValue("");
      this->Script(
        "grid remove %s",
        this->StoppingConditionsBiasIterationsEntry->GetWidgetName());
      }
    }

  // Update the print weight

  if (this->NodeParametersPrintWeightCheckButton)
    {
    vtkKWCheckButton *cb = 
      this->NodeParametersPrintWeightCheckButton->GetWidget();
    if (has_valid_selection)
      {
      this->NodeParametersPrintWeightCheckButton->SetEnabled(enabled);
      sprintf(buffer, "NodeParametersPrintWeightCallback %d", 
              static_cast<int>(sel_vol_id));
      cb->SetCommand(this, buffer);
      cb->SetSelectedState(mrmlManager->GetTreeNodePrintWeight(sel_vol_id));
      }
    else
      {
      this->NodeParametersPrintWeightCheckButton->SetEnabled(0);
      cb->SetCommand(NULL, NULL);
      cb->SetSelectedState(0);
      }
    }

  // Update the print quality

  if (this->NodeParametersPrintQualityCheckButton)
    {
    vtkKWCheckButton *cb = 
      this->NodeParametersPrintQualityCheckButton->GetWidget();
    if (has_valid_selection && sel_is_leaf_node)
      {
      sprintf(buffer, "NodeParametersPrintQualityCallback %d", 
              static_cast<int>(sel_vol_id));
      cb->SetCommand(this, buffer);
      cb->SetSelectedState(mrmlManager->GetTreeNodePrintQuality(sel_vol_id));
      this->Script("pack %s -side top -anchor nw -padx 2 -pady 2", 
                   this->NodeParametersPrintQualityCheckButton->GetWidgetName());
      }
    else
      {
      cb->SetCommand(NULL, NULL);
      cb->SetSelectedState(0);
      this->Script(
        "pack forget %s", 
        this->NodeParametersPrintQualityCheckButton->GetWidgetName());
      }
    }

  // Update the print frequency

  if (this->NodeParametersPrintFrequencyScale)
    {
    if (has_valid_selection && !sel_is_leaf_node)
      {
      sprintf(
        buffer, "NodeParametersPrintFrequencyChangedCallback %d", 
        static_cast<int>(sel_vol_id));
      this->NodeParametersPrintFrequencyScale->SetEndCommand(this, buffer);
      this->NodeParametersPrintFrequencyScale->SetEntryCommand(this, buffer);
      this->NodeParametersPrintFrequencyScale->
        SetValue(mrmlManager->GetTreeNodePrintFrequency(sel_vol_id));
      this->Script("pack %s -side top -anchor nw -padx 2 -pady 2", 
                   this->NodeParametersPrintFrequencyScale->GetWidgetName());
      }
    else
      {
      this->NodeParametersPrintFrequencyScale->SetEndCommand(NULL, NULL);
      this->NodeParametersPrintFrequencyScale->SetEntryCommand(NULL, NULL);
      this->Script("pack forget %s", 
                   this->NodeParametersPrintFrequencyScale->GetWidgetName());
      }
    }

  // Update the print bias

  if (this->NodeParametersPrintBiasCheckButton)
    {
    vtkKWCheckButton *cb = 
      this->NodeParametersPrintBiasCheckButton->GetWidget();
    if (has_valid_selection && !sel_is_leaf_node)
      {
      sprintf(buffer, "NodeParametersPrintBiasCallback %d", 
              static_cast<int>(sel_vol_id));
      cb->SetCommand(this, buffer);
      cb->SetSelectedState(mrmlManager->GetTreeNodePrintBias(sel_vol_id));
      this->Script("pack %s -side top -anchor nw -padx 2 -pady 2", 
                   this->NodeParametersPrintBiasCheckButton->GetWidgetName());
      }
    else
      {
      cb->SetCommand(NULL, NULL);
      cb->SetSelectedState(0);
      this->Script("pack forget %s", 
                   this->NodeParametersPrintBiasCheckButton->GetWidgetName());
      }
    }

  // Update the print label map

  if (this->NodeParametersPrintLabelMapCheckButton)
    {
    vtkKWCheckButton *cb = 
      this->NodeParametersPrintLabelMapCheckButton->GetWidget();
    if (has_valid_selection && !sel_is_leaf_node)
      {
      sprintf(buffer, "NodeParametersPrintLabelMapCallback %d", 
              static_cast<int>(sel_vol_id));
      cb->SetCommand(this, buffer);
      cb->SetSelectedState(mrmlManager->GetTreeNodePrintLabelMap(sel_vol_id));
      this->Script("pack %s -side top -anchor nw -padx 2 -pady 2", 
        this->NodeParametersPrintLabelMapCheckButton->GetWidgetName());
      }
    else
      {
      cb->SetCommand(NULL, NULL);
      cb->SetSelectedState(0);
      this->Script(
        "pack forget %s", 
        this->NodeParametersPrintLabelMapCheckButton->GetWidgetName());
      }
    }

  // Update the print EM label map

  if (this->NodeParametersPrintEMLabelMapConvergenceCheckButton)
    {
    vtkKWCheckButton *cb = 
      this->NodeParametersPrintEMLabelMapConvergenceCheckButton->GetWidget();
    if (has_valid_selection && !sel_is_leaf_node)
      {
      sprintf(buffer, "NodeParametersPrintEMLabelMapCallback %d", 
              static_cast<int>(sel_vol_id));
      cb->SetCommand(this, buffer);
      cb->SetSelectedState(
        mrmlManager->GetTreeNodePrintEMLabelMapConvergence(sel_vol_id));
      }
    else
      {
      cb->SetCommand(NULL, NULL);
      cb->SetSelectedState(0);
      }
    }

  // Update the print EM weights

  if (this->NodeParametersPrintEMWeightsConvergenceCheckButton)
    {
    vtkKWCheckButton *cb = 
      this->NodeParametersPrintEMLabelMapConvergenceCheckButton->GetWidget();
    if (has_valid_selection && !sel_is_leaf_node)
      {
      sprintf(buffer, "NodeParametersPrintEMWeightsCallback %d", 
              static_cast<int>(sel_vol_id));
      cb->SetCommand(this, buffer);
      cb->SetSelectedState(
        mrmlManager->GetTreeNodePrintEMWeightsConvergence(sel_vol_id));
      }
    else
      {
      cb->SetCommand(NULL, NULL);
      cb->SetSelectedState(0);
      }
    }

  // Update the print MFA label map

  if (this->NodeParametersPrintMFALabelMapConvergenceCheckButton)
    {
    vtkKWCheckButton *cb = 
      this->NodeParametersPrintMFALabelMapConvergenceCheckButton->GetWidget();
    if (has_valid_selection && !sel_is_leaf_node)
      {
      sprintf(buffer, "NodeParametersPrintMFALabelMapCallback %d", 
              static_cast<int>(sel_vol_id));
      cb->SetCommand(this, buffer);
      cb->SetSelectedState(
        mrmlManager->GetTreeNodePrintMFALabelMapConvergence(sel_vol_id));
      }
    else
      {
      cb->SetCommand(NULL, NULL);
      cb->SetSelectedState(0);
      }
    }

  // Update the print MFA label map

  if (this->NodeParametersPrintMFAWeightsConvergenceCheckButton)
    {
    vtkKWCheckButton *cb = 
      this->NodeParametersPrintMFALabelMapConvergenceCheckButton->GetWidget();
    if (has_valid_selection && !sel_is_leaf_node)
      {
      sprintf(buffer, "NodeParametersPrintMFAWeightsCallback %d", 
              static_cast<int>(sel_vol_id));
      cb->SetCommand(this, buffer);
      cb->SetSelectedState(
        mrmlManager->GetTreeNodePrintMFAWeightsConvergence(sel_vol_id));
      }
    else
      {
      cb->SetCommand(NULL, NULL);
      cb->SetSelectedState(0);
      }
    }

  // Update the print convergence frame
  if (this->PrintConvergenceFrame)
    {
    if (has_valid_selection && !sel_is_leaf_node)
      {
      this->Script(
        "pack %s -side left -anchor nw -fill x -expand y -padx 5 -pady 2", 
        this->PrintConvergenceFrame->GetWidgetName());
      }
    else
      {
      this->Script(
        "pack forget %s", 
        this->PrintConvergenceFrame->GetWidgetName());
      }
    }

  // Update the class interaction matrices

  if (this->NodeParametersInteractionMatricesFrame)
    {
    if (has_valid_selection && !sel_is_leaf_node)
      {
      this->Script(
        "pack %s -side top -anchor nw -fill x -padx 2 -pady 2", 
        this->NodeParametersInteractionMatricesFrame->GetWidgetName());
      }
    else
      {
      this->Script(
        "pack forget %s", 
        this->NodeParametersInteractionMatricesFrame->GetWidgetName());
      }
    }

  if (this->NodeParametersInteractionMatrices2DCheckButton)
    {
      vtkKWCheckButton *cb =  this->NodeParametersInteractionMatrices2DCheckButton->GetWidget();
       if (has_valid_selection && !sel_is_leaf_node)
       {
      sprintf(buffer, "NodeParametersInteractionMatrices2DCallback %d", 
              static_cast<int>(sel_vol_id));
      cb->SetCommand(this, buffer);
      cb->SetSelectedState(mrmlManager->GetTreeNodeInteractionMatrices2DFlag(sel_vol_id));
      this->Script("pack %s -side top -anchor nw -padx 2 -pady 2", 
        this->NodeParametersInteractionMatrices2DCheckButton->GetWidgetName());
      } 
    else
      {
      cb->SetCommand(NULL, NULL);
      cb->SetSelectedState(0);
      this->Script(
        "pack forget %s", 
        this->NodeParametersInteractionMatrices2DCheckButton->GetWidgetName());
      }
    }

  // Update the inhomogeneity matrices

  if (this->NodeParametersInhomogeneityFrame)
    {
    if (has_valid_selection && !sel_is_leaf_node)
      {
      this->Script(
        "pack %s -side top -anchor nw -fill x -padx 2 -pady 2", 
        this->NodeParametersInhomogeneityFrame->GetWidgetName());
      }
    else
      {
      this->Script(
        "pack forget %s", 
        this->NodeParametersInhomogeneityFrame->GetWidgetName());
      }
    }
  
  // Update miscellaeneous Exclude from incomplete EStep parameters

  if(this->NodeParametersExcludeIncompleteEStepCheckButton)
    {
    vtkKWCheckButton *cb = 
      this->NodeParametersExcludeIncompleteEStepCheckButton->GetWidget();
    if (has_valid_selection)
      {
      sprintf(buffer, "ExcludeIncompleteEStepCallback %d", 
              static_cast<int>(sel_vol_id));
      cb->SetCommand(this, buffer);
      if (!sel_is_leaf_node) 
    {
          cb->SetSelectedState(mrmlManager->GetTreeNodeExcludeFromIncompleteEStep(sel_vol_id));
    }
    this->NodeParametersExcludeIncompleteEStepCheckButton->SetEnabled(enabled);
      }
    else
      {
      cb->SetCommand(NULL, NULL);
      cb->SetSelectedState(0);
      this->NodeParametersExcludeIncompleteEStepCheckButton->SetEnabled(0);
      }
    }

  // Update miscellaneous Generate Background probability parameters

  if(this->NodeParametersGenerateBackgroundProbabilityCheckButton)
    {
    vtkKWCheckButton *cb = 
     this->NodeParametersGenerateBackgroundProbabilityCheckButton->GetWidget();
    if (has_valid_selection && !sel_is_leaf_node)
      {
      sprintf(buffer, "GenerateBackgroundProbabilityCallback %d", 
              static_cast<int>(sel_vol_id));
      cb->SetCommand(this, buffer);
      cb->SetSelectedState(
        mrmlManager->GetTreeNodeGenerateBackgroundProbability(sel_vol_id));
      this->Script(
        "pack %s -side top -anchor nw -padx 2 -pady 2", 
        this->NodeParametersGenerateBackgroundProbabilityCheckButton->GetWidgetName());
      }
    else
      {
      cb->SetCommand(NULL, NULL);
      cb->SetSelectedState(0);
      this->Script(
        "pack forget %s", 
        this->NodeParametersGenerateBackgroundProbabilityCheckButton->GetWidgetName());
      }
    }

    // Class Overview Window 
   if (has_valid_selection)
     {
       this->DefineClassOverviewWeightWindow(sel_vol_id,enabled);
     }
   else 
     {
       this->DefineClassOverviewWeightWindow(-1,0);
     }

}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::NodeParametersGlobalPriorChangedCallback(vtkIdType sel_class_id, double value)
{
  this->ClassWeightChangedCallback(sel_class_id,value);
}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::NodeParametersSpatialPriorWeightChangedCallback(
  vtkIdType sel_vol_id, double value)
{
  // The spatial prior weight has changed because of user interaction

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (mrmlManager)
    {
    mrmlManager->SetTreeNodeSpatialPriorWeight(sel_vol_id, value);
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::NodeParametersInputChannelWeightChangedCallback(vtkIdType sel_vol_id, int row, double value)
{
  // The input channel weight has changed because of user interaction
  if (value >= 0 && value <= 1.0)
    {
      vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
      if (mrmlManager)
      {
      mrmlManager->SetTreeNodeInputChannelWeight(sel_vol_id, row, value);
      }
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::NodeParametersAlphaChangedCallback(
  vtkIdType sel_vol_id, double value)
{
  // The alpha has changed because of user interaction

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (mrmlManager)
    {
    mrmlManager->SetTreeNodeAlpha(sel_vol_id, value);
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::StoppingConditionsEMCallback(
  vtkIdType sel_vol_id, int value)
{
  // The EM stopping condition has changed because of user interaction

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (mrmlManager && value != mrmlManager->GetTreeNodeStoppingConditionEMType(sel_vol_id))
    {
    mrmlManager->SetTreeNodeStoppingConditionEMType(sel_vol_id, value);
    // Needs to go there so that iterations field get disabled or enabled depending one the selection
    this->DisplaySelectedNodeParametersCallback();
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::StoppingConditionsEMIterationsCallback(
  vtkIdType sel_vol_id, const char *value)
{
  // The EM iterations has changed because of user interaction

  int v = atoi(value);
  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (mrmlManager && v != mrmlManager->GetTreeNodeStoppingConditionEMIterations(sel_vol_id))
    {
      if (v < 0) 
    {
          mrmlManager->SetTreeNodeStoppingConditionEMIterations(sel_vol_id, 0);
          // Update user interface accordingly 
          this->DisplaySelectedNodeParametersCallback(); 
    } 
      else 
    {
      mrmlManager->SetTreeNodeStoppingConditionEMIterations(sel_vol_id, v);
    } 
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::StoppingConditionsEMValueCallback(
  vtkIdType sel_vol_id, const char *value)
{
  // The EM value has changed because of user interaction

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (mrmlManager && (atof(value) !=  mrmlManager->GetTreeNodeStoppingConditionEMValue(sel_vol_id)))
    {
      mrmlManager->SetTreeNodeStoppingConditionEMValue(sel_vol_id, atof(value));
      // there is no such policy if v < 0 then iterations is not set to v !  
      // this->DisplaySelectedNodeParametersCallback();
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::StoppingConditionsMFACallback(
  vtkIdType sel_vol_id, int value)
{
  // The MFA stopping condition has changed because of user interaction

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (mrmlManager && value != mrmlManager->GetTreeNodeStoppingConditionMFAType(sel_vol_id))
    {
    mrmlManager->SetTreeNodeStoppingConditionMFAType(sel_vol_id, value);

    // Needs to go there so that iterations field get disabled or enabled depending one the selection
    this->DisplaySelectedNodeParametersCallback();
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::StoppingConditionsMFAIterationsCallback(
  vtkIdType sel_vol_id, const char *value)
{
  // The MFA iterations has changed because of user interaction

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (!mrmlManager)
    {
    return;
    }
  int v = atoi(value);
  if (v != mrmlManager->GetTreeNodeStoppingConditionMFAIterations(sel_vol_id))
    {
      if (v < 0) 
    {
          mrmlManager->SetTreeNodeStoppingConditionMFAIterations(sel_vol_id, 0);
          // Update user interface accordingly 
          this->DisplaySelectedNodeParametersCallback(); 
    } 
      else 
    {
      mrmlManager->SetTreeNodeStoppingConditionMFAIterations(sel_vol_id, v);
    } 
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::StoppingConditionsMFAValueCallback(
  vtkIdType sel_vol_id, const char *value)
{
  // The MFA value has changed because of user interaction

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (mrmlManager && (atof(value) !=  mrmlManager->GetTreeNodeStoppingConditionMFAValue(sel_vol_id)))
    {
    mrmlManager->SetTreeNodeStoppingConditionMFAValue(sel_vol_id, atof(value));
    // There is no such condition: if v < 0 then iterations is not set to v !  
    // this->DisplaySelectedNodeParametersCallback();
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::StoppingConditionsBiasIterationsCallback(
  vtkIdType sel_vol_id, const char *value)
{
  // The Bias calculation max iterations value 
  // has changed because of user interaction

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (mrmlManager)
    {
    mrmlManager->SetTreeNodeBiasCalculationMaxIterations(
      sel_vol_id, atoi(value));
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::NodeParametersPrintWeightCallback(
  vtkIdType sel_vol_id, int value)
{
  // The print weight has changed because of user interaction

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (mrmlManager)
    {
    mrmlManager->SetTreeNodePrintWeight(sel_vol_id, value);
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::NodeParametersPrintQualityCallback(
  vtkIdType sel_vol_id, int value)
{
  // The print quality has changed because of user interaction

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (mrmlManager)
    {
    mrmlManager->SetTreeNodePrintQuality(sel_vol_id, value);
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::NodeParametersPrintFrequencyChangedCallback(
  vtkIdType sel_vol_id, int value)
{
  // The print frequency has changed because of user interaction

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (mrmlManager)
    {
    mrmlManager->SetTreeNodePrintFrequency(sel_vol_id, value);
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::NodeParametersPrintBiasCallback(
  vtkIdType sel_vol_id, int value)
{
  // The print bias has changed because of user interaction

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (mrmlManager)
    {
    mrmlManager->SetTreeNodePrintBias(sel_vol_id, value);
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::NodeParametersPrintLabelMapCallback(
  vtkIdType sel_vol_id, int value)
{
  // The print label map has changed because of user interaction

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (mrmlManager)
    {
    mrmlManager->SetTreeNodePrintLabelMap(sel_vol_id, value);
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::NodeParametersPrintEMLabelMapCallback(
  vtkIdType sel_vol_id, int value)
{
  // The print EM label map convergence has changed because of user interaction

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (mrmlManager)
    {
    mrmlManager->SetTreeNodePrintEMLabelMapConvergence(sel_vol_id, value);
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::NodeParametersPrintEMWeightsCallback(
  vtkIdType sel_vol_id, int value)
{
  // The print EM weight convergence has changed because of user interaction

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (mrmlManager)
    {
    mrmlManager->SetTreeNodePrintEMWeightsConvergence(sel_vol_id, value);
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::NodeParametersPrintMFALabelMapCallback(
  vtkIdType sel_vol_id, int value)
{
  // The print MFA label map convergence has changed because of user interaction
  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (mrmlManager)
    {
    mrmlManager->SetTreeNodePrintMFALabelMapConvergence(sel_vol_id, value);
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::NodeParametersPrintMFAWeightsCallback(
  vtkIdType sel_vol_id, int value)
{
  // The print MFA weight convergence has changed because of user interaction

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (mrmlManager)
    {
    mrmlManager->SetTreeNodePrintMFAWeightsConvergence(sel_vol_id, value);
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::ExcludeIncompleteEStepCallback(
  vtkIdType sel_vol_id, int state)
{
  // The exclude from incomplete EStep checkbox has changed 
  // because of user interaction

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (mrmlManager)
    {
    mrmlManager->SetTreeNodeExcludeFromIncompleteEStep(sel_vol_id, state);
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::GenerateBackgroundProbabilityCallback(
  vtkIdType sel_vol_id, int state)
{
  // The generate background probability checkbox has changed
  // because of user interaction

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (mrmlManager)
    {
    mrmlManager->SetTreeNodeGenerateBackgroundProbability(sel_vol_id, state);
    }
}

void vtkEMSegmentNodeParametersStep::HideUserInterface()
{
  this->Superclass::HideUserInterface();
  // Need to do this bc otherwise when going back to the step from another one 
  // the famous flicker bug appears when the frame is expanded and has three entries 
  if (this->NodeParametersClassOverviewWeightGlobalFrame)
    {
      this->NodeParametersClassOverviewWeightGlobalFrame->CollapseFrame();
    }
}


//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::Validate()
{
 
  if (this->NodeParametersClassOverviewWeightGlobalFrame)
    {
      this->NodeParametersClassOverviewWeightGlobalFrame->CollapseFrame();
    }

  vtkKWWizardWorkflow *wizard_workflow = 
    this->GetGUI()->GetWizardWidget()->GetWizardWorkflow();
  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (!mrmlManager)
    {
    return;
    }
  // make sure that the probability for each set of sibling nodes sums
  // to unity

  if (mrmlManager->GetTreeRootNode() != NULL)
    {
    vtkIdType firstBadTreeID = 
      mrmlManager->GetTreeNodeFirstIDWithChildProbabilityError();
    if (firstBadTreeID >= 0)
      {
      std::stringstream ss;
      ss << "Child probabilities must sum to one for node "
         << mrmlManager->GetTreeNodeName(firstBadTreeID)
         << "; right now they sum to "
         << mrmlManager->GetTreeNodeChildrenSumClassProbability(firstBadTreeID)
         << ".  Please fix before continuing---"
         << "you should edit the \"Class Weight\" fields for the"
         << " children nodes of "
         << mrmlManager->GetTreeNodeName(firstBadTreeID) << ".";
      
      std::string parentNodeName = 
        mrmlManager->GetTreeNodeName(firstBadTreeID);
      std::string errorMessage   = parentNodeName  + 
        ": Child probabilities must sum to one!  " +
        "Please fix before continuing.";
      vtkKWMessageDialog::PopupMessage(this->GetApplication(),
                                       NULL,
                                       "Node Parameters Error",
                                       ss.str().c_str(),
                                       vtkKWMessageDialog::ErrorIcon | 
                                       vtkKWMessageDialog::InvokeAtPointer);
      
      // there was an error; stay on this step
      wizard_workflow->
        PushInput(vtkKWWizardStep::GetValidationFailedInput());
      wizard_workflow->ProcessInputs();
      }
    // go through tree and make sure that the atlas weight of all classes is set to zero who have no atlas assigned  
    this->GetGUI()->GetLogic()->AutoCorrectSpatialPriorWeight(mrmlManager->GetTreeRootNodeID());

     
  }

  this->Superclass::Validate();
}

//----------------------------------------------------------------------------
// void vtkEMSegmentNodeParametersStep::RightClickOnInputChannelWeightsListCallback(int row, int col, int vtkNotUsed(x), int vtkNotUsed(y))
// {
//   vtkKWMultiColumnList *list =   this->NodeParametersInputChannelWeightsList->GetWidget()->GetWidget();
//   list->EditCell(row, col);
// }

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::ClassWeightAutoChangedCallback(vtkIdType  sel_class_id, int state)
{

  if (state) 
    {      
      this->ClassWeightChangedCallback(sel_class_id, 0);
    }
}
//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::ClassWeightChangedCallback(vtkIdType  sel_class_id, double value)
{
  // General checks 
  if (this->ClassOverviewWeightAutomaticRecalculateFlag)
    {
      return;
    }

  if (value < 0 && value > 1.0)
    {
      this->DisplaySelectedNodeParametersCallback();
      return;
    }


  // Define General Variables 
  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  // just check it so that you do not get an error message
  if ( mrmlManager->GetTreeRootNodeID() == sel_class_id)
    {
      return ;
    }

  vtkIdType parent = mrmlManager->GetTreeNodeParentNodeID(sel_class_id);
  if (!parent) {
    return;
  }

  // vtkKWMultiColumnList *list = this->ClassOverviewWeightList->GetWidget()->GetWidget();

  // Check if we have to do anything 
  mrmlManager->SetTreeNodeClassProbability(sel_class_id, value);
  double NormProb = mrmlManager->GetTreeNodeChildrenSumClassProbability(parent);

  if (NormProb == 0.0 || (fabs(1 - NormProb) < 0.00005 ))
    {
      return ;
    }

  // Start Normalizing
  this->ClassOverviewWeightAutomaticRecalculateFlag = 1;
  int numChildren = mrmlManager->GetTreeNodeNumberOfChildren(parent);
  double FixedProb = 0.0;
  double FlexProb  = 0.0;
  std::vector<int> autoState;
  autoState.resize(numChildren);
  std::vector<vtkIdType> childID;
  childID.resize(numChildren);
  std::vector<double> prob;
  prob.resize(numChildren);

  int autoStateFlag =0 ;
  for (int index = 0; index < numChildren; index++)
    {
      if ((index < (int) this->NodeParametersClassOverviewWeightAuto.size()) && this->NodeParametersClassOverviewWeightAuto[index] &&  this->NodeParametersClassOverviewWeightAuto[index]->GetSelectedState())
      {
        autoState[index] = 1;
        autoStateFlag = 1;
      } 
      else 
      {
        autoState[index] = 0;
      }

      childID[index] = mrmlManager->GetTreeNodeChildNodeID(parent,index);
      prob[index] = mrmlManager->GetTreeNodeClassProbability(childID[index]);
      if (autoState[index]) {
         FlexProb += prob[index];
      } else {
         FixedProb += prob[index];
      }
    }
     
  if (autoStateFlag)
    {   
    double LeftProb = 1 - FixedProb; 
    if (LeftProb < 0) 
      {
        for (int index = 0; index < numChildren; index++)
      {
        if (autoState[index]) {
          mrmlManager->SetTreeNodeClassProbability(childID[index],0.0);
        }
      }
      }
    else if (FlexProb > 0 ) 
      {
        double norm = LeftProb/FlexProb;
        for (int index = 0; index < numChildren; index++)
        {
          if (autoState[index]) 
          {
            mrmlManager->SetTreeNodeClassProbability(childID[index],norm*prob[index]);
          }
        }
      }
    else  if (LeftProb  > 0)
      {
        int index = 0;
        while (!autoState[index] && (index < numChildren)) 
      {
        index++;
      }
        if (index < numChildren)
        {
      mrmlManager->SetTreeNodeClassProbability(childID[index],LeftProb);
        }
      }
    }
  this->DisplaySelectedNodeParametersCallback();
  this->ClassOverviewWeightAutomaticRecalculateFlag = 0;
}
 
//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::DefineInputChannelWeightOverviewWindow(int enabled) 
{
  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  vtkMRMLEMSVolumeCollectionNode *inputNodes = mrmlManager->GetTargetInputNode();
  vtkMRMLEMSGlobalParametersNode* globalNode = mrmlManager->GetGlobalParametersNode();
  if (!inputNodes || !globalNode) 
    {
      return;
    }

  int oldSize = this->NodeParametersInputChannelWeight.size();
  int newSize = mrmlManager->GetTargetNumberOfSelectedVolumes();

  if (!this->NodeParametersInputChannelWeightFrame) 
    {
      this->NodeParametersInputChannelWeightFrame = vtkKWFrameWithLabel::New();
    }
  if (!this->NodeParametersInputChannelWeightFrame->IsCreated())
    {
      this->NodeParametersInputChannelWeightFrame->SetParent(this->NodeParametersNotebook->GetFrame("Basic"));
      this->NodeParametersInputChannelWeightFrame->Create();
      this->NodeParametersInputChannelWeightFrame->SetLabelText("Input Channel Weights");
    }
  this->Script("grid %s -column 1 -row 1 -rowspan 3 -sticky news -padx 2 -pady 2", this->NodeParametersInputChannelWeightFrame->GetWidgetName());

  if (oldSize > newSize)
    {
      for (int i = newSize ; i < oldSize; i++) 
      {
        if (this->NodeParametersInputChannelWeight[i])
        {
      this->NodeParametersInputChannelWeight[i]->Unpack(); 
          this->NodeParametersInputChannelWeight[i]->Delete();
        }
      }  
    }

  this->NodeParametersInputChannelWeight.resize(newSize);
  
  if (oldSize < newSize)
    {
      for (int i = oldSize ; i < newSize; i++) 
       {
          this->NodeParametersInputChannelWeight[i] = NULL;
       }
    }

  for (int i = 0 ; i < newSize; i++)
    {      
       if (!this->NodeParametersInputChannelWeight[i])
        {
           this->NodeParametersInputChannelWeight[i] =  vtkKWScaleWithEntry::New();
        }
        if (!this->NodeParametersInputChannelWeight[i]->IsCreated())
        {
           this->NodeParametersInputChannelWeight[i]->SetParent(this->NodeParametersInputChannelWeightFrame->GetFrame());
           this->NodeParametersInputChannelWeight[i]->PopupModeOn();
           this->NodeParametersInputChannelWeight[i]->Create();
           this->NodeParametersInputChannelWeight[i]->SetEntryWidth(4);
       this->NodeParametersInputChannelWeight[i]->GetLabel()-> SetWidth(EMSEG_WIDGETS_LABEL_WIDTH - 13);
           this->NodeParametersInputChannelWeight[i]->SetRange(0.0, 1.0);
           this->NodeParametersInputChannelWeight[i]->SetResolution(0.01);
           this->NodeParametersInputChannelWeight[i]->GetEntry()->SetCommandTriggerToAnyChange();
           this->NodeParametersInputChannelWeight[i]->SetBalloonHelpString("Weight of each channel in the segmentation decision.  Right-click or double-click to modify weights.  Weights should be in the range [0,1]; 0 indicates that the channel is ignored and 1 indicates the maximum channel weight.  The weights are not dependent on each other or any other weights.");
           this->Script( "pack %s -side top -anchor nw -padx 2 -pady 2", this->NodeParametersInputChannelWeight[i]->GetWidgetName());
    }
    this->NodeParametersInputChannelWeight[i]->SetLabelText(globalNode->GetNthTargetInputChannelName(i));
    this->NodeParametersInputChannelWeight[i]->SetEnabled(enabled);      
    }
}



//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::DefineClassOverviewWeightWindow(vtkIdType sel_tree_class_id, int enabled) 
{
  if (!this->NodeParametersClassOverviewWeightGlobalFrame)
    {
      return;
    }
  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  int oldSize = this->NodeParametersClassOverviewWeightEntry.size();
  assert(oldSize ==  int(this->NodeParametersClassOverviewWeightFrame.size()));
  assert(oldSize ==  int(this->NodeParametersClassOverviewWeightAuto.size()));

  // to avoid errror messages;
   vtkIdType parent = NULL;
   if ( mrmlManager->GetTreeRootNodeID() != sel_tree_class_id)
     {
        parent = mrmlManager->GetTreeNodeParentNodeID(sel_tree_class_id);
     }
  int newSize =0;
  if (parent)
    {
      newSize = mrmlManager->GetTreeNodeNumberOfChildren(parent);
    }

  if (oldSize > newSize)
    {
      for (int i = newSize ; i < oldSize; i++) 
      {
        if (this->NodeParametersClassOverviewWeightEntry[i])
        {
          this->NodeParametersClassOverviewWeightEntry[i]->Unpack(); 
          this->NodeParametersClassOverviewWeightEntry[i]->Delete();
          this->NodeParametersClassOverviewWeightAuto[i]->Unpack(); 
          this->NodeParametersClassOverviewWeightAuto[i]->Delete();
          this->NodeParametersClassOverviewWeightFrame[i]->Unpack(); 
          this->NodeParametersClassOverviewWeightFrame[i]->Delete();
        }
      }  
    }

  this->NodeParametersClassOverviewWeightFrame.resize(newSize);
  this->NodeParametersClassOverviewWeightEntry.resize(newSize);
  this->NodeParametersClassOverviewWeightAuto.resize(newSize);

  
  if (oldSize < newSize)
    {
      for (int i = oldSize ; i < newSize; i++) 
       {
          if (!this->NodeParametersClassOverviewWeightFrame[i])
          {
             this->NodeParametersClassOverviewWeightFrame[i] =  vtkKWFrame::New();
          }
          if (!this->NodeParametersClassOverviewWeightFrame[i]->IsCreated())
          {
            this->NodeParametersClassOverviewWeightFrame[i]->SetParent(this->NodeParametersClassOverviewWeightGlobalFrame->GetFrame());
            this->NodeParametersClassOverviewWeightFrame[i]->Create();
          }
        this->Script( "pack %s -side top -anchor nw -padx 2 -pady 2", this->NodeParametersClassOverviewWeightFrame[i]->GetWidgetName());

         if (!this->NodeParametersClassOverviewWeightEntry[i])
        {
           this->NodeParametersClassOverviewWeightEntry[i] =  vtkKWScaleWithEntry::New();
        }
        if (!this->NodeParametersClassOverviewWeightEntry[i]->IsCreated())
        {
           this->NodeParametersClassOverviewWeightEntry[i]->SetParent(this->NodeParametersClassOverviewWeightFrame[i]);
           this->NodeParametersClassOverviewWeightEntry[i]->PopupModeOn();
           this->NodeParametersClassOverviewWeightEntry[i]->Create();
           this->NodeParametersClassOverviewWeightEntry[i]->SetEntryWidth(4);
           this->NodeParametersClassOverviewWeightEntry[i]->GetLabel()-> SetWidth(EMSEG_WIDGETS_LABEL_WIDTH - 9);
           this->NodeParametersClassOverviewWeightEntry[i]->SetRange(0.0, 1.0);
           this->NodeParametersClassOverviewWeightEntry[i]->SetResolution(0.01);
           // this->NodeParametersClassOverviewWeightEntry[i]->GetEntry()->SetCommandTriggerToAnyChange();
           // this->NodeParametersClassOverviewWeightEntry[i]->SetBalloonHelpString("Weight of each channel in the segmentation decision.  Right-click or double-click to modify weights.  Weights should be in the range [0,1]; 0 indicates that the channel is ignored and 1 indicates the maximum channel weight.  The weights are not dependent on each other or any other weights.");  
           this->Script( "pack %s -side left ", this->NodeParametersClassOverviewWeightEntry[i]->GetWidgetName()); 
        }

        if (!this->NodeParametersClassOverviewWeightAuto[i])
        {
           this->NodeParametersClassOverviewWeightAuto[i] =  vtkKWCheckButton::New();
        }

    if (!this->NodeParametersClassOverviewWeightAuto[i]->IsCreated())
        {
           this->NodeParametersClassOverviewWeightAuto[i]->SetParent(this->NodeParametersClassOverviewWeightFrame[i]);
           this->NodeParametersClassOverviewWeightAuto[i]->Create();
       this->NodeParametersClassOverviewWeightAuto[i]->SetBalloonHelpString ("Automatically update class weight, so that class weight across siblings sums up to 1 !");
       this->NodeParametersClassOverviewWeightAuto[i]->SetSelectedState(0);
           this->Script( "pack %s -side left ", this->NodeParametersClassOverviewWeightAuto[i]->GetWidgetName());

    }

        //  this->NodeParametersClassOverviewWeightFrame[i] = NULL;
        //  this->NodeParametersClassOverviewWeightEntry[i] = NULL;
        //  this->NodeParametersClassOverviewWeightAuto[i] = NULL;


       }
    }

  // Disable any callbacks as it otherwise can cause loops
  for (int i = 0 ; i < newSize; i++)
    {
      this->NodeParametersClassOverviewWeightEntry[i]->DisableCommandsOn();
    }

  for (int i = 0 ; i < newSize; i++)
    {    
        char buffer[256];
        // Important: First change callback before values
        vtkIdType class_id = mrmlManager->GetTreeNodeChildNodeID(parent,i);
       
    this->NodeParametersClassOverviewWeightEntry[i]->SetLabelText(mrmlManager->GetTreeNodeName(class_id));
        this->NodeParametersClassOverviewWeightEntry[i]->SetValue(mrmlManager->GetTreeNodeClassProbability(class_id));
    this->NodeParametersClassOverviewWeightEntry[i]->GetEntry()->SetEnabled(enabled);

        sprintf(buffer, "ClassWeightChangedCallback %d", static_cast<int>(class_id));
        this->NodeParametersClassOverviewWeightEntry[i]->SetEndCommand(this, buffer);
        this->NodeParametersClassOverviewWeightEntry[i]->SetEntryCommand(this, buffer);

    this->NodeParametersClassOverviewWeightAuto[i]->SetEnabled(enabled);

        sprintf(buffer, "ClassWeightAutoChangedCallback %d", static_cast<int>(class_id));
        this->NodeParametersClassOverviewWeightAuto[i]->SetCommand(this,buffer);

    }

    // Reset AutoUpdate to previous setting

    for (int i = 0 ; i <  newSize; i++) 
    {
      this->NodeParametersClassOverviewWeightEntry[i]->DisableCommandsOff();
    }

}

//----------------------------------------------------------------------------
void vtkEMSegmentNodeParametersStep::NodeParametersInteractionMatrices2DCallback(
  vtkIdType sel_vol_id, int value)
{
  // The print weight has changed because of user interaction

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (mrmlManager)
    {
    mrmlManager->SetTreeNodeInteractionMatrices2DFlag(sel_vol_id, value);
    }
}
