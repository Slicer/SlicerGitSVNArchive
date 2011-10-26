/*=========================================================================

Program:   MIMX Meshing Toolkit
Module:    $RCSfile: vtkMimxBoundingBoxWidget.h,v $
Language:  C++
Date:      $Date: 2007/07/12 14:15:21 $
Version:   $Revision: 1.6 $

 Musculoskeletal Imaging, Modelling and Experimentation (MIMX)
 Center for Computer Aided Design
 The University of Iowa
 Iowa City, IA 52242
 http://www.ccad.uiowa.edu/mimx/
 
Copyright (c) The University of Iowa. All rights reserved.
See MIMXCopyright.txt or http://www.ccad.uiowa.edu/mimx/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even 
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

// .NAME vtkMimxBoundingBoxWidget - orthogonal hexahedron 3D widget
// .SECTION Description
// This 3D widget defines a region of interest that is represented by an
// arbitrarily oriented hexahedron with interior face angles of 90 degrees
// (orthogonal faces). The object creates 7 handles that can be moused on and
// manipulated. The first six correspond to the six faces, the seventh is in
// the center of the hexahedron. In addition, a bounding box outline is shown,
// the "faces" of which can be selected for object rotation or scaling. A
// nice feature of the object is that the vtkBoundingBoxWidget, like any 3D widget,
// will work with the current interactor style. That is, if vtkBoundingBoxWidget does
// not handle an event, then all other registered observers (including the
// interactor style) have an opportunity to process the event. Otherwise, the
// vtkBoundingBoxWidget will terminate the processing of the event that it handles.
//
// To use this object, just invoke SetInteractor() with the argument of the
// method a vtkRenderWindowInteractor.  You may also wish to invoke
// "PlaceWidget()" to initially position the widget. The interactor will act
// normally until the "i" key (for "interactor") is pressed, at which point the
// vtkMimxBoundingBoxWidget will appear. (See superclass documentation for information
// about changing this behavior.) By grabbing the six face handles (use the
// left mouse button), faces can be moved. By grabbing the center handle
// (with the left mouse button), the entire hexahedron can be
// translated. (Translation can also be employed by using the
// "shift-left-mouse-button" combination inside of the widget.) Scaling is
// achieved by using the right mouse button "up" the render window (makes the
// widget bigger) or "down" the render window (makes the widget smaller). To
// rotate vtkBoundingBoxWidget, pick a face (but not a face handle) and move the left
// mouse. (Note: the mouse button must be held down during manipulation.)
// Events that occur outside of the widget (i.e., no part of the widget is
// picked) are propagated to any other registered obsevers (such as the
// interaction style).  Turn off the widget by pressing the "i" key again.
// (See the superclass documentation on key press activiation.)
//
// The vtkMimxBoundingBoxWidget is very flexible. It can be used to select, cut, clip, or
// perform any other operation that depends on an implicit function (use the
// GetPlanes() method); or it can be used to transform objects using a linear
// transformation (use the GetTransform() method). Typical usage of the
// widget is to make use of the StartInteractionEvent, InteractionEvent, and
// EndInteractionEvent events. The InteractionEvent is called on mouse
// motion; the other two events are called on button down and button up 
// (either left or right button).
//
// Some additional features of this class include the ability to control the
// rendered properties of the widget. You can set the properties of the
// selected and unselected representations of the parts of the widget. For
// example, you can set the property for the handles, faces, and outline in
// their normal and selected states.

// .SECTION Caveats
// Note that handles can be picked even when they are "behind" other actors.
// This is an intended feature and not a bug.

// .SECTION See Also
// vtk3DWidget vtkLineWidget

#ifndef __vtkMimxBoundingBoxWidget_h
#define __vtkMimxBoundingBoxWidget_h

#include "vtk3DWidget.h"
#include "vtkMimxWidgetsWin32Header.h"


class vtkActor;
class vtkCellPicker;
class vtkCollection;
class vtkPlanes;
class vtkPoints;
class vtkMimxModPointWidget;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkProp;
class vtkProperty;
class vtkBBWPWCallback;
class vtkSphereSource;
class vtkTransform;

class VTK_MIMXWIDGETS_EXPORT vtkMimxBoundingBoxWidget : public vtk3DWidget
{
public:
        // Description:
        // Instantiate the object.
        static vtkMimxBoundingBoxWidget *New();

        vtkTypeRevisionMacro(vtkMimxBoundingBoxWidget,vtk3DWidget);
        void PrintSelf(ostream& os, vtkIndent indent);

        // Description:
        // Methods that satisfy the superclass' API.
        virtual void SetEnabled(int);
        virtual void PlaceWidget(double bounds[6]);
        void PlaceWidget()
        {this->Superclass::PlaceWidget();}
        void PlaceWidget(double xmin, double xmax, double ymin, double ymax, 
                double zmin, double zmax)
        {this->Superclass::PlaceWidget(xmin,xmax,ymin,ymax,zmin,zmax);}
        void SetPoint(int i, double x[3]);

        // Description:
        // Get the planes describing the implicit function defined by the box
        // widget. The user must provide the instance of the class vtkPlanes. Note
        // that vtkPlanes is a subclass of vtkImplicitFunction, meaning that it can
        // be used by a variety of filters to perform clipping, cutting, and
        // selection of data.  (The direction of the normals of the planes can be
        // reversed enabling the InsideOut flag.)
        void GetPlanes(vtkPlanes *planes);

        // Description:
        // Set/Get the InsideOut flag. When off, the normals point out of the
        // box. When on, the normals point into the hexahedron.  InsideOut
        // is off by default.
        vtkSetMacro(InsideOut,int);
        vtkGetMacro(InsideOut,int);
        vtkBooleanMacro(InsideOut,int);

        // Description:
        // Retrieve a linear transform characterizing the transformation of the
        // box. Note that the transformation is relative to where PlaceWidget
        // was initially called. This method modifies the transform provided. The
        // transform can be used to control the position of vtkProp3D's, as well as
        // other transformation operations (e.g., vtkTranformPolyData).
        void GetTransform(vtkTransform *t);

        // Description:
        // Grab the polydata (including points) that define the box widget. The
        // polydata consists of 6 quadrilateral faces and 15 points. The first
        // eight points define the eight corner vertices; the next six define the
        // -x,+x, -y,+y, -z,+z face points; and the final point (the 15th out of 15
        // points) defines the center of the hexahedron. These point values are
        // guaranteed to be up-to-date when either the InteractionEvent or
        // EndInteractionEvent events are invoked. The user provides the
        // vtkPolyData and the points and cells are added to it.
        void GetPolyData(vtkPolyData *pd);

        // Description:
        // Get the handle properties (the little balls are the handles). The 
        // properties of the handles when selected and normal can be 
        // set.
        vtkGetObjectMacro(HandleProperty,vtkProperty);
        vtkGetObjectMacro(SelectedHandleProperty,vtkProperty);

        // Description:
        // Get the face properties (the faces of the box). The 
        // properties of the face when selected and normal can be 
        // set.

        // Description:
        // Get the outline properties (the outline of the box). The 
        // properties of the outline when selected and normal can be 
        // set.
        vtkGetObjectMacro(OutlineProperty,vtkProperty);
        vtkGetObjectMacro(SelectedOutlineProperty,vtkProperty);

        // Description:
        // Control the representation of the outline. This flag enables
        // face wires. By default face wires are off.
        vtkSetMacro(OutlineFaceWires,int);
        vtkGetMacro(OutlineFaceWires,int);
        vtkBooleanMacro(OutlineFaceWires,int);

        // Description:
        // Control the representation of the outline. This flag enables
        // the cursor lines running between the handles. By default cursor
        // wires are on.
        vtkSetMacro(OutlineCursorWires,int);
        vtkGetMacro(OutlineCursorWires,int);
        vtkBooleanMacro(OutlineCursorWires,int);

        // Description:
        // Control the behavior of the widget. Translation, rotation, and
        // scaling can all be enabled and disabled.

protected:
        vtkMimxBoundingBoxWidget();
        ~vtkMimxBoundingBoxWidget();

        int State;
        enum WidgetState
        {
                Start=0,
                MovingHandle,
                Scaling,
                Outside
        };

        // Handles the events
        static void ProcessEvents(vtkObject* object, 
                unsigned long event,
                void* clientdata, 
                void* calldata);

        // ProcessEvents() dispatches to these methods.
        void OnMouseMove();
        void OnLeftButtonDown();
        void OnLeftButtonUp();
        void OnMiddleButtonDown();
        void OnMiddleButtonUp();
        void OnRightButtonDown();
        void OnRightButtonUp();
        vtkPoints *Points;  //used by others as well

        // the hexahedron (6 faces)
        vtkActor          *HexActor;
        vtkPolyDataMapper *HexMapper;
        vtkPolyData       *HexPolyData;

        
        // glyphs representing hot spots (e.g., handles)
        vtkCollection *Handle;
//      vtkActor          **Handle;
        vtkCollection *HandleMapper;
//      vtkPolyDataMapper **HandleMapper;
        vtkCollection *HandleGeometry;
//      vtkSphereSource   **HandleGeometry;
        void PositionHandles();
        void HandlesOn(double length);
        void HandlesOff();
        int HighlightHandle(vtkProp *prop); //returns cell id
        void HighlightOutline(int highlight);
        virtual void SizeHandles();

        // wireframe outline
        vtkActor          *HexOutline;
        vtkPolyDataMapper *OutlineMapper;
        vtkPolyData       *OutlinePolyData;

        // Do the picking
        vtkCellPicker *HandlePicker;
        vtkCellPicker *HexPicker;
        vtkActor *CurrentHandle;

        
        // Properties used to control the appearance of selected objects and
        // the manipulator in general.
        vtkProperty *HandleProperty;
        vtkProperty *SelectedHandleProperty;
        vtkProperty *OutlineProperty;
        vtkProperty *SelectedOutlineProperty;
        void CreateDefaultProperties();

        // Control the orientation of the normals
        int InsideOut;
        int OutlineFaceWires;
        int OutlineCursorWires;
        void GenerateOutline();
        // Pointwidget handles
        vtkCollection* PointWidget;
        //vtkModPointWidget *PointWidget1;
        vtkBBWPWCallback *PWCallback;
        vtkMimxModPointWidget *CurrentPointWidget;
        int ForwardEvent(unsigned long event);
        void EnablePointWidget();
        void DisablePointWidget();
private:
        vtkMimxBoundingBoxWidget(const vtkMimxBoundingBoxWidget&);  //Not implemented
        void operator=(const vtkMimxBoundingBoxWidget&);  //Not implemented
};

#endif
