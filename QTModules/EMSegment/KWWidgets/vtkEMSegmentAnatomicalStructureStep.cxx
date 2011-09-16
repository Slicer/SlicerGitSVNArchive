#include "vtkEMSegmentAnatomicalStructureStep.h"

#include "vtkEMSegmentGUI.h"
#include "vtkEMSegmentMRMLManager.h"

#include "vtkKWWizardWidget.h"
#include "vtkKWWizardWorkflow.h"
#include "vtkKWChangeColorButton.h"
#include "vtkKWEntry.h"
#include "vtkKWEntryWithLabel.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWMenuButton.h"
#include "vtkKWMenuButtonWithLabel.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWTree.h"
#include "vtkKWTreeWithScrollbars.h"
#include "vtkKWPushButtonSet.h"
#include "vtkKWMultiColumnList.h"
#include "vtkKWMultiColumnListWithScrollbars.h"

#include "vtkSlicerWidget.h"
#include "vtkKWApplication.h"
#include "vtkKWCheckButton.h"
#include "vtkMRMLFreeSurferProceduralColorNode.h"
#include "vtkCallbackCommand.h"
#include "vtkMRMLEMSGlobalParametersNode.h"
#include "vtkKWLabelWithLabel.h"
#include "vtkSlicerNodeSelectorWidget.h"
#include "vtkSlicerApplication.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkEMSegmentAnatomicalStructureStep);
vtkCxxRevisionMacro(vtkEMSegmentAnatomicalStructureStep, "$Revision: 1.2 $");

//----------------------------------------------------------------------------
vtkEMSegmentAnatomicalStructureStep::vtkEMSegmentAnatomicalStructureStep()
{
  this->SetName("3/9. Define Anatomical Tree");
  this->SetDescription("Define a hierarchy of structures.");

  this->ContextMenu  = NULL;

  this->AnatomicalStructureTree          = NULL;
  this->AnatomicalStructureFrame         = NULL;
  this->AnatomicalStructureTreeButtonSet = NULL;



  this->AnatomicalNodeAttributesFrame        = NULL;
  this->AnatomicalNodeAttributeNameEntry     = NULL;
  this->AnatomicalNodeIntensityLabelEntry    = NULL;
  this->AnatomicalNodeAttributeColorButton    = NULL;
  this->AnatomicalNodeAttributeColorLabel    = NULL;
  this->ShowOnlyNamedColorsCheckButton       = NULL;
  this->ColorMultiColumnList                 = NULL;
  this->NumberOfColumns = 3;
  this->MultiSelectMode = false;

  this->ColorSelectorWidget  = NULL;

  this->SelectedColormapChangedCallbackCommand = vtkCallbackCommand::New();
  this->SelectedColormapChangedCallbackCommand->SetClientData(reinterpret_cast<void *>(this));
  this->SelectedColormapChangedCallbackCommand->SetCallback( 
    vtkEMSegmentAnatomicalStructureStep::SelectedColormapChangedCallback );

  this->SelectedColorChangedCallbackCommand = vtkCallbackCommand::New();
  this->SelectedColorChangedCallbackCommand->SetClientData(reinterpret_cast<void *>(this));
  this->SelectedColorChangedCallbackCommand->SetCallback( 
    vtkEMSegmentAnatomicalStructureStep::SelectedColorChangedCallback );

  this->LockSelectedColorChangedMessage = false;
  this->LabelTopLevel = NULL;
  this->LabelApply = NULL;
  this->LabelTopLevelFrame = NULL;

}

//----------------------------------------------------------------------------
vtkEMSegmentAnatomicalStructureStep::~vtkEMSegmentAnatomicalStructureStep()
{
  this->RemoveSelectedColorChangedObserver(); 

  if (this->ContextMenu)
    {
    this->ContextMenu->Delete();
    this->ContextMenu = NULL;
    }

  if (this->AnatomicalNodeAttributesFrame)
    {
    this->AnatomicalNodeAttributesFrame->Delete();
    this->AnatomicalNodeAttributesFrame = NULL;
    }

  if (this->AnatomicalNodeAttributeNameEntry)
    {
    this->AnatomicalNodeAttributeNameEntry->Delete();
    this->AnatomicalNodeAttributeNameEntry = NULL;
    }

  if (this->AnatomicalNodeIntensityLabelEntry)
    {
    this->AnatomicalNodeIntensityLabelEntry->Delete();
    this->AnatomicalNodeIntensityLabelEntry = NULL;
    }

  if (this->AnatomicalNodeAttributeColorLabel)
    {
    this->AnatomicalNodeAttributeColorLabel->SetParent(NULL);
    this->AnatomicalNodeAttributeColorLabel->Delete();
    this->AnatomicalNodeAttributeColorLabel = NULL;
    }

  if (this->AnatomicalNodeAttributeColorButton)
    {
    this->AnatomicalNodeAttributeColorButton->Delete();
    this->AnatomicalNodeAttributeColorButton = NULL;
    }

  if (this->ShowOnlyNamedColorsCheckButton)
    {
    this->ShowOnlyNamedColorsCheckButton->Delete();
    this->ShowOnlyNamedColorsCheckButton = NULL;
    }

  if (this->ColorMultiColumnList)
    {
      this->ColorMultiColumnList->Delete();
      this->ColorMultiColumnList = NULL;
    }

  if (this->ColorSelectorWidget)
    {
    this->ColorSelectorWidget->Delete();
    this->ColorSelectorWidget = NULL;
    }

  if ( this->SelectedColormapChangedCallbackCommand )
    {
    this->SelectedColormapChangedCallbackCommand->Delete();
    this->SelectedColormapChangedCallbackCommand = NULL;
    }

  if ( this->SelectedColorChangedCallbackCommand )
    {
    this->SelectedColorChangedCallbackCommand->Delete();
    this->SelectedColorChangedCallbackCommand = NULL;
    }

  if (this->LabelTopLevel)
    {
      this->LabelTopLevel->Delete();
      this->LabelTopLevel = NULL;
    }

  if (  this->LabelApply )
    {
      this->LabelApply->Delete();
      this->LabelApply = NULL;
    }

  if (this->LabelTopLevelFrame )
    {
      this->LabelTopLevelFrame->Delete();
      this->LabelTopLevelFrame = NULL;
    }

  this->RemoveAnatomicalStructureTree();
}

//----------------------------------------------------------------------------
void  vtkEMSegmentAnatomicalStructureStep::RemoveAnatomicalStructureTree()
{
  if (this->AnatomicalStructureTree)
    {
    this->AnatomicalStructureTree->Delete();
    this->AnatomicalStructureTree = NULL;
    }

  if (this->AnatomicalStructureFrame)
    {
    this->AnatomicalStructureFrame->Delete();
    this->AnatomicalStructureFrame = NULL;
    }

  if (this->AnatomicalStructureTreeButtonSet)
    {
    this->AnatomicalStructureTreeButtonSet->Delete();
    this->AnatomicalStructureTreeButtonSet = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentAnatomicalStructureStep::ShowAnatomicalStructureTree(vtkKWFrame * parent)
{
  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();

  // Create the frame
  if (!parent)
    {
      vtkKWWizardWidget *wizard_widget = this->GetGUI()->GetWizardWidget();
      parent = wizard_widget->GetClientArea();
    } 

  if (!this->AnatomicalStructureFrame)
    {
    this->AnatomicalStructureFrame = vtkKWFrameWithLabel::New();
    }
  if (!this->AnatomicalStructureFrame->IsCreated())
    {
      this->AnatomicalStructureFrame->SetParent(parent);
    this->AnatomicalStructureFrame->Create();
    this->AnatomicalStructureFrame->SetLabelText("Anatomical Tree");
    }

  this->Script("pack %s -side top -expand n -fill both -padx 0 -pady 2", 
               this->AnatomicalStructureFrame->GetWidgetName());

  // Create the tree

  if (!this->AnatomicalStructureTree)
    {
    this->AnatomicalStructureTree = vtkKWTreeWithScrollbars::New();
    }
  vtkKWTree *tree = this->AnatomicalStructureTree->GetWidget();
  if (!this->AnatomicalStructureTree->IsCreated())
    {
    this->AnatomicalStructureTree->SetParent(
      this->AnatomicalStructureFrame->GetFrame());
    this->AnatomicalStructureTree->Create();
    this->AnatomicalStructureTree->SetPadX(0);
    this->AnatomicalStructureTree->SetPadY(0);
    this->AnatomicalStructureTree->SetBorderWidth(2);
    this->AnatomicalStructureTree->SetReliefToSunken();
    this->AnatomicalStructureTree->SetHorizontalScrollbarVisibility(0);

    tree->SetHighlightThickness(0);
    tree->RedrawOnIdleOn();
    tree->SetHeight(7);
    
    this->Script("pack %s -side left -expand y -fill both -padx 0 -pady 0", 
                 this->AnatomicalStructureTree->GetWidgetName());
    }

  // Create the tree buttons (expand/collapse, for convenience)

  if (!this->AnatomicalStructureTreeButtonSet)
    {
    this->AnatomicalStructureTreeButtonSet = vtkKWPushButtonSet::New();
    }

  if (!this->AnatomicalStructureTreeButtonSet->IsCreated())
    {
    this->AnatomicalStructureTreeButtonSet->SetParent(
      this->AnatomicalStructureFrame->GetFrame());
    this->AnatomicalStructureTreeButtonSet->PackHorizontallyOff();
    this->AnatomicalStructureTreeButtonSet->Create();
    this->AnatomicalStructureTreeButtonSet->SetWidgetsPadX(0);
    this->AnatomicalStructureTreeButtonSet->SetWidgetsPadY(2);

    vtkKWPushButton *button;
    int button_id = 0;

    button = this->AnatomicalStructureTreeButtonSet->AddWidget(++button_id);
    if (button)
      {
      button->SetCommand(this, "OpenTreeCallback");
      button->SetBalloonHelpString("Open all nodes");
      button->SetImageToPredefinedIcon(vtkKWIcon::IconTreeOpen);
      }
    
    button = this->AnatomicalStructureTreeButtonSet->AddWidget(++button_id);
    if (button)
      {
      button->SetCommand(this, "CloseTreeCallback");
      button->SetBalloonHelpString("Close all nodes");
      button->SetImageToPredefinedIcon(vtkKWIcon::IconTreeClose);
      }
    
    this->Script("pack %s -anchor nw -fill none -expand n -padx 2", 
                 this->AnatomicalStructureTreeButtonSet->GetWidgetName());
    }
  
  tree->SetSelectionChangedCommand(NULL, NULL);
  tree->SetRightClickOnNodeCommand(NULL, NULL);
  tree->SetNodeParentChangedCommand(NULL, NULL);
  tree->EnableReparentingOff();

  this->SetAnatomicalTreeNodesSelectableOn();

  // Populate the tree

  if (mrmlManager)
    {
    vtkIdType root_id = mrmlManager->GetTreeRootNodeID();
    if (root_id)
      {
      this->PopulateAnatomicalStructureTree(NULL, root_id);
      }
    }
}
//----------------------------------------------------------------------------
void vtkEMSegmentAnatomicalStructureStep::ShowUserInterface()
{
  //   cout << "vtkEMSegmentAnatomicalStructureStep::ShowUserInterface() start" << endl;
  this->Superclass::ShowUserInterface();

  vtkKWWizardWidget *wizard_widget = this->GetGUI()->GetWizardWidget();
  wizard_widget->GetCancelButton()->SetEnabled(0);

  this->ShowAnatomicalStructureTree();

  // Override the tree callbacks for that specific step

  vtkKWTree *tree = this->AnatomicalStructureTree->GetWidget();
  tree->SetSelectionChangedCommand(
    this, "SelectedAnatomicalNodeChangedCallback");
  tree->SetRightClickOnNodeCommand(this, "PopupNodeContextMenuCallback");
  tree->SetNodeParentChangedCommand(this, "NodeParentChangedCallback");
  tree->EnableReparentingOn();


  //create Colormap frame 
  // Create NodeAttribute frame

  if (!this->AnatomicalNodeAttributesFrame)
    {
    this->AnatomicalNodeAttributesFrame = vtkKWFrameWithLabel::New();
    }
  if (!this->AnatomicalNodeAttributesFrame->IsCreated())
    {
    this->AnatomicalNodeAttributesFrame->SetParent(
      wizard_widget->GetClientArea());
    this->AnatomicalNodeAttributesFrame->Create();
    this->AnatomicalNodeAttributesFrame->SetLabelText("Node Attributes");
    }

  this->Script("pack %s -side top -expand n -fill both -padx 0 -pady 2", 
               this->AnatomicalNodeAttributesFrame->GetWidgetName());

  // Create the node name

  if (!this->AnatomicalNodeAttributeNameEntry)
    {
    this->AnatomicalNodeAttributeNameEntry = vtkKWEntryWithLabel::New();
    }
  if (!this->AnatomicalNodeAttributeNameEntry->IsCreated())
    {
    this->AnatomicalNodeAttributeNameEntry->SetParent(
      this->AnatomicalNodeAttributesFrame->GetFrame());
    this->AnatomicalNodeAttributeNameEntry->Create();
    this->AnatomicalNodeAttributeNameEntry->SetLabelText("Name: ");
    this->AnatomicalNodeAttributeNameEntry->SetLabelWidth(
      EMSEG_WIDGETS_LABEL_WIDTH - 12);

    vtkKWEntry *entry = this->AnatomicalNodeAttributeNameEntry->GetWidget();
    entry->SetWidth(16);
    entry->SetCommandTriggerToAnyChange();
    }
  this->Script("grid %s -column 0 -columnspan 2 -row 0 -sticky nw -padx 2 -pady 2", 
               this->AnatomicalNodeAttributeNameEntry->GetWidgetName());

  // Create the node label

  if (!this->AnatomicalNodeIntensityLabelEntry)
    {
    this->AnatomicalNodeIntensityLabelEntry = vtkKWEntryWithLabel::New();
    }
  if (!this->AnatomicalNodeIntensityLabelEntry->IsCreated())
    {
    this->AnatomicalNodeIntensityLabelEntry->SetParent(
      this->AnatomicalNodeAttributesFrame->GetFrame());
    this->AnatomicalNodeIntensityLabelEntry->Create();
    this->AnatomicalNodeIntensityLabelEntry->SetLabelText("Label: ");
    this->AnatomicalNodeIntensityLabelEntry->SetLabelWidth(
      EMSEG_WIDGETS_LABEL_WIDTH - 12);
    this->AnatomicalNodeIntensityLabelEntry->GetWidget()->SetWidth(6);
    this->AnatomicalNodeIntensityLabelEntry->GetWidget()
      ->SetRestrictValueToInteger();
    this->AnatomicalNodeIntensityLabelEntry->GetWidget()
      ->SetCommandTriggerToAnyChange();
    }

  // Create the node color label

  if (!this->AnatomicalNodeAttributeColorLabel)
    {
    this->AnatomicalNodeAttributeColorLabel = vtkKWLabelWithLabel::New();  
    }
  if (!this->AnatomicalNodeAttributeColorLabel->IsCreated())
    {
    this->AnatomicalNodeAttributeColorLabel->SetParent(
      this->AnatomicalNodeAttributesFrame->GetFrame());
    this->AnatomicalNodeAttributeColorLabel->Create();
    this->AnatomicalNodeAttributeColorLabel->SetWidth(2);
    this->AnatomicalNodeAttributeColorLabel->SetHeight(20);
    this->AnatomicalNodeAttributeColorLabel->SetLabelWidth( 
      EMSEG_WIDGETS_LABEL_WIDTH - 12); 
    this->AnatomicalNodeAttributeColorLabel->SetLabelText("Color: ");
    this->AnatomicalNodeAttributeColorLabel->SetLabelPositionToRight();
    }

   if ( !this->LabelTopLevel )
     {
       this->LabelTopLevel = vtkKWTopLevel::New ( );
     }
   if(!this->LabelTopLevel->IsCreated())
     {
      vtkSlicerApplication *app = vtkSlicerApplication::GetInstance();
      this->LabelTopLevel->SetApplication ( app );
      this->LabelTopLevel->ModalOn();
      this->LabelTopLevel->Create ( );
      this->LabelTopLevel->SetMasterWindow ( app->GetApplicationGUI()->GetMainSlicerWindow() );
      this->LabelTopLevel->HideDecorationOn ( );
      this->LabelTopLevel->Withdraw ( );
      this->LabelTopLevel->SetBorderWidth ( 2 );
      this->LabelTopLevel->SetReliefToGroove ( );
     }
 
    if (!this->LabelTopLevelFrame) {
       this->LabelTopLevelFrame = vtkKWFrame::New ( );
    } 
    if (!this->LabelTopLevelFrame->IsCreated())
      {
      this->LabelTopLevelFrame->SetParent ( this->LabelTopLevel );
      this->LabelTopLevelFrame->Create ( );
      }
    this->Script ( "pack %s -side left -anchor w -padx 2 -pady 2 -fill x -fill y -expand n", this->LabelTopLevelFrame->GetWidgetName ( ) );

  // Create the node color frame

  if (!this->AnatomicalNodeAttributeColorButton)
    {
    this->AnatomicalNodeAttributeColorButton = vtkKWPushButton::New ();  
    }
  if (!this->AnatomicalNodeAttributeColorButton->IsCreated())
    {
      this->AnatomicalNodeAttributeColorButton->SetParent(
      this->AnatomicalNodeAttributesFrame->GetFrame());
      this->AnatomicalNodeAttributeColorButton->Create();
      this->AnatomicalNodeAttributeColorButton->SetWidth(2);
      this->AnatomicalNodeAttributeColorButton->SetHeight(1);
      this->AnatomicalNodeAttributeColorButton->Raise();
      this->AnatomicalNodeAttributeColorButton->SetCommand (this, "PopUpLabelColorSelect");
    }

  // Create a MultiColumnList for displaying the color lookup table

  if (!this->ColorMultiColumnList)
    {
    this->ColorMultiColumnList = vtkKWMultiColumnListWithScrollbars::New ( );
    }
  if(!this->ColorMultiColumnList->IsCreated())
    {
     this->ColorMultiColumnList->SetParent (this->LabelTopLevelFrame);
    this->ColorMultiColumnList->Create ( );
    this->ColorMultiColumnList->SetHeight(4);
    this->ColorMultiColumnList->GetWidget()->SetSelectionTypeToRow();
    this->ColorMultiColumnList->GetWidget()->MovableRowsOff();
    this->ColorMultiColumnList->GetWidget()->MovableColumnsOff();
    // set up the columns of data for each table entry
    // refer to the header file for the order
    this->ColorMultiColumnList->GetWidget()->AddColumn("Entry");
    this->ColorMultiColumnList->GetWidget()->AddColumn("Name");
    this->ColorMultiColumnList->GetWidget()->AddColumn("Color");
    
    if(this->MultiSelectMode)
      {
      this->ColorMultiColumnList->GetWidget()->SetSelectionModeToMultiple();
      }
    else
      {
      // we should never get here. ColorMultiColumnList is set in Single Selection mode.
      this->ColorMultiColumnList->GetWidget()->SetSelectionModeToSingle();
      }
    

    // now set attribs that are equal across the columns
    int col;
    for (col = 0; col < this->NumberOfColumns; col++)
      {
      this->ColorMultiColumnList->GetWidget()->SetColumnWidth(col, 6);
      this->ColorMultiColumnList->GetWidget()->SetColumnAlignmentToLeft(col);
      this->ColorMultiColumnList->GetWidget()->ColumnEditableOff(col);
      }
    // set the name and colour column widths to be higher
    this->ColorMultiColumnList->GetWidget()->SetColumnWidth(NameColumn, 20);
    this->ColorMultiColumnList->GetWidget()->SetColumnWidth(ColourColumn, 20);
    
    this->ColorMultiColumnList->GetWidget()->SetCellUpdatedCommand(this, "UpdateElement");
    }

  // create ShowOnlyNamedColors button

  if (!this->ShowOnlyNamedColorsCheckButton)
    {
    this->ShowOnlyNamedColorsCheckButton = vtkKWCheckButton::New();
    }
  if (!this->ShowOnlyNamedColorsCheckButton->IsCreated())
    {
      this->ShowOnlyNamedColorsCheckButton->SetParent(this->LabelTopLevelFrame);
      //      this->AnatomicalNodeAttributesFrame->GetFrame() );
    this->ShowOnlyNamedColorsCheckButton->Create();
    this->ShowOnlyNamedColorsCheckButton->SelectedStateOff();
    this->ShowOnlyNamedColorsCheckButton->SetText("Show Only Named Colors");
    }

  // Create a ColorSelectorWidget for the user to selecting a colormap
  if (!this->ColorSelectorWidget)
    {
    this->ColorSelectorWidget = vtkSlicerNodeSelectorWidget::New();
    }
  if (!this->ColorSelectorWidget->IsCreated())
    {
      // this->ColorSelectorWidget->SetParent( this->AnatomicalNodeAttributesFrame->GetFrame());
      this->ColorSelectorWidget->SetParent(this->LabelTopLevelFrame );
      this->ColorSelectorWidget->Create();
      this->ColorSelectorWidget->SetNodeClass("vtkMRMLColorNode", NULL, NULL, NULL);
      this->ColorSelectorWidget->AddExcludedChildClass("vtkMRMLDiffusionTensorDisplayPropertiesNode");
    // don't allow new nodes to be created until can edit them
    // this->ColorSelectorWidget->NewNodeEnabledOn();
    this->ColorSelectorWidget->ShowHiddenOn();
    this->ColorSelectorWidget->SetMRMLScene(
      this->GetGUI()->GetMRMLManager()->GetMRMLScene());
    this->ColorSelectorWidget->SetBorderWidth(2);
    this->ColorSelectorWidget->SetPadX(2);
    this->ColorSelectorWidget->SetPadY(2);
    //this->ColorSelectorWidget->GetWidget()->IndicatorVisibilityOff();
    this->ColorSelectorWidget->GetWidget()->SetWidth(20);
    this->ColorSelectorWidget->SetLabelText( "Select colormap: ");
    this->ColorSelectorWidget
        ->SetBalloonHelpString("Select a colormap from the current mrml scene.");
    this->ColorSelectorWidget->SetLabelWidth(
      EMSEG_WIDGETS_LABEL_WIDTH - 12);
    }


  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  vtkMRMLScene            *mrmlScene   = this->ColorSelectorWidget->GetMRMLScene();
  if (mrmlManager->GetColorNodeID() && mrmlScene )
    {
    this->ColorSelectorWidget->SetSelected( mrmlScene->GetNodeByID( mrmlManager->GetColorNodeID() ) );
    }

  this->ColorSelectorWidget->SetEnabled(1);
  this->ShowOnlyNamedColorsCheckButton->SetEnabled(1);
  this->ColorMultiColumnList->SetEnabled(1);
  this->Script ("pack %s %s %s -side top -padx 0 -pady 2", this->ColorSelectorWidget->GetWidgetName(), this->ColorMultiColumnList->GetWidgetName(), this->ShowOnlyNamedColorsCheckButton->GetWidgetName());

  this->AddSelectedColorChangedObserver();
  vtkEMSegmentAnatomicalStructureStep::SelectedColormapChangedCallback(this, 0, this, NULL);  

    if (!this->LabelApply)
       { 
      this->LabelApply = vtkKWPushButton::New ();
       }
    if (!this->LabelApply->IsCreated())
       {
         this->LabelApply->SetParent (this->LabelTopLevelFrame);
         this->LabelApply->Create ( );
         this->LabelApply->SetText ("Apply");
         this->LabelApply->SetCommand (this, "LabelWindowCollapseCallback");
     }
     this->Script ( "pack %s -side top -padx 4 -anchor c", LabelApply->GetWidgetName());


  // Update the UI with the proper value, if there is a selection
  this->SelectedAnatomicalNodeChangedCallback();
}

//----------------------------------------------------------------------------
void vtkEMSegmentAnatomicalStructureStep::SelectedAnatomicalNodeChangedCallback()
{
  // Update the UI with the proper value, if there is a selection

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();


  if (!mrmlManager)
    {
    return;
    }
  vtkKWTree *tree = this->AnatomicalStructureTree->GetWidget();
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

  // Update the node name

  if (this->AnatomicalNodeAttributeNameEntry)
    {
    vtkKWEntry *entry = this->AnatomicalNodeAttributeNameEntry->GetWidget();
    if (has_valid_selection)
      {
      vtksys_stl::string name(tree->GetNodeText(sel_node.c_str()));
      this->AnatomicalNodeAttributeNameEntry->SetEnabled(enabled);
      sprintf(buffer, "SelectedNodeNameChangedCallback %d", 
              static_cast<int>(sel_vol_id));
      entry->SetCommand(this, buffer);
      entry->SetValue(name.c_str());
      }
    else
      {
      this->AnatomicalNodeAttributeNameEntry->SetEnabled(0);
      entry->SetCommand(NULL, NULL);
      entry->SetValue(NULL);
      }
    }
 
  // Update the node intensity label

  if (this->AnatomicalNodeIntensityLabelEntry)
    {
    vtkKWEntry *entry = this->AnatomicalNodeIntensityLabelEntry->GetWidget();
    if (has_valid_selection && sel_is_leaf_node)
      {
      int intLabel = mrmlManager->GetTreeNodeIntensityLabel(sel_vol_id);
      this->AnatomicalNodeIntensityLabelEntry->SetEnabled(enabled);
      sprintf(buffer, 
              "SelectedNodeIntensityLabelChangedCallback %d", 
              static_cast<int>(sel_vol_id));
      entry->SetCommand(this, buffer);
      entry->SetValueAsInt(intLabel);
      this->Script("grid %s -column 0 -row 1 -sticky nw -padx 2 -pady 2", 
                   this->AnatomicalNodeIntensityLabelEntry->GetWidgetName());
      }
    else
      {
      this->AnatomicalNodeIntensityLabelEntry->SetEnabled(0);
      entry->SetCommand(NULL, NULL);
      entry->SetValue(NULL);
      this->Script("grid forget %s", 
                   this->AnatomicalNodeIntensityLabelEntry->GetWidgetName());
      }
    }

  // Update the node color label
 
  if (this->AnatomicalNodeAttributeColorLabel)
    {
    if (has_valid_selection && sel_is_leaf_node)
      {
      this->AnatomicalNodeAttributeColorLabel->SetEnabled(enabled);
      this->Script("grid %s -column 1 -row 1 -sticky ne -padx 2 -pady 2",  
                   this->AnatomicalNodeAttributeColorLabel->GetWidgetName());
     }
    else 
      {
      this->AnatomicalNodeAttributeColorLabel->SetEnabled(0);
      this->Script("grid forget %s", 
                   this->AnatomicalNodeAttributeColorLabel->GetWidgetName());
      }
    }

  // Update the node color frame
 
  if (this->AnatomicalNodeAttributeColorButton)
    {
    if (has_valid_selection && sel_is_leaf_node)
      {
      this->AnatomicalNodeAttributeColorButton->SetEnabled(enabled);
      this->Script("grid %s -column 2 -row 1 -sticky ne -padx 2 -pady 2",  
                   this->AnatomicalNodeAttributeColorButton->GetWidgetName());
      this->UpdateAnatomicalNodeAttributeColorButton();

      //this->Script("grid columnconfigure .t 0 -weight 1",  
      //             this->AnatomicalNodeAttributeColorButton->GetWidgetName());
     }
    else 
      {
      this->AnatomicalNodeAttributeColorButton->SetEnabled(0);
      this->Script("grid forget %s", 
                   this->AnatomicalNodeAttributeColorButton->GetWidgetName());
      }
    }

  // update the color selection 

  
  if(this->ColorMultiColumnList && this->ShowOnlyNamedColorsCheckButton 
     && this->ColorSelectorWidget)
    {
    if (has_valid_selection  && sel_is_leaf_node)
      {
    // this->Script ("grid %s -column 0 -columnspan 3 -row 3 -sticky nw -padx 2 -pady 2",  this->ColorMultiColumnList->GetWidgetName(),this->AnatomicalNodeAttributesFrame->GetFrame()->GetWidgetName());
    //    this->Script("grid %s -column 0 -columnspan 2 -row 4 -sticky nw -padx 2 -pady 2", this->ShowOnlyNamedColorsCheckButton->GetWidgetName(),this->AnatomicalNodeAttributesFrame->GetFrame()->GetWidgetName());
    // this->Script ("grid %s  -column 0 -columnspan 4 -row 2 -sticky nw -padx 2 -pady 2", this->ColorSelectorWidget->GetWidgetName());
    //this->ColorMultiColumnList->SetEnabled(enabled);      
      } 
    else
      {
    //this->ColorSelectorWidget->SetEnabled(0);
    //this->Script ("grid forget %s", this->ColorSelectorWidget->GetWidgetName());
    // this->ColorMultiColumnList->SetEnabled(0);
    //this->Script ("grid forget %s", this->ColorMultiColumnList->GetWidgetName());
    //this->ShowOnlyNamedColorsCheckButton->SetEnabled(0);
        //this->Script("grid forget %s", this->ShowOnlyNamedColorsCheckButton->GetWidgetName());
      }
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentAnatomicalStructureStep::SelectedColormapChangedCallback(vtkObject *vtkNotUsed(caller),
                                      unsigned long vtkNotUsed(eid), void *clientData, void *vtkNotUsed(callData))
{
  vtkEMSegmentAnatomicalStructureStep *self = 
        reinterpret_cast<vtkEMSegmentAnatomicalStructureStep *>(clientData);

  if (!self)
    {
    return;
    }

  vtkMRMLColorNode *colorNode = vtkMRMLColorNode::SafeDownCast(
     self->ColorSelectorWidget->GetSelected());

  if (colorNode == NULL)
    {
    return;
    }

  int numColours = 0;
  if (vtkMRMLColorTableNode::SafeDownCast(colorNode) != NULL)
    {
    numColours = vtkMRMLColorTableNode::SafeDownCast(colorNode)->GetNumberOfColors();
    }
  else if (vtkMRMLFreeSurferProceduralColorNode::SafeDownCast(colorNode) != NULL &&
           vtkMRMLFreeSurferProceduralColorNode::SafeDownCast(colorNode)->GetLookupTable() != NULL)
    {
    numColours = vtkMRMLFreeSurferProceduralColorNode::SafeDownCast(colorNode)
                 ->GetLookupTable()->GetNumberOfColors();
    }
  
  bool showOnlyNamedColors;
  if (self->ShowOnlyNamedColorsCheckButton->GetSelectedState())
    {
    showOnlyNamedColors = true;
    }
  else
    {
    showOnlyNamedColors = false;
    }
    
  bool deleteFlag = true;
  if (numColours > self->ColorMultiColumnList->GetWidget()->GetNumberOfRows())
    {
    // add rows to the table
    int numToAdd = numColours - self->ColorMultiColumnList->GetWidget()->GetNumberOfRows();
    self->ColorMultiColumnList->GetWidget()->AddRows(numToAdd);
    }
  if (numColours < self->ColorMultiColumnList->GetWidget()->GetNumberOfRows())
    {
    // delete some rows
    for (int r = self->ColorMultiColumnList->GetWidget()->GetNumberOfRows(); r >= numColours; r--)
      {
      self->ColorMultiColumnList->GetWidget()->DeleteRow(r);
      }
    }
  if (showOnlyNamedColors || numColours != self->ColorMultiColumnList->GetWidget()->GetNumberOfRows())
    {
    // clear out the multi column list box and fill it in with the new list
    // - if only showing named colours, there might not be numColours rows
    vtkDebugWithObjectMacro(self, "Clearing out the colours MCLB, numColours = " << numColours);
    self->ColorMultiColumnList->GetWidget()->DeleteAllRows();
    }
  else
    {
    deleteFlag = false;
    }
  
  // a row for each colour
  double *colour = NULL;
  const char *name;
  // keep track of where to add the current colour into the table
  int thisRow = 0;
  for (int row = 0; row < numColours; row++)
    {
    // get the colour
    if (colorNode->GetLookupTable() != NULL)
      {
      colour = colorNode->GetLookupTable()->GetTableValue(row);
      }
    if (colour == NULL)
      {
      vtkErrorWithObjectMacro (self, "SetGUIFromNode: at " << row << "th colour, got a null pointer" << endl);
      }
    // get the colour label
    name = colorNode->GetColorName(row);
    if (!showOnlyNamedColors ||
        (showOnlyNamedColors && strcmp(name, colorNode->GetNoName()) != 0))
      {
      // update this colour
      if (deleteFlag)
        {
        self->ColorMultiColumnList->GetWidget()->AddRow();
        }
      // now set the table
      // which entry is it in the colour table?
      if (thisRow == 0 || row == 0 ||
          self->ColorMultiColumnList->GetWidget()->GetCellTextAsInt(thisRow, EntryColumn) != row)
        {
        vtkDebugWithObjectMacro(self, "Setting entry column #" << thisRow << " to " << row);
        self->ColorMultiColumnList->GetWidget()->SetCellTextAsInt(thisRow, EntryColumn, row);
        }
      
      // what's it's name?
      if (strcmp(self->ColorMultiColumnList->GetWidget()->GetCellText(thisRow,NameColumn), name) != 0)
        {
        self->ColorMultiColumnList->GetWidget()->SetCellText(thisRow,NameColumn,name);
        }
      
      // what's the colour?
      if (colour != NULL)
        {
          self->ColorMultiColumnList->GetWidget()->SetCellBackgroundColor(thisRow, ColourColumn, colour);
        }
      else
        {
        self->ColorMultiColumnList->GetWidget()->ClearCellBackgroundColor(thisRow, ColourColumn);
        }
      thisRow++;
      } 
    vtkDebugWithObjectMacro(self, "Done rebuilding table, row = " << row << ", thisRow = " << thisRow);
    }

  vtkEMSegmentMRMLManager *mrmlManager = self->GetGUI()->GetMRMLManager();
  vtkKWTree *tree = self->AnatomicalStructureTree->GetWidget();

  if (!mrmlManager || !tree)
    {
    return;
    }

  mrmlManager->SetColorNodeID(colorNode->GetID());
  if (tree->HasSelection())
    {
    vtksys_stl::string sel_node = tree->GetSelection();
    vtkIdType sel_vol_id = tree->GetNodeUserDataAsInt(sel_node.c_str());
    int labelIndex = mrmlManager->GetTreeNodeIntensityLabel(sel_vol_id);

    self->SelectRowByIntensityLabelEntryValue(labelIndex);
    }
  
  self->UpdateAnatomicalNodeAttributeColorButton();
 
}
 
//----------------------------------------------------------------------------
void vtkEMSegmentAnatomicalStructureStep::SelectedColorChangedCallback(vtkObject *caller,
                                       unsigned long vtkNotUsed(eid), void *clientData, void *vtkNotUsed(callData))
{
  vtkEMSegmentAnatomicalStructureStep *self = 
      reinterpret_cast<vtkEMSegmentAnatomicalStructureStep *>(clientData);

  if (!self)
    {
    return;
    }


  if (!self->ColorMultiColumnList || !self->AnatomicalNodeIntensityLabelEntry)
    {
    return;
    }

  //Here we need different behaviors for MultiSelection and Single Selection
  if(self->MultiSelectMode)
  {
    // we should never get here. ColorMultiColumnList is set in Single Selection mode.
    return;
  }

  vtkEMSegmentMRMLManager *mrmlManager = self->GetGUI()->GetMRMLManager();
  vtkKWTree *tree = self->AnatomicalStructureTree->GetWidget();
  if (!mrmlManager || !tree)
    {
    return;
    }

  if (!tree->HasSelection())
    {
    return;
    }

  vtksys_stl::string  sel_node = tree->GetSelection();
  vtkIdType sel_vol_id = tree->GetNodeUserDataAsInt(sel_node.c_str());

  if (!mrmlManager->GetTreeNodeIsLeaf(sel_vol_id))
    {
    return;
    }
  
  if(self->LockSelectedColorChangedMessage)
    {
    return;
    }
  //enter critical section.
  self->LockSelectedColorChangedMessage = true;

  int numRows = self->ColorMultiColumnList->GetWidget()->GetNumberOfSelectedRows();
  if (numRows == 0 && self->ColorMultiColumnList->GetWidget()->GetNumberOfRows() > 1)
    {
    // no selection was made, use the value stored in mrmlManager 
    int row = mrmlManager->GetTreeNodeIntensityLabel(sel_vol_id);
    self->SelectRowByIntensityLabelEntryValue(row);
    numRows = self->ColorMultiColumnList->GetWidget()->GetNumberOfSelectedRows();
    vtkDebugWithObjectMacro(self, 
      "No rows were selected, forcing selection of row " << row << ", numRows = " << numRows);
    } 

  int rowInLabelEntry = self->AnatomicalNodeIntensityLabelEntry->GetWidget()->GetValueAsInt();
  int rowInColumnList = self->GetIntensityLabelEntryValueOfFirstSelectedRow();

  if (rowInLabelEntry == rowInColumnList)
    {
     //leave critical section.
    self->LockSelectedColorChangedMessage = false;
    return;
    }

  // event triggered by IntensityLabelEntry, need to change the selection of ColorMultiColumnList
  // note: the intensity label maybe an invalid one (negative), in this case, we do not update
  if (self->AnatomicalNodeIntensityLabelEntry->GetWidget() == vtkKWEntry::SafeDownCast(caller))
    {    
    if (rowInLabelEntry < 0)
      {
      vtkWarningWithObjectMacro(self, 
        "Error in label value: " << rowInLabelEntry << " found, intensity label should be non-negative.");
      
      //leave critical section.
      self->LockSelectedColorChangedMessage = false;
      return;
      }
    mrmlManager->SetTreeNodeIntensityLabel(sel_vol_id, rowInLabelEntry);
    self->SelectRowByIntensityLabelEntryValue(rowInLabelEntry);
    self->UpdateAnatomicalNodeAttributeColorButton();
    }


  // event triggered by ColorMultiColumnList, need to change the value in IntensityLabelEntry
  // note: the selected row number maybe an invalid one (negative), this could happen when the
  // list is cleared and we use the index stored in mrmlManager to select an non-existent row. 
  // in this case, we do not update
  if (self->ColorMultiColumnList->GetWidget() == vtkKWMultiColumnList::SafeDownCast(caller))
    {
    if (rowInColumnList < 0)
      {
      vtkWarningWithObjectMacro(self, 
        "Error in selection: " << rowInLabelEntry << " found, selected row should be non-negative.");

      //leave critical section.
      self->LockSelectedColorChangedMessage = false;
      return;
      }

    mrmlManager->SetTreeNodeIntensityLabel(sel_vol_id, rowInColumnList);

    if (self->AnatomicalNodeIntensityLabelEntry)
      { 
      self->AnatomicalNodeIntensityLabelEntry->GetWidget()->SetValueAsInt(rowInColumnList);
      }

    self->UpdateAnatomicalNodeAttributeColorButton();
    }
 
  //leave critical section.
  self->LockSelectedColorChangedMessage = false;
  return;
}

//----------------------------------------------------------------------------
void vtkEMSegmentAnatomicalStructureStep::SetAnatomicalTreeNodesSelectableOn()
{
  if (!this->AnatomicalStructureTree ||
      !this->AnatomicalStructureTree->IsCreated())
    {
    return;
    }
  
  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (!mrmlManager)
    {
    return;
    }
  vtkKWTree *tree = this->AnatomicalStructureTree->GetWidget();
  vtkIdType vol_id = mrmlManager->GetTreeRootNodeID();
  const char *root_node = tree->FindNodeWithUserDataAsInt(NULL, vol_id);
  if (root_node && *root_node)
    {
    vtksys_stl::string rootnode = root_node;
    this->SetAnatomicalTreeLeafNodeSelectableState(rootnode.c_str(), 1);
    this->SetAnatomicalTreeParentNodeSelectableState(rootnode.c_str(), 1);
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentAnatomicalStructureStep::SetAnatomicalTreeLeafNodeSelectableState(
  const char* parent,  int state)
{
  if (!this->AnatomicalStructureTree ||
     !this->AnatomicalStructureTree->IsCreated())
    {
    return;
    }

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (!mrmlManager)
    {
    return;
    }
  vtksys_stl::string parent_node = parent;
  vtkKWTree *tree = this->AnatomicalStructureTree->GetWidget();
  vtkIdType vol_id = tree->GetNodeUserDataAsInt(parent_node.c_str());
  vtkIdType child_id;

  int nb_children = mrmlManager->GetTreeNodeNumberOfChildren(vol_id);
  for (int i = 0; i < nb_children; i++)
    {
    child_id = mrmlManager->GetTreeNodeChildNodeID(vol_id, i);
    const char* node = 
      tree->FindNodeWithUserDataAsInt(parent_node.c_str(), child_id);
    if (node)
      {
      if (mrmlManager->GetTreeNodeIsLeaf(child_id))
        {
        tree->SetNodeSelectableFlag(node, state);
        }
      else
        {
        this->SetAnatomicalTreeLeafNodeSelectableState(node, state);
        }
      }
    }

  if (nb_children == 0)
    {
    tree->SetNodeSelectableFlag(parent_node.c_str(), state);
    }

  if (!state && tree->HasSelection())
    {
    const char* sel_node = tree->GetSelection();
    if (sel_node && !tree->GetNodeSelectableFlag(sel_node))
      {
      tree->ClearSelection();
      }
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentAnatomicalStructureStep::SetAnatomicalTreeParentNodeSelectableState(
  const char* parent, int state)
{
  if (!this->AnatomicalStructureTree ||
    !this->AnatomicalStructureTree->IsCreated())
    {
    return;
    }

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (!mrmlManager)
    {
    return;
    }
  vtksys_stl::string parent_node = parent;
  vtkKWTree *tree = this->AnatomicalStructureTree->GetWidget();
  vtkIdType vol_id = tree->GetNodeUserDataAsInt(parent_node.c_str());
  vtkIdType child_id;

  int nb_children = mrmlManager->GetTreeNodeNumberOfChildren(vol_id);
  for (int i = 0; i < nb_children; i++)
    {
    child_id = mrmlManager->GetTreeNodeChildNodeID(vol_id, i);
    const char* node = 
      tree->FindNodeWithUserDataAsInt(parent_node.c_str(), child_id);
    if (node)
      {
      if (!mrmlManager->GetTreeNodeIsLeaf(child_id))
        {
        this->SetAnatomicalTreeParentNodeSelectableState(node, state);
        }
      }
    }

  if (nb_children > 0)
    {
    tree->SetNodeSelectableFlag(parent_node.c_str(), state);
    }

  if (!state && tree->HasSelection())
    {
    const char* sel_node = tree->GetSelection();
    if (sel_node && !tree->GetNodeSelectableFlag(sel_node))
      {
      tree->ClearSelection();
      }
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentAnatomicalStructureStep::PopulateAnatomicalStructureTree(
  const char *parent, vtkIdType vol_id)
{
  vtkKWTree *tree = this->AnatomicalStructureTree->GetWidget();

  // Insert that volume if not found, or just update its label
  // Our aim here is to update the tree without removing all the nodes
  // first (and lose the selection as well as the open/close state of
  // all the nodes). 
  // Right now, if a node is found, it will updated. However, if the
  // node position had changed since last call (i.e. the node was already
  // there but was moved), this will not be updated accordingly (todo)

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (!mrmlManager)
    {
    return;
    }
  const char *found = tree->FindNodeWithUserDataAsInt(parent, vol_id);
  vtksys_stl::stringstream node;
  vtksys_stl::string nodename(mrmlManager->GetTreeNodeName(vol_id));
  if (!found)
    {
    node << tree->GetTclName() << vol_id;
    tree->AddNode(parent, node.str().c_str(), nodename.c_str());
    }
  else
    {
    // need to copy node otherwise the tree gets all messed up as node points to nodename !
    node << found;
    tree->SetNodeText(node.str().c_str(), nodename.c_str());
    }
  tree->SetNodeUserDataAsInt(node.str().c_str(), vol_id); 

  // Insert its children

  int nb_children = mrmlManager->GetTreeNodeNumberOfChildren(vol_id);
  for (int i = 0; i < nb_children; i++)
    {
    this->PopulateAnatomicalStructureTree(
      node.str().c_str(), mrmlManager->GetTreeNodeChildNodeID(vol_id, i));
    }
}
   
//----------------------------------------------------------------------------
void vtkEMSegmentAnatomicalStructureStep::OpenTreeCallback()
{
  vtkKWTree *tree = this->AnatomicalStructureTree->GetWidget();
  tree->OpenFirstNode();
}

//----------------------------------------------------------------------------
void vtkEMSegmentAnatomicalStructureStep::CloseTreeCallback()
{
  vtkKWTree *tree = this->AnatomicalStructureTree->GetWidget();
  tree->CloseFirstNode();
}

//----------------------------------------------------------------------------
void vtkEMSegmentAnatomicalStructureStep::PopupNodeContextMenuCallback(
  const char *node)
{
  if (!node || !*node)
    {
    return;
    }

  // If we have a node selection, offer to delete the corresponding node, or
  // create a child underneath.

  vtkKWTree *tree = this->AnatomicalStructureTree->GetWidget();
  tree->SelectNode(node);
  vtkIdType vol_id = tree->GetNodeUserDataAsInt(node);

  char buffer[256];

  if (!this->ContextMenu)
    {
    this->ContextMenu = vtkKWMenu::New();
    }
  if (!this->ContextMenu->IsCreated())
    {
    this->ContextMenu->SetParent(tree);
    this->ContextMenu->Create();
    }

  this->ContextMenu->DeleteAllItems();
  sprintf(buffer, "AddChildNodeCallback %d", static_cast<int>(vol_id));
  this->ContextMenu->AddCommand("Add sub-class", this, buffer);
  if (strcmp(node, "root_node"))
    {
    sprintf(buffer, "DeleteNodeCallback %d", static_cast<int>(vol_id));
    this->ContextMenu->AddCommand("Delete sub-class", this, buffer);
    }

  int px, py;
  vtkKWTkUtilities::GetMousePointerCoordinates(tree, &px, &py);
  this->ContextMenu->PopUp(px, py);
}

//----------------------------------------------------------------------------
void vtkEMSegmentAnatomicalStructureStep::DeleteNodeCallback(vtkIdType sel_vol_id)
{
  vtkKWTree *tree = this->AnatomicalStructureTree->GetWidget();
  vtksys_stl::string sel_node(tree->FindNodeWithUserDataAsInt(NULL, sel_vol_id));
  vtksys_stl::string parent_node(tree->GetNodeParent(sel_node.c_str()));

  // Check whether user wants to delete the root node
  if (parent_node.compare("root") == 0)
    {
    vtkKWMessageDialog::PopupMessage (this->GetApplication (), NULL,
        "Error",
        "Root node cannot be deleted",
        vtkKWMessageDialog::ErrorIcon | vtkKWMessageDialog::InvokeAtPointer);
    return;
    }

  // Check if MRML manager exists
  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (!mrmlManager)
    {
    return;
    }

  // Delete node if user agrees
  if (vtkKWMessageDialog::PopupYesNo( 
        this->GetApplication(), 
        NULL, 
        "Delete node?",
        "Are you sure you want to delete this sub-class and its children?",
        vtkKWMessageDialog::WarningIcon | vtkKWMessageDialog::InvokeAtPointer))
    {
    tree->DeleteNode(sel_node.c_str());
    tree->SelectSingleNode(parent_node.c_str());
    mrmlManager->RemoveTreeNode(sel_vol_id);
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentAnatomicalStructureStep::AddChildNodeCallback(vtkIdType sel_vol_id)
{
  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  if (!mrmlManager)
    {
    return;
    }
  vtkKWTree *tree = this->AnatomicalStructureTree->GetWidget();
  vtksys_stl::string sel_node(tree->FindNodeWithUserDataAsInt(NULL, sel_vol_id));
  char child_node[256];
  vtkIdType child_id = mrmlManager->AddTreeNode(sel_vol_id);
  sprintf(child_node, "node_%d", static_cast<int>(child_id));
  tree->AddNode(sel_node.c_str(), child_node, child_node);
  tree->SetNodeUserDataAsInt(child_node, child_id); 

  tree->OpenNode(sel_node.c_str());
  tree->SelectNode(child_node);
  this->SelectedAnatomicalNodeChangedCallback();
}

//----------------------------------------------------------------------------
void vtkEMSegmentAnatomicalStructureStep::NodeParentChangedCallback(
  const char *node, const char *new_parent, const char*)
{
  // Reparent a node (by drag and dropping)

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  vtkKWTree *tree = this->AnatomicalStructureTree->GetWidget();
  vtkIdType vol_id = tree->GetNodeUserDataAsInt(node);
  vtkIdType new_parent_vol_id = tree->GetNodeUserDataAsInt(new_parent);
  if (mrmlManager)
    {
    mrmlManager->SetTreeNodeParentNodeID(vol_id, new_parent_vol_id);
    }
}

// not easy to fix - during editing the node attribute 'name' this
// function gets called each time the user presses a key
// if a user wants to create a new name, he would probably erase the current name(pressing backspace...)
// at one point he will reach "", so it must be possible to set a name like ""
//----------------------------------------------------------------------------
void vtkEMSegmentAnatomicalStructureStep::SelectedNodeNameChangedCallback(
  vtkIdType sel_vol_id, const char *value)
{
  // The node name has changed because of user interaction

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  vtkKWTree *tree = this->AnatomicalStructureTree->GetWidget();
  const char* found_node = tree->FindNodeWithUserDataAsInt(NULL, sel_vol_id);
  if (found_node && mrmlManager && value)
      {
    vtksys_stl::string val_no_white(value); 
    while (val_no_white.size() && !val_no_white.find(" "))
      {
        val_no_white.erase(0,1);
      }

      if (val_no_white.size())
      {
        mrmlManager->SetTreeNodeName(sel_vol_id, value);
            // if you do not copy the node id then you get wired errors
            vtksys_stl::string node(found_node);
        tree->SetNodeText(node.c_str(), value);
      } 
    else 
      {
        this->AnatomicalNodeAttributeNameEntry->GetWidget()->SetValue(mrmlManager->GetTreeNodeName(sel_vol_id));
      }
    }
}

//----------------------------------------------------------------------------
template<typename T, typename P>
T remove_if(T beg, T end, P pred)
{
    T dest = beg;
    for (T itr = beg;itr != end; ++itr)
        if (!pred(*itr))
            *(dest++) = *itr;
    return dest;
}


void vtkEMSegmentAnatomicalStructureStep::SelectedNodeIntensityLabelChangedCallback(
  vtkIdType sel_vol_id, int value)
{
  // The node label has changed because of user interaction

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  vtkKWTree *tree = this->AnatomicalStructureTree->GetWidget();
  const char *found_node = tree->FindNodeWithUserDataAsInt(NULL, sel_vol_id);
  if (found_node)
    {
    if (mrmlManager)
      {
      mrmlManager->SetTreeNodeIntensityLabel(sel_vol_id, value);
      }
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentAnatomicalStructureStep::SelectedNodeColorChangedCallback(
  vtkIdType sel_vol_id, double r, double g, double b)
{
  // The node color has changed because of user interaction

  vtkEMSegmentMRMLManager *mrmlManager = this->GetGUI()->GetMRMLManager();
  double rgb[3]; rgb[0] = r; rgb[1] = g; rgb[2] = b;
  if (mrmlManager)
    {
    mrmlManager->SetTreeNodeColor(sel_vol_id, rgb);
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentAnatomicalStructureStep::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkEMSegmentAnatomicalStructureStep::RemoveSelectedColorChangedObserver()
{
  if (this->ColorSelectorWidget)
    {
    this->ColorSelectorWidget->RemoveObservers (vtkSlicerNodeSelectorWidget::NodeSelectedEvent, 
                                              (vtkCommand *)this->SelectedColormapChangedCallbackCommand );  
    }
  if (this->ShowOnlyNamedColorsCheckButton)
    {
    this->ShowOnlyNamedColorsCheckButton->RemoveObservers (vtkKWCheckButton::SelectedStateChangedEvent, 
                                              (vtkCommand *)this->SelectedColormapChangedCallbackCommand );
    }
  if (this->ColorMultiColumnList)
    {
    this->ColorMultiColumnList->GetWidget()->RemoveObservers(vtkKWMultiColumnList::SelectionChangedEvent,
                                             (vtkCommand *)this->SelectedColorChangedCallbackCommand );
    }
  if (this->AnatomicalNodeIntensityLabelEntry)
    {
    this->AnatomicalNodeIntensityLabelEntry->GetWidget()->RemoveObservers (vtkKWEntry::EntryValueChangedEvent,
                                              (vtkCommand *)this->SelectedColorChangedCallbackCommand );
    }
}

//----------------------------------------------------------------------------
void vtkEMSegmentAnatomicalStructureStep::AddSelectedColorChangedObserver()
{
 // add observers
  this->ColorSelectorWidget->AddObserver (vtkSlicerNodeSelectorWidget::NodeSelectedEvent, 
                                          (vtkCommand *)this->SelectedColormapChangedCallbackCommand );  
  this->ShowOnlyNamedColorsCheckButton->AddObserver    (vtkKWCheckButton::SelectedStateChangedEvent, 
                                          (vtkCommand *)this->SelectedColormapChangedCallbackCommand );
  this->ColorMultiColumnList->GetWidget()->AddObserver(vtkKWMultiColumnList::SelectionChangedEvent,
                                          (vtkCommand *)this->SelectedColorChangedCallbackCommand );
  this->AnatomicalNodeIntensityLabelEntry->GetWidget()->AddObserver (vtkKWEntry::EntryValueChangedEvent,
                                          (vtkCommand *)this->SelectedColorChangedCallbackCommand );
}

//----------------------------------------------------------------------------
int vtkEMSegmentAnatomicalStructureStep::
SelectRowByIntensityLabelEntryValue(int entryValue, int columnIndex)
{
  int numRows = this->ColorMultiColumnList->GetWidget()->GetNumberOfRows();
  for(int rowIndex=0; rowIndex<numRows; rowIndex++)
    {
    if (this->ColorMultiColumnList->GetWidget()->GetCellTextAsInt(rowIndex, columnIndex) == entryValue)
      {
      this->ColorMultiColumnList->GetWidget()->SelectSingleRow(rowIndex);
      return rowIndex;
      }
    }
  this->ColorMultiColumnList->GetWidget()->ClearSelection();
  return -1;
}

//----------------------------------------------------------------------------
int vtkEMSegmentAnatomicalStructureStep::
GetIntensityLabelEntryValueOfFirstSelectedRow(int columnIndex)
{
  int numRows = this->ColorMultiColumnList->GetWidget()->GetNumberOfSelectedRows();
  if (numRows<1)
    {
    return -1;
    }
  int rowIndex = this->ColorMultiColumnList->GetWidget()->GetIndexOfFirstSelectedRow();
  return this->ColorMultiColumnList->GetWidget()->GetCellTextAsInt(rowIndex, columnIndex);
}

//----------------------------------------------------------------------------
void vtkEMSegmentAnatomicalStructureStep::UpdateAnatomicalNodeAttributeColorButton()
{
  int rowIndex = this->ColorMultiColumnList->GetWidget()->GetIndexOfFirstSelectedRow();
  if( rowIndex < 0) 
    {
    this->AnatomicalNodeAttributeColorButton->SetBackgroundColor(0.0, 0.0, 0.0);
    }
  else
    {
    double* colour = this->ColorMultiColumnList->GetWidget()->GetCellBackgroundColor(rowIndex, ColourColumn);
    this->AnatomicalNodeAttributeColorButton->SetBackgroundColor(colour);
    }
}


//---------------------------------------------------------------------------
void vtkEMSegmentAnatomicalStructureStep::PopUpLabelColorSelect()
{
 
  // Get the position of the mouse, position the popup
  //int x, y;
  //vtkKWTkUtilities::GetMousePointerCoordinates(this->ParameterSetMenuButton->GetWidget()->GetMenu(), &x, &y);
  //this->LabelTopLevel->SetPosition(x, y);
  vtkSlicerApplication *app = vtkSlicerApplication::SafeDownCast(this->GetGUI()->GetApplication());
  app->ProcessPendingEvents();
  this->LabelTopLevel->DeIconify();
  this->LabelTopLevel->Raise();
}

//---------------------------------------------------------------------------
void vtkEMSegmentAnatomicalStructureStep::LabelWindowCollapseCallback()
{
  if ( !this->LabelTopLevel )
    {
    return;
    }
  this->LabelTopLevel->Withdraw();
}



