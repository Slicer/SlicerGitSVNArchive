/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkAnnotationROIWidget.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Annotations includes
#include "vtkAnnotationROIWidget2D.h"
#include "vtkAnnotationROIRepresentation2D.h"

// VTK includes
#include "vtk/Common/Core/vtkCommand.h"
#include "vtk/Common/Core/vtkCallbackCommand.h"
#include "vtk/Rendering/Core/vtkRenderWindowInteractor.h"
#include "vtk/Common/Core/vtkObjectFactory.h"
#include "vtk/Interaction/Widgets/vtkWidgetEventTranslator.h"
#include "vtk/Interaction/Widgets/vtkWidgetCallbackMapper.h"
#include "vtk/Interaction/Widgets/vtkEvent.h"
#include "vtk/Interaction/Widgets/vtkWidgetEvent.h"
#include "vtk/Rendering/Core/vtkRenderWindow.h"
#include "vtk/Rendering/Core/vtkRenderer.h"


vtkStandardNewMacro(vtkAnnotationROIWidget2D);

//----------------------------------------------------------------------------
vtkAnnotationROIWidget2D::vtkAnnotationROIWidget2D()
{
}

//----------------------------------------------------------------------------
vtkAnnotationROIWidget2D::~vtkAnnotationROIWidget2D()
{
}

//----------------------------------------------------------------------
void vtkAnnotationROIWidget2D::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
    {
    this->WidgetRep = vtkAnnotationROIRepresentation2D::New();
    }
}

//----------------------------------------------------------------------------
void vtkAnnotationROIWidget2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}




