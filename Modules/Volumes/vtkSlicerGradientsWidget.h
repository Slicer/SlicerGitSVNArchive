// .NAME vtkSlicerGradientsWidget 
// .SECTION Description
// This class implements Slicer's DWI Gradients widget, part of the DiffusionEditor GUI.
// Inherits most behavior from vtkSlicerWidget.
#ifndef __vtkSlicerGradientsWidget_h
#define __vtkSlicerGradientsWidget_h

#include "vtkVolumes.h"
#include "vtkSlicerWidget.h"
#include "vtkSlicerDiffusionEditorLogic.h"
#include "vtkSlicerApplication.h"

class vtkMRMLDiffusionWeightedVolumeNode;
class vtkDoubleArray;
//widgets
class vtkKWFrameWithLabel;
class vtkKWFrame;
class vtkKWLoadSaveButtonWithLabel;
class vtkKWCheckButton;
class vtkKWTextWithScrollbars ;
class vtkKWLabel;
class vtkKWMessageDialog;
class vtkKWPushButton;

class VTK_VOLUMES_EXPORT vtkSlicerGradientsWidget : public vtkSlicerWidget
  {
  public:

    // Description:
    // Usual vtk class functions.
    static vtkSlicerGradientsWidget* New();
    vtkTypeRevisionMacro(vtkSlicerGradientsWidget,vtkSlicerWidget);
    void PrintSelf(ostream& os, vtkIndent indent);

    // Description:
    // Add/Remove observers on widgets in the GUI.
    virtual void AddWidgetObservers();
    virtual void RemoveWidgetObservers();

    // Description:
    // Propagates events generated in GUI to logic / mrml.
    void ProcessWidgetEvents(vtkObject *caller, unsigned long event, void *callData );

    // Description:
    // Updates the widget when a new ActiveVolumeNode is loaded.
    void UpdateWidget(vtkMRMLDiffusionWeightedVolumeNode *dwiNode);

    // Description:
    // Propagates keypress-events generated in the GradientsTextbox.
    void TextFieldModifiedCallback();

    // Description:
    // Event is invoked when a matrix value changed.
    enum
      {
      ChangedEvent = 1234,
      };

    // Description:
    // Sets the Logic to the current vtkSlicerDiffusionEditorLogic of the editor.
    virtual void SetLogic(vtkSlicerDiffusionEditorLogic *logic);

    // Description:
    // Enables/Disables the whole widget, also collapses the widget.
    void SetStatus(int status);

    // Description:
    // Sets the Application to the current vtkSlicerApplication.
    vtkSetObjectMacro(Application, vtkSlicerApplication);
    virtual void SetApplication (vtkKWApplication* arg)
      { this->Superclass::SetApplication( arg ); }

  protected:
    vtkSlicerGradientsWidget(void);
    virtual ~vtkSlicerGradientsWidget(void);

    // Description:
    // Creates the widget.
    virtual void CreateWidget();

    // Description:
    // Updates the GradientsTextbox containing bValue and gradients (GUI).
    void UpdateGradients();

    // Description:
    // Updates the StatusLabel for the gradients (GUI).
    void UpdateStatusLabel(int status);

    // Description:
    // Saves changes of the gradients/bValues to the ActiveVolumeNode.
    void SaveGradients();

    // Description:
    // Displays a message dialog to the user (GUI).
    void DisplayMessageDialog(const char* message);

    vtkSlicerApplication *Application;

    vtkMRMLDiffusionWeightedVolumeNode *ActiveVolumeNode;
    vtkDoubleArray *Gradients;
    vtkDoubleArray *BValues;
    vtkSlicerDiffusionEditorLogic *Logic;
    //widgets (GUI)
    vtkKWFrameWithLabel *GradientsFrame;
    vtkKWFrame *ButtonsFrame;
    vtkKWLoadSaveButtonWithLabel *LoadGradientsButton;
    vtkKWCheckButton *EnableGradientsButton;
    vtkKWTextWithScrollbars *GradientsTextbox;
    vtkKWLabel *StatusLabel;
    vtkKWMessageDialog *MessageDialog;

  private:
    vtkSlicerGradientsWidget (const vtkSlicerGradientsWidget&); // Not implemented.
    void operator = (const vtkSlicerGradientsWidget&); //Not implemented.
  };

#endif 
